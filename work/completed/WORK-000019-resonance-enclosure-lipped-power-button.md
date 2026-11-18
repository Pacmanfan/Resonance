# WORK-000019: Resonance enclosure: lipped power button seated in the side wall

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000019

## Scope

Resonance enclosure: lipped power button seated in the side wall

## Implementation

- `hardware/case/resonance_case.scad`: the power button is a separate part with a lip. The wall is thickened by 3 mm inside at the button; it has a finger dish outside, a hole for the shaft, and a flat and a 45-degree seat for the lip inside; a stem reaches the switch. The operator asked for "a separate button with a lip that fits into a recess in the side of the case", and had not found a place for the button in the first previews.
- `hardware/case/README.md`: the button; an explanation of the holes the operator asked about (they are in the tray floor, inside the case; the outer back has only the two lid screws); four preview images.

## Tests

- Rendered with OpenSCAD 2025.02.11: all five parts export as simple solids. Previews of the button side, the inside seat, the button part and the assembly were looked at: dish, shaft hole, seat, lip and stem are present, and the dish no longer breaks the top edge of the shell (it did at 12 mm; it is 9.5 mm).
- NOT verified: fit, and the position and direction of the switch on the board. Nothing printed.

**Test-Run:** 20261118T002442-708256

## Regression impact and documentation

- None; no firmware change.

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000019 -> evidence (pending)

## Transitions

- 2026-11-18T05:18:55.467142+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:24:52.451722+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
