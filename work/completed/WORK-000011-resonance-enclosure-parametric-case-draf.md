# WORK-000011: Resonance enclosure: parametric case draft for board and 90x60x11 battery

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000011

## Scope

Resonance enclosure: parametric case draft for board and 90x60x11 battery

## Implementation

- `hardware/case/resonance_case.scad` (new): parametric OpenSCAD enclosure - front shell, four spacers, battery tray with a 2 mm divider floor, lid, printed power button; openings for USB-C, microSD and the button only.
- `hardware/case/README.md`, two preview images: parts, hardware, assembly, the measurements needed before printing, limits.
- `docs/requirements/resonance.md`: RES-HW-007 (enclosure). `docs/architecture/HARDWARE.md`: mechanical data from the manufacturer's sheet. `docs/README.md`: entry.
- The operator's inputs: battery about 90 x 60 x 11 mm; access to SD card, USB and an on/off button, possibly printed; no expansion ports.

## Tests

- Rendered with the operator's OpenSCAD (2025.02.11): `front`, `spacer`, `tray`, `lid` and `button` each export to STL as a simple (manifold) solid; outer size 104 x 65.6 x 30.2 mm. Previews of the front shell and the tray were looked at: window, four bosses, USB-C opening, card slot with scallop, button hole, countersunk holes, lead slot and lid bosses are present.
- NOT verified: any fit. Nothing was printed, and eleven positions and heights are guesses pending measurement of the board (README table). The side and orientation of the power button are assumed.

**Test-Run:** 20261117T234533-606220

## Regression impact and documentation

- None; no firmware change.

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000011 -> evidence (pending)

## Transitions

- 2026-11-18T04:42:38.663517+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:45:33.891702+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
