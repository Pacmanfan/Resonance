# Resonance firmware

Firmware for the ESP32-2432S032C-I. Architecture:
`docs/architecture/ARCHITECTURE.md`. Requirements:
`docs/requirements/resonance.md`.

Current state (0.10.0): the binaural tone, sound files from the card (WAV,
MP3), noise and timed sessions, mixed, played over Bluetooth to a headset or
speaker. An advanced menu (ADV tab, `ADVANCED`) adds a layer mixer, a
breathing pacer, a session builder, frequency sequences and the Resonance
Lab; their settings are kept in `/RESONANCE/ADVANCED.TXT` on the card.

There is one operating mode. The device always starts Bluetooth audio,
playback and sessions; everything is set on the touchscreen (tabs HOME,
FREQ, SND, SESS, ADV) and nothing needs a restart. There is no Wi-Fi and no
web interface: both, and the separate Setup mode, were removed in 0.10.0
(ADR-000205).

## Requirements

- PlatformIO Core **6.2.0 or later** (6.1.19 refuses the platform).
- A C++ compiler on the build host (`g++` or `clang++`) for the unit tests.
- The board on a USB port; it appears as a CH340 serial adapter
  (`/dev/ttyUSB0` on Linux; the user must be in the `dialout` group).

The platform and library versions are pinned in `platformio.ini`:
pioarduino 55.03.313 (Arduino core 3.3.13), LovyanGFX 1.3.0, ESP32-A2DP
1.8.12 and the Helix MP3 decoder (arduino-libhelix 0.9.4). The build warns
that the AudioTools library is missing; it is not needed.

The ESP-IDF libraries are rebuilt without BLE (`custom_sdkconfig` in
`platformio.ini`), which frees 57 kB of RAM on the device. **The first build
compiles ESP-IDF and takes several minutes.** It generates `sdkconfig.*`,
`CMakeLists.txt`, `managed_components/` and `dependencies.lock` here; they
are not tracked. After changing a `custom_sdkconfig` option, delete
`sdkconfig.resonance`, or the old one stays in force. The first
build downloads the toolchain.

## Build and flash

Run from this directory.

```bash
pio run                                        # build
pio run -t upload --upload-port /dev/ttyUSB0   # build and flash
```

Firmware is updated over USB only: the flash holds one 3 MB application slot
and no OTA slot (ADR-000204).

## Unit tests

The portable core (`lib/resonance_core`) is tested on the build host, with
no board and no PlatformIO:

```bash
python3 tools/host_tests.py
```

The same command is the suite `resonance_core` of the repository's test
runner (`python3 scripts/run_tests.py --tier t1 --suite resonance_core`).

## Serial diagnostics

115200 baud. The device prints a report at start: heap at each start-up
step, then `REPORT` lines for the firmware version, the reset reason, the
settings, display, touch, SD card, Bluetooth, the user's values and the
image size.

```bash
python3 tools/serial_capture.py --seconds 10                 # the start-up report
python3 tools/serial_capture.py --seconds 20 --send heap     # then run a command
```

Opening the serial port restarts the board; that is how the USB adapter is
wired.

| Command | Effect |
|---|---|
| `report` | Print the report again |
| `heap` | Free heap, minimum free heap, largest free block |
| `bench` | Display benchmark: drawing times at 24, 40 and 80 MHz, a buffered frame, the screen redraw, touch read, loop rate. Shows test patterns |
| `caps` | Every memory region, as ESP-IDF prints it |
| `bench audio` | Audio render benchmark: share of one core and the worst block |
| `radio` | Start Bluetooth, if it was skipped at start (see Troubleshooting) |
| `mktest` | Write a 5-second test tone to `CUSTOM/test.wav` on the card |
| `play <path>` | Play a file below `RESONANCE/AUDIO`, e.g. `play CUSTOM/test.wav` |
| `player` | Player state, position, buffer fill, underruns, last error |
| `pause` / `resume` | Pause and resume |
| `stop` | Stop the player |
| `put <path> <bytes>` | Receive a file over serial; use `tools/serial_put.py` |
| `session` | Session state, stage, time left, beat frequency, level |
| `session start <n>` / `session stop` | Start program n (0 = timer) or stop with the fade |
| `bt` | Bluetooth state, the audio devices in range, and a `STREAM` line: longest gap between audio requests, longest render, stutters (gaps over 60 ms) and underruns since last asked |
| `bt connect <n>` | Connect to found device number n |
| `volume <n>` | Set the master volume in percent |
| `audio on` / `audio off` | Start or stop the tone |
| `tap <x> <y>` / `ui` | Act as if the screen had been touched there / describe the screen now shown |

The advanced functions have commands too:

| Command | Effect |
|---|---|
| `layer` | The four mixer layers (tone, partials, noise, voice): on or off, level, fade |
| `layer <tone\|partials\|noise\|voice> on\|off`, `... level <percent>`, `... fade <seconds>` | Set one layer |
| `voice fade <in s> <out s>` | How a recording starts, stops and gives way to the next |
| `noise off\|white\|pink\|brown [percent]` | Noise type and level |
| `tone` | Tone state, frequencies and channel phases |
| `tone phase <L deg> <R deg> [seconds]`, `tone invert <L 0\|1> <R 0\|1>`, `tone partial <1\|2> <percent>` | Channel phase, inversion, and the two golden partials |
| `sweep lin\|exp <from primary> <to primary> <from beat> <to beat> <seconds>` | Build a frequency sweep as the custom session and start it |
| `descent lin\|exp <start beat> <transition s> <hold s> <beat 1> [<beat 2> ...]` | Build a stepwise descent as the custom session |
| `breath`, `breath <in s> <hold s> <out s> <hold s> [depth percent]`, `breath off` | Breathing pacer: state, start, stop |
| `seq`, `seq default`, `seq step <n> <Hz> <seconds>`, `seq remove <n>`, `seq glide <seconds>`, `seq beat <Hz>`, `seq save\|load\|delete <slot>`, `seq use` | The frequency sequence: show, reset, edit, keep on the card, turn into the custom session |
| `usession`, `usession breath ...`, `usession voice <stage> <percent> [path]`, `usession run`, `usession save\|load\|delete <slot>` | The custom session: show, give a stage a breathing pattern or a recording, start, keep on the card |
| `lab` | Both Resonance Lab settings (A and B) |
| `state`, `state save`, `state clear` | The advanced settings as kept in `/RESONANCE/ADVANCED.TXT`: show, write now, remove the file |

