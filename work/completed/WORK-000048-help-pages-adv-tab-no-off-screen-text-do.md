# WORK-000048: Help pages, ADV tab, no off-screen text, Done stops sound

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-23-resonance-master-development-d-req-chat-20261123-001.md
**Requirements:** RES-GUI-007
**Evidence:** VER-000049

## Scope

The owner's remarks after first using Mindfield on the device (2026-11-23):
unclear which sounds are generated and where they are controlled; Stop then
Done should stop the tones; the SET tab to be named ADV; the result screen
crowded and text running past the right edge on several screens - audit and
fix; a help page or pages; the noise-circuit message on the source screen to
go. While the owner was using the device, injected touches and serial
commands from the measurements of this session were starting experiments
and sounds as well; part of the confusion reported was that.

## Implementation

- `rc_help` (new): 17 help topics as plain text.
- `gui/ui_help.cpp` (new): the help overlay, a word wrap to the screen
  width, the list of topics. `gui/ui.cpp`: "?" in every header, title
  shrunk when too wide, tab named ADV, the "On:" line on Home, text that is
  too wide cut and noted. `gui/ui_advanced.cpp`: the `ui` listing reports
  text drawn off the screen.
- `gui/ui_mind.cpp`: result on two pages with MORE, compact source screen,
  shorter lines, wrapped messages. `mind/rng_sources`, `rc_rng`: short
  names and status lines; no mention of a noise circuit.
- `main.cpp`: DONE stops a session, the tone, the noise and a sound file.
- Version 0.13.0.

## Tests

- Host: `test_help` (5 cases): every topic has a title and text, plain
  ASCII, no word longer than a line, none of fifteen words that would
  promise an effect, the Mindfield topics say what it is not, the first
  topic names every sound and its screen. 20 programs.
- Board, 0.13.0, with the new audit: all five tabs, the nine advanced
  screens, the six Mindfield screens with the longest settings, both result
  pages, the help topic list and several help pages: no text off the screen,
  no label too wide, no overlap. Before the fixes the audit found four texts
  off the screen (two on Mindfield screens, two in the help list) and the
  SESSION BUILDER title one pixel too wide.
- Not verified: any of it by eye; DONE stopping sound by ear; screens in
  states not reached (error messages, long device and file names) are
  covered only by the cutting rule.

**Test-Run:** 20261123T141928-1216184

## Regression impact and documentation

- The Bluetooth button on Home is five pixels lower in height to make room
  for the second status line.
- DONE on a result now also ends a running session and pauses a file.
- RES-GUI-007; RES-GUI-006 and RES-MIND-006 amended for the tab name;
  README, CHANGELOG, ARCHITECTURE, MINDFIELD, firmware/README.

## Traceability

REQ-CHAT-20261123-001 -> RES-GUI-007 -> WORK-000048 -> evidence (pending)

## Transitions

- 2026-11-23T19:19:27.725278+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T19:21:16.100594+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
