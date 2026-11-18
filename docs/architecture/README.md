# Current Architecture

This directory holds the current architecture documents of Resonance, the
binaural meditation and ambient audio device on the ESP32-2432S032C-I
(`docs/requirements/resonance.md`). It is the "current system explanation"
layer of the authority chain in `AGENTS.md` (below requirements, decisions
and evidence). The architecture audit (`./tools/audit`,
`docs/verification/architecture-audit.md`) checks that the paths these
documents name exist.

## Documents

| Document | Content |
|---|---|
| `HARDWARE.md` | The board as audited: display, touch, SD, onboard audio, connectors, power and battery, the external DAC and amplifier, and the measurements still required on the board |
| `GPIO_MAP.md` | Ownership of every GPIO, what is free, and the I2S wiring proposal |
| `ARCHITECTURE.md` | Firmware layers, the audio path, control and state, tasks, the radio policy and the open decisions |
| `LIBRARY_EVALUATION.md` | The selected libraries and versions, the rejected ones, and the flash and RAM budgets |
| `API.md` | The web interface: endpoints, authentication, upload rules |
| `IMPLEMENTATION_PLAN.md` | Stages 2-8 step by step, the decision gates and the risks |

These five documents are the Stage 1 audit (RES-PROC-001, WORK-000001). They
were produced from published documents. The first readings from the board
(WORK-000002) are added in `HARDWARE.md` and `LIBRARY_EVALUATION.md`;
everything not listed there is still unmeasured, and each document says so
where it matters.

The firmware is in `firmware/` (`firmware/README.md`: build, flash, serial
commands, tests). It is at Stage 2, the board foundation: no audio yet.
Decisions taken after the audit are in ADR-000204
(`docs/decisions/ADR-000204-bluetooth-only-output-and-space-first-firmware.md`)
and each document names the parts they changed.
