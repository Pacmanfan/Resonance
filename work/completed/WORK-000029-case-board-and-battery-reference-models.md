# WORK-000029: Case: board and battery reference models; board screwed to the front, no posts behind it

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000030

## Scope

Case: board and battery reference models; board screwed to the front, no posts behind it

## Implementation

- `hardware/case/esp32_2432s032.scad` (new): reference model of the board from the manufacturer's drawing, a vendor rendering and two published cases; every number marked with its source.
- `hardware/case/battery.scad` (new): reference model of the cell.
- `hardware/case/resonance_case.scad`: draft v1. Board screwed to the front bosses only; spacers and the screws under the cell removed; tray held by three screws beside the cell plus a rim and groove; lid rim only at the ends; cell clearance 1.5 mm; USB-C notch open to the wall top; LED and sensor windows moved to the far end; positions from the research; assertions for the button dish; section and exploded views with the reference parts.
- `hardware/case/README.md` and previews.

## Tests

- OpenSCAD renders `front`, `tray`, `lid`, `button` as simple (valid) solids with no warnings; the two assertions pass.
- Not verified: anything against the real board or cell; nothing printed.

**Test-Run:** 20261118T113245-2201198

## Regression impact and documentation

- Case only. `hardware/case/README.md` rewritten for v1.

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000029 -> evidence (pending)

## Transitions

- 2026-11-18T16:25:20.527811+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T16:33:10.141927+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
