# Resonance Firmware Architecture

**Requirement:** RES-ARCH-001..005 and the domains they structure (`docs/requirements/resonance.md`)
**Decision:** ADR-000204 (Bluetooth-only output, space first, operating modes)
**Work:** WORK-000001 (design), WORK-000002 and WORK-000003 (Stage 2), WORK-000004 (Stage 3)
**Status:** Stages 2 and 3 are implemented: board, display, touch, SD, settings, operating modes, interface, the binaural tone engine and the Bluetooth audio output. The tone has not yet been heard through a headset. Sections marked *planned* describe code that does not exist yet.
**Inputs:** `HARDWARE.md`, `GPIO_MAP.md`, `LIBRARY_EVALUATION.md`

## Principles

1. **Audio first.** The audio path has the highest priority and never waits
   for storage, network, GUI or logging (RES-AUDIO-012).
2. **One owner per resource.** Each bus, peripheral and buffer belongs to
   exactly one module (RES-ARCH-001).
3. **Commands in, state out.** The interface and the web server change the
   device only by reporting events or posting commands; they read it only
   from a status snapshot (RES-ARCH-002).
4. **Portable core.** Everything that is arithmetic or logic compiles and is
   tested on a PC. Only thin adapters touch the hardware.
5. **Space first.** The smaller means is used, and a subsystem the current
   mode does not need is never started (RES-ARCH-005).

## Operating modes

The measured heap decides this design: with Bluetooth and Wi-Fi both
running, 50 kB remain before any audio code exists
(`LIBRARY_EVALUATION.md`, "Measured"). So each mode starts one radio.

| | Play | Setup |
|---|---|---|
| Purpose | Listening | Administration |
| Radio | Bluetooth Classic (A2DP) | Wi-Fi (station, or the set-up access point) |
| Runs | Audio engine, SD playback, sessions, interface | Web server, file management, interface |
| Never started | Wi-Fi | Bluetooth; its reserved memory is returned to the heap |
| Free heap at start, measured | 84 kB with A2DP started and scanning | 157 kB |

- The mode is a field of the settings record. It changes only through a
  restart, requested on the Settings screen (two taps) or by the serial
  command `mode play` / `mode setup`. A restart takes about two seconds.
- Stopping a radio at run time is not used: ESP-IDF releases Bluetooth
  memory one way only, and start/stop cycles fragment the heap.
- The header of every screen shows the mode.
- Consequence: in Setup mode there is no audio, so the web interface manages
  files, presets, sessions and settings but cannot control live playback.
  This limits RES-WEB-003 and is reported as RES-PROC-005 requires.

### Radio start guard

Starting a radio draws a current surge. On the first start the board lost
power sixteen times in a row (`HARDWARE.md`, section 8), which without a
guard is an endless restart loop. So:

- an attempt counter in non-volatile storage is raised just before a radio
  starts and cleared after three seconds of stable running;
- a start that finds three failed attempts in a row does not start the
  radio, shows "Radio off: power dropped at start" on the Home screen, and
  clears the counter, so the following start tries again;
- one or two failed attempts are tolerated, because a reset from the USB
  adapter or the reset button during those seconds cannot be told from a
  power loss (both read as a power-on reset).

The device therefore always reaches a usable screen.

## Layers

```text
┌──────────────────────────────────────────────────────────────────┐
│ Presentation      gui/ (screens, widgets)    web/ (planned)      │
└───────────────┬──────────────────────────────────┬───────────────┘
                │ events                    status │
┌───────────────▼──────────────────────────────────▼───────────────┐
│ Application       main.cpp: mode, status snapshot, serial        │
│                   SessionManager, PresetManager (planned)        │
└───────────────┬──────────────────────────────────────────────────┘
                │
┌───────────────▼──────────────────────────────────────────────────┐
│ Core (portable)   rc_settings: settings, ranges, validation      │
│                   rc_tone: oscillators, ramps, tone engine       │
│                   mixer, limiter, sweep (planned)                │
└───────────────┬──────────────────────────────────────────────────┘
                │
┌───────────────▼──────────────────────────────────────────────────┐
│ Platform          hal/ board · gui/display · storage/ · radio/   │
│                   audio/bt_audio (A2DP) · decoders (planned)     │
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
  src/
    main.cpp                  start-up, mode, status, serial diagnostics
    version.h
    hal/board_pins.h          the only place with pin numbers
    hal/board.h/.cpp          quiet pins, status LED, reset reason
    gui/display.h/.cpp        ST7789 + GT911 + backlight (LovyanGFX)
    gui/theme.h               colours and layout
    gui/ui.h/.cpp             five screens, tab bar, buttons, dimming
    storage/settings_store    settings in NVS; radio start guard
    storage/sdcard            the card and its SPI bus; /RESONANCE tree
    audio/bt_audio            Bluetooth A2DP source: scan, connect, audio feed
    radio/bt_stack            Bluetooth memory hook and release (Setup mode)
    radio/wifi_link           station, set-up access point, mDNS
  tests/                      host tests of the portable core
  tools/host_tests.py         builds and runs them with the host compiler
  tools/serial_capture.py     reads the device's serial report
```

