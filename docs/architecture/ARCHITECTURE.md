# Resonance Firmware Architecture

**Requirement:** RES-ARCH-001..005 and the domains they structure (`docs/requirements/resonance.md`)
**Decision:** ADR-000204 (Bluetooth-only output, space first), ADR-000205 (one operating mode: no Wi-Fi, no web administration)
**Work:** WORK-000001 (design), WORK-000002 and WORK-000003 (Stage 2), WORK-000004 (Stage 3), WORK-000007 (Stage 4), WORK-000009 (Stage 6)
**Status:** Stages 2 to 4 and the core of Stage 6 are implemented: board, display, touch, SD, settings, interface, the binaural tone, the Bluetooth audio output, WAV and MP3 playback from the card, the mixer, the noise generator and sessions (timers, factory programs, fades). The tone and sound files have been heard through a Bluetooth headset. Sections marked *planned* describe code that does not exist yet. Since firmware 0.10.0 there is one operating mode; Setup mode, Wi-Fi and the web interface are removed (ADR-000205).
**Inputs:** `HARDWARE.md`, `GPIO_MAP.md`, `LIBRARY_EVALUATION.md`

## Principles

1. **Audio first.** The audio path has the highest priority and never waits
   for storage, network, GUI or logging (RES-AUDIO-012).
2. **One owner per resource.** Each bus, peripheral and buffer belongs to
   exactly one module (RES-ARCH-001).
3. **Commands in, state out.** The interface changes the device only by
   reporting events or posting commands; it reads it only from a status
   snapshot (RES-ARCH-002).
4. **Portable core.** Everything that is arithmetic or logic compiles and is
   tested on a PC. Only thin adapters touch the hardware.
5. **Space first.** The smaller means is used, and a subsystem the device
   does not need is never started: Wi-Fi never is (RES-ARCH-005).

## One operating mode

The device has one operating mode (ADR-000205, firmware 0.10.0). Every start
brings up Bluetooth audio (A2DP source), playback from the card and
sessions. Wi-Fi is never started. No setting needs a restart: everything is
set on the touchscreen.

- Up to firmware 0.9.6 there were two modes, each entered by a restart:
  Play (Bluetooth, audio, sessions) and Setup (Wi-Fi and a web page for
  files and settings). They existed because the measured heap does not carry
  both radios: with Bluetooth and Wi-Fi both running, 50 kB remained before
  any audio code existed (`LIBRARY_EVALUATION.md`, "Measured"). Setup mode,
  Wi-Fi and the web interface were removed in 0.10.0 (ADR-000205). The
  measurement still holds: a network function would need a new decision.
- Sound files reach the card by putting the card in a computer, or over the
  serial line (`firmware/tools/serial_put.py`, the `put` command). There is
  no upload over the network.
- The radio is not stopped at run time: ESP-IDF releases Bluetooth memory
  one way only, and start/stop cycles fragment the heap.
- `radio/bt_stack.cpp` holds one hook, `btInUse()`, which tells the Arduino
  start-up code that Bluetooth is in use, so that the memory reserved for it
  is not handed back to the heap.
