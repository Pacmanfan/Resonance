# Resonance firmware

Firmware for the ESP32-2432S032C-I. Architecture:
`docs/architecture/ARCHITECTURE.md`. Requirements:
`docs/requirements/resonance.md`. Current state: the binaural tone, sound files from the card
(WAV, MP3), noise and timed sessions, mixed, over Bluetooth; no web
interface yet.

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
step, then `REPORT` lines for mode, display, touch, SD card, radios and
image size.

```bash
python3 tools/serial_capture.py --seconds 10                 # the start-up report
python3 tools/serial_capture.py --seconds 20 --send scan     # then run a command
```

Opening the serial port restarts the board; that is how the USB adapter is
wired.

| Command | Effect |
|---|---|
| `report` | Print the report again |
| `heap` | Free heap, minimum free heap, largest free block |
| `scan` | Scan for Wi-Fi networks. In Play mode this starts Wi-Fi beside Bluetooth, which is only for measuring the heap |
| `radio` | Start the current mode's radio, if it was skipped at start |
| `mktest` | Write a 5-second test tone to `CUSTOM/test.wav` on the card |
| `play <path>` | Play a file below `RESONANCE/AUDIO`, e.g. `play CUSTOM/test.wav` |
| `player` | Player state, position, buffer fill, underruns, last error |
| `stop` | Stop the player |
| `put <path> <bytes>` | Receive a file over serial; use `tools/serial_put.py` |
| `session` | Session state, stage, time left, beat frequency, level |
| `session start <n>` / `session stop` | Start program n (0 = timer) or stop with the fade |
| `bt` | Bluetooth state, the audio devices in range, and a `STREAM` line: longest gap between audio requests, longest render, stutters (gaps over 60 ms) and underruns since last asked |
| `bt connect <n>` | Connect to found device number n |
| `volume <n>` | Set the master volume in percent |
| `audio on` / `audio off` | Start or stop the tone |
| `mode play` / `mode setup` | Store the operating mode and restart |
| `wifi <ssid> <password>` | Store a Wi-Fi network for Setup mode. The password is not echoed |
| `wifi clear` | Forget the stored network |

## Listening

1. Start in Play mode (no `SETUP` marker at the top of the screen).
2. Put the headset or speaker in pairing mode.
3. On HOME tap the `BT:` button, then tap the device in the list.
4. Back on HOME, tap `OFF` to start the tone; `-` and `+` set the volume.

The device reconnects to the last headset at start. Audio never starts by
itself.

## Sound files

Copy `.wav` (PCM) and `.mp3` files into `RESONANCE/AUDIO` on the card, in
its folders (`OCEAN`, `RAIN`, `CUSTOM`, ...) or your own. On the SND tab, tap
a folder to open it and a file to play it; `UP` goes back, `<` and `>` turn
pages. The button below shows `PLAY` or `PAUSE` and the background volume;
`-` and `+` change it. Noise (white, pink, brown) is on the FREQ tab.

For a test file without removing the card:

```bash
python3 tools/serial_put.py some.mp3 CUSTOM/some.mp3   # about 10 kB per second
```

## Frequency presets

On the FREQ tab, `<` and `>` beside the primary value step through carrier
presets (100, Om 136.1, the Solfeggio set, Verdi 432, Concert 440, ...), and
the pair beside the beat value steps through beat presets (Delta, Theta, Schumann 7.8, Alpha,
SMR, Beta, Gamma). The buttons below still adjust in steps of 0.1 to 10 Hz.
The lists are in `lib/resonance_core/src/rc_presets.cpp`.

Holding any `-`, `+`, step, `<` or `>` button repeats it: after a short
pause at a steady rate, faster if you keep holding.

## Sessions

On the SESS tab choose the program (tap to cycle: Timer, then the built-in
programs), its length (the timer: 10 to 60 minutes or Continuous; a program:
its own length or 10 to 60 minutes, with every stage stretched in
proportion) and the fade at the end, then START. The audio starts with the session and stops, after
the fade, at its end. PAUSE holds the clock and the audio; STOP ends through
the fade.

## Operating modes

| Mode | Radio | For |
|---|---|---|
| Play | Bluetooth | Listening |
| Setup | Wi-Fi | Administration from a browser (not built yet) |

Change the mode on the Settings screen (tap the button twice) or with the
serial command. In Setup mode without a stored network the device opens the
access point `Resonance-Setup`; its password and address are on the
Settings screen.

## Troubleshooting

| Symptom | Cause and remedy |
|---|---|
| "Radio off: power dropped at start" on the Home screen | The board lost power three times in a row while the radio was starting, and the firmware skipped the radio once so as not to loop. Restart; if it returns, use a USB port directly on the computer, a shorter cable, or connect a battery |
| The serial port disappears and returns repeatedly | The same power loss, seen from the computer |
| Touch does nothing | Set `-DRESONANCE_TOUCH_DEBUG=1` in `platformio.ini`: every press is then printed as `TOUCH x,y` and marked on the screen. The report line `REPORT touch chip` shows what the controller says about itself |
| Screen black, serial shows "No bootable app partitions" | An upload was interrupted (the board was restarted or unplugged while flashing). Flash again and leave the board alone until it finishes |
| `SD: no card` | No card, or not FAT32. The board cannot sense a card; it finds out by trying |
| Display stays dark | Brightness cannot go below 5 %; check the report line `REPORT display` |
