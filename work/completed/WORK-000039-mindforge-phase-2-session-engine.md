# WORK-000039: Mindforge phase 2: session engine

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-SESS-007, RES-SESS-008, RES-SESS-009
**Evidence:** VER-000040

## Scope

Mindforge phase 2: session engine

## Implementation

- `firmware/lib/resonance_core/src/rc_session.h/.cpp`: `Stage::transitionS`, `flags`, `partial`, `noiseType`, `noiseVolume`; `Session::endFadeS`; `kMaxStages` 6 -> 10; `Descent`, `descentSession`; `SessionRunner::setLayersNow`, `setsPartials`, `setsNoise`, `partialPercent`, `noiseVolume`, `noiseType`; scaling keeps a transition's share; validation of all of it.
- `firmware/lib/resonance_core/src/rc_session_text.h/.cpp` (new): the text form of a session, written and parsed a line at a time.
- `firmware/src/storage/session_store.h/.cpp` (new): twelve slots under /RESONANCE/SESSIONS, write-then-rename. `storage/sdcard`: `openForRead`.
- `firmware/src/main.cpp`: the session's layers are applied in `tickSession`; `customSession`; serial commands `descent`, `usession ...`. `version.h`: 0.9.1.
- `docs/requirements/resonance.md`, `requirements.txt`: RES-SESS-007..009.

## Tests

- `firmware/tests/test_session.cpp`: 7 new cases (transition then hold, stage layers, session end fade, descent with pause and stop, descent without lead-in and bad descents, scaling of transitions, ten stages). `firmware/tests/test_session_text.cpp` (new): 4 cases (round trip, factory programs and widest line, lenient reading of hand-made files, damaged and hostile files).
- On the board (0.9.1): `descent exp 12 6 4 8 5 3`, `usession run`, `session` at intervals, `session stop`; `usession save` without a card fails with a reason.

**Test-Run:** 20261122T122208-2044106

## Regression impact and documentation

- The factory programs and the timer run on the changed runner; their existing tests pass unchanged. `IMPLEMENTATION_PLAN.md` updated. Open: storing and loading on a real card; hearing a stage change partials and noise.

## Traceability

REQ-CHAT-20261117-001 -> RES-SESS-007, RES-SESS-008, RES-SESS-009 -> WORK-000039 -> evidence (pending)

## Transitions

- 2026-11-22T17:18:12.510027+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-22T17:22:47.246803+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
