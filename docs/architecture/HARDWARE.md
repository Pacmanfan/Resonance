# Resonance Hardware Audit

**Requirement:** RES-PROC-001, RES-PROC-002, RES-HW-001..006 (`docs/requirements/resonance.md`)
**Work:** WORK-000001 (REQ-CHAT-20261117-001)
**Status:** Stage 1 audit from documents, with the first readings from the board added on 2026-11-17 (section "Measured on the board"). Everything else is still unmeasured.
**Decisions since the audit:** ADR-000204 — the external DAC, amplifier and jack (section 9) are **on hold**, and battery measurement is **dropped**.
**Pin table:** `GPIO_MAP.md`

## How to read this document

Every fact carries one of these confidence marks:

| Mark | Meaning |
|---|---|
| **S** | Read from the manufacturer schematic (V1.0, 2023-02-03, title block "ESP32-2432S032", Shenzhen Jingcai) |
| **B** | Stated by a board-definition file written for the **C** variant (rzeldent, CircuitPython, openHASP) |
| **F** | Forum-grade or secondary source; may be wrong |
| **U** | Unconfirmed: needs inspection or measurement on the board (see "Required measurements") |

Three limits apply to everything marked **S**:

- One drawing covers the N (no touch), R (resistive) and C (capacitive)
  variants. It shows both the XPT2046 resistive circuit and the GT911
  connector; it does not say which parts are fitted on a C board.
- It is revision V1.0 from 2023. The board in hand may be a later revision.
- Several parts carry no part number or value: the audio amplifier (U5), the
  power IC (U8) and every resistor and capacitor of the audio input network.

## Measured on the board

Read from the device's own start-up report over USB serial on 2026-11-17
(firmware 0.2.0, evidence VER-000002). These are the only facts in this file
that come from the board in hand.

| Item | Reading | Closes |
|---|---|---|
| Processor | ESP32-D0WD-V3 revision 3.0, dual core, 40 MHz crystal, 4 MB flash | Section 1 |
| USB bridge | CH340 (USB id 1a86:7523) | Section 1 |
| Touch controller | Answers on IO33/IO32 at address **0x5D** | M6 |
| Display | Initialises without error at 24 MHz, SPI mode 0. **Read-back returns zero** (display ID and status), so MISO on IO12 does not deliver data on this board. Whether the picture and colours are right has not been confirmed by eye | M7, partly |
| microSD | Mount failed with "card failed" on command 0: no card answered. Not yet tested with a card inserted | — |
| Bluetooth Classic | Starts and stays up | — |
| Wi-Fi | Access point starts; a scan found 8 networks | — |
| Supply at radio start | **The board lost power 16 times in 37 seconds** while Bluetooth was first being started (the USB bridge dropped off the bus each time), then started normally and has been stable since. See section 8 | — |

## Sources

