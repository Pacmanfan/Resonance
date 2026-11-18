# WORK-000018: Resonance: remember the names of Bluetooth devices

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-BT-002, RES-BT-003
**Evidence:** VER-000018

## Scope

Resonance: remember the names of Bluetooth devices

## Implementation

- `firmware/src/storage/settings_store.*`: a device's name is stored in NVS against its Bluetooth address (key `bt` + address).
- `firmware/src/audio/bt_audio.cpp`: on connection the name is taken from the user's choice, else from storage, else from the authentication event, else requested from the device (`esp_bt_gap_read_remote_name`), and then stored. The operator asked for the name of a reconnected device instead of "a remembered device".

## Tests

- No host test: Bluetooth adapter and NVS code.
- On the device (0.6.0, serial): after a restart, `BT connected to 'TouchBone'` and `BT state 3 peer 'TouchBone'`, although the headset had never been chosen from the list under this firmware. A first attempt (0.5.8) relied on the authentication event alone and got no name: a bonded device that reconnects gives none; the explicit name request fixed it.

**Test-Run:** 20261118T002442-708256

## Regression impact and documentation

- One NVS entry per device ever connected; they are never removed yet (forgetting devices is Stage 5).

## Traceability

REQ-CHAT-20261117-001 -> RES-BT-002, RES-BT-003 -> WORK-000018 -> evidence (pending)

## Transitions

- 2026-11-18T05:12:38.045806+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:24:51.937837+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
