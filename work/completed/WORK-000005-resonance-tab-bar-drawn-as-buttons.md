# WORK-000005: Resonance: tab bar drawn as buttons

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-001, RES-GUI-002
**Evidence:** VER-000005

## Scope

Resonance: tab bar drawn as buttons

## Implementation

- `firmware/src/gui/ui.cpp`: the tab bar is five outlined, filled buttons; the open tab is filled with the highlight colour. The operator asked for the bottom row to look like buttons. Version 0.3.1.
- `docs/architecture/*.md`, `firmware/README.md`: tab bar, SD card result, interrupted-upload symptom.
- Outside the repository, at the operator's request: the 32 GB SD card on the laptop (`/dev/sda1`, old phone content, backed up by the operator) was formatted FAT32 with the label RESONANCE.

## Tests

- No host test: drawing code only.
- On the device: 0.3.1 starts; `REPORT sd mounted total 30509 MB free 30508 MB` with the formatted card.
- Incident: the first upload of 0.3.1 was interrupted (the board restarted while it was being flashed), leaving no bootable application and a black screen; the operator saw it and power-cycled without effect. A second upload restored it.
- NOT verified: how the new tab bar looks (not yet seen by the operator).

**Test-Run:** 20261117T223653-330998

## Regression impact and documentation

- None beyond the tab bar.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-001, RES-GUI-002 -> WORK-000005 -> evidence (pending)

## Transitions

- 2026-11-18T03:33:41.227874+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T03:36:55.875107+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
