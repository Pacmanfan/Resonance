# WORK-000013: Resonance: consistent font on the frequency step buttons

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-002
**Evidence:** VER-000013

## Scope

Resonance: consistent font on the frequency step buttons

## Implementation

- `firmware/src/gui/ui.cpp`: the shrink-to-fit margin of button labels is 4 px instead of 8, so that "+0.1" keeps the normal font like the rest of its row (reported by the operator). Version 0.5.3.

## Tests

- No host test: drawing code. On the device: 0.5.3 starts. NOT verified: that "+0.1" now matches its row - the label width was not measured, so this needs the operator's eyes.

**Test-Run:** 20261117T235434-629693

## Regression impact and documentation

- Other long labels may now also stay in the normal font where they just fit.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-002 -> WORK-000013 -> evidence (pending)

## Transitions

- 2026-11-18T04:53:03.207159+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:54:44.500297+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
