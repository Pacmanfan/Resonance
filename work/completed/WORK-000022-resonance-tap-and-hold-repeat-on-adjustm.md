# WORK-000022: Resonance: tap-and-hold repeat on adjustment buttons

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-005, RES-GUI-002
**Evidence:** VER-000022

## Scope

Resonance: tap-and-hold repeat on adjustment buttons

## Implementation

- `firmware/lib/resonance_core/src/rc_repeat.h` (new): `HoldRepeat` - first repeat after 450 ms, then every 140 ms, every 60 ms after 2.5 s held; at most one repeat per call.
- `firmware/src/gui/ui.*`: while the finger stays on a step button (frequency steps, preset `<` `>`, the volumes, brightness) its action repeats; sliding off ends it; switching, starting, opening and cycling buttons never repeat. Version 0.6.2.
- The operator asked for tap-and-hold to repeat after a brief pause, "research and implement". The timing follows the keyboard / stepper convention: operating systems default to a 400-500 ms delay, touch interfaces treat about 500 ms as a long press, and steppers accelerate on a long hold. These are conventions recalled, not sources fetched for this change.

## Tests

- Added `firmware/tests/test_repeat.cpp` (5 cases): a tap never repeats; a hold repeats after the pause at about seven a second; a long hold speeds up to about sixteen; a stalled interface gets one repeat, not a burst; each press starts slow again and the millisecond counter wrapping is harmless. 73 host cases in all.
- On the device: 0.6.2 starts. NOT verified: the feel of the repeat under a finger, which nobody has tried yet.

**Test-Run:** 20261118T002855-720835

## Regression impact and documentation

- Each repeat of a frequency step redraws the values and, on the preset buttons, steps a preset; a long hold on a volume button reaches its limit in a few seconds.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-005, RES-GUI-002 -> WORK-000022 -> evidence (pending)

## Transitions

- 2026-11-18T05:26:57.267683+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:29:05.605618+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
