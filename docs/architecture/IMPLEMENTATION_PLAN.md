# Resonance Implementation Plan

**Requirement:** RES-PROC-001, RES-PROC-003, RES-PROC-004, RES-PROC-005 (`docs/requirements/resonance.md`)
**Work:** WORK-000001 (REQ-CHAT-20261117-001)
**Status:** Stage 1 complete as a document audit. Stages 2–8 not started.
**Inputs:** `HARDWARE.md`, `GPIO_MAP.md`, `ARCHITECTURE.md`, `LIBRARY_EVALUATION.md`

## Where the project stands

| Stage | Content | State |
|---|---|---|
| 1 | Hardware and software audit | Done from documents. **Board measurements M1–M10 are outstanding** |
| 2 | Board foundation | Not started |
| 3 | Stereo audio prototype | Not started |
| 4 | Playback and mixing | Not started |
| 5 | Bluetooth | Not started |
| 6 | Session engine | Not started |
| 7 | Web interface | Not started |
| 8 | Product refinement | Not started |

## What the audit found that changes the plan

1. **RAM, not features, decides the product's shape.** Wi-Fi and Bluetooth
   A2DP together leave too little heap on stock libraries for the GUI, the
   MP3 decoder and the web server. The plan therefore measures early
   (gates G1 and G2) and carries a defined fallback: the web interface and
   Bluetooth output as exclusive modes.
2. **OTA may not fit in 4 MB.** Decided at gate G3 from the measured image.
3. **The I2S DAC needs soldering on the board.** Only one free
   output-capable pin (IO22) is on a connector; the other signals come from
   the RGB LED pins.
4. **There is no battery measurement on the board.** Battery status needs an
   external divider to IO35.
5. **The onboard amplifier cannot be disabled**, and the connector P1 pin
   order in the request does not match the schematic.

None of these makes a required feature impossible. Items 1 and 2 may limit
features running **simultaneously**; if so, that is reported, not hidden
(RES-PROC-005).

## Decision gates

A gate is a measurement on the real board whose result is recorded in
`evidence/` and settles the open decisions listed in `ARCHITECTURE.md`.

| Gate | End of stage | Measure | Decides |
|---|---|---|---|
| G1 | 2 | Free heap, minimum free heap and largest block after: boot; display and LVGL; SD mounted; Wi-Fi station; then Bluetooth Classic started. Once on stock libraries, once on a Classic-only custom build | Custom or stock libraries; LVGL version and buffer sizes |
| G2 | 5 | The same figures while A2DP streams with MP3 decoding, with a WebSocket client, and during an upload. Audible dropouts and underrun counters during Wi-Fi scan, reconnect, upload and full-screen redraw | Radio policy level A, B or C; second decoder for crossfade |
| G3 | 7 | Size of the complete firmware image | OTA with two slots, or USB update only |

## Stage 2 — Board foundation

Prerequisite: measurements M1–M7 and M9 of `HARDWARE.md`.

| Step | Deliverable | Test |
|---|---|---|
| 2.1 | PlatformIO project, pinned platform and libraries, `native` test environment; `devsys.json` build adapter and test suite registered | Builds for the board and runs an empty native test |
| 2.2 | `hal/` board pins from `GPIO_MAP.md`; IO26 held low; serial diagnostics | Boot log shows reset reason and heap |
| 2.3 | Display: LovyanGFX configuration, backlight PWM | Colour bars and text; settles colour order and SPI clock (M7) |
| 2.4 | Touch: GT911 at the probed address | Touch coordinates in all four corners |
| 2.5 | SD card: mount, `/RESONANCE` tree created, removal handled | Read/write test; pull the card while idle and while reading |
| 2.6 | Configuration in NVS; validation rules in the portable core | Native unit tests for validation; corrupt-value test on the device |
| 2.7 | Wi-Fi: station, access point for configuration, mDNS | Connect, lose the network, recover; fallback access point |
| 2.8 | LVGL with the theme and the five-tab navigation shell | Navigation works; frame time logged |
| 2.9 | **Gate G1** | Recorded in `evidence/` |

## Stage 3 — Stereo audio prototype

Prerequisite: PCM5102A wired per `GPIO_MAP.md` (M8).

| Step | Deliverable | Test |
|---|---|---|
| 3.1 | Portable DSP: oscillator, gain ramp, limiter | Native tests: frequency accuracy by zero-crossing count over a long buffer, no discontinuity across a frequency change, ramp shape, limiter bound |
| 3.2 | `I2SAudioOutput` and the audio task | 200 Hz left / 210 Hz right at the DAC |
| 3.3 | Volume, mute, fade in/out, soft-mute line | No click on start, stop and mute, by ear and on a scope or sound-card capture |
| 3.4 | Stereo separation check | Capture both channels: each contains only its own frequency |
| 3.5 | Display, touch and SD re-tested with the DAC attached | Confirms the pin proposal (RES-HW-002) |

## Stage 4 — Playback and mixing

