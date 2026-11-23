# WORK-000046: Mindfield: random sources, raw collection, statistics

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-23-resonance-master-development-d-req-chat-20261123-001.md
**Requirements:** RES-MIND-001, RES-MIND-002, RES-MIND-003, RES-MIND-004
**Evidence:** VER-000047

## Scope

First stage of the owner's master directive of 2026-11-23 (phases 2 and 3):
the random-number sources, the collection of raw samples, the statistics
engine, and experiments with their files, history, recovery and export.
Driven over the serial line; the screens and the visualizations are the next
stage. The directive's repository, licence and mode-removal parts and its
audio phases were already done (WORK-000038..045).

## Implementation

- Core: `rc_rng` (source interface, software control source, placeholder for
  an external generator), `rc_mindstats` (bit balance, runs, bytes, lags,
  windows with a corrected threshold, comparison of two conditions),
  `rc_mindfile` (raw-file writer and reader, summary text, table rows).
- Device: `mind/rng_sources` (hardware generator tied to the Bluetooth
  controller's state, ADC on IO34 probed for variation, noise input and
  external generator reported as not fitted, timing jitter), `mind/mindfield`
  (sampling task on 1 ms ticks, ring, block writes, end record, summary
  file, list, recovery, export, replay).
- `storage/sdcard`: Mindfield folders, `visitFiles`; `storage/settings_store`:
  experiment numbers; `main.cpp`: `mind` commands; version 0.11.0.

## Tests

- Host: `test_rng` (6), `test_mindstats` (14), `test_mindfile` (13): data
  with known answers (alternating, all ones, 3-in-4 bias, flat bytes,
  repeating runs, drifting windows, five seeds of the control source),
  pieces of any size, interrupted and altered files, refused headers.
- Board, 0.11.0: hardware generator 2000 samples z -0.07, lag-1 z 1.29;
  timing jitter lag-1 z -19.4 (marked not for experiments); ADC reading
  fixed at 0 (reported unavailable); software run 20 000/20 000 samples at
  1000/s with pause and resume, no gaps, replay identical and file intact;
  hardware run 15 000/15 000, no gaps; a run interrupted by reset recovered
  with 7424 samples as Interrupted; export of 3 rows. Free heap 98 kB idle,
  85 kB while collecting, returned afterwards.
- Not verified: collection during MP3 playback over Bluetooth and its effect
  on the stream (no headset connected); runs longer than a minute.

**Test-Run:** 20261123T130244-1045992

## Regression impact and documentation

- Idle cost: 1.5 kB of static memory and 41 kB of flash; nothing runs
  unless an experiment is started.
- `docs/MINDFIELD.md` (what is implemented, measured, and not), README,
  CHANGELOG, ROADMAP, ARCHITECTURE; RES-MIND-001..004.

## Traceability

REQ-CHAT-20261123-001 -> RES-MIND-001, RES-MIND-002, RES-MIND-003, RES-MIND-004 -> WORK-000046 -> evidence (pending)

## Transitions

- 2026-11-23T18:01:36.479107+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T18:04:30.767481+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
