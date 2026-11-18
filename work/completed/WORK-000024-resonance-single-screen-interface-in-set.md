# WORK-000024: Resonance: single-screen interface in Setup mode

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-ARCH-005, RES-GUI-001, RES-NET-003
**Evidence:** VER-000024

## Scope

Resonance: single-screen interface in Setup mode

## Implementation

- `firmware/src/gui/ui.*`: in Setup mode the interface is one screen titled SETUP, without the tab bar: network and password (access point), address, card, firmware, brightness, and `Back to PLAY` (one tap). The Settings tab of Play mode loses its Setup-mode branch. Version 0.6.4.
- The operator asked for everything not specific to setup to be removed in Setup mode. Of the questions put to the operator earlier, only the layout was decided by this request; the choices made here without an answer are: one tap to leave, no on-screen keyboard, no time-out.

## Tests

- No host test: drawing code. On the device: 0.6.4 starts, in Play mode. NOT verified: the Setup screen itself - the device was not switched to Setup mode for this, and nobody has seen the new screen.

**Test-Run:** 20261118T004109-817381

## Regression impact and documentation

- In Setup mode the screen timeout cannot be reached and no other setting can be changed on the device; that waits for the web interface.

## Traceability

REQ-CHAT-20261117-001 -> RES-ARCH-005, RES-GUI-001, RES-NET-003 -> WORK-000024 -> evidence (pending)

## Transitions

- 2026-11-18T05:39:46.946404+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:41:10.411894+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
