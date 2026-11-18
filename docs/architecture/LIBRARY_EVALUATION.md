# Resonance Library Evaluation

**Requirement:** RES-PROC-001, RES-PROC-002, RES-PROC-005 (`docs/requirements/resonance.md`)
**Work:** WORK-000001 (REQ-CHAT-20261117-001)
**Status:** Stage 1 evaluation from published sources, researched 2026-11-17, with the first build and heap measurements added (section "Measured"). The rest has not been compiled or measured for this project.
**Decisions since the audit:** ADR-000204 — no LVGL, no I2S output, one application slot without OTA, exclusive operating modes.

Versions and dates were read from each project's release page or repository
on the research date. Items marked *unverified* could not be confirmed from a
primary source. Memory figures are reports by other users, mostly on Arduino
core 2.0.x, unless stated otherwise; none is a measurement of Resonance.

## Summary

- **The limiting resource is internal RAM**, not flash and not library
  availability. Wi-Fi together with Bluetooth Classic A2DP on an ESP32
  without PSRAM is described as "tight" or "not reliable" by every source
  found, including the A2DP library's own documentation. Adding a GUI, an
  MP3 decoder and a web server on top is not a documented working
  configuration.
- **Dual-slot OTA is in doubt** on 4 MB of flash. The A2DP library itself
  recommends the 3 MB single-slot layout.
- **Since measured:** the reduced library build recovers 57 kB in Play mode
  (section "Reduced library build").
- A workable stack exists for every feature taken alone. Whether the
  features can run **at the same time** is the open question, and only
  measurement on the board answers it (`IMPLEMENTATION_PLAN.md`, gates G1–G3).

## Measured

Firmware 0.2.0 (Stage 2: display, touch, SD, settings, both radio stacks, the
interface shell; no audio, no decoder, no web server) on the board,
2026-11-17, stock libraries of core 3.3.13. Evidence VER-000002.

| State | Free heap | Largest free block |
|---|---|---|
| At start | 183 kB | 111 kB |
| Display and interface running | 177 kB | 111 kB |
| **Play mode:** Bluetooth Classic host started (no A2DP yet) | 101 kB | 94 kB |
| **Setup mode:** Bluetooth memory released | 218 kB | 111 kB |
| **Setup mode:** Wi-Fi access point started | 157 kB | 111 kB |
| Bluetooth host **and** Wi-Fi station together | **50 kB** | **47 kB** |
| **Play mode, firmware 0.3.0:** A2DP source started and scanning, tone engine loaded | 84 kB | 78 kB |

- The combined figure confirms the published reports: with both radios up,
  50 kB remain before A2DP, the decoder, the mixer and a web server are
  added. The exclusive modes of ADR-000204 are therefore the working design,
  not a fallback. Gate G1 is answered for stock libraries.
- The display and the interface of our own cost about 6 kB of heap. LVGL
  was budgeted at 34-42 kB.
- ESP32-A2DP 1.8.12 builds and runs on core 3.3.13 without the AudioTools
  library (it prints a build warning about it; nothing of it is needed for
  the source with a frame callback).
- **Image size: 1,877,824 bytes** at 0.2.0 and about 1.89 MB at 0.3.0 with
  A2DP and the tone engine, 60 % of the 3 MB slot, already holding both
  radio stacks. It would leave under 90 kB in a 1.875 MB OTA slot, so
  OTA is not possible without removing BLE and HFP from the libraries.

### Reduced library build (firmware 0.4.0)

With sound playback added, the stock libraries left too little: 38 kB free
in Play mode at start and 29 kB while a WAV file played, against about
33 kB that the MP3 decoder allocates. The libraries are therefore rebuilt
through pioarduino's `custom_sdkconfig` with Bluetooth Classic only (no BLE,
no BLE Mesh, no hands-free or serial-port profile). Measured on the board,
same firmware source, 2026-11-17 (evidence VER-000007):

| Play mode | Stock libraries | Reduced libraries |
|---|---|---|
| Free heap after Bluetooth A2DP start | 42 kB | 99 kB |
| While a WAV file plays | 29 kB | 86 kB |
| While an MP3 file is decoded | not possible | 53 kB (largest block 30 kB) |
| Image size | 1.94 MB | 1.58 MB |

- Setup mode with the reduced libraries: Wi-Fi starts and scans, 144 kB free.
- With a headset connected and an MP3 streaming to it (0.5.0): 45 kB free,
  minimum 36 kB.
- The image would now fit a 1.875 MB OTA slot with about 290 kB to spare,
  before the web server is added. OTA stays off (ADR-000204); the question
  can be reopened when the web interface exists.
