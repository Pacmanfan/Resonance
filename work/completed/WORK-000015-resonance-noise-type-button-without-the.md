# WORK-000015: Resonance: noise type button without the percentage

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-002
**Evidence:** VER-000015

## Scope

Resonance: noise type button without the percentage

## Implementation

- `firmware/src/gui/ui.cpp`: the noise button on FREQ shows only the type; the volume moved to the "Noise" line above it. With the percentage in the button, "BROWN 40%" was too wide and was drawn in the small font (reported by the operator). Version 0.5.5.

## Tests

- No host test: drawing code. On the device: 0.5.5 starts. NOT verified: the look of the four type names, which needs the operator's eyes.

**Test-Run:** 20261118T000608-659957

## Regression impact and documentation

- None.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-002 -> WORK-000015 -> evidence (pending)

## Transitions

- 2026-11-18T05:04:17.717709+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:06:09.545019+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