Modules named in the request that do not exist yet arrive with their stage
(`IMPLEMENTATION_PLAN.md`). `BatteryManager` and `I2SAudioOutput` are not
planned (ADR-000204).

## Settings and storage

- One fixed-size settings record in NVS, so the device works without a card
  (`rc::Settings`). Every load passes `rc::sanitize()`: a record of another
  version, an unknown mode, an out-of-range or non-finite frequency, an
  unterminated text field are all corrected to safe values (RES-CFG-002).
- The start-up volume is capped at 60 %, so a restart cannot come back loud
  (RES-CFG-003). Nothing plays at start.
- Edits made on the touchscreen are stored when the user leaves the screen,
  not on every tap, to spare the flash.
- The set-up access point's password is generated once from the hardware
  random generator and shown on the Settings screen. It is not derived from
  anything the device broadcasts.
- Wi-Fi passwords are never printed or shown (RES-NET-004).
- `sdcard` owns the card. It creates the `/RESONANCE` tree (RES-PLAY-004).
  The board has no card-detect signal, so a failing operation is treated as
  a possible removal and the card is reported as absent.
- *Planned:* presets, sessions and playlists as JSON files on the card,
  written to a temporary name and renamed.

## Interface

- LovyanGFX draws directly to the panel; there is no GUI framework and no
  frame buffer. Screens are redrawn only where something changed.
- Five tabs: Home, Frequency Studio, Sound Library, Sessions, Settings
  (RES-GUI-001). The tab bar is a row of five buttons, each outlined
  and filled; the open one is filled with the highlight colour. Controls that are not built yet say so on the screen.
- Home has the audio on/off control with the volume beside it, and a
  Bluetooth button that opens the device list: found devices as buttons,
  tap one to connect, and Disconnect when connected.
- Contrast comes first: near-black background, white text, bright outlines
  on every control, and no text smaller than 12 pixels. The first scheme
  (dim grey on dark blue) was unreadable on the panel.
- Touch is polled over I2C on every pass of the loop. The controller's
  interrupt line is not used: it idles high on this board, and the driver
  reads only while it is low.
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
 implemented:   ToneEngine (left f0, right f0 + fb) ──> Bluetooth A2DP
 planned:       + background player and noise through a mixer and limiter
```

- **44.1 kHz stereo, 16-bit at the output**, as A2DP fixes it. Internal
  samples are 32-bit float; the ESP32 has a single-precision FPU.
- Two independent oscillators, each a 32-bit phase accumulator read through
  a 1024-entry sine table with linear interpolation. A frequency change
  alters only the phase increment, so it is phase-continuous by
  construction; the change itself glides over 50 ms.
- Left carries f0, right carries f0 + fb. The channels are never summed.
- Volume (square law) and on/off are ramped: 50 ms for a volume step,
  500 ms for start and stop. When off, the oscillators rest at phase zero,
  so the next start rises from a zero crossing.
- The tone is limited to half of full scale, which leaves headroom for the
  layers to come, and one bit of triangular dither is added.
- **The Bluetooth stack pulls audio through a callback, and the callback
  renders directly from the tone engine.** There is no render task and no
  ring buffer yet: rendering two sines is far cheaper than the callback's
  period, never blocks, allocates or logs (RES-AUDIO-012), and costs no
  extra RAM. The interface passes parameters to the engine through atomic
  values, which the engine picks up once per block.
- `audio/bt_audio` scans for audio devices, lists up to five by name,
  connects to the one chosen on the screen, and lets the library reconnect
  to the last device at start. Link keys stay in the Bluetooth stack's own
  storage (RES-BT-003).
- Audio is off at every start; only the Home screen's control or the serial
  command turns it on (RES-CFG-003).
- *Planned (Stage 4):* a decoder task reads the card and decodes into a ring
  buffer; a mixer and limiter combine it and the noise layer with the tone.
  An empty ring silences only the background layer.
- `AudioOutput` as a formal interface arrives when a second output exists;
  until then the engine's `render()` is the boundary, and nothing in the
  core knows about Bluetooth.

## Web administration (*planned*, Stage 7, Setup mode only)

- Asynchronous server, REST under `/api/v1/`, assets gzipped in the image.
- Handlers hold no reference to hardware objects; file operations go through
  `sdcard`.
- Authentication for everything but the login page; uploads to a temporary
  file, signature checked, renamed on success; one portable function
  validates every path.

## Tasks

| Task | State | Notes |
|---|---|---|
| Arduino loop (core 1) | exists | Interface, status, serial diagnostics |
| Bluetooth controller and host (core 0) | exists | Started only in Play mode |
| Wi-Fi and network (core 0) | exists | Started only in Setup mode |
| Bluetooth audio callback (inside the stack's task) | exists | Renders the tone; no blocking, no logging |
| Decoder (core 1, below audio) | planned | SD reads and MP3/WAV decoding |

## Open decisions

| Decision | Depends on |
|---|---|
| A combined mode beside Play and Setup | A2DP heap in Stage 3; unlikely at 50 kB |
| Custom library build without BLE and HFP | Whether Play mode runs short of heap with the decoder |
| Second decoder for crossfade | Play-mode heap in Stage 4 |
| Return of the wired output | The operator |