- The MP3 decoder is the Helix decoder (arduino-libhelix 0.9.4, pinned to a
  commit). It decoded 128 kbit/s stereo at 44.1 kHz, 320 kbit/s at 48 kHz and
  48 kbit/s mono at 22.05 kHz on the board.
- Mounting the SD card costs 20 kB of heap, more than estimated.

## Selected stack

| Layer | Choice | Version found | Licence |
|---|---|---|---|
| Build system | PlatformIO Core 6.2.0 with the pioarduino platform | 55.03.313 | Apache-2.0 |
| Arduino core | arduino-esp32 3.3.x (ESP-IDF 5.5) | 3.3.13 | LGPL-2.1 |
| I2S output | None: the wired output is on hold (ADR-000204) | — | — |
| Bluetooth A2DP source | pschatzmann/ESP32-A2DP | 1.8.12 | Apache-2.0 |
| MP3 decoding | Helix decoder (pschatzmann/arduino-libhelix), standalone | 0.9.4 | RPSL (Helix) |
| WAV decoding | Own parser (PCM only) | — | — |
| Mixer, oscillators, noise | Own code | — | — |
| Display and touch driver | LovyanGFX (ST7789 and GT911 built in) | 1.3.0 | FreeBSD |
| GUI | A widget layer of our own on LovyanGFX; **no LVGL** (ADR-000204) | — | — |
| Web server | ESP32Async/ESPAsyncWebServer with AsyncTCP | 3.12.1 / 3.6.1 | LGPL-3.0 |
| JSON | ArduinoJson 7 | 7.4.x | MIT |
| mDNS, Wi-Fi, SD, Update | Bundled with the core | — | LGPL-2.1 |
| Web assets | Gzipped, embedded in the firmware image | — | — |
| Unit tests | Host compiler with a small harness of our own (`firmware/tools/host_tests.py`) | — | — |

Exact versions are pinned in `firmware/platformio.ini`, which is the
dependency list (RES-DOC-001). The pioarduino platform needs PlatformIO Core
6.2.0 or later; 6.1.19 refuses it.

## 1. Arduino core and platform

| Option | State found | Verdict |
|---|---|---|
| Core 2.0.17 (ESP-IDF 4.4) | Last 2.0.x release, May 2024. Current audio libraries have dropped it | Rejected: end of line |
| Core 3.3.x (ESP-IDF 5.5) | 3.3.13 current. Bluetooth Classic and A2DP are enabled in the prebuilt libraries for the plain ESP32 | **Selected** |
| Core 4.0 (ESP-IDF 6.1) | Release candidate. Refactors Bluetooth, removes the legacy I2S driver | Rejected for now: not stable |

- The official PlatformIO `espressif32` platform ships core 3.3.12 since its
  8.0.0 release (October 2026); before that it shipped 2.0.17.
- **pioarduino is preferred** because it offers `custom_sdkconfig`, which
  rebuilds the Arduino libraries with changed ESP-IDF options. That is the
  only practical way to remove BLE and hands-free profile support and to
  shrink Bluetooth and Wi-Fi buffers. The prebuilt libraries compile in
  dual-mode Bluetooth (BLE, Classic and HFP), which costs RAM and flash that
  Resonance does not use.
- Risk: one linking bug has been reported with that feature. The fallback is
  the official platform with stock libraries, at a higher RAM cost.
- The legacy I2S driver (`driver/i2s.h`) must not be pulled in by any
  library: mixing it with the new driver is reported to abort at start
  (*unverified*).

## 2. Bluetooth A2DP source

**ESP32-A2DP** (`BluetoothA2DPSource`) is the only maintained Arduino option
and supports core 3.3.x.

What it provides, against RES-BT-002:

| Need | Library support |
|---|---|
| Scan and list devices | `set_ssid_callback(name, address, rssi)`: called per discovered device; returning true connects |
| Connect to a chosen device | By the callback, or `start(name)` |
| Automatic reconnection | `set_auto_reconnect(address, retries)`; the last address is kept in NVS |
| Forget a device | `clean_last_connection()` for the last one; other devices through the ESP-IDF bond API |
| List remembered devices | **Not provided.** Use `esp_bt_gap_get_bond_device_list()` and keep names in Resonance's own configuration |
| Audio feed | A data callback that the Bluetooth task calls; it must not block |

Limits:

- 44.1 kHz, 16-bit stereo, SBC. This fixes the engine's sample rate at
  44.1 kHz (`ARCHITECTURE.md`).
- One connection at a time.
- Link keys stay inside the Bluetooth stack's NVS storage, as RES-BT-003
  requires.
- Reconnection after an ESP32 restart is reported to fail when the headset
  did not notice the disconnection.

Memory reported by users (core 2.0.x, stock libraries):

