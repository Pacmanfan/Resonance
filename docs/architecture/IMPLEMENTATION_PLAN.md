# Resonance Implementation Plan

**Requirement:** RES-PROC-001, RES-PROC-003, RES-PROC-004, RES-PROC-005 (`docs/requirements/resonance.md`)
**Decision:** ADR-000204 (Bluetooth-only output, no battery measurement, space first, operating modes)
**Work:** WORK-000001 (Stage 1), WORK-000002 and WORK-000003 (Stage 2), WORK-000004 (Stage 3), WORK-000007 (Stage 4)
**Inputs:** `HARDWARE.md`, `GPIO_MAP.md`, `ARCHITECTURE.md`, `LIBRARY_EVALUATION.md`

## Where the project stands

| Stage | Content | State |
|---|---|---|
| 1 | Hardware and software audit | Done |
| 2 | Board foundation | Done on the board: picture, touch and both modes confirmed by the operator; SD card mounts |
| 3 | Audio prototype over Bluetooth | **Implemented; the tone is verified in host tests, Bluetooth starts and scans on the board. Not yet heard: no audio device has been connected** |
| 4 | Playback and mixing | **Implemented. WAV and MP3 decode on the board into the mixer; mixer, limiter, noise, WAV and resampler verified in host tests. Not yet heard. Playlists, shuffle, repeat and previous/next are not built** |
| 5 | Bluetooth device management | Not started |
| 6 | Session engine | Not started |
| 7 | Web interface (Setup mode) | Not started |
| 8 | Product refinement | Not started |

The stages of the request are kept, with two changes from ADR-000204: the
audio prototype of Stage 3 plays over Bluetooth instead of an I2S DAC, so
the minimum of Bluetooth moves into Stage 3; and battery monitoring is gone
from Stage 8.

## Stage 2 — what was verified, and how

| Item | Result | Verified by |
|---|---|---|
| Build | 1,877,824-byte image, 60 % of the 3 MB slot | The build |
| Settings validation | 15 host test cases pass | `firmware/tools/host_tests.py` |
| Start-up, both modes | Reaches the report in Play and in Setup mode; mode switch by restart works | Serial report from the board |
| Touch controller | Answers at 0x5D | Serial report |
| Bluetooth Classic | Starts; 101 kB free afterwards | Serial report |
| Wi-Fi | Access point starts, scan finds networks; 157 kB free | Serial report |
| Both radios at once | 50 kB free: not a basis for the product | Serial report |
| Display | The operator saw the interface; the first colour scheme was too low in contrast and was replaced in 0.2.1, which the operator found brighter. The flashing of the first corrected build is gone | The operator |
| Touch input | Did not work in 0.2.0 (the driver waited for an interrupt line that idles high). Fixed in 0.2.1 by polling: 30 presses by the operator, on the tabs and the audio button, registered at the right places | Serial log of presses |
| microSD | Mounted a 32 GB FAT32 card: 30,509 MB total, 30,508 MB free after the `/RESONANCE` tree was created (firmware 0.3.1). The tree itself has not been looked at on a computer | Serial report |
| Radio start guard | Built after the board lost power 16 times at the first radio start. The skip path was seen working once, set off by a reset from the USB adapter during the guard window; that false alarm is why three failed starts in a row are now required. The three-attempt version has not been triggered | Kernel log of USB disconnects; serial report |

### Stage 3 — what was verified, and how

| Item | Result | Verified by |
|---|---|---|
| Frequencies | Left f0 and right f0 + fb within 0.01 Hz at 20, 100.3, 200, 432 and 1000 Hz | Host tests (zero-crossing count over 10-20 s) |
| Channel independence | The left channel's envelope is flat: nothing of the right leaks in | Host test |
| Frequency change | Phase-continuous: no sample step larger than the sine itself makes | Host test |
| Start, stop, volume step | Faded; silence before start and after stop | Host tests |
| Ceiling | Output never exceeds half of full scale | Host test |
| Invalid values | Clamped; non-finite ignored | Host test |
| Bluetooth A2DP on the board | Starts and scans; 84 kB of heap free; image 1.89 MB | Serial report |
| Scan list, connect, the tone in a headset, clicks, reconnect at start | **Not verified: no audio device was in pairing mode** | Needs a headset |

### Stage 4 — what was verified, and how

| Item | Result | Verified by |
|---|---|---|
| WAV header and PCM conversion | Plain, with extra chunks, extensible format; float, compressed, multichannel refused; truncated files refused | Host tests |
| Sample-rate conversion | Pitch kept within 0.05 Hz from 8, 22.05, 32 and 48 kHz; no step at block boundaries | Host tests |
| Mixer and limiter | Layers keep their channels; all layers at maximum stay under the ceiling without wrapping; pause fades and keeps the place; an empty buffer does not disturb the tone | Host tests |
| Noise | White, pink, brown: no offset, in range, the expected colour | Host tests |
| WAV on the board | A test file written by the device plays into the buffer; a missing file is reported | Serial (`mktest`, `play`, `player`) |
| MP3 on the board | 128 kbit/s stereo 44.1 kHz, 320 kbit/s 48 kHz and 48 kbit/s mono 22.05 kHz decode into the buffer; 53 kB of heap free meanwhile | Serial |
| Heap | Reduced libraries: 99 kB free in Play mode, 86 kB with a WAV, 53 kB with an MP3 | Serial |
| The sound itself, underruns while streaming, the SND and noise controls, track following track | **Not verified: no Bluetooth device was connected, so nothing drained the buffer, and nobody has used the new screens** | Needs a headset and a finger |

