# Resonance Firmware Architecture

**Requirement:** RES-PROC-001, RES-ARCH-001..004 and the domains they structure (`docs/requirements/resonance.md`)
**Work:** WORK-000001 (REQ-CHAT-20261117-001)
**Status:** Stage 1 design. No firmware exists yet; the directory names below are created in Stage 2.
**Inputs:** `HARDWARE.md`, `GPIO_MAP.md`, `LIBRARY_EVALUATION.md`

## Principles

1. **Audio first.** The audio path has the highest priority and never waits
   for storage, network, GUI or logging (RES-AUDIO-012).
2. **One owner per resource.** Each bus, peripheral and buffer belongs to
   exactly one module (RES-ARCH-001).
3. **Commands in, state out.** The GUI, the web server and the session
   engine change the device only by posting commands; they read it only
   from a published state snapshot (RES-ARCH-002).
4. **Portable core.** Everything that is arithmetic or logic compiles and is
   tested on a PC. Only thin adapters touch the hardware.
5. **Fixed memory.** Buffers and stacks are sized at build time and
   allocated once at start (RES-ARCH-004).

## Layers

```text
┌────────────────────────────────────────────────────────────────────┐
│ Presentation      gui/ (LVGL screens)        web/ (REST, WebSocket)│
└───────────────┬─────────────────────────────────────┬──────────────┘
                │ commands                    state   │
┌───────────────▼─────────────────────────────────────▼──────────────┐
│ Application       ResonanceApp · CommandBus · StateStore           │
│                   SessionManager · PresetManager · ConfigManager   │
└───────────────┬────────────────────────────────────────────────────┘
                │
┌───────────────▼────────────────────────────────────────────────────┐
│ Core (portable)   AudioEngine · BinauralGenerator · NoiseGenerator │
│                   AudioMixer · Limiter · FrequencySweepEngine      │
│                   Session model · validation · path rules          │
└───────────────┬────────────────────────────────────────────────────┘
                │ interfaces
┌───────────────▼────────────────────────────────────────────────────┐
│ Platform          I2SAudioOutput · BluetoothAudioOutput            │
│                   AmbientAudioPlayer (SD, decoders) · SDCardManager│
│                   DisplayManager · TouchManager · WiFiManager      │
│                   BluetoothManager · BatteryManager · RadioPolicy  │
└────────────────────────────────────────────────────────────────────┘
```

A layer depends only on the layers below it. The core layer includes no
Arduino, ESP-IDF, LVGL or web header; an include rule in `devsys.json`
`invariants` will enforce that once the source tree exists.

## Source layout (planned)

```text
firmware/
  platformio.ini            environments: resonance (device), native (unit tests)
  partitions.csv
  lib/
    resonance_core/         portable: dsp/, session/, config/, util/
  src/
    app/                    ResonanceApp, CommandBus, StateStore
    audio/                  outputs, player, decoders, audio task
    storage/                SDCardManager, ConfigurationManager, PresetManager
    net/                    WiFiManager, WebServerManager, UploadManager, api/
    bt/                     BluetoothManager
    gui/                    DisplayManager, TouchManager, screens/, theme
    hal/                    board pins (from GPIO_MAP.md), BatteryManager
  web/                      interface sources; gzipped into the image at build
  test/                     Unity tests for resonance_core (native)
```

The modules suggested in the request all appear; `StorageManager` is the
storage directory as a whole, and `CommandBus`, `StateStore` and `RadioPolicy`
are additions explained below.

## Audio path

```text
 SD card ──> decoder task ──> PCM ring ──┐
                                         ▼
 BinauralGenerator ──────────────> AudioMixer ──> Limiter ──> AudioOutput
 NoiseGenerator ─────────────────────────┘            (I2S or Bluetooth)
        ▲
        └── smoothed parameters <── CommandBus <── GUI / web / session
```

### Format

- **44.1 kHz stereo.** Bluetooth A2DP in the chosen library is fixed at
  44.1 kHz, so one rate serves both outputs and no resampler sits on the
  output side. 48 kHz is not offered (RES-AUDIO-001 allows either).
- **Internal samples are 32-bit float.** The ESP32 has a single-precision
  FPU. This satisfies "32-bit internal processing" and gives headroom while
  mixing.
- **Block size 256 frames** (5.8 ms). The engine renders one block per call.
- Output conversion: 32-bit slots for the PCM5102A, 16-bit with dither for
  Bluetooth.

### Binaural generator (RES-AUDIO-002..005)