| State | Free heap reported |
|---|---|
| Wi-Fi only | about 230 kB |
| Wi-Fi plus Bluetooth Classic started | about 58 kB |
| A2DP connected | about 41 kB |
| A2DP streaming | 21–36 kB, unstable, crashes during Wi-Fi traffic |

The library's documentation states that Wi-Fi and A2DP cannot be used
together reliably. Espressif's coexistence table rates a Wi-Fi station
together with Bluetooth Classic as stable, and an **access point with
connected clients as "performance unstable"**. This bears directly on
RES-BT-004 and RES-WEB-007.

## 3. Audio decoding

| Library | Version found | Verdict | Reason |
|---|---|---|---|
| Helix MP3, standalone | 0.9.4 | **Selected** | Delivers decoded PCM to a callback and does not own the output. About 28–35 kB of RAM per decoder (figures from other ports; to be measured) |
| pschatzmann/arduino-audio-tools | 1.3.0 | Not used as the framework | Capable, but GPL-3.0, and its mixers are described by its own wiki as needing extra memory. Its A2DP stream is documented as less efficient than the raw callback |
| earlephilhower/ESP8266Audio | 2.4.3 | Rejected | GPL-3.0; built around owning the output and being pumped from `loop()`; no simple way to feed an external mixer and Bluetooth |
| schreibfaul1/ESP32-audioI2S | 4.0.1 | Rejected | **Requires PSRAM** since 3.3.0, and always owns the I2S output |
| lieff/minimp3 | untagged | Reserve | Public domain, frame-by-frame. Decodes in floating point; RAM use undocumented |

- WAV needs only a header parser and a block reader. PCM 8/16/24-bit, mono
  or stereo, is in scope.
- Files that are not 44.1 kHz need sample-rate conversion in the decoder
  task. A linear or small polyphase converter is own code.
- MP3 at 320 kbit/s needs the larger Helix buffer settings.
- The Helix licence (RPSL) must be read before the firmware is distributed.

## 4. Display, touch and GUI

| Library | Version found | Verdict | Reason |
|---|---|---|---|
| LovyanGFX | 1.3.0 | **Selected** | Actively maintained, DMA SPI, ST7789 and GT911 in one library, permissive licence. It has no ready profile for this board: the pin configuration comes from `GPIO_MAP.md` |
| TFT_eSPI | 2.5.43 (March 2024) | Rejected | No release since 2024, several hundred open issues, reported build failures on core 3.x |
| Arduino_GFX | 1.6.9 | Reserve | Maintained; no touch driver |
| rzeldent/esp32-smartdisplay | 2.1.x | Not used; **its board file is a pin reference** | Supports this exact board, but forces LVGL 9 and allocates a 38 kB draw buffer by default; its licence metadata is inconsistent |
| bb_captouch | 1.3.1 | Reserve | If LovyanGFX's GT911 support proves insufficient |

LVGL:

- 8.4.0 is selected over 9.x on the expectation of lower RAM and flash use.
  That expectation is engineering judgement, **not a documented figure**; 9.x
  is the maintained line. Gate G1 in `IMPLEMENTATION_PLAN.md` revisits it.
- Planned settings: LVGL heap 24–32 kB, one draw buffer of 240 × 20 lines
  (9.6 kB). LVGL recommends at least a tenth of the screen (15.4 kB); the
  smaller buffer trades redraw speed for RAM.

## 5. Web stack

- **ESP32Async/ESPAsyncWebServer** and **AsyncTCP** are the maintained forks;
  the me-no-dev and mathieucarbou repositories are archived. They provide
  streamed uploads, WebSocket and static responses, and work without a
  blocking server loop.
- AsyncTCP's task stack defaults to 16 kB; it is set to 4 kB and reviewed
  against its high-water mark.
- One user reports that web requests need about 50 kB of free heap to be
  reliable. Compare this with the A2DP figures above.
- **No captive-portal library.** tzapu/WiFiManager uses the synchronous web
  server, which cannot share a firmware with the asynchronous one. The
  configuration access point is a page of Resonance's own server; credentials
  can also be entered on the touchscreen.
- **OTA through the core's `Update` class** in an upload handler. ElegantOTA
  4.0.0 is AGPL-3.0 and is not used.
- Web assets are gzipped and embedded in the firmware, so no filesystem
  partition is needed.

## 6. Flash budget

Partition layouts available for 4 MB:

| Layout | Application | Filesystem | OTA |
|---|---|---|---|
| `default` | 2 × 1.25 MB | 1.375 MB | yes |
| `min_spiffs` | 2 × 1.875 MB | 128 kB | yes |
| `minimal` | 2 × 1.9375 MB | none | yes |
| `huge_app` | 1 × 3 MB | 896 kB | **no** |

