# WORK-000009: Resonance Stage 6: session engine, timers, sweeps, fades

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-SESS-001, RES-SESS-002, RES-SESS-003, RES-SESS-004, RES-SESS-005
**Decision:** ADR-000204
**Evidence:** VER-000009

## Scope

Resonance Stage 6: session engine, timers, sweeps, fades

## Implementation

- `firmware/lib/resonance_core/src/rc_session.h/.cpp` (new): session model, validation, timer, four factory programs, `SessionRunner`.
- `firmware/lib/resonance_core/src/rc_audio.*`: `setSessionGain()` (ramped output level for the end fade); `setBackgroundEnding()` so that a track's end is not counted as an underrun.
- `firmware/src/main.cpp`: session selection, start, tick, pause, stop, finish; serial `session` commands; `STREAM` diagnostics in `bt`. `firmware/src/gui/ui.*`: SESS screen, time left on Home, frequencies follow a running session; the per-screen button limit raised from 10 to 12 (the FREQ noise `+` was dropped: reported by the operator).
- `firmware/src/audio/bt_audio.*`: stream timing counters. `firmware/src/audio/player.cpp`: marks the end of a track. Version 0.5.0.
- `docs/architecture/*.md`, `firmware/README.md`.

## Tests

- Added `firmware/tests/test_session.cpp` (10 cases) and two engine cases; 58 host cases in all.
- On the device with the operator's headset connected (serial): `STREAM worst gap 25 ms, worst render 1426 us, 0 long gap(s), 0 underrun(s)` and `worst gap 22 ms ... 0 long gap(s), 0 underrun(s)` over two windows of about ten seconds with the tone and MP3 playing; the four test files followed one another; `SESSION started: 10 to 6 Hz, 20 min` appeared when the operator started a program on the screen; heap 44.6 kB free (minimum 35.7 kB) with an MP3 streaming.
- Reported by the operator: the tone was heard; one stutter, while the verbose-logging build was on the board.
- NOT verified: a session running to its end on the device with its fade; pause and resume on the device; the look of the SESS screen (no report yet); absence of stutter over a long time.

**Test-Run:** 20261117T233539-582322

## Regression impact and documentation

- Session time is wall-clock, not audio frames as the Stage 1 design said (`docs/architecture/ARCHITECTURE.md`, "Sessions").
- A running session overrides frequency edits made on the FREQ tab for the values it controls.

## Traceability

REQ-CHAT-20261117-001 -> RES-SESS-001, RES-SESS-002, RES-SESS-003, RES-SESS-004, RES-SESS-005 -> WORK-000009 -> evidence (pending)

## Transitions

- 2026-11-18T04:25:05.417326+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:35:49.089407+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
