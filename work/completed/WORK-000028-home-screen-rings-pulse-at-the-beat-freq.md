# WORK-000028: Home-screen rings pulse at the beat frequency; display clock 40 MHz

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-004, RES-GUI-005
**Evidence:** VER-000029

## Scope

Home-screen rings pulse at the beat frequency; display clock 40 MHz

## Implementation

- `firmware/lib/resonance_core/src/rc_pulse.h`: pulse phase, shown rate (beat halved until 5 Hz or slower), per-ring offset with outward lag, 30 frames/s limit.
- `firmware/src/gui/ui.cpp`, `ui.h`: `drawRing`, `animateHome` (erase and redraw a ring only when it moved; no buffer); rings rest while audio is off; new resting radii 34/42/50 leave room for 4 px of travel.
- `firmware/src/gui/display.cpp`: write clock 24 -> 40 MHz.
- `firmware/src/version.h`: 0.8.0.

## Tests

- `firmware/tests/test_pulse.cpp`: 7 cases (shown rate, no motion without a beat, cycles per second, frame limit, outward travel, stall, reset).
- On the device: 0.8.0 starts in Play mode; with the tone streaming to a headset and audio on, worst gap 22 ms, no underrun over 5 s.
- Not verified: the look of the pulse and the picture at 40 MHz (need the operator's eyes).

**Test-Run:** 20261118T110644-2139574
**Test-Run:** 20261118T110735-2142396

## Regression impact and documentation

- Every screen draws at the higher clock; a marginal panel would show noise or wrong colours. Home ring radii changed.
- Updated: RES-GUI-004, `HARDWARE.md`, `IMPLEMENTATION_PLAN.md`, the display-refresh audit.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-004, RES-GUI-005 -> WORK-000028 -> evidence (pending)

## Transitions

- 2026-11-18T16:02:18.113733+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T16:07:40.075043+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
