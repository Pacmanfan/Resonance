# WORK-000051: Screenshots of the device in the README

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-23-resonance-master-development-d-req-chat-20261123-001.md
**Requirements:** RES-GUI-007
**Evidence:** VER-000052

## Scope

Screenshots of the device in the README

## Implementation

- `firmware/src/main.cpp`: serial command `shot` reads the panel back row by row and sends it run-length coded.
- `firmware/tools/screenshot.py`: turns that into a PNG (corrects the one-byte lag of the panel's answer).
- `assets/screenshots/*.png`: eight screens; `README.md` shows them. `version.h`: 0.13.2.

## Tests

- Captures compared with the theme colours in `gui/theme.h` (background and panel colours match to the panel's 6 bits) and looked at. Read-back works on one of the two boards attached; the other's panel returns zeros.

**Test-Run:** 20261123T221030-2219213

## Regression impact and documentation

- The main loop stands still for a few seconds during `shot`; development use only. firmware/README, assets/README, CHANGELOG updated.

## Traceability

REQ-CHAT-20261123-001 -> RES-GUI-007 -> WORK-000051 -> evidence (pending)

## Transitions

- 2026-11-24T02:53:45.921959+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-24T03:10:30.924561+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
