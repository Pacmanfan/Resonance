# Resonance Implementation Plan

**Requirement:** RES-PROC-001, RES-PROC-003, RES-PROC-004, RES-PROC-005 (`docs/requirements/resonance.md`)
**Decision:** ADR-000204 (Bluetooth-only output, no battery measurement, space first, operating modes)
**Work:** WORK-000001 (Stage 1), WORK-000002 (Stage 2)
**Inputs:** `HARDWARE.md`, `GPIO_MAP.md`, `ARCHITECTURE.md`, `LIBRARY_EVALUATION.md`

## Where the project stands

| Stage | Content | State |
|---|---|---|
| 1 | Hardware and software audit | Done |
| 2 | Board foundation | **Implemented and running on the board.** Three checks need a person: the picture, touch, and a card in the slot |
| 3 | Audio prototype over Bluetooth | Not started |
| 4 | Playback and mixing | Not started |
| 5 | Bluetooth device management | Not started |
| 6 | Session engine | Not started |
| 7 | Web interface (Setup mode) | Not started |
| 8 | Product refinement | Not started |

The stages of the request are kept, with two changes from ADR-000204: the
audio prototype of Stage 3 plays over Bluetooth instead of an I2S DAC, so
the minimum of Bluetooth moves into Stage 3; and battery monitoring is gone
from Stage 8.

## Stage 2 — what was verified, and how

| Item | Result | Verified by |
|---|---|---|
| Build | 1,877,824-byte image, 60 % of the 3 MB slot | The build |
| Settings validation | 15 host test cases pass | `firmware/tools/host_tests.py` |
| Start-up, both modes | Reaches the report in Play and in Setup mode; mode switch by restart works | Serial report from the board |
| Touch controller | Answers at 0x5D | Serial report |
| Bluetooth Classic | Starts; 101 kB free afterwards | Serial report |
| Wi-Fi | Access point starts, scan finds networks; 157 kB free | Serial report |
| Both radios at once | 50 kB free: not a basis for the product | Serial report |
| Display | Initialises without error. **Picture not confirmed**: the panel gives no read-back | Needs eyes |
| Touch input | **No press has been made** | Needs a finger |
| microSD | **No card answered** | Needs a card |
| Radio start guard | Built after the board lost power 16 times at the first radio start. The skip path was seen working once, set off by a reset from the USB adapter during the guard window; that false alarm is why three failed starts in a row are now required. The three-attempt version has not been triggered | Kernel log of USB disconnects; serial report |

### To check on the bench

1. The screen shows a dark blue interface titled RESONANCE with three rings,
   a tab bar of five tabs, and `PLAY` at the top right. Colours: cyan,
   teal and violet. If red and blue look swapped, the colour order is wrong.
2. Each tab opens; on FREQ the buttons change the two frequencies; on SET
   the brightness buttons work and "Restart in SETUP mode" (two taps)
   restarts the device, which then shows the access point's name, password
   and address.
3. With a FAT32 card inserted, SOUND shows its size, and the card afterwards
   contains the `/RESONANCE` directory tree.
4. A cold start from a direct USB port, and one with a battery connected,
   to see whether the power loss at radio start returns (M11).

## Stage 3 — Audio prototype over Bluetooth

| Step | Deliverable | Test |
|---|---|---|
| 3.1 | Portable DSP: oscillator, gain ramp, limiter | Host tests: frequency accuracy by zero-crossing count, no discontinuity across a frequency change, ramp shape, limiter bound, channel independence |
| 3.2 | `AudioOutput` interface and the Bluetooth A2DP implementation; render task and ring buffer | Tone in a headset: 200 Hz left, 210 Hz right |
| 3.3 | Minimal pairing: scan, list on the screen, connect, reconnect to the last device | With one headset |
| 3.4 | Home screen audio on/off, volume, mute, fades | No click on start, stop and mute, by ear |
| 3.5 | Heap in Play mode with A2DP streaming | Recorded; decides whether a custom library build is needed |

## Stage 4 — Playback and mixing

