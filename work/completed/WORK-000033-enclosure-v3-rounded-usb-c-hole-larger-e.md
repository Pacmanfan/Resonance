# WORK-000033: Enclosure v3: rounded USB-C hole, larger edge radii

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000034

## Scope

Enclosure v3: rounded USB-C hole, larger edge radii

## Implementation

- `hardware/case/resonance_case_v3.scad`: USB-C opening is a round-ended hole in the front shell (the back shell's tab is gone, the skirt is cut clear of it); `corner_r` 4.5, `edge_r` 2.2. `README.md`, four `v3_*.png`.

## Tests

- No firmware change. Each part exported as a simple one-piece solid; assertions hold. Not printed.

**Test-Run:** 20261118T162054-2807219

## Regression impact and documentation

- The board now goes in at a slant; the inside corners are rounded and rely on the board's estimated corner radius (README).

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000033 -> evidence (pending)

## Transitions

- 2026-11-18T21:20:27.955553+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T21:20:55.494451+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
