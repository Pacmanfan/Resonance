# WORK-000001: Resonance Stage 1: hardware and software audit

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-PROC-001, RES-PROC-002
**Evidence:** VER-000001

## Scope

Stage 1 of the request (§18): the hardware and software audit that must exist
before any firmware is written. Documents only; no firmware, no build, no
measurement on the board. Stages 2-8 are separate work under the same request.

## Implementation

- `docs/requirements/resonance.md` (new) and its line in `docs/requirements/requirements.txt`: the product requirements `RES-*`, derived from the request.
- `docs/architecture/HARDWARE.md` (new): the board from the V1.0 schematic and the C-variant board-definition files, each fact marked with its confidence; external DAC and amplifier; battery options; required measurements M1-M10.
- `docs/architecture/GPIO_MAP.md` (new): ownership of every GPIO and the I2S wiring proposal (BCK IO22, LRCK IO16, DIN IO17, soft mute IO4).
- `docs/architecture/LIBRARY_EVALUATION.md` (new): selected and rejected libraries, flash and RAM budgets.
- `docs/architecture/ARCHITECTURE.md` (new): layers, audio path, control and state, tasks, radio policy, open decisions.
- `docs/architecture/IMPLEMENTATION_PLAN.md` (new): stages 2-8, decision gates G1-G3, risks.
- `docs/architecture/README.md`, `docs/README.md`: index entries for the above.

## Tests

- No test added: this work produces documents. The t2 tier ran the structure, drift, documentation-integrity and invariant checks on them.
- Not verified by this work: every hardware fact (nothing was measured on the board) and every library figure (nothing was compiled). The documents state this.

**Test-Run:** 20261117T210110-59370

## Regression impact and documentation

- No code exists, so nothing can regress. The documents fix the starting assumptions of Stage 2; measurements M1-M10 may correct them.
- No ADR: the architecture is a proposal whose open decisions wait for gates G1-G3 (`docs/architecture/ARCHITECTURE.md` "Open decisions").

## Traceability

REQ-CHAT-20261117-001 -> RES-PROC-001, RES-PROC-002 -> WORK-000001 -> evidence (pending)

## Transitions

- 2026-11-18T01:52:15.201625+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T02:01:30.927149+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