| Source | What it is |
|---|---|
| [MCU sheet](https://raw.githubusercontent.com/rzeldent/platformio-espressif32-sunton/main/assets/schematics/ESP32-2432S032-MCU-V1.0.jpg), [LCM sheet](https://raw.githubusercontent.com/rzeldent/platformio-espressif32-sunton/main/assets/schematics/ESP32-2432S032-LCM-V1.0.jpg) | Manufacturer schematic, as republished by rzeldent |
| [rzeldent `esp32-2432S032C.json`](https://raw.githubusercontent.com/rzeldent/platformio-espressif32-sunton/main/esp32-2432S032C.json) | PlatformIO board definition for the C variant |
| [CircuitPython `sunton_esp32_2432S032C`](https://github.com/adafruit/circuitpython/tree/main/ports/espressif/boards/sunton_esp32_2432S032C) | `pins.c`, `board.c`, `mpconfigboard.h` for the C variant |
| [openHASP `esp32-2432s032.ini`](https://raw.githubusercontent.com/HASwitchPlate/openHASP/master/user_setups/esp32/esp32-2432s032.ini) | Environment `esp32-2432s032c_4MB` |
| [mischianti.org](https://mischianti.org/esp32-2432s032-3-2%e2%80%b3-sunton-cheap-yellow-display-v2-pinout-datasheet-schema-and-specs/), [espboards.dev](https://www.espboards.dev/esp32/cyd-esp32-2432s032/) | Secondary summaries. Both contain errors (below) and are used only where marked **F** |
| [witnessmenow PINS.md](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md), [hexeguitar ESP32_TFT_PIO](https://github.com/hexeguitar/ESP32_TFT_PIO) | The **2.8-inch resistive ESP32-2432S028R**, a different board. Used only for I2S precedent |

Not obtained: the manufacturer download package (`3.2inch_ESP32-2432S032.zip`
on jczn1688.com now answers with an HTML page, not the file), and therefore
any schematic newer than V1.0 and the manufacturer's example code.

## 1. Board identity

- ESP32-WROOM-32 module: dual-core ESP32, 240 MHz, 4 MB flash, no PSRAM. **B**
- 3.2-inch 240 × 320 IPS panel, capacitive touch. **B**
- USB: the schematic and the rzeldent photo show Type-C with a CH340C
  USB-serial bridge; the rzeldent README table says micro USB. **S**, conflicting **F**
- It is **not** pin-compatible with the 2.8-inch ESP32-2432S028R. On that
  board IO21 is the backlight and IO27 is on a connector; here the backlight
  is IO27 and IO27 is on no connector. Recipes for the 2.8-inch board do not
  transfer.
- It is **not** pin-compatible with the resistive 3.2-inch ESP32-2432S032R
  either: that variant puts an XPT2046 on the display SPI bus with CS on IO33.

## 2. Display

| Item | Value | Mark |
|---|---|---|
| Controller | ST7789 | **B** (the schematic labels only "3.2_TFT_MCU") |
| Bus | 4-wire SPI on the HSPI pins | **S** |
| SCLK / MOSI / MISO | IO14 / IO13 / IO12 (MISO through R24, 0 Ω) | **S** |
| CS / DC | IO15 / IO2 | **S** |
| Reset | No GPIO. The panel reset is tied to the ESP32 EN net | **S**, **B** |
| Backlight | IO27 drives an N-FET gate (10 kΩ pull-down), active high, PWM-capable | **S** |
| Inversion | On (IPS panel) | **B** |

Open points:

- **Colour order:** rzeldent declares RGB, CircuitPython's init sequence sets
  the BGR bit. **U** — decide on the panel.
- **SPI mode and clock:** rzeldent uses mode 3 at 24 MHz, CircuitPython mode 0
  at 26.6 MHz, openHASP 65 MHz write. **U** — start at 24 MHz and raise it
  only with a test.
- espboards.dev says some units carry an ILI9341. **F**, unconfirmed.
- mischianti.org gives a reset GPIO of 15 or 35. The schematic contradicts
  it: IO15 is CS and IO35 goes only to connector P3.

## 3. Capacitive touch

| Item | Value | Mark |
|---|---|---|
| Controller | GT911 | **B** |
| I2C SDA / SCL | IO33 / IO32, 4.7 kΩ pull-ups to 3.3 V | **S**, **B** |
| Reset | IO25 | **S**, **B** |
| Interrupt | IO21 through R25 (0 Ω) | **S** |
| Address | 0x5D (rzeldent); openHASP notes "0x5D or 0x14" | **B**, **U** |

- The GT911 selects its address from the level of INT at reset, and IO21 has
  a 10 kΩ pull-up. The firmware must probe both addresses.
- **The touch I2C bus is not on the external connectors.** The request
  describes IO21/IO22 as "associated with the capacitive touch interface".
  Per the schematic only IO21 is, and only as the interrupt line; the bus
  itself is IO33/IO32. IO22 has no on-board function.
- Touch can be polled without the interrupt; openHASP does so.

## 4. microSD

- SPI, not SDMMC, on the VSPI pins: CS IO5, SCLK IO18, MOSI IO23, MISO IO19,
  with 10 kΩ pull-ups. **S**, **B**
- A separate bus from the display. Display traffic and SD traffic do not
  contend for one SPI host. **S**
- Card-detect is not routed to a GPIO: removal can be detected only by a
  failing read or a periodic probe. **S**

## 5. Onboard audio

- IO26 (DAC2) feeds amplifier U5 through an RC network. **S**, **B**
- U5 has the 8002-family pinout. The part is unlabelled on the schematic;
  rzeldent names FM8002A, mischianti.org names PAM8002A, the request names
  NS8002. **U** — read the marking.
- **The amplifier cannot be switched off.** Its SHUTDOWN pin is tied to
  ground, so it is always enabled. **S**
- The output is bridge-tied to P4 (2-pin, 1.25 mm). Both pins are driven;
  neither is ground (RES-HW-004). **S**
- The values of the input network (R4, R7, R8, R9, C10, C11, C12), and so the
  gain, are not printed. **U**

Consequences for Resonance:

- The onboard path is mono, 8-bit DAC, and unsuitable for binaural audio. It
  is not used.
- Because the amplifier is always on, IO26 must be held at a steady level so
  that nothing is amplified into a connected speaker. The firmware drives
  IO26 low at start, as CircuitPython does for this board. With no speaker
  on P4 the amplifier is inaudible either way.

## 6. RGB LED and light sensor

- RGB LED: red IO4, green IO16, blue IO17, common anode through 1 kΩ each,
  **active low**. **S**, **B**
- Light sensor: IO34 at the midpoint of a 1 MΩ / 1 MΩ divider with a
  photoresistor across the lower half. **S**. Whether it is fitted on a given
  C board is **U** (CircuitPython: "not present on all boards").

## 7. Connectors

Pin numbers as on the schematic.

| Connector | Pin 1 | Pin 2 | Pin 3 | Pin 4 |
|---|---|---|---|---|
| P3 (4-pin 1.25 mm) | IO21 | IO22 | IO35 | GND |
| CN1 (4-pin 1.25 mm) | 3.3 V | IO21 | IO22 | GND |
| P1 (4-pin 1.25 mm) | VIN | TX (GPIO1, 100 Ω) | RX (GPIO3, 100 Ω) | GND |
| P4 (2-pin 1.25 mm) | amplifier out 1 | amplifier out 2 | | |
| P5 (2-pin 1.25 mm) | BAT− | BAT+ | | |

Against the connectors listed in the request:

- `GND, IO35, IO22, IO21` is P3 read from pin 4. Confirmed.
- `GND, IO22, IO21, 3.3V` is CN1 read from pin 4. Confirmed.
- `+5V, GND, TX, RX` does **not** match P1 on the schematic, which has
  VIN, TX, RX, GND with ground at the opposite end from VIN. The board in
  hand may be a different revision or the silkscreen may be read
  differently. **Check with a meter before wiring anything to P1.**

## 8. Power and battery

- U8 is unlabelled; its pinout matches the IP5306 and secondary sources name
  it. **S** (pinout), **U** (marking)
- **No battery measurement exists.** There are no I2C lines from U8 to the
  ESP32, its LED outputs are unconnected, and the only ADC divider on the
  board is the light sensor. **S**
- Power path: USB VBUS → diode → U8 → two AMS1117-3.3 regulators, one for
  the ESP32 and one for display, SD, amplifier and connectors. **S**
- A push button (SW1) goes to the power IC's KEY pin. **S**
- **Measured:** on first use the board repeatedly lost power at the moment
  the radio started, on USB power through a hub and without a battery. A
  first radio start runs a full calibration with a high current peak; the
  supply did not carry it. Later starts have been stable. The cause is not
  established: a weak USB port or cable, or the power IC running without a
  cell, are the candidates. The firmware guards against the resulting restart
  loop (`ARCHITECTURE.md`, "Radio start guard"). A directly connected USB port
  or a connected battery is the thing to try if it recurs.
- P1 VIN is on the USB side of the power IC. Whether it carries voltage on
  battery alone is inferred, not measured. **U**
- The IP5306 family switches its output off at light load (reported as about
  45 mA for 32 s). **F** for this board. Resonance draws far more than that
  while running, but deep sleep on battery may power the board off entirely.
- No battery protection circuit is confirmed. A protected cell or an external
  protection board is required (RES-HW-005).
- Battery polarity at P5 must be checked with a meter before a cell is
  connected; 1.25 mm battery leads are not wired to one standard.

### Battery monitoring options

**Dropped by ADR-000204.** No divider is fitted and the firmware shows no
battery status. The table is kept as the record of what was possible.

| Option | What it needs | Verdict |
|---|---|---|
| Read the power IC | I2C lines to U8 | Not possible: none are routed |
| External divider to IO35 | Two resistors (for example 220 kΩ / 100 kΩ) from BAT+ to P3 pin 3, a small capacitor at the pin | Feasible. IO35 is input-only, on ADC1 (usable while Wi-Fi runs) and on a connector. Requires a soldered tap on BAT+ |
| External divider to IO34 | As above, replacing the light sensor | Feasible, but needs rework on the board and loses the sensor |
| No measurement | Nothing | The default until a divider is fitted |

Battery status is therefore **not available** in the firmware (RES-HW-005),
and an imminent brown-out cannot be detected before an SD write (RES-HW-006).

## 9. External stereo audio hardware

**On hold (ADR-000204).** Audio is delivered over Bluetooth and none of this
hardware is connected. The section is kept for when the wired output returns.

Wired output per RES-HW-003. The GPIO assignment is in `GPIO_MAP.md`.

```text
ESP32 (3 I2S signals) ──> PCM5102A DAC module ──> headphone amplifier ──> 3.5 mm TRS jack
                               │ line level (about 2.1 V RMS full scale)
                               └──────────────> optional line-out jack
```

### PCM5102A module

The common purple "GY-PCM5102" module is assumed; its strap pads must be
checked, because modules ship with different defaults.

| Module pin | Connect to | Why |
|---|---|---|
| VIN | 3.3 V (CN1 pin 1) | The 5 V net is on the USB side only (section 8) |
| GND | GND | Single ground reference, short wire |
| BCK, LCK, DIN | The three I2S GPIOs | See `GPIO_MAP.md` |
| SCK | GND | No master clock: the PCM5102A derives it from BCK |
| FMT | Low | I2S format |
| FLT, DEMP | Low | Normal filter, no de-emphasis |
| XSMT | High, or a GPIO | Soft mute. A GPIO here gives a silent start and stop |

### Headphone amplifier

The PCM5102A is a line driver, not a headphone driver. A PAM8403 is a
speaker amplifier and is excluded (RES-HW-003).

| Candidate | Notes |
|---|---|
| MAX97220A | Ground-referenced "DirectDrive" output: no output coupling capacitors. Suggested by the request. Breakout availability must be checked before ordering |
| TPA6132A2 | Ground-referenced, selectable gain, low pop. Breakouts exist |
| TPA6120A2 | High quality but needs a split supply: unsuitable here |

The choice is open until a part is ordered: see `IMPLEMENTATION_PLAN.md`.
Whatever is chosen, the full-scale output must be limited (gain strap and
the firmware's volume limit, RES-AUDIO-008).

### Supply and grounding

- Power the DAC and amplifier from the board's 3.3 V peripheral rail only
  after measuring the rail under load: that AMS1117 also feeds the display,
  the SD card and the onboard amplifier. **U**
- Add a 10 µF and a 100 nF capacitor at the DAC and at the amplifier supply
  pins if the modules lack them.
- Run one ground wire from the board to the DAC; take the amplifier and jack
  grounds from the DAC, not from a second point on the board.
- Keep the I2S wires short (under 10 cm) and away from the antenna.

### Preliminary bill of materials

| Item | Qty | Notes |
|---|---|---|
| PCM5102A I2S DAC module | 1 | Check the strap pads |
| Stereo headphone amplifier module (MAX97220A or TPA6132A2) | 1 | Open choice |
| 3.5 mm TRS jack, panel mount, optionally with a switch contact | 1 | The switch contact allows plug detection later |
| 4-pin 1.25 mm pre-crimped leads | 2 | For CN1 and P3 |
| Protected single-cell LiPo with a 1.25 mm 2-pin lead | 1 | Verify polarity |
| Resistors 220 kΩ and 100 kΩ, capacitor 100 nF | 1 each | Battery divider, optional |
| Capacitors 10 µF and 100 nF | 2 each | Decoupling if not on the modules |

## Required measurements

To be made on the board before or during Stage 2. Each one closes a **U**
above.

| # | Check | Closes |
|---|---|---|
| M1 | Markings on U5 (amplifier) and U8 (power IC) | Sections 5, 8 |
| M2 | P1 pin order and voltage on VIN, on USB and on battery | Sections 7, 8 |
| M3 | P5 polarity | Section 8 |
| M4 | 3.3 V rail voltage with display, SD and DAC running | Section 9 |
| M5 | Whether U3 (XPT2046), R3 and the light sensor are fitted | Sections 3, 6 |
| M6 | ~~I2C scan on IO33/IO32~~ **Done: 0x5D** | Section 3 |
| M7 | Display colour order and picture, **by eye**; highest reliable clock. (Initialises at 24 MHz mode 0; no read-back) | Section 2 |
| M8 | Solderable access to IO16, IO17 and IO4 — only if the wired output returns | `GPIO_MAP.md` |
| M9 | Board revision printed on the PCB, against schematic V1.0 | All of **S** |
| M10 | Behaviour on battery: run time, cut-off, light-load shutdown | Section 8 |
| M11 | Power at radio start: repeat a cold start on a direct USB port and with a battery connected | Section 8 |
| M12 | microSD with a card inserted | Section 4 |
