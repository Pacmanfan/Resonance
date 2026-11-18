# WORK-000030: Case: export STL files; button stem allows for the switch actuator's overhang

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000031

## Scope

Case: export STL files; button stem allows for the switch actuator's overhang

## Implementation

- `hardware/case/resonance_case.scad`: `btn_overhang` (0.6 mm): the stem was sized to the board's edge, so a switch actuator standing over the edge would have been held pressed by 0.1 mm.
- `hardware/case/README.md`: print orientation; the two numbers to adjust for the button.
- STL files exported to `tmp/case/stl/` (not tracked).

## Tests

- OpenSCAD exports `front`, `tray`, `lid`, `button`: each a simple solid, one piece, lowest point on the bed; sizes read back from the STL files.
- Not verified: nothing printed; no dimension checked on the real board.

**Test-Run:** 20261118T120002-2261364

## Regression impact and documentation

- The button only. README updated.

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000030 -> evidence (pending)

## Transitions

- 2026-11-18T16:59:42.954194+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T17:00:13.111732+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