### To check on the bench

1. Put a Bluetooth headset or speaker in pairing mode. On HOME tap the
   `BT:` button; the device should appear in the list within about half a
   minute. Tap it; the button on HOME then shows its name.
2. Tap `OFF 30%` to start the tone: 200 Hz in the left ear, 210 Hz in the
   right, fading in over half a second with no click. `+` and `-` change
   the volume in steps of 5 %. On FREQ the buttons change the tone while it
   plays, without clicks.
3. Restart the device with the headset on: it should reconnect by itself,
   and stay silent until audio is turned on.
4. Colours: cyan, teal and violet on near-black. If red and blue look
   swapped, the colour order is wrong.
5. With sound files in `RESONANCE/AUDIO` on the card, the SND tab lists
   them; tapping one plays it under the tone, `PAUSE`/`PLAY` and `-`/`+`
   work, and the next file follows when one ends. On FREQ the noise button
   cycles OFF, WHITE, PINK, BROWN.
6. With a FAT32 card inserted, the Home screen shows its free space, and the card afterwards
   contains the `/RESONANCE` directory tree.
7. A cold start from a direct USB port, and one with a battery connected,
   to see whether the power loss at radio start returns (M11).

## Stage 3 — Audio prototype over Bluetooth

Implemented (firmware 0.3.0); see the table above for what is verified.
Remaining for this stage: the bench checks 1-3, and the Play-mode heap
while a device is connected and streaming.

## Stage 4 — Playback and mixing

Implemented (firmware 0.4.0); see the table above. Remaining for this
stage: the bench checks; previous/next, repeat, shuffle and playlists
(RES-PLAY-002); crossfade (optional); a two-hour playback run with heap and
underrun counters logged.

## Stage 5 — Bluetooth device management

| Step | Deliverable | Test |
|---|---|---|
| 5.1 | Remembered devices, forget, selection | With two sinks |
| 5.2 | Connection loss and manual reconnect | Restart the device; restart the headset; walk out of range |
| 5.3 | Bluetooth screen in Settings | On the device |

## Stage 6 — Session engine

| Step | Deliverable | Test |
|---|---|---|
| 6.1 | Portable sweep engine and session model | Host tests: linear, stepped, hold, multi-stage, pause and resume exactness |
| 6.2 | `SessionManager`, durations, actions, end-of-session fades | Short sessions on the device; time derived from audio frames |
| 6.3 | Presets and sessions as JSON on the card, factory presets | Host tests for parsing and rejection of corrupt files |

## Stage 7 — Web interface (Setup mode)

| Step | Deliverable | Test |
|---|---|---|
| 7.1 | REST API, authentication, Wi-Fi configuration page | API tests from a script; unauthenticated requests refused |
| 7.2 | Library and file management through `sdcard` | Host tests for the path rules (traversal, absolute paths, bad extensions) |
| 7.3 | Upload: several files, progress, cancel, atomic rename, signature check, overwrite confirmation | Interrupt an upload; upload a renamed non-audio file |
| 7.4 | Presets, sessions, settings, backup and restore | Desktop and phone browsers |
| 7.5 | USB firmware update procedure documented (no OTA) | Followed once from the document |

## Stage 8 — Product refinement

| Step | Deliverable |
|---|---|
| 8.1 | Complete screens, visualisations, reduced-animation mode |
| 8.2 | Start-up behaviour, error reporting, recovery after power loss |
| 8.3 | The acceptance run of the request §19, recorded as executed or not run |
| 8.4 | Deliverable documents of RES-DOC-001 |

## Reported limitations

Per RES-PROC-005. Each follows from a decision or a measurement, and none is
silent.

| Requested | Delivered | Why |
|---|---|---|
| Wired stereo output | On hold | ADR-000204 |
| Battery status | None | ADR-000204; the board has no measurement circuit |
| OTA update | USB update | Decided when the image did not fit two slots; it now would (1.58 MB), to be revisited with the web interface |
| Web control of live playback; browser mirroring the touchscreen | Web administration only in Setup mode, where no audio runs | 50 kB of heap with both radios up |
| LVGL-class interface | Own widget layer | Heap and flash |

## How the work is tracked

- Each stage is a work record under request REQ-CHAT-20261117-001, through
  the repository's lifecycle (`AGENTS.md`).
- Host tests are the suite `resonance_core` in `scripts/test_manifest.json`
  and run in tier t1.
- Still to do through a process-change request, because `devsys.json` is a
  protected artifact: the build adapter, `project.source_roots`, and the
  layer rule of `ARCHITECTURE.md` as an invariant.
- Tests that need the device are recorded with their real outcome. What
  needs a person at the bench is listed as such and is never recorded as
  passed without one (RES-PROC-004).

## Risks

| Risk | Likelihood | Effect | Response |
|---|---|---|---|
| Play-mode heap too small with a device connected and an MP3 playing | Low to medium: 53 kB free with an MP3 decoding, before a connection | Dropouts or failure to start | Measure at 3.5; custom library build without BLE and HFP |
| Power loss at radio start recurs | Seen once (16 events) | Device restarts; the guard skips the radio | Direct USB port, battery; M11 |
| Board differs from schematic V1.0 | Medium | Wrong pins | So far the touch and display pins behave as drawn |
| Dropouts from SD, display or radio contention | Medium | Audible glitches | Separate SPI buses; ring buffers; priorities; counters |
| Display is not as configured (colour order, inversion) | Unknown until seen | Wrong colours | One configuration line |
| Reconnection to some headsets unreliable | Medium | Manual reconnect needed | Manual reconnect is specified; document tested devices |
