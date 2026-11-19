# WORK-000035: Stutter while a file is loaded

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-PLAY-003
**Evidence:** VER-000036

## Scope

Stutter while a file is loaded

## Implementation

- Cause: with an MP3 decoder loaded only 8 kB of data memory were left, the Bluetooth stack could not get a 4.1 kB buffer for every packet and dropped a third of the audio (29900 instead of 44100 frames/s). `rc_ring.h` + `main.cpp`: the 16 kB ring holds 32-bit words and lives in the 32-bit-only region. `sdcard.cpp`: two open-file slots instead of four. `bt_audio.*`, `main.cpp`: the `bt` report shows frames/s; `heap` shows data memory only; `caps`, `pause`, `resume` commands. Version 0.8.1.

## Tests

- `test_audio.cpp` adapted to the ring's word storage; 89 host cases pass. On the device: 44100 frames/s during MP3 play, pause, resume and after stop; 33 kB data memory free while playing (lowest 24 kB). The operator has not yet listened to the final build.

**Test-Run:** 20261118T215528-3502083

## Regression impact and documentation

- `docs/architecture/HARDWARE.md`: data memory and the Bluetooth stream. `player.cpp`, `player.h` were registered but not changed.

## Traceability

REQ-CHAT-20261117-001 -> RES-PLAY-003 -> WORK-000035 -> evidence (pending)

## Transitions

- 2026-11-19T02:18:25.750444+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-19T02:55:57.317744+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
