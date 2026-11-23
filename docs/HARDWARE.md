# Resonance hardware

A summary of the board Resonance runs on and what the firmware uses of it.
The full audit — every fact with its source and a confidence mark, the
measurements still open, and the proposal for a wired audio output — is
[architecture/HARDWARE.md](architecture/HARDWARE.md); the pin table is
[architecture/GPIO_MAP.md](architecture/GPIO_MAP.md). Where this summary and
the audit differ, the audit is right.

## The board

**ESP32-2432S032C** (Sunton / Shenzhen Jingcai), the 3.2-inch member of the
"Cheap Yellow Display" family, in the variant with **capacitive** touch and
USB-C. Other variants (N: no touch, R: resistive) share the schematic but
are not supported by this firmware as it stands.

| Part | Detail | How known |
|---|---|---|
| Module | ESP32-WROOM-32: ESP32-D0WD-V3 rev. 3.0, dual core, 40 MHz crystal, 4 MB flash, no PSRAM | Read from the device |
| USB | USB-C through a CH340 serial bridge | Read from the device |
| Display | 3.2", 240 × 320, ST7789 controller on SPI, backlight on a PWM pin; run at a 40 MHz write clock | Works on the device; the panel does not answer identification reads |
| Touch | GT911 on I2C, address 0x5D, five points; coordinates match the display directly | Read from the device; presses by the owner landed where expected |
| microSD | SPI at 20 MHz, FAT32 | Mounted and played from on the device |
| Radio | Bluetooth Classic (A2DP source) is used. Wi-Fi exists on the chip and is never started | Used by the owner |
| RGB LED, light sensor | Present; the LED is a status light, the sensor is unused | Schematic |
| Onboard audio | A small mono amplifier on IO26 with a speaker connector (P4) | Schematic; **not used** |
| Power | USB-C, or a lithium cell on connector P5 through a charger / boost IC (unmarked, pinout of an IP5306); a push button to that IC | Schematic; the IC's marking is unconfirmed |

## Display and touch

The interface is drawn directly to the panel through LovyanGFX: there is no
frame buffer, so only what changes is redrawn. The panel cannot be read
back, which is why this repository has no screenshots. The touch
controller's interrupt line idles high on this board, so the firmware polls
it.

## SD card

The card holds sound files and the user's sessions, sequences and advanced
settings under `/RESONANCE/`. The board has no card-detect signal: a card
inserted while the device is running is seen only after a restart. The
device works without a card.

## Audio output

Audio leaves the device over **Bluetooth A2DP** (SBC) to headphones or a
speaker (ADR-000204). Not used or not built:

- The **onboard amplifier and speaker connector**: mono, and its shutdown
  pin is tied so that it cannot be switched off; the firmware holds its
  input low.
- A **wired stereo output** through an external I2S DAC: proposed in the
  audit, on hold. It would need soldering to pins the RGB LED uses.

## Power

- Runs from USB-C or from a lithium cell on P5.
- **There is no battery measurement**: the board has no connection from the
  power IC to the processor and no divider on the cell, so the firmware
  shows no battery level and cannot warn of a low cell (ADR-000204).
- **No battery protection circuit is confirmed on the board. Use a
  protected cell.**
- On first use the board repeatedly lost power at the moment the radio
  started, on USB through a hub and without a cell; it has been stable
  since. The firmware counts radio starts that did not survive and, after
  three in a row, skips the radio once and says so on the screen.

## Limits that shape the firmware

| Limit | Consequence |
|---|---|
| 4 MB flash | One 3 MB application slot, no over-the-air update; firmware 0.10.0 takes about 1.07 MB of it |
| No PSRAM | About 98 kB of heap free at rest with Bluetooth started |
| Bluetooth and an MP3 decoder together | The lowest free data memory measured while streaming an MP3 with tone, partials and noise was 22 kB; the working limit is 16 kB. The decoder's ring buffer is kept in memory the Bluetooth stack cannot use |
| Audio pipeline | Nothing on the render path may allocate, block or touch a file; one file decoder at a time |
| No frame buffer | Direct drawing, partial redraws, no GUI framework |
| No card detect, no battery gauge | See above |

## Enclosure

A printable case for this board is in [`hardware/`](../hardware/README.md),
with the reference model of the board that it was designed against and a
record of which dimensions are measured, which are taken from drawings and
which are estimates.

## Not verified

The audit lists the measurements still open. Among them: the picture at the
40 MHz display clock judged by eye, current draw and battery run time, and
whether the power IC switches off at light load on a cell.