| Step | Deliverable | Test |
|---|---|---|
| 4.1 | WAV parser and decoder task with ring buffer | Host tests on header variants; playback of 16-bit 44.1 kHz |
| 4.2 | Helix MP3 decoding | 128, 192 and 320 kbit/s files; 44.1 and 48 kHz |
| 4.3 | Sample-rate conversion to 44.1 kHz | Host test: tone in, tone out at the right frequency |
| 4.4 | Three-layer mixer, noise generator | Host tests: all layers at maximum stay within the ceiling; noise spectra slope |
| 4.5 | Transport, playlists, folder browsing, shuffle and repeat | On the device |
| 4.6 | Sustained playback | Two hours with heap and underrun counters logged |

## Stage 5 — Bluetooth device management

| Step | Deliverable | Test |
|---|---|---|
| 5.1 | Remembered devices, forget, selection | With two sinks |
| 5.2 | Connection loss and manual reconnect | Restart the device; restart the headset; walk out of range |
| 5.3 | Bluetooth screen in Settings | On the device |

## Stage 6 — Session engine

| Step | Deliverable | Test |
|---|---|---|
| 6.1 | Portable sweep engine and session model | Host tests: linear, stepped, hold, multi-stage, pause and resume exactness |
| 6.2 | `SessionManager`, durations, actions, end-of-session fades | Short sessions on the device; time derived from audio frames |
| 6.3 | Presets and sessions as JSON on the card, factory presets | Host tests for parsing and rejection of corrupt files |

## Stage 7 — Web interface (Setup mode)

| Step | Deliverable | Test |
|---|---|---|
| 7.1 | REST API, authentication, Wi-Fi configuration page | API tests from a script; unauthenticated requests refused |
| 7.2 | Library and file management through `sdcard` | Host tests for the path rules (traversal, absolute paths, bad extensions) |
| 7.3 | Upload: several files, progress, cancel, atomic rename, signature check, overwrite confirmation | Interrupt an upload; upload a renamed non-audio file |
| 7.4 | Presets, sessions, settings, backup and restore | Desktop and phone browsers |
| 7.5 | USB firmware update procedure documented (no OTA) | Followed once from the document |

## Stage 8 — Product refinement

| Step | Deliverable |
|---|---|
| 8.1 | Complete screens, visualisations, reduced-animation mode |
| 8.2 | Start-up behaviour, error reporting, recovery after power loss |
| 8.3 | The acceptance run of the request §19, recorded as executed or not run |
| 8.4 | Deliverable documents of RES-DOC-001 |

## Reported limitations

Per RES-PROC-005. Each follows from a decision or a measurement, and none is
silent.

| Requested | Delivered | Why |
|---|---|---|
| Wired stereo output | On hold | ADR-000204 |
| Battery status | None | ADR-000204; the board has no measurement circuit |
| OTA update | USB update | The image does not fit two slots in 4 MB |
| Web control of live playback; browser mirroring the touchscreen | Web administration only in Setup mode, where no audio runs | 50 kB of heap with both radios up |
| LVGL-class interface | Own widget layer | Heap and flash |

## How the work is tracked

- Each stage is a work record under request REQ-CHAT-20261117-001, through
  the repository's lifecycle (`AGENTS.md`).
- Host tests are the suite `resonance_core` in `scripts/test_manifest.json`
  and run in tier t1.
- Still to do through a process-change request, because `devsys.json` is a
  protected artifact: the build adapter, `project.source_roots`, and the
  layer rule of `ARCHITECTURE.md` as an invariant.
- Tests that need the device are recorded with their real outcome. What
  needs a person at the bench is listed as such and is never recorded as
  passed without one (RES-PROC-004).

## Risks

| Risk | Likelihood | Effect | Response |
|---|---|---|---|
| Play-mode heap too small once A2DP and the MP3 decoder run | Medium: 101 kB before both | Dropouts or failure to start | Measure at 3.5; custom library build without BLE and HFP |
| Power loss at radio start recurs | Seen once (16 events) | Device restarts; the guard skips the radio | Direct USB port, battery; M11 |
| Board differs from schematic V1.0 | Medium | Wrong pins | So far the touch and display pins behave as drawn |
| Dropouts from SD, display or radio contention | Medium | Audible glitches | Separate SPI buses; ring buffers; priorities; counters |
| Display is not as configured (colour order, inversion) | Unknown until seen | Wrong colours | One configuration line |
| Reconnection to some headsets unreliable | Medium | Manual reconnect needed | Manual reconnect is specified; document tested devices |
