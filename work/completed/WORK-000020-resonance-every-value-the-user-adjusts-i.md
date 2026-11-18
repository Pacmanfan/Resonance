# WORK-000020: Resonance: every value the user adjusts is saved and restored

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-CFG-002, RES-CFG-003
**Evidence:** VER-000020

## Scope

Resonance: every value the user adjusts is saved and restored

## Implementation

- `firmware/lib/resonance_core/src/rc_settings.*`: settings version 3 adds the background and noise volumes and the four session choices, with defaults and bounds.
- `firmware/src/main.cpp`: these values, and the master volume (as the capped start-up volume), are restored at start and written four seconds after the last change; a mode switch writes them at once. `REPORT user values` line; serial `volume <n>`.
- `firmware/src/gui/ui.cpp`: on the SND tab `<` is on the left, `UP` in the middle and `>` on the right (the operator found `<` centred). Version 0.6.1.
- The operator asked that everything the user manipulates be saved.

## Tests

- `firmware/tests/test_settings.cpp`: the new fields' defaults and bounds. 68 host cases in all.
- On the device (0.6.0, serial): `volume 45`, then after a restart `REPORT user values: volume 45, background 70, noise 40, ...`.
- NOT verified on the device: the other values (changed only by touch), and the new position of the paging buttons.
- Deliberately not stored: the noise type and whether audio was on; a volume above 60 % comes back as 60 % (RES-CFG-003).

**Test-Run:** 20261118T002442-708256

## Regression impact and documentation

- Settings stored by earlier versions are replaced by defaults once (version 3), including the set-up access point's password.

## Traceability

REQ-CHAT-20261117-001 -> RES-CFG-002, RES-CFG-003 -> WORK-000020 -> evidence (pending)

## Transitions

- 2026-11-18T05:19:40.910214+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:24:52.973917+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
