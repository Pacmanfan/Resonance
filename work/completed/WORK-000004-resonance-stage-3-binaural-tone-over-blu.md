# WORK-000004: Resonance Stage 3: binaural tone over Bluetooth A2DP

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-AUDIO-001, RES-AUDIO-002, RES-AUDIO-003, RES-AUDIO-005, RES-AUDIO-011, RES-AUDIO-012, RES-BT-001, RES-BT-002, RES-CFG-003, RES-GUI-003
**Decision:** ADR-000204
**Evidence:** VER-000004

## Scope

Stage 3 as replanned by ADR-000204: the binaural tone, delivered over
Bluetooth A2DP, with scan and connect on the touchscreen. The operator asked
how to scan and connect to a Bluetooth audio device; this is what provides
it. Not in scope: sound files, mixer, noise (Stage 4); remembered-device
management beyond reconnecting to the last one (Stage 5).

## Implementation

- `firmware/lib/resonance_core/src/rc_tone.h/.cpp` (new): oscillator (32-bit phase accumulator, sine table), ramp, `ToneEngine` (left f0, right f0 + fb, glide, volume and start/stop fades, ceiling, dither).
- `firmware/src/audio/bt_audio.h/.cpp` (new): ESP32-A2DP source; scan list of up to five named devices, connect to the chosen one, reconnect to the last; the stack's audio callback renders from the engine.
- `firmware/src/radio/bt_stack.*`: reduced to the Bluetooth memory hook and its release in Setup mode; the stack is started by `bt_audio`.
- `firmware/src/gui/ui.*`: Home has volume -/+, audio on/off and a `BT:` button opening the device list; frequency edits apply to the running tone; Setup mode says the web page is not built yet (the operator opened the address and found nothing).
- `firmware/src/main.cpp`: engine, audio and volume state, events, serial commands `bt`, `bt connect`, `audio on/off`. Version 0.3.0.
- `firmware/platformio.ini`: ESP32-A2DP 1.8.12; `firmware/src/hal/board_pins.h`: comment.
- `docs/architecture/*.md`, `firmware/README.md`: audio path as built, listening instructions, measurements.

## Tests

- Added `firmware/tests/test_tone.cpp`, 10 cases: silence until enabled; left = primary and right = primary + beat within 0.01 Hz (200/210 Hz and four more pairs across the range); channel independence; phase continuity across a frequency change; faded start and stop; ceiling; square-law volume; ramped volume step; out-of-range and non-finite values.
- On the device (firmware 0.3.0, serial): Play mode starts the A2DP source and scans; heap 91.8 kB after start, 83.5 kB while scanning; image 1,889,338 bytes; no power loss.
- NOT verified: anything that needs an audio device - the scan list showing a device, connecting, the tone actually heard, absence of clicks, reconnection at start, heap while streaming. No device was found by the scan during the capture (none was in pairing mode as far as is known).

**Test-Run:** 20261117T221415-280173

## Regression impact and documentation

- Play-mode heap fell from 101 kB (stack only) to about 84 kB (A2DP source scanning).
- RES-AUDIO-011 asks for a common output interface; with one output the engine's `render()` is the boundary and the formal interface is deferred (`docs/architecture/ARCHITECTURE.md`, "Audio path").
- The settings record is unchanged.

## Traceability

REQ-CHAT-20261117-001 -> RES-AUDIO-001, RES-AUDIO-002, RES-AUDIO-003, RES-AUDIO-005, RES-AUDIO-011, RES-AUDIO-012, RES-BT-001, RES-BT-002, RES-CFG-003, RES-GUI-003 -> WORK-000004 -> evidence (pending)

## Transitions

- 2026-11-18T03:07:01.425514+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T03:14:38.401372+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
