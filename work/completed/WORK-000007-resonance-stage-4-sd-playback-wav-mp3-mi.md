# WORK-000007: Resonance Stage 4: SD playback (WAV, MP3), mixer, noise

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-PLAY-001, RES-PLAY-002, RES-PLAY-003, RES-PLAY-005, RES-AUDIO-007, RES-AUDIO-008, RES-AUDIO-010, RES-AUDIO-012
**Decision:** ADR-000204
**Evidence:** VER-000007

## Scope

Stage 4: sound files from the SD card (WAV, MP3) mixed with the tone, the noise generator, the limiter, and the sound browser on the touchscreen. The operator asked what remains and to proceed; this is the first item of that list.

## Implementation

- `firmware/lib/resonance_core/src/`: `rc_ring.h` (lock-free frame ring), `rc_wav` (header through a byte source, PCM to stereo 16-bit), `rc_resample` (linear, to 44.1 kHz), `rc_noise` (white, pink, brown), `rc_audio` (`AudioEngine`: tone, background and noise layers, master volume, limiter).
- `firmware/src/audio/player.*` (new): decoder task - WAV and MP3 (Helix) from the card into the ring; pause keeps the place; errors reported.
- `firmware/src/audio/bt_audio.*`: renders from `AudioEngine`.
- `firmware/src/storage/sdcard.*`: directory listing, next-file lookup, cached card sizes.
- `firmware/src/gui/ui.*`: SND tab is a file browser with play/pause and background volume; noise control on FREQ; long button labels fall back to the small font.
- `firmware/src/main.cpp`: engine and ring, browsing state, events, auto-advance, serial commands `mktest`, `play`, `player`, `stop`, `put`. Version 0.4.0.
- `firmware/platformio.ini`, `firmware/partitions.csv`, `firmware/.gitignore`: Helix decoder pinned; **reduced library build** (`custom_sdkconfig`: Bluetooth Classic only), which the heap measurements made necessary.
- `firmware/tools/serial_put.py` (new): sends a test file to the card over serial.
- `docs/architecture/*.md`, `firmware/README.md`.

## Tests

- Added `firmware/tests/test_wav.cpp` (10 cases) and `firmware/tests/test_audio.cpp` (11 cases); 46 host cases in all.
- On the device (firmware 0.4.0, serial; `evidence/VER-000007/device-report.txt`): a WAV written by the device and three MP3 files sent over serial (128 kbit/s stereo 44.1 kHz, 320 kbit/s 48 kHz, 48 kbit/s mono 22.05 kHz) open, decode and fill the ring; a missing file gives an error; switching tracks works; Setup mode still starts Wi-Fi.
- Heap, stock against reduced libraries: 42 -> 99 kB after Bluetooth start, 29 -> 86 kB with a WAV, 53 kB with an MP3; image 1.94 -> 1.58 MB.
- NOT verified: any sound (no Bluetooth device was connected, so the ring was filled but never drained: track position stayed at 0 s and underruns could not occur); playing a track to its end and the next one following; the SND and noise controls on the screen (not touched by anyone); a long playback run.
- Not built from RES-PLAY-002: previous/next buttons, repeat, shuffle, playlists.

**Test-Run:** 20261117T230343-428117

## Regression impact and documentation

- The library build changed for the whole firmware: BLE, BLE Mesh, HFP and SPP are gone. Bluetooth Classic A2DP and Wi-Fi were re-checked on the board after the change.
- The first build now compiles ESP-IDF (minutes) and writes generated files into `firmware/` (git-ignored).
- Static RAM use rose by the ring (16 kB) and the player's buffers.

## Traceability

REQ-CHAT-20261117-001 -> RES-PLAY-001, RES-PLAY-002, RES-PLAY-003, RES-PLAY-005, RES-AUDIO-007, RES-AUDIO-008, RES-AUDIO-010, RES-AUDIO-012 -> WORK-000007 -> evidence (pending)

## Transitions

- 2026-11-18T03:40:04.740200+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:03:50.410649+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
