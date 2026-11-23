# WORK-000042: Mindforge phase 5: advanced screens

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-006
**Evidence:** VER-000043

## Scope

Mindforge phase 5: advanced screens

## Implementation

- `rc_edit` (new): the edits a screen makes to a session or sequence, each keeping it valid. `gui/ui_advanced.cpp` (new): menu, mixer, breathing, builder, stage, slots, sequence screens, the breathing bar, and the `press`/`describe` development aids. `gui/ui.h`, `gui/ui.cpp`: the Advanced record, events, the two entry buttons, the header as the way back. `main.cpp`: applies the record, reads slots, captures layers into a stage, serial `tap` and `ui`. Version 0.9.4.

## Tests

- test_edit (8 cases) is new, including 20000 random edits that must leave the session valid (153 cases in all).

**Test-Run:** 20261122T220403-3234272

## Regression impact and documentation

- The SESS tab START button is narrower (BUILD beside it); the SET tab has an ADVANCED button; Home gains a 3-pixel bar only while the pacer runs. `docs/architecture/IMPLEMENTATION_PLAN.md` updated. The screens have not been seen by a person.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-006 -> WORK-000042 -> evidence (pending)

## Transitions

- 2026-11-23T02:50:40.791738+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T03:04:55.152343+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
