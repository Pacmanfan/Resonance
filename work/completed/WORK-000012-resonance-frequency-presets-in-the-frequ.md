# WORK-000012: Resonance: frequency presets in the Frequency Studio

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-AUDIO-006, RES-PROC-006
**Evidence:** VER-000012

## Scope

Resonance: frequency presets in the Frequency Studio

## Implementation

- `firmware/lib/resonance_core/src/rc_presets.h/.cpp` (new): fourteen carrier presets and nine beat presets, next-preset and name lookup.
- `firmware/src/gui/ui.*`: on FREQ the two values are buttons that step through the presets; the preset's name is shown above the value; button limit 14. Version 0.5.2.
- The operator asked for presets to cycle through in the frequency studio, from the lists "for those in the know".

## Tests

- Added `firmware/tests/test_presets.cpp` (6 cases): in range, on the 0.1 Hz step, rising; every band of RES-AUDIO-006 covered; cycling visits all and wraps; cycling from a hand-set value; naming; names contain no claim words. 66 host cases in all.
- On the device: 0.5.2 starts. The preset buttons have not been seen or tapped yet.

**Test-Run:** 20261117T235234-623779

## Regression impact and documentation

- The big frequency figures on FREQ are drawn smaller (18 px in a button instead of 24 px).

## Traceability

REQ-CHAT-20261117-001 -> RES-AUDIO-006, RES-PROC-006 -> WORK-000012 -> evidence (pending)

## Transitions

- 2026-11-18T04:50:40.035931+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:52:43.816666+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
