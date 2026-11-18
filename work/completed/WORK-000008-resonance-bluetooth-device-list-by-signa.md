# WORK-000008: Resonance: Bluetooth device list by signal strength, rescan after disconnect

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-BT-002, RES-BT-005
**Evidence:** VER-000008

## Scope

Resonance: Bluetooth device list by signal strength, rescan after disconnect

## Implementation

- `firmware/src/audio/bt_audio.*`: the device table keeps the strongest signals (a stronger device replaces the weakest; 45 s without a sighting drops an entry; nameless devices are listed by address); the list shown is a sorted snapshot; after a disconnect the scan is restarted through a small subclass of the library's source; choosing a device while another is connected drops that one first.
- `firmware/src/gui/ui.cpp`: the connected view says to disconnect in order to choose another device. `firmware/src/main.cpp`: list update each second. Version 0.4.1.
- Reported by the operator: the headset did not appear, other devices did, and the television was paired and remembered.

## Tests

- No host test: Bluetooth adapter code.
- On the device, with the library's log enabled (`CORE_DEBUG_LEVEL=4`, since set back to 1): reconnect to the remembered television at 11.8 s, connected and media started at 13.4 s; the operator's Disconnect at 24.7 s; "Discovery started" at 26.8 s; two compatible devices scanned; `BT state 2 peer 'TouchBone', 1 device(s) found`. The operator then reported "it found it. connecting now".
- NOT verified: that the headset connection completed and that audio is heard. The board still carries the build with verbose logging.

**Test-Run:** 20261117T232112-548256

## Regression impact and documentation

- A device far away may now be pushed off the list by nearer ones, which is intended.

## Traceability

REQ-CHAT-20261117-001 -> RES-BT-002, RES-BT-005 -> WORK-000008 -> evidence (pending)

## Transitions

- 2026-11-18T04:16:18.187002+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:21:17.966360+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
