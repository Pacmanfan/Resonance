# WORK-000034: Enclosure version 4: 2 mm walls

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000035

## Scope

Enclosure version 4: 2 mm walls

## Implementation

- `hardware/case/resonance_case_v4.scad` (new): version 3 with 2.0 mm side walls. `README.md`, three `v4_*.png`.

## Tests

- No firmware change. Each part exported as a simple one-piece solid; assertions hold. Not printed.

**Test-Run:** 20261118T201530-3288850

## Regression impact and documentation

- Earlier versions unchanged. The stutter reported in the same message is handled separately.

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000034 -> evidence (pending)

## Transitions

- 2026-11-19T01:15:07.718439+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-19T01:15:30.896846+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
