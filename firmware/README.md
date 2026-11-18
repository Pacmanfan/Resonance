# Resonance firmware

Firmware for the ESP32-2432S032C-I. Architecture:
`docs/architecture/ARCHITECTURE.md`. Requirements:
`docs/requirements/resonance.md`. Current state: Stage 3 — the binaural tone plays over
Bluetooth; no sound files, sessions or web interface yet.

## Requirements

- PlatformIO Core **6.2.0 or later** (6.1.19 refuses the platform).
- A C++ compiler on the build host (`g++` or `clang++`) for the unit tests.
- The board on a USB port; it appears as a CH340 serial adapter
  (`/dev/ttyUSB0` on Linux; the user must be in the `dialout` group).

The platform and library versions are pinned in `platformio.ini`:
pioarduino 55.03.313 (Arduino core 3.3.13), LovyanGFX 1.3.0 and ESP32-A2DP
1.8.12. The build warns that the AudioTools library is missing; it is not
needed. The first
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
| `bt` | Bluetooth state and the audio devices found so far |
| `bt connect <n>` | Connect to found device number n |
| `audio on` / `audio off` | Start or stop the tone |
| `mode play` / `mode setup` | Store the operating mode and restart |
| `wifi <ssid> <password>` | Store a Wi-Fi network for Setup mode. The password is not echoed |
| `wifi clear` | Forget the stored network |

## Listening

1. Start in Play mode (`PLAY` at the top right of the screen).
2. Put the headset or speaker in pairing mode.
3. On HOME tap the `BT:` button, then tap the device in the list.
4. Back on HOME, tap `OFF 30%` to start the tone; `-` and `+` set the volume.

The device reconnects to the last headset at start. Audio never starts by
itself.

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
| `SD: no card` | No card, or not FAT32. The board cannot sense a card; it finds out by trying |
| Display stays dark | Brightness cannot go below 5 %; check the report line `REPORT display` |
