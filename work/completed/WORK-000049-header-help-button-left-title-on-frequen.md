# WORK-000049: Header help button, left title on Frequency, no card free-space line

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-23-resonance-master-development-d-req-chat-20261123-001.md
**Requirements:** RES-GUI-007
**Evidence:** VER-000050

## Scope

Header help button, left title on Frequency, no card free-space line

## Implementation

- `firmware/src/gui/ui.cpp`: the "?" in the header is a button 22 px wide; FREQUENCY STUDIO starts at the left edge (205 px wide, ends 4 px before the button); a title too wide is drawn in the small font (the title and text fonts turned out to be the same width, so the earlier shrink did nothing) and a title that reaches the button is reported by the layout listing; Home shows no free-space line for a mounted card, "SD: no card" stays.
- `firmware/src/gui/ui.h`, `ui_advanced.cpp`: `headerTitle`, `describeTitle` - the `ui` listing prints the title width in the three fonts.
- `firmware/src/version.h`: 0.13.1.

## Tests

- No host-testable code changed. On the board: five tabs, the advanced menu, its five screens and Mindfield walked with injected taps; the `ui` listing reported no title reaching the help button, no text off the screen, no label too wide. Not seen by eye by the developer.
- Measurement owed from WORK-000047: Mindfield at 1000 samples/s with an MP3 over Bluetooth, two 3-minute runs (IMPLEMENTATION_PLAN.md).

**Test-Run:** 20261123T175303-1662249

## Regression impact and documentation

- Header of every screen. CHANGELOG, README version, RES-GUI-007 wording, measurement in IMPLEMENTATION_PLAN.md.

## Traceability

REQ-CHAT-20261123-001 -> RES-GUI-007 -> WORK-000049 -> evidence (pending)

## Transitions

- 2026-11-23T22:45:35.739031+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T22:55:08.042323+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
