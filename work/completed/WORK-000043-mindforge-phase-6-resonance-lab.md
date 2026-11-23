# WORK-000043: Mindforge phase 6: Resonance Lab

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-AUDIO-017
**Evidence:** VER-000044

## Scope

Mindforge phase 6: Resonance Lab

## Implementation

- `rc_lab` (new): width, interaural delay, bilateral alternation, trim, A-weighted loudness and matching. `rc_audio`: the Lab on tone, partials and noise, bypassed at rest. `gui/ui.h`, `gui/ui_advanced.cpp`: the Lab screen and its record (two sides, matching). `main.cpp`: applies a side, switches sides, sweep, serial `lab`. Version 0.9.5.

## Tests

- test_lab (10 cases) is new (163 in all).

**Test-Run:** 20261122T221419-3257758

## Regression impact and documentation

- At rest the Lab is not in the signal path (sample-for-sample test, also after use). The advanced menu has a fifth entry. `docs/architecture/IMPLEMENTATION_PLAN.md` updated with state and render cost. Not heard; no stream statistics with the Lab active.

## Traceability

REQ-CHAT-20261117-001 -> RES-AUDIO-017 -> WORK-000043 -> evidence (pending)

## Transitions

- 2026-11-23T03:06:09.598553+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T03:15:22.308419+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
