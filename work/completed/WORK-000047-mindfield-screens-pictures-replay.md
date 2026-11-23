# WORK-000047: Mindfield: screens, pictures, replay

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-23-resonance-master-development-d-req-chat-20261123-001.md
**Requirements:** RES-MIND-005, RES-MIND-006
**Evidence:** VER-000048

## Scope

Second stage of the owner's master directive (phases 4 and 5 and the
Mindfield interface): the screens, the twelve pictures and replay.

## Implementation

- Core: `rc_mindviz` - the score a picture shows, its range and smoothing;
  Tree, Mandala and Lotus growth; flow channels and counts; the labyrinth.
- Device: `gui/ui_mind.cpp` - home, source, set-up, active, result and
  history screens and the drawing of each picture; `gui/ui.h` (the Mind
  record and events), `gui/ui.cpp` and `gui/ui_advanced.cpp` (entry on SET,
  navigation); `mind/mindfield` (window sample, animated replay);
  `main.cpp` (events, result with baseline comparison, history, export).
- Version 0.12.0.

## Tests

- Host: `test_mindviz` (11 cases): zero is the centre, no side favoured,
  smoothing never passes or changes the value, chance keeps a score picture
  near the centre over 400 000 samples and does not creep out, a real bias
  moves it in its direction only, the same samples give the same tree, maze
  path, lotus and mandala, the maze is one connected maze. 19 programs, 211
  cases in all.
- Board, 0.12.0: every Mindfield screen listed without a label too wide or
  an overlap; a one-minute directed Orb experiment at 1000 samples/s started,
  paused, resumed and finished from the screen with 60 000 samples and no
  gap, the result opening by itself; each of the other eleven pictures run
  for some seconds without a lost sample or a restart; replay, history
  paging, opening an older experiment and export. Heap 94 kB idle, 80 kB
  lowest; audio render benchmark unchanged at 18.1 %.
- Not verified: any screen or picture by eye; collection and drawing during
  MP3 playback over Bluetooth; runs longer than a minute.

**Test-Run:** 20261123T132204-1090924

## Regression impact and documentation

- The SET tab's ADVANCED button moved up to make room for MINDFIELD.
- 1.7 kB more static memory, 19 kB more flash.
- Fifteen test experiments from this work are on the owner's card.
- RES-MIND-005, RES-MIND-006; `docs/MINDFIELD.md` with the list of what is
  not built; README, CHANGELOG, ROADMAP, ARCHITECTURE.

## Traceability

REQ-CHAT-20261123-001 -> RES-MIND-005, RES-MIND-006 -> WORK-000047 -> evidence (pending)

## Transitions

- 2026-11-23T18:21:08.958791+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T18:23:48.469595+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
