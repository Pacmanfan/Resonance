# WORK-000003: Resonance Stage 2 fixes: touch input and display contrast

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-002, RES-GUI-005, RES-HW-002
**Evidence:** VER-000003

## Scope

The operator's bench feedback on firmware 0.2.0: the screen was very low in
contrast, nothing could be selected, and (on the first corrected build) the
screen flashed about once a second and touches left yellow dots.

## Implementation

- `firmware/src/gui/display.cpp`: touch is polled (`pin_int = -1`). With the interrupt pin configured, the driver reads the controller only while IO21 is low, and IO21 idles high on this board, so no touch was ever read. Added `touchInfo()` / `rawTouch()` diagnostics.
- `firmware/src/gui/theme.h`, `ui.cpp`: high-contrast scheme (near-black background, white text, bright double outlines, saturated highlights), no text below 12 px, pressed buttons invert.
- `firmware/src/gui/ui.cpp`: the status snapshot redraws only the lines that changed; the full redraw every second was the flashing. The yellow dots were the touch-debug marks (`RESONANCE_TOUCH_DEBUG`), now off by default.
- `firmware/lib/resonance_core/src/rc_settings.h`: brightness default 100 %, settings version 2.
- `firmware/src/main.cpp`: `REPORT touch chip` line. Version 0.2.1.
- `docs/architecture/*.md`, `firmware/README.md`: IO21 and polling, contrast rule, bench results.

## Tests

- No new host test: the changes are in the display and touch adapters.
- On the device (`evidence/VER-000003/device-report.txt`): the controller identifies as GT911, 240x320, 5 points; with polling, 30 presses by the operator were logged, walking the five tabs right and back left, then the audio button, at the coordinates of those controls. The operator then used the Settings screen to restart into Setup mode.
- Reported by the operator, not measured: "the screen is brighter".
- NOT verified: that the flashing is gone and that no dots remain (the last build was flashed after the operator's report); the SD card (no card yet).

**Test-Run:** 20261117T220607-257794

## Regression impact and documentation

- IO21 is no longer driven or read by the firmware.
- Settings stored by 0.2.0 are replaced by defaults once (version 2).

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-002, RES-GUI-005, RES-HW-002 -> WORK-000003 -> evidence (pending)

## Transitions

- 2026-11-18T02:57:05.123213+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T03:06:29.832572+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
