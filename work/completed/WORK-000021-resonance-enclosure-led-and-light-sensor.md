# WORK-000021: Resonance enclosure: LED and light-sensor windows, views as held

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000021

## Scope

Resonance enclosure: LED and light-sensor windows, views as held

## Implementation

- `hardware/case/resonance_case.scad`: windows in the front face over the RGB LED and the light sensor (the operator: "make sure the case exposes these"); the microSD slot moved towards the centre/top of the left edge; an `exploded` view; a note of how the device is held.
- `hardware/case/README.md`, six preview images: views as held (front, right with the button, left with the card slot), exploded view; parameters for the LED and the sensor.
- `docs/requirements/resonance.md`: RES-HW-007 now asks for the lipped button and for the LED and sensor to be exposed.

## Tests

- Rendered with OpenSCAD 2025.02.11: all five parts export as simple solids. The views were looked at: held upright with USB-C at the bottom, the card slot is on the left edge above centre, the button low on the right edge, the two windows below the screen; this matches the operator's description of the board.
- NOT verified: the positions of the LED, the sensor, the card slot and the button (guesses), and that the LED and sensor are on the screen side of the board. Nothing printed.

**Test-Run:** 20261118T002619-713278

## Regression impact and documentation

- None; no firmware change.

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000021 -> evidence (pending)

## Transitions

- 2026-11-18T05:25:32.362533+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:26:20.326050+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
