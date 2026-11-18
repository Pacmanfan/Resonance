# WORK-000032: Enclosure version 3: window to the measured glass

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-HW-007
**Evidence:** VER-000033

## Scope

Enclosure version 3: window to the measured glass

## Implementation

- `hardware/case/resonance_case_v3.scad` (new): version 2 with the window cut to the measured glass, 77.6 x 55.0 x 4.0 mm. `esp32_2432s032.scad`: the reference board's glass to the measured size. `README.md`, three `v3_*.png`.

## Tests

- No firmware change. Each part exported as a simple one-piece solid; assertions hold, including two new ones for the window against the bosses. Not printed.

**Test-Run:** 20261118T161808-2800282

## Regression impact and documentation

- Versions 1 and 2 unchanged. The glass's position along the board is still unmeasured (README).

## Traceability

REQ-CHAT-20261117-001 -> RES-HW-007 -> WORK-000032 -> evidence (pending)

## Transitions

- 2026-11-18T21:17:28.739341+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T21:18:09.244340+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