An unknown command prints the list. The commands `mode play`, `mode setup`,
`wifi <ssid> <password>`, `wifi clear` and `scan`, and the report lines
`REPORT mode`, `REPORT wifi` and `REPORT web`, were removed in 0.10.0 with
Setup mode and Wi-Fi (ADR-000205).

## Listening

1. Switch the device on. Bluetooth starts by itself.
2. Put the headset or speaker in pairing mode.
3. On HOME tap the `BT:` button, then tap the device in the list.
4. Back on HOME, tap `OFF` to start the tone; `-` and `+` set the volume.

The device reconnects to the last headset at start. Audio never starts by
itself.

## Sound files

Put the card in a computer and copy `.wav` (PCM) and `.mp3` files into
`RESONANCE/AUDIO`, in
its folders (`OCEAN`, `RAIN`, `CUSTOM`, ...) or your own. On the SND tab, tap
a folder to open it and a file to play it; `UP` goes back, `<` and `>` turn
pages. The button below shows `PLAY` or `PAUSE` and the background volume;
`-` and `+` change it. Noise (white, pink, brown) is on the FREQ tab.

To send a file without removing the card, use the serial line (the `put`
command):

```bash
python3 tools/serial_put.py some.mp3 CUSTOM/some.mp3   # about 10 kB per second
```

There is no upload over the network.

## Frequency presets

On the FREQ tab, `<` and `>` beside the primary value step through carrier
presets (100, Om 136.1, the Solfeggio set, Verdi 432, Concert 440, ...), and
the pair beside the beat value steps through beat presets (Delta, Theta, Schumann 7.8, Alpha,
SMR, Beta, Gamma). The buttons below still adjust in steps of 0.1 to 10 Hz.
The lists are in `lib/resonance_core/src/rc_presets.cpp`.

Holding any `-`, `+`, step, `<` or `>` button repeats it: after a short
pause at a steady rate, faster if you keep holding.

## Sessions

The built-in programs are four plain sweeps (10 to 6 Hz, 8 to 2 Hz, 14 Hz
hold, 6-10-6 Hz) and three practice programs: Trance (10 down to 4.5 Hz,
30 min), Deep trance (8 down to 3.5 Hz, 40 min) and Astral (10 down to
6.3 Hz, then 4 Hz, 45 min). The device plays the frequencies; it makes no
claim about what they do.

On the SESS tab choose the program (tap to cycle: Timer, then the built-in
programs), its length (the timer: 10 to 60 minutes or Continuous; a program:
its own length or 10 to 60 minutes, with every stage stretched in
proportion) and the fade at the end, then START. The audio starts with the session and stops, after
the fade, at its end. PAUSE holds the clock and the audio; STOP ends through
the fade.

## Settings and the advanced menu

The ADV tab (called SET before 0.13.0) has the brightness and `ADVANCED`, and shows the firmware
version and the free memory. `ADVANCED` opens a menu: LAYER MIXER,
BREATHING, SESSION BUILDER (also reached with `BUILD` on the SESS tab),
SEQUENCE and RESONANCE LAB. A `<` in the header leads back one level.

- The mixer, breathing and Lab settings are kept in
  `/RESONANCE/ADVANCED.TXT` on the card, written about four seconds after
  the last change and brought back at start. Without a card, or without the
  file, the device starts as the plain binaural device.
- Sessions made in the builder are kept as text files
  `/RESONANCE/SESSIONS/USER01.TXT` to `USER12.TXT`, sequences as
  `/RESONANCE/SEQUENCE/SEQ01.TXT` to `SEQ08.TXT`.
- The other settings (frequencies, volumes, brightness, session choices) are
  kept in the device itself. A record stored by firmware 0.9.6 or earlier is
  read once and rewritten; this removes the Wi-Fi credentials it held, and
  the start-up report says `REPORT settings repaired` for that start.

## Troubleshooting

| Symptom | Cause and remedy |
|---|---|
| "Radio off: power dropped" on the Home screen | The board lost power three times in a row while Bluetooth was starting, and the firmware skipped the radio once so as not to loop. Restart, or send the serial command `radio`; if it returns, use a USB port directly on the computer, a shorter cable, or connect a battery |
| The serial port disappears and returns repeatedly | The same power loss, seen from the computer |
| Touch does nothing | Set `-DRESONANCE_TOUCH_DEBUG=1` in `platformio.ini`: every press is then printed as `TOUCH x,y` and marked on the screen. The report line `REPORT touch chip` shows what the controller says about itself |
| Screen black, serial shows "No bootable app partitions" | An upload was interrupted (the board was restarted or unplugged while flashing). Flash again and leave the board alone until it finishes |
| `SD: no card` | No card, or not FAT32. The board cannot sense a card; it finds out by trying |
| Display stays dark | Brightness cannot go below 5 %; check the report line `REPORT display` |
