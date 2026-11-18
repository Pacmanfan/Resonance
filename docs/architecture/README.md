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
| `IMPLEMENTATION_PLAN.md` | Stages 2-8 step by step, the decision gates and the risks |

These five documents are the Stage 1 audit (RES-PROC-001, WORK-000001). They
were produced from published documents; none of their hardware facts has been
measured on the board yet, and each says so where it matters.

No firmware exists yet. The source tree `firmware/` is created in Stage 2.