- Two independent oscillators, each a 32-bit phase accumulator. Frequency
  resolution is 44100 / 2³² ≈ 0.00001 Hz, far finer than the 0.1 Hz step.
- Sine by table lookup with linear interpolation; triangle from the phase
  directly.
- A frequency change alters only the phase increment, never the phase, so
  it is phase-continuous by construction. The increment itself is ramped
  over a few milliseconds.
- Left carries f0, right carries f0 + fb. The two are kept separate through
  the mixer and the limiter: no stage sums or cross-feeds the channels of
  the binaural layer.
- Parameters are clamped to the configured range before they reach the
  oscillator; a value that is not finite is rejected.

### Mixer and limiter (RES-AUDIO-007, RES-AUDIO-008)

- Three layers, each with on/off, volume, mute and its own fade ramp; then
  master volume and balance.
- Every gain is a ramped value: a volume step becomes a short linear ramp,
  a fade a long one. Start and stop are fades, which is what removes clicks.
- "Immediate mute" is a ramp of about 5 ms: fast, but not a discontinuity.
- A look-ahead-free soft limiter follows the sum, with a hard clamp after
  it. All layers at full level produce limited output, not wrap-around.
- A configurable output ceiling caps headphone level.

### Noise (RES-AUDIO-010)

White from a fast pseudo-random generator, pink by a filter bank over white,
brown by leaky integration. One noise type is active at a time in the noise
layer, with its own volume.

### Background player (RES-PLAY-001..003)

- A **decoder task** reads the SD card and decodes into a PCM ring buffer.
  The audio task only reads the ring. If the ring is empty the background
  layer outputs silence for that block and an underrun is counted: the
  binaural tone continues undisturbed.
- MP3 through the Helix decoder, WAV through a parser of our own. Material
  that is not 44.1 kHz is converted in the decoder task.
- One decoder instance. Crossfade between two files (RES-PLAY-006) would
  need a second decoder, about 30 kB: decided at gate G2.

### Outputs (RES-AUDIO-011)

```cpp
class AudioOutput {
public:
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual size_t write(const float* interleaved, size_t frames) = 0;  // may accept fewer
    virtual bool ready() const = 0;
};
```

- `I2SAudioOutput` writes to the ESP-IDF I2S driver, whose DMA queue paces
  the audio task.
- `BluetoothAudioOutput` writes to a ring buffer that the Bluetooth stack's
  callback drains; the callback never blocks and delivers silence when the
  ring is empty.
- **One output is active at a time.** The two sinks run on different clocks;
  feeding both would need drift compensation, for no stated benefit.
  Switching outputs fades out, swaps, and fades in.
- The DAC's soft-mute line (IO4, if wired) is released after the I2S clocks
  are running and asserted before they stop.

## Control and state

- **CommandBus:** a fixed-size FreeRTOS queue of small command structures
  (set frequency, set layer volume, transport, session actions). The audio
  task drains it at each block boundary. Producers never touch engine
  objects.
- **StateStore:** one snapshot structure with a version counter, written by
  the application task. The GUI and the web server compare the version and
  redraw or push only on change. This is the single mechanism behind
  "changes on the touchscreen appear in the browser" (RES-WEB-003).
- Validation happens once, where the command is accepted, using the same
  portable rules for touch input, REST input and files loaded from the card
  (RES-AUDIO-005, RES-CFG-002).

## Sessions and automation (RES-SESS-001..005)

- `FrequencySweepEngine` is portable and knows nothing of oscillators: it
  maps elapsed time to target values for beat frequency, primary frequency
  and the layer volumes (linear, stepped, hold, multi-stage).
- `SessionManager` runs in the application task on a 20 ms tick, advances
  the sweep engine, and posts ordinary commands. It keeps running whatever
  the display shows.
- Session time is derived from the count of audio frames rendered, not from
  wall-clock time, so a pause is exact and the timeline cannot drift from
  the audio.

## Storage and configuration (RES-CFG-001..003, RES-PLAY-004..005)

- Device settings (Wi-Fi, hostname, output, brightness, credentials) live in
  NVS, so the device works without a card.
- Presets, sessions and playlists are JSON files under `/RESONANCE` on the
  card. Factory presets are compiled in.
- A file is written to a temporary name and renamed; a file that fails
  validation is ignored and reported, and defaults are used.
- **Volume is not restored to a loud level.** At boot the engine is stopped
  and the start-up volume from the settings applies; the reset reason is
  recorded so that an unexpected restart is visible.
