# WORK-000037: Case v4 slim variant for the 5.75 mm cell

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000038

## Scope

Case v4 slim variant for the 5.75 mm cell

## Implementation

- `hardware/case/resonance_case_v4_slim.scad` (new): includes version 4 and sets `bat_t = 5.75` for the operator's thinner cell; outside 106 x 67 x 19.85 mm, back shell 6.85 mm. Front shell and button are unchanged parts.
- `hardware/case/v4_slim_{front,back,exploded,section}.png` (new): renderings.

## Tests

- No test added: a mechanical drawing. OpenSCAD exported front, back and button as simple one-piece solids with no warnings and all asserts of version 4 holding.

**Test-Run:** 20261121T232238-390172

## Regression impact and documentation

- Version 4 is untouched. `hardware/case/README.md` has a new section "Version 4 slim". Not printed.

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000037 -> evidence (pending)

## Transitions

- 2026-11-22T04:21:57.462207+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-22T04:22:50.595813+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
