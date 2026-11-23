# WORK-000040: Mindforge phase 3: layer mixer, guided voice, stage recordings

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-AUDIO-016, RES-PLAY-007, RES-SESS-010
**Evidence:** VER-000041

## Scope

Mindforge phase 3: layer mixer, guided voice, stage recordings

## Implementation

- `rc_tone`: gains for the fundamental and the partials as layers; ramps longer than half a second are computed from the samples left, so a long fade ends without a rounding jump. `rc_audio`: four layers with enable, level and fade; fade time for the recording coming in and going out. `rc_session`, `rc_session_text`: a stage names a recording and its level. `audio/player`: fade-in, fade-out, a track being replaced keeps playing while it goes out; newest command wins. `main.cpp`: serial commands `layer`, `voice fade`, `noise`, `usession voice`; session stages start and stop recordings. Version 0.9.2.

## Tests

- 15 host cases added in test_audio, test_tone, test_session, test_session_text (131 in all).

**Test-Run:** 20261122T213512-3168171

## Regression impact and documentation

- With every layer on at its default level the output is unchanged (tone checksum test, mixer equality test). The player now fades a replaced track; with the default 50 ms fades its behaviour is as before. `docs/architecture/IMPLEMENTATION_PLAN.md` updated with state and the heap table.

## Traceability

REQ-CHAT-20261117-001 -> RES-AUDIO-016, RES-PLAY-007, RES-SESS-010 -> WORK-000040 -> evidence (pending)

## Transitions

- 2026-11-22T23:12:25.894707+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T02:35:47.548249+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