- `SDCardManager` owns the card and its SPI bus. Other modules ask it for
  file operations; the web server never opens a file itself. There is no
  card-detect pin, so presence is probed and every failure is treated as a
  possible removal.
- All paths from the web interface pass one portable function that resolves
  them under `/RESONANCE`, refuses `..`, absolute paths and control
  characters, and checks the extension (RES-WEB-005).

## Network and web (RES-NET-*, RES-WEB-*)

- `WiFiManager`: station with saved credentials; configuration access point
  on first boot and as fallback; mDNS name `resonance.local`.
- `WebServerManager`: asynchronous server, REST under `/api/v1/`, one
  WebSocket for state pushes, assets gzipped in the image.
- Handlers run in the network task. They **post commands and read the
  snapshot**; they hold no reference to audio or hardware objects.
- Authentication: a device password set at first configuration. Every
  request except the static shell and the login endpoint requires a session
  token. Wi-Fi passwords are write-only in the API.
- Uploads stream to a temporary file through `SDCardManager`, with the
  signature checked on the first block, and are renamed on success. Stale
  temporary files are removed at start.

## Radio policy (RES-BT-004, RES-WEB-007)

`RadioPolicy` is the one place that decides which radio services may run
together. It exists because `LIBRARY_EVALUATION.md` shows that Wi-Fi with
A2DP is the scarce case, and because the answer is not known until it is
measured.

| Level | Behaviour | When |
|---|---|---|
| A | Wi-Fi station, web and A2DP together | If measurement shows a safe heap margin and no dropouts |
| B | Together, but uploads and OTA are deferred while A2DP streams, with a warning in the interface | If only heavy transfers disturb playback |
| C | Web interface and Bluetooth output are exclusive modes chosen by the user | If the heap does not allow both |

The level is set by a build-time constant backed by gate G2 in
`IMPLEMENTATION_PLAN.md`, and the chosen level is stated in the user
documentation. The access point is never combined with A2DP streaming:
Espressif rates that combination as unstable.

## GUI (RES-GUI-*)

- `DisplayManager` owns the display SPI bus and the backlight;
  `TouchManager` owns the touch I2C bus.
- LVGL runs in its own task. Screens read the state snapshot and post
  commands.
- Visualisations are drawn from parameters in the snapshot (frequencies,
  levels), not from audio samples, so the GUI never reads audio buffers.
- Frame rate is capped; the reduced-animation mode and the dimmed state
  lower it further. The GUI task's priority is below every audio task, so a
  slow redraw cannot delay audio.

## Tasks

| Task | Core | Priority | Stack | Blocks on |
|---|---|---|---|---|
| Audio | 1 | highest of ours | 4 kB | I2S write, or the Bluetooth ring having space |
| Decoder | 1 | below audio | 8 kB | SD reads, ring buffer space |
| Application (commands, sessions, state) | 1 | middle | 4 kB | 20 ms tick, queue |
| GUI (LVGL) | 1 | low | 8 kB | LVGL timer |
| Network (AsyncTCP) | 0 | low | 4 kB | sockets |
| Wi-Fi, Bluetooth controller and host | 0 | set by ESP-IDF | set by ESP-IDF | — |

- Radio stacks stay on core 0 and audio on core 1, as Espressif recommends
  for coexistence.
- Stack sizes are starting values; each is checked against its high-water
  mark in Stage 2 onward.
- Logging from the audio task is forbidden. It increments counters
  (underruns, limiter activity, late blocks) that the application task
  reports.

## Memory

The budget, its sources and its uncertainty are in `LIBRARY_EVALUATION.md`
section 7. The rules that follow from it:

- All ring buffers, the LVGL heap, the draw buffer and the decoder are
  allocated once at start, largest first, before the radios start.
- No `String` concatenation or JSON document building in the audio or
  decoder task.
- Free heap, minimum free heap and largest free block are part of the state
  snapshot in development builds.

## Open decisions

Each is recorded as an ADR in `docs/decisions/` when it is taken; none can
be taken responsibly before the measurement it names.

| Decision | Depends on |
|---|---|
| Radio policy level A, B or C | Gate G2 |
| OTA or USB-only update; partition layout | Gate G3 |
| LVGL 8.4 or 9.x; draw buffer size | Gate G1 |
| Custom library build (Classic-only) or stock libraries | Gate G1 |
| Headphone amplifier part | Availability; `HARDWARE.md` section 9 |
| Battery divider on IO35 | Measurements M2, M3, M10 |
| Second decoder for crossfade | Gate G2 |
