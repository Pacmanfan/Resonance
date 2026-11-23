# WORK-000044: Advanced settings kept on the card

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-CFG-004
**Evidence:** VER-000045

## Scope

Operator request of 2026-11-23: keep the layer mixer, breathing and Resonance
Lab settings on the card, so that they survive a restart (RES-CFG-004). They
were deliberately not kept before; the settings record in the device's own
storage is left as it is, so no stored user setting is lost.

## Implementation

- `firmware/lib/resonance_core/src/rc_state.h/.cpp` (new): the record
  `AdvancedState`, its validation and comparison, its text form and parser.
  `LabSide` moved here from the interface.
- `firmware/src/storage/session_store.h/.cpp`: `statestore`, one file
  `/RESONANCE/ADVANCED.TXT`, written through the same temporary-file-and-rename
  path as sessions and sequences.
- `firmware/src/main.cpp`: `restoreState()` at start (nothing is touched
  without a valid file), `tickState()` once a second writing a change that has
  stood for four seconds, serial `state`, `state save`, `state clear`; the
  serial `layer` commands now also update the record the mixer screen shows.
- `firmware/src/gui/ui.h`: uses `rc::LabSide`.
- `firmware/src/version.h`: 0.9.6.

## Tests

- `firmware/tests/test_state.cpp` (new, 8 cases): defaults at rest, text
  round trip at the limits, later-version lines and keys ignored, 36 bad
  values each refusing the whole text, validation, small buffers.
- On the board (0.9.6, 2026-11-23): values set over serial and by injected
  presses on the Lab screen were written about 4 s after the last change;
  after a restart the board reported `STATE restored` with the pacer running,
  layers and both Lab sides as set; a write succeeded while an MP3 was being
  decoded; after `state clear` and a restart it reported `nothing stored` and
  the defaults.
- Not verified: stream statistics during a write (no headset connected); a
  damaged file on the device (host tests only); nothing was heard.

**Test-Run:** 20261123T002657-3680129

## Regression impact and documentation

- Without a file the engine is not touched at start, so a device without a
  card, or one that never changed these settings, behaves as 0.9.5.
- A device with the pacer left running starts with it running (audio itself
  stays off, RES-CFG-003). Values a session stage sets for the partials
  become the device's, as before, and are therefore kept too.
- Documentation: RES-CFG-004 in `docs/requirements/resonance.md`, manifest
  line, `docs/architecture/IMPLEMENTATION_PLAN.md`.

## Traceability

REQ-CHAT-20261117-001 -> RES-CFG-004 -> WORK-000044 -> evidence (pending)

## Transitions

- 2026-11-23T05:26:52.716898+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T05:28:30.297047+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