| Step | Deliverable | Test |
|---|---|---|
| 4.1 | WAV parser and decoder task with ring buffer | Native tests on header variants; playback of 16-bit 44.1 kHz |
| 4.2 | Helix MP3 decoding | 128, 192 and 320 kbit/s files; 44.1 and 48 kHz |
| 4.3 | Sample-rate conversion to 44.1 kHz | Native test: tone in, tone out at the right frequency |
| 4.4 | Three-layer mixer, noise generator | Native tests: all layers at maximum stay within the ceiling; noise spectra slope |
| 4.5 | Transport, playlists, folder browsing, shuffle and repeat | On the device |
| 4.6 | Sustained playback | Two hours with heap and underrun counters logged |

## Stage 5 — Bluetooth

| Step | Deliverable | Test |
|---|---|---|
| 5.1 | `BluetoothAudioOutput` behind the output interface | Tone to a headset |
| 5.2 | Scan, pair, connect, disconnect | With at least two different sinks |
| 5.3 | Remembered devices, forget, automatic reconnection | Restart the device; restart the headset; walk out of range |
| 5.4 | Output switching with fades | Wired to Bluetooth and back while playing |
| 5.5 | **Gate G2**, and `RadioPolicy` set to the level it supports | Recorded in `evidence/` |

## Stage 6 — Session engine

| Step | Deliverable | Test |
|---|---|---|
| 6.1 | Portable sweep engine and session model | Native tests: linear, stepped, hold, multi-stage, pause and resume exactness |
| 6.2 | `SessionManager`, durations, actions, end-of-session fades | Short sessions on the device; time derived from audio frames |
| 6.3 | Presets and sessions as JSON on the card, factory presets | Native tests for parsing and rejection of corrupt files |

## Stage 7 — Web interface

| Step | Deliverable | Test |
|---|---|---|
| 7.1 | REST API, WebSocket state, authentication | API tests from a script; unauthenticated requests refused |
| 7.2 | Dashboard and audio configuration | Two-way sync with the touchscreen |
| 7.3 | Library and file management through `SDCardManager` | Native tests for the path rules (traversal, absolute paths, bad extensions) |
| 7.4 | Upload: several files, progress, cancel, atomic rename, signature check, overwrite confirmation | Interrupt an upload; upload a renamed non-audio file; upload during playback |
| 7.5 | Session and Bluetooth panels, system panel, backup and restore | Desktop and phone browsers |
| 7.6 | **Gate G3**, then OTA or documented USB update | Recorded in `evidence/` |

## Stage 8 — Product refinement

| Step | Deliverable |
|---|---|
| 8.1 | Complete GUI screens, visualisations, reduced-animation mode, dimming and timeout |
| 8.2 | Battery divider and `BatteryManager`, if the measurements support it |
| 8.3 | Start-up behaviour, error reporting, recovery after power loss |
| 8.4 | The full acceptance run of the request §19, recorded as executed or not run |
| 8.5 | Deliverable documents of RES-DOC-001: build, wiring, API, configuration format, update, troubleshooting, bill of materials |

## How the work is tracked

- Each stage is one or more work records under the request, through the
  repository's lifecycle (`AGENTS.md`).
- At step 2.1 the project is registered with the tooling: the build adapter
  and source roots in `devsys.json` (a protected artifact, changed through a
  process-change request), the native unit tests as a suite in
  `scripts/test_manifest.json`, and the layer rule of `ARCHITECTURE.md` as an
  invariant.
- Tests that need the device are recorded as hardware runs with their real
  outcome. An agent without the board cannot produce them; they need a
  person at the bench (RES-PROC-004).

## Risks

| Risk | Likelihood | Effect | Response |
|---|---|---|---|
| Heap too small for Wi-Fi with A2DP | High on stock libraries | Web and Bluetooth cannot run together | Custom library build; else radio policy B or C |
| Image larger than a 1.9 MB slot | Medium | No OTA | USB update; documented |
| Custom library build fails or is fragile | Medium | Higher RAM use | Stock libraries and a stricter radio policy |
| Board differs from schematic V1.0 | Medium | Wrong pins | Measurements M1–M9 before wiring |
| Dropouts from SD, display or radio contention | Medium | Audible glitches | Separate SPI buses already; ring buffers; priorities; counters to locate the cause |
| 3.3 V rail overloaded by DAC and amplifier | Low to medium | Noise or brown-out | Measure (M4); separate regulator if needed |
| Display is ILI9341 on this unit | Low | Driver change | LovyanGFX supports both |
| Reconnection to some headsets unreliable | Medium | Manual reconnect needed | Manual reconnect is specified; document tested devices |

## Needed from the bench

1. Measurements M1–M10 in `HARDWARE.md`; M1–M3 and M9 need only a meter and
   a magnifier and unblock Stage 2.
2. Parts from the bill of materials in `HARDWARE.md` section 9, including
   the choice of headphone amplifier.
3. Agreement to solder to the RGB LED pins, or the alternative in
   `GPIO_MAP.md`.
