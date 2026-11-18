# Resonance GPIO Map

**Requirement:** RES-PROC-001, RES-PROC-002, RES-HW-002, RES-HW-003 (`docs/requirements/resonance.md`)
**Work:** WORK-000001 (REQ-CHAT-20261117-001)
**Status:** derived from the V1.0 schematic and the C-variant board-definition files. **Not verified on the physical board.**
**Sources and confidence marks:** `HARDWARE.md`

## Ownership table

Every GPIO of the ESP32-WROOM-32 on the ESP32-2432S032C. "Owner" is the
Resonance module that will drive the pin (`ARCHITECTURE.md`).

| GPIO | Wired to on this board | Owner in Resonance | Notes |
|---|---|---|---|
| 0 | BOOT button, 10 kΩ pull-up, auto-reset circuit | none | Strapping pin |
| 1 | UART0 TX to CH340C; P1 pin 2 through 100 Ω | serial diagnostics | Flashing and console |
| 2 | Display DC | display | Strapping pin |
| 3 | UART0 RX from CH340C; P1 pin 3 through 100 Ω | serial diagnostics | Flashing and console |
| 4 | RGB LED red, active low | **audio output: DAC soft mute (XSMT)** | Proposed, see below |
| 5 | microSD CS, 10 kΩ pull-up | SD card | Strapping pin |
| 6–11 | Module flash | none | Never usable |
| 12 | Display MISO through 0 Ω | display | Strapping pin (flash voltage): must not be pulled high at reset |
| 13 | Display MOSI | display | |
| 14 | Display SCLK | display | |
| 15 | Display CS | display | Strapping pin |
| 16 | RGB LED green, active low | **audio output: I2S LRCK** | Proposed, see below |
| 17 | RGB LED blue, active low | **audio output: I2S DIN** | Proposed, see below |
| 18 | microSD SCLK | SD card | |
| 19 | microSD MISO | SD card | |
| 21 | GT911 interrupt through 0 Ω; P3 pin 1; CN1 pin 2; 10 kΩ pull-up | touch (interrupt, optional) | Shared with two connectors |
| 22 | P3 pin 2 and CN1 pin 3 only | **audio output: I2S BCK** | The only free output-capable pin on a connector |
| 23 | microSD MOSI | SD card | |
| 25 | GT911 reset | touch | DAC1 is therefore not available |
| 26 | Onboard amplifier input (DAC2) | audio output: held low | Amplifier is always enabled |
| 27 | Backlight FET gate, 10 kΩ pull-down | display (backlight PWM) | On no connector |
| 32 | GT911 SCL, 4.7 kΩ pull-up | touch | |
| 33 | GT911 SDA, 4.7 kΩ pull-up | touch | |
| 34 | Light-sensor divider | none (reserved) | Input only; sensor may be absent |
| 35 | P3 pin 3 | battery (ADC, optional) | Input only, no pull-up, ADC1 |
| 36 | XPT2046 interrupt net of the R variant, 10 kΩ pull-up if fitted | none | Input only |
| 39 | Not connected | none | Input only |
| EN | Reset button, auto-reset circuit, display reset | none | |

GPIO 20, 24 and 28–31 do not exist on the ESP32; 37 and 38 are not brought
out by the module.

## What is free

| Pin | Output capable | Access | Cost of using it |
|---|---|---|---|
| IO22 | yes | P3 / CN1 connector | None |
| IO16, IO17, IO4 | yes | Solder to the RGB LED or its 1 kΩ resistors | The LED colour on that pin is lost; it will flicker unless the LED or resistor is lifted |
| IO21 | yes | P3 / CN1 connector | Remove R25 and poll touch without the interrupt. Nobody has documented this; the GT911 address selection changes |
| IO26 | yes | Solder at R4 | Onboard speaker path is given up; edges are amplified into a connected speaker |
| IO35 | **no** | P3 connector | Input only: usable for the battery divider or plug detection, never for I2S |
| IO34, IO36, IO39 | **no** | Solder | Input only |

No documented I2S modification of the ESP32-2432S032 itself was found. The
published ones are for the 2.8-inch ESP32-2432S028R and use IO27 from its
connector; on this board IO27 is the backlight, so they do not apply.

## Can the GPIO matrix help?

Yes, within limits. The ESP32 routes I2S BCK, WS and data out through the
GPIO matrix to **any output-capable pin**, at these bit-clock rates without
penalty. So the choice of which of the free pins carries which I2S signal is
free. The matrix cannot make an input-only pin (34–39) an output, and it
cannot free a pin that a peripheral on the board is wired to.

## I2S wiring proposal

Three signals are needed; the PCM5102A needs no master clock (`HARDWARE.md`
section 9).

| Signal | GPIO | Where to connect | PCM5102A pin |
|---|---|---|---|
| BCK (bit clock) | IO22 | CN1 pin 3 (or P3 pin 2) | BCK |
| LRCK (word select) | IO16 | Green LED cathode or its 1 kΩ resistor | LCK |
| DIN (data) | IO17 | Blue LED cathode or its 1 kΩ resistor | DIN |
| Soft mute (optional) | IO4 | Red LED cathode or its 1 kΩ resistor | XSMT |
| 3.3 V | — | CN1 pin 1 | VIN |
| Ground | — | CN1 pin 4 | GND, and SCK tied to GND |

Why this set:

- It leaves the display, the touch controller (including its interrupt), the
  SD card and the serial console untouched (RES-HW-002).
- One of the three signals, the supply and the ground come from a connector
  without soldering.
- IO16 and IO17 are free on a WROOM-32 because the module has no PSRAM.
- None of the four pins is a strapping pin, so the DAC cannot disturb boot.
- IO21 stays the touch interrupt and IO35 stays free for the battery divider.

What it costs:

- Two solder joints on the board for I2S, a third for soft mute.
- The RGB LED is no longer a status light. The green and blue elements will
  glow with the I2S signals unless their resistors (R17, R20 per the
  schematic; confirm on the board) are lifted.

**Status of the proposal:** consistent with the schematic and with all three
board-definition files. It becomes verified only when the Stage 3 test
measures the three signals at the DAC and the display, touch and SD tests
still pass with the DAC attached. Measurement M8 in `HARDWARE.md` (solderable
access to the LED pins) decides whether it can be built as drawn.

### Alternative without any LED pin

If soldering at the LED is not acceptable: BCK IO22, LRCK IO21 (R25 removed,
touch polled), DIN IO26 (soldered at R4). This still needs soldering, changes
the touch controller's address selection, and has no precedent. It is not
recommended.

## Conflicts and required modifications

| Conflict | Resolution |
|---|---|
| The board has only one free output-capable connector pin (IO22); I2S needs three | Reuse the RGB LED pins (two or three solder joints) |
| The onboard amplifier is always enabled and listens to IO26 | Firmware holds IO26 low; leave P4 unconnected |
| No battery measurement on the board | Optional external divider from BAT+ to IO35 |
| No card-detect signal | Detect removal by probing, in the storage module |
| IO21 is on two connectors and is also the touch interrupt | Nothing may be attached to IO21 on CN1 or P3 |
