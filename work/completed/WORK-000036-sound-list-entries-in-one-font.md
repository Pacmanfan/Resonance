# WORK-000036: Sound list entries in one font

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-004
**Evidence:** VER-000037

## Scope

Sound list entries in one font

## Implementation

- `firmware/src/gui/ui.cpp`, `ui.h`: list entries are always drawn in the text font, without the file extension, trimmed by measured width with two dots; names were cut to 21 characters before and wide ones fell back to the small font. Version 0.8.2.

## Tests

- Display code, no host test. Built and flashed; the screen has not been seen by the agent, the operator is asked to confirm.

**Test-Run:** 20261118T220341-3520151

## Regression impact and documentation

- Other buttons keep the fall-back to the small font.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-004 -> WORK-000036 -> evidence (pending)

## Transitions

- 2026-11-19T03:02:37.626661+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-19T03:03:42.438436+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