- No published size exists for this combination of features. The estimate
  is 1.9–2.4 MB with stock libraries: **an estimate, not a measurement**.
- Consequence: the `default` layout is ruled out; a dual 1.9 MB layout is
  marginal; `huge_app` should fit but gives up OTA.
- Space can be recovered by removing BLE and HFP through `custom_sdkconfig`
  (claimed about 300 kB, unmeasured), by trimming LVGL fonts and widgets,
  and by optimising for size.
- RES-WEB-008 asks for OTA "where the flash layout allows it". Whether it
  does is decided at gate G3 from the measured image size. If it does not,
  firmware update is over USB and the limitation is documented
  (RES-PROC-005).

## 7. RAM budget

Estimated demand of Resonance's own parts, without the radio stacks:

| Consumer | Estimate |
|---|---|
| LVGL heap | 24–32 kB |
| LVGL draw buffer | 9.6 kB |
| Helix MP3 decoder | 28–35 kB |
| PCM ring buffers (decoder to mixer, mixer to Bluetooth) | 8–16 kB |
| I2S DMA buffers | 4–8 kB |
| Task stacks (audio, decoder, GUI, session, network) | 20–28 kB |
| AsyncTCP task and per-connection state | 8–20 kB |
| SD / FAT and open files | 4–8 kB |
| **Total** | **about 105–155 kB** |

Against the reports in section 2, this fits beside Wi-Fi alone, and fits
beside Bluetooth alone with little margin. It **does not fit beside Wi-Fi and
A2DP together on stock libraries**, where 35–58 kB is reported to remain.

Options, in the order they will be tried:

1. Rebuild the libraries Classic-only (no BLE, no HFP), with smaller
   Bluetooth task stacks and Wi-Fi buffers. Claimed saving about 50 kB RAM,
   unmeasured.
2. Keep Wi-Fi in station mode and idle during Bluetooth playback; refuse or
   defer uploads while A2DP is streaming, with a warning (RES-WEB-007 allows
   this).
3. Make the web interface and Bluetooth output mutually exclusive modes,
   chosen by the user.

Option 3 is the fallback that is known to be achievable. Which option ships
is decided at gate G2 from measurement, and is reported as RES-PROC-005 and
RES-BT-004 require.

## 8. Licences

| Licence | Components | Effect if the firmware is distributed |
|---|---|---|
| LGPL-2.1 / LGPL-3.0 | Arduino core, async web stack | Must allow relinking; publish the build |
| RPSL | Helix MP3 decoder | Read the terms before distribution |
| MIT, Apache-2.0, FreeBSD | LVGL, ArduinoJson, ESP32-A2DP, LovyanGFX | Attribution |
| GPL-3.0 / AGPL-3.0 | audio-tools, ESP8266Audio, ESP32-audioI2S, ElegantOTA | **Not used** |

MP3 patents have expired. No licence question blocks a personal build.

## Sources

- Core and platforms: <https://github.com/espressif/arduino-esp32/releases>, <https://github.com/platformio/platform-espressif32/releases>, <https://github.com/pioarduino/platform-espressif32/releases>, <https://github.com/espressif/esp32-arduino-lib-builder/blob/idf-release_v5.5/configs/defconfig.esp32>
- A2DP: <https://github.com/pschatzmann/ESP32-A2DP> (wiki pages "WIFI and A2DP Coexistence", "Program Memory", "Device Discovery", "Auto Reconnect"; issue 19), <https://github.com/espressif/arduino-esp32/issues/6451>, <https://github.com/espressif/esp32-arduino-lib-builder/issues/108>
- Coexistence and RAM: <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/coexist.html>, <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/performance/ram-usage.html>
- Decoders: <https://github.com/pschatzmann/arduino-libhelix>, <https://github.com/pschatzmann/arduino-audio-tools>, <https://github.com/earlephilhower/ESP8266Audio>, <https://github.com/schreibfaul1/ESP32-audioI2S>, <https://github.com/lieff/minimp3>
- Display and GUI: <https://github.com/lovyan03/LovyanGFX>, <https://github.com/Bodmer/TFT_eSPI>, <https://github.com/rzeldent/esp32-smartdisplay>, <https://github.com/lvgl/lvgl>, <https://github.com/bitbank2/bb_captouch>
- Web: <https://github.com/ESP32Async/ESPAsyncWebServer>, <https://github.com/ESP32Async/AsyncTCP>, <https://github.com/bblanchon/ArduinoJson>, <https://github.com/ayushsharma82/ElegantOTA>, <https://github.com/tzapu/WiFiManager>
- Partitions: <https://github.com/espressif/arduino-esp32/tree/master/tools/partitions>
