# WORK-000014: Resonance: previous/next preset buttons in the Frequency Studio

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-AUDIO-006, RES-GUI-001
**Evidence:** VER-000014

## Scope

Resonance: previous/next preset buttons in the Frequency Studio

## Implementation

- `firmware/lib/resonance_core/src/rc_presets.*`: previous-preset lookup.
- `firmware/src/gui/ui.*`: `<` and `>` beside the primary value, as the operator asked, and beside the beat value for consistency; a tap on the value still steps forward; button limit 18. Version 0.5.4.
- The operator confirmed that the `+0.1` button of 0.5.3 looks right.

## Tests

- `firmware/tests/test_presets.cpp`: stepping back visits every preset in reverse, wraps to the highest, and undoes a step forward. 67 host cases in all.
- On the device: 0.5.4 starts. The new buttons have not been seen or tapped yet.

**Test-Run:** 20261117T235836-640457

## Regression impact and documentation

- The value buttons on FREQ are narrower (132 px instead of 224 px).

## Traceability

REQ-CHAT-20261117-001 -> RES-AUDIO-006, RES-GUI-001 -> WORK-000014 -> evidence (pending)

## Transitions

- 2026-11-18T04:57:20.678587+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:58:45.738356+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
