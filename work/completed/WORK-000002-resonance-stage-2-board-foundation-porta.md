# WORK-000002: Resonance Stage 2: board foundation, portable core, operating modes

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-PROC-003, RES-HW-001, RES-HW-002, RES-ARCH-001, RES-ARCH-004, RES-ARCH-005, RES-CFG-002, RES-CFG-003
**Decision:** ADR-000204
**Evidence:** VER-000002

## Scope

Stage 2 of the request (board foundation) under the operator's follow-up of
2026-11-17: no battery measurement, wired I2S output on hold, Bluetooth as
the output, space-conservative code, "page swapping" (ADR-000204). The board
was connected, so the firmware was flashed and its serial report read.
Not in scope: audio (Stage 3), the web server (Stage 7).

## Implementation

- `docs/decisions/ADR-000204-...md`, `docs/requirements/resonance.md`, manifests: the operator's decisions; RES-ARCH-005 (operating modes) new; RES-HW-003/005/006, RES-AUDIO-001/011, RES-WEB-003/008 amended.
- `firmware/platformio.ini`: pioarduino 55.03.313 (core 3.3.13), LovyanGFX 1.3.0, 3 MB single slot.
- `firmware/lib/resonance_core/`: portable settings record, ranges and validation.
- `firmware/src/hal/`: pin table from GPIO_MAP.md; quiet pins (IO26 low, LED off); reset reason.
- `firmware/src/gui/`: ST7789 + GT911 + backlight on LovyanGFX; theme; five-tab interface of our own (no LVGL) with frequency adjustment, brightness, dimming, mode switch.
- `firmware/src/storage/`: settings in NVS with validation on load; radio start guard; SD card owner and `/RESONANCE` tree.
- `firmware/src/radio/`: Bluetooth Classic bring-up and memory release; Wi-Fi station / set-up access point / mDNS.
- `firmware/src/main.cpp`: start-up by operating mode, status snapshot, serial diagnostics.
- `firmware/tests/`, `firmware/tools/host_tests.py`, `scripts/test_manifest.json`: host tests, suite `resonance_core` (t1).
- `firmware/tools/serial_capture.py`, `firmware/README.md`, root `README.md`.
- `docs/architecture/*.md`, `docs/README.md`: decisions and measurements; ARCHITECTURE.md and IMPLEMENTATION_PLAN.md rewritten to the current design.
- Outside the repository: PlatformIO Core in `~/.platformio/penv` upgraded 6.1.19 -> 6.2.0 (the platform requires it).

## Tests

- Added `firmware/tests/test_settings.cpp`: 15 cases on settings validation (defaults, foreign version, garbage record, frequency clamp / rounding / non-finite, start volume cap, brightness floor, mode fallback, hostname rules, text fields, access-point password).
- On the device (serial report, `evidence/VER-000002/device-report.txt`): start in Play and Setup mode, mode switch by restart, touch controller at 0x5D, Bluetooth start, Wi-Fi access point and scan, heap at each step, image size.
- NOT verified: the picture on the display (the panel gives no read-back), any touch press (0 presses recorded), the SD card (no card answered), the three-attempt radio guard (not triggered since it was changed).
- The first t2 run (20261117T214047-148078) FAILED in `scripts/unit`: five `test_root_readme` cases, because `devsys.py install` does not copy a root README.md. PRE-EXISTING, fixed here by writing `README.md`.

**Test-Run:** 20261117T214430-183455

## Regression impact and documentation

- New code only; nothing existing can regress. The radio start guard writes NVS at each start (one counter).
- The board lost power 16 times in 37 s at the first Bluetooth start (kernel log of USB disconnects), then ran stably; cause not established (`docs/architecture/HARDWARE.md` section 8, M11).
- Open: `devsys.json` build adapter, source roots and the core-layer invariant need a process-change request (protected artifact).

## Traceability

REQ-CHAT-20261117-001 -> RES-PROC-003, RES-HW-001, RES-HW-002, RES-ARCH-001, RES-ARCH-004, RES-ARCH-005, RES-CFG-002, RES-CFG-003 -> WORK-000002 -> evidence (pending)

## Transitions

- 2026-11-18T02:14:57.033886+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T02:48:28.004019+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
