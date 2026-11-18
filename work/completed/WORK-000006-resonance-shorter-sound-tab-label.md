# WORK-000006: Resonance: shorter Sound tab label

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-001
**Evidence:** VER-000006

## Scope

Resonance: shorter Sound tab label

## Implementation

- `firmware/src/gui/ui.cpp`: the Sound Library tab is labelled SND; SOUND did not fit its button (operator). Version 0.3.2.

## Tests

- No host test: a label. On the device: 0.3.2 starts. Not yet seen by the operator.

**Test-Run:** 20261117T223852-335737

## Regression impact and documentation

- None.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-001 -> WORK-000006 -> evidence (pending)

## Transitions

- 2026-11-18T03:37:24.309359+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T03:38:53.525886+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
