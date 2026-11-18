# WORK-000031: Enclosure version 2: snap-fit, flush glass, no divider

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000032

## Scope

Enclosure version 2: snap-fit, flush glass, no divider

## Implementation

- `hardware/case/resonance_case_v2.scad` (new): two snap-fit shells and the button; glass window flush, 1.5 mm walls, insert holes, no divider, rounded face edges, stem 0.5 mm longer. `hardware/case/README.md`: version 2 section; six `v2_*.png` renderings.

## Tests

- No firmware change. Every part exported from OpenSCAD as a simple one-piece solid; the source's assertions hold. Not printed.

**Test-Run:** 20261118T160716-2775093

## Regression impact and documentation

- Version 1 source kept unchanged. Glass outline, glass height and snap strength are unconfirmed until printed (README).

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000031 -> evidence (pending)

## Transitions

- 2026-11-18T21:04:54.910902+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T21:07:28.723226+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
