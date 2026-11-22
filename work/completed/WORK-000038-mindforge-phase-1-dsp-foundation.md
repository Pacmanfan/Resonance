# WORK-000038: Mindforge phase 1: DSP foundation

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-AUDIO-013, RES-AUDIO-014, RES-AUDIO-015, RES-SESS-006
**Evidence:** VER-000039

## Scope

Mindforge phase 1: DSP foundation

## Implementation

- `firmware/lib/resonance_core/src/rc_tone.h/.cpp`: `PhaseRamp` (integer phase offset ramp), `ToneEngine::setPhase`, `setInverted`, `setPartial`, `phaseDegrees`, `phaseMoving`; four partial oscillators; the render keeps its old path when nothing new is active.
- `firmware/lib/resonance_core/src/rc_session.h/.cpp`: `Sweep::Exponential`, `sweepValue`, `Stage::fromPrimaryHz/fromBeatHz`, `sweepSession`, validation of the new fields.
- `firmware/src/main.cpp`: serial commands `tone`, `tone phase`, `tone invert`, `tone partial`, `sweep`, `bench audio`; `startCustomSession`. `version.h`: 0.9.0.
- `docs/requirements/resonance.md`, `requirements.txt`: RES-AUDIO-013..015, RES-SESS-006.

## Tests

- `firmware/tests/test_tone.cpp`: 11 new cases (identity at rest, phase offset per channel, inversion per channel, no step on changes, bad values, phase sweep, hour-long sweep precision, offset while off, partial frequencies and levels and the kept beat, partial switching, limits). `test_session.cpp`: 5 new cases (sweep shapes, exponential stage, custom start, custom sweep session, bad sweeps).
- One-off comparison: a fixed scenario rendered by the 0.8.2 engine and by this one gives the same checksum (ed8d9e878d1fc8ae).
- On the board: `bench audio` in four configurations (see IMPLEMENTATION_PLAN.md).

**Test-Run:** 20261122T121321-2022743

## Regression impact and documentation

- The render path of every sound; guarded by the identity test and the unchanged existing tests. `docs/architecture/IMPLEMENTATION_PLAN.md` has a new section. Not yet heard by the operator: clicks on phase and inversion changes, the sound of the partials.

## Traceability

REQ-CHAT-20261117-001 -> RES-AUDIO-013, RES-AUDIO-014, RES-AUDIO-015, RES-SESS-006 -> WORK-000038 -> evidence (pending)

## Transitions

- 2026-11-22T17:06:27.400316+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-22T17:13:54.309032+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
