# WORK-000016: Resonance: no percentage shown for something that is off

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-002
**Evidence:** VER-000016

## Scope

Resonance: no percentage shown for something that is off

## Implementation

- `firmware/src/gui/ui.cpp`: the Home audio button reads `OFF` without a percentage while the audio is off (`ON 30%` when on), and the Noise line on FREQ shows a percentage only while a noise type is selected. The operator: "when it says OFF, don't put a % after that". Version 0.5.6.
- `firmware/README.md`, `docs/architecture/IMPLEMENTATION_PLAN.md`: the button's label in the instructions.

## Tests

- No host test: drawing code. On the device: 0.5.6 starts. NOT verified by eye yet.

**Test-Run:** 20261118T000746-664749

## Regression impact and documentation

- The master volume is not visible on Home while the audio is off; `-` and `+` still change it.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-002 -> WORK-000016 -> evidence (pending)

## Transitions

- 2026-11-18T05:06:38.035733+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:07:47.667555+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