- Free heap at rest with Bluetooth started and no headset connected:
  97,872 bytes, measured on 0.10.0 (`IMPLEMENTATION_PLAN.md`, "Firmware
  0.10.0").

### Radio start guard

Starting the radio draws a current surge. On the first start the board lost
power sixteen times in a row (`HARDWARE.md`, section 8), which without a
guard is an endless restart loop. So:

- an attempt counter in non-volatile storage is raised just before the radio
  starts and cleared after three seconds of stable running;
- a start that finds three failed attempts in a row does not start the
  radio, shows "Radio off: power dropped" on the Home screen, and
  clears the counter, so the following start tries again;
- one or two failed attempts are tolerated, because a reset from the USB
  adapter or the reset button during those seconds cannot be told from a
  power loss (both read as a power-on reset).

The device therefore always reaches a usable screen.

## Layers

```text
┌──────────────────────────────────────────────────────────────────┐
│ Presentation      gui/ (screens, widgets)                        │
└───────────────┬──────────────────────────────────┬───────────────┘
                │ events                    status │
┌───────────────▼──────────────────────────────────▼───────────────┐
│ Application       main.cpp: start-up, status snapshot, serial    │
│                   SessionManager, PresetManager (planned)        │
└───────────────┬──────────────────────────────────────────────────┘
                │
┌───────────────▼──────────────────────────────────────────────────┐
│ Core (portable)   rc_settings: settings, ranges, validation      │
│                   rc_tone, rc_audio (mixer, limiter), rc_noise   │
│                   rc_ring, rc_wav, rc_resample · sweep (planned) │
└───────────────┬──────────────────────────────────────────────────┘
                │
┌───────────────▼──────────────────────────────────────────────────┐
│ Platform          hal/ board · gui/display · storage/ · radio/   │
│                   audio/bt_audio (A2DP) · audio/player (decoders)│
└──────────────────────────────────────────────────────────────────┘
```

The core includes no Arduino, ESP-IDF or display header. That rule will be
declared as an invariant in `devsys.json` (a protected artifact, changed
through a process-change request).

## Source layout

```text
firmware/
  platformio.ini              one environment: the device
  lib/resonance_core/src/     portable core
    rc_settings.h/.cpp        settings record, ranges, validation
    rc_tone.h/.cpp            oscillator, ramp, binaural tone engine
    rc_audio.h/.cpp           engine: three layers, master volume, limiter
    rc_noise.h/.cpp           white, pink and brown noise
    rc_ring.h                 lock-free ring of stereo frames
    rc_wav.h/.cpp             WAV header and PCM conversion
    rc_resample.h/.cpp        conversion to 44.1 kHz
    rc_session.h/.cpp         sessions: stages, sweeps, runner, factory programs
    rc_presets.h/.cpp         frequency presets for the Frequency Studio
    rc_paths.h/.cpp           rules for file names and paths on the card (written for the
                              removed web interface; a session stage's recording path
                              is still checked with them)
    rc_repeat.h               tap-and-hold timing
  src/
    main.cpp                  start-up, status, serial diagnostics
    version.h
    hal/board_pins.h          the only place with pin numbers
    hal/board.h/.cpp          quiet pins, status LED, reset reason
    gui/display.h/.cpp        ST7789 + GT911 + backlight (LovyanGFX)
    gui/theme.h               colours and layout
    gui/ui.h/.cpp             five screens, tab bar, buttons, dimming
    storage/settings_store    settings in NVS; radio start guard
    storage/sdcard            the card and its SPI bus; /RESONANCE tree
    audio/bt_audio            Bluetooth A2DP source: scan, connect, audio feed
    audio/player              decoder task: WAV and MP3 from the card into the ring
    radio/bt_stack.cpp        Bluetooth memory hook (btInUse)
  tests/                      host tests of the portable core
  tools/host_tests.py         builds and runs them with the host compiler
  tools/serial_capture.py     reads the device's serial report
  tools/serial_put.py         sends a sound file to the card over serial
  partitions.csv              one 3 MB application slot
```

Modules named in the request that do not exist yet arrive with their stage
(`IMPLEMENTATION_PLAN.md`). `BatteryManager` and `I2SAudioOutput` are not
planned (ADR-000204).

## Settings and storage

- One fixed-size settings record in NVS, so the device works without a card
  (`rc::Settings`). Every load passes `rc::sanitize()`: a record of another
  version is replaced by the defaults, and an out-of-range value or a
  non-finite frequency is corrected to a safe one (RES-CFG-002).
- The record is version 4. A stored version 3 record (firmware up to 0.9.6)
  is read once, the user's values are carried over, and it is rewritten
  (`rc::migrateFromV3`). Version 3 also held the operating mode, a device
  name and Wi-Fi credentials; these are not carried, so the rewrite removes
  the stored credentials from the device. The start-up report says
  `settings repaired` for that one start.
- The device comes back as it was left: the frequencies, the master,
  background and noise volumes, brightness, and the session choices
  (program, lengths, fade) are part of the settings record. They are written
  four seconds after the last change, not on every tap, to spare the flash.
- The master volume is stored capped at 60 %, so a restart cannot come back
  loud, and the noise type is not stored at all: nothing plays at start
  (RES-CFG-003).
- `sdcard` owns the card. It creates the `/RESONANCE` tree (RES-PLAY-004).
  The board has no card-detect signal, so a failing operation is treated as
  a possible removal and the card is reported as absent.
- *Planned:* presets, sessions and playlists as JSON files on the card,
  written to a temporary name and renamed.

## Interface

- LovyanGFX draws directly to the panel; there is no GUI framework and no
  frame buffer. Screens are redrawn only where something changed.
- Five tabs: Home, Frequency Studio, Sound Library, Sessions, Settings
  (RES-GUI-001), labelled HOME, FREQ, SND, SESS and SET. The tab bar is a
  row of five buttons, each outlined
  and filled; the open one is filled with the highlight colour. Controls that are not built yet say so on the screen.
- The ADV tab (SET before 0.13.0) has the brightness, an ADVANCED entry (layer mixer,
  breathing, session builder, sequence, Resonance Lab), the firmware
  version and the free memory. Nothing on it needs a restart.
- The SND tab browses `RESONANCE/AUDIO` on the card: folders and `.wav` /
  `.mp3` files, four to a page; tapping a file plays it. Below are
  play/pause with the background volume and the track's elapsed time. The
  noise control (type and volume) is on the FREQ tab.
- On the FREQ tab each frequency value sits between `<` and `>`, which step
  to the preset below or above the current value and wrap at the ends; a
  tap on the value itself also steps forward
  (fourteen carriers, nine beats covering every band of RES-AUDIO-006). The
  line above a value shows the preset's name while the value is on one. A
  preset only sets the value; the step buttons still move it freely. Names
  identify a frequency or its tradition and promise no effect
  (RES-PROC-006); a host test refuses names that would.
- The SESS tab chooses a program (the timer or a factory program), the
  timer's length and the fade, and starts it; while it runs it shows the
  time left, the stage and the beat frequency, with Pause and Stop. Home
  shows the time left.
- Home has the audio on/off control with the volume beside it, and a
  Bluetooth button that opens the device list: found devices as buttons,
  tap one to connect, and Disconnect when connected.
- Contrast comes first: near-black background, white text, bright outlines
  on every control, and no text smaller than 12 pixels. The first scheme
  (dim grey on dark blue) was unreadable on the panel.
- Touch is polled over I2C on every pass of the loop. The controller's
  interrupt line is not used: it idles high on this board, and the driver
  reads only while it is low.
- Tap-and-hold repeats on buttons that change a value by a step (frequency
  steps, the preset `<` and `>`, the volumes, brightness): the first action
  is immediate, repeats begin after 450 ms at about seven a second, and
  after 2.5 s of holding at about sixteen a second (`rc::HoldRepeat`).
  Sliding off the button ends the repeat. Buttons that switch, start, open
  or cycle something never repeat.
- A button is at least 38 pixels high. Frequencies are adjusted in steps of
  0.1, 1 and 10 Hz and validated by the portable core.
- The screen dims after the configured timeout; the first touch only wakes
  it. Brightness never goes below 5 %, because the screen is the only
  control.
- The interface runs in the Arduino loop task. Its priority is below every
  audio task (*planned*), so a slow redraw cannot delay audio.
- *Planned:* visualisations drawn from parameters in the status snapshot,
  never from audio samples.

## Audio path

```text
 SD card ──> player task ──> ring (4096 frames) ──┐
             WAV / MP3, to 44.1 kHz stereo        ▼
 ToneEngine (left f0, right f0 + fb) ────────> AudioEngine ──> Bluetooth A2DP
 NoiseGenerator ──────────────────────────────────┘  mix, master, limiter
```

- **44.1 kHz stereo, 16-bit at the output**, as A2DP fixes it. Mixing is in
  32-bit float; the ESP32 has a single-precision FPU.
- **Tone:** two independent oscillators, each a 32-bit phase accumulator
  read through a 1024-entry sine table with linear interpolation. A
  frequency change alters only the phase increment, so it is
  phase-continuous by construction, and glides over 50 ms. Left carries f0,
  right f0 + fb; the two are never summed. Start and stop fade over 500 ms.
- **Background:** the player task reads the card, decodes WAV (8/16/24/32-bit
  PCM, mono or stereo) or MP3 (Helix), converts to stereo at 44.1 kHz by
  linear interpolation, and writes into a lock-free ring. The render only
  reads the ring. An empty ring silences the background for that block,
  counts an underrun, and leaves tone and noise untouched.
- **Pause** stops the render reading the ring, after a 50 ms fade; the ring
  fills and the player task waits where it is, so the track keeps its place.
- **Noise:** white, pink (three-pole filter) or brown (leaky integrator), the
  same in both ears. The type changes only while its gain is at zero.
- **Mixer:** each layer has its own ramped gain (tone ceiling 0.5 of full
  scale, background 0.6, noise 0.3), then the master volume (square law).
- **Limiter:** transparent below 0.7 of full scale, bending smoothly towards
  a ceiling of 0.98 above it. All layers at maximum stay under the ceiling
  and never wrap (RES-AUDIO-008).
- **The Bluetooth stack pulls audio through a callback, and the callback
  renders directly from the engine.** Rendering never blocks, allocates or
  logs (RES-AUDIO-012). The interface passes parameters through atomic
  values that the engine picks up once per block.
- The MP3 decoder (about 33 kB) is allocated for a track and freed after
  it. A track that ends is followed by the next sound file of its folder.
- `audio/bt_audio` scans for audio devices and lists the five with the
  strongest signal; a device not seen for 45 seconds drops out, and one that
  sends no name is listed by the end of its address. It connects to the one
  chosen on the screen and lets the library reconnect to the last device at
  start. The name of a device is stored against its address in NVS when it
  is connected from the list, and looked up when the device reconnects by
  itself, so the screen shows the name either way. After a disconnect it
  starts the scan again itself: the library
  stops scanning for good once a disconnect was requested. Link keys stay in the Bluetooth stack's own
  storage (RES-BT-003).
- Audio is off at every start; only the Home screen's control or a serial
  command turns the tone on, and a track plays only when chosen
  (RES-CFG-003).
- `AudioOutput` as a formal interface arrives when a second output exists;
  until then the engine's `render()` is the boundary, and nothing in the
  core knows about Bluetooth.

## Sessions

- A session is up to six stages. A stage lasts a given time and either holds
  its frequencies or glides to them in a straight line from where the
  previous stage ended; a frequency of zero means "leave it as it is". The
  plain timer is a one-stage session that holds the current frequencies;
  with a length of zero it runs until stopped.
- `rc::SessionRunner` is portable and knows nothing of oscillators, screens
  or files: it turns elapsed time into target frequencies and a level
  (RES-SESS-001). `main.cpp` advances it every 100 ms from `loop()`,
  whatever screen is shown, and applies the values to the engine
  (RES-SESS-005).
- At the end, and on Stop, the level of the whole output falls in a straight
  line over the chosen fade (none, 30 s, 1 min, 5 min); then the tone is
  switched off, a playing track is paused, and the level is restored for
  the next start. The engine ramps between the ten updates a second, so the
  fade has no steps.
- Pause stops the clock and the audio; Resume continues exactly where it
  was.
- Session time is wall-clock time. (The Stage 1 design derived it from
  audio frames; that would stop the clock whenever no Bluetooth device is
  connected, so it was not built that way.)
- Seven factory programs are compiled in. Four are named for what the
  frequencies do; three (Trance, Deep trance, Astral) are named for the
  practice they are meant to accompany, and are slow descents from alpha
  into the theta range with long holds, using the values commonly quoted
  for those practices. A name says what a program is for; neither the
  firmware nor its documents claim that it produces an effect
  (RES-PROC-006).
  A program can be run at its own length or at 10 to 60 minutes: every
  stage keeps its share of the whole (`rc::scaledSession`).
- The frequencies a session sets are the device's frequencies while it
  runs: the screens show them, and they stay when it ends. They reach
  storage only with the next change the user makes.
- Sessions made by the user are built on the touchscreen (ADVANCED, session
  builder) and stored as text files on the card (`IMPLEMENTATION_PLAN.md`,
  Mindforge phases 2 and 5).

## Web administration (removed in 0.10.0)

Firmware 0.7.0 to 0.9.6 had a web interface in Setup mode: the Arduino
core's blocking `WebServer` served from `loop()`, one page held in flash,
Digest authentication on every request, and file management on the card
through `sdcard`. It was removed in 0.10.0 together with Setup mode and
Wi-Fi (ADR-000205); the files `net/webui.cpp`, `net/webui.h`,
`net/webpage.h`, `radio/wifi_link.cpp`, `radio/wifi_link.h` and
`radio/bt_stack.h` no longer exist. The interface as it was built is
described in `docs/history/web-api-0.9.md`.

## Tasks

| Task | State | Notes |
|---|---|---|
| Arduino loop (core 1) | exists | Interface, status, serial diagnostics |
| Bluetooth controller and host (core 0) | exists | Started at every start, unless the radio start guard skips it |
| Bluetooth audio callback (inside the stack's task) | exists | Renders the tone; no blocking, no logging |
| Player (core 1, priority 4, 6 kB stack) | exists | SD reads, WAV/MP3 decoding, resampling; waits when the ring is full |

## Open decisions

| Decision | Depends on |
|---|---|
| Second decoder for crossfade | Heap while an MP3 plays to a connected headset |
| Return of the wired output | The operator |

Closed by ADR-000205 (firmware 0.10.0): a combined mode beside Play and
Setup is no longer considered; OTA waited for the web interface, which is
gone, and firmware update stays over USB.
