# WORK-000023: Resonance: centred titles in one font, no PLAY marker

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-002
**Evidence:** VER-000023

## Scope

Resonance: centred titles in one font, no PLAY marker

## Implementation

- `firmware/src/gui/ui.cpp`: every screen's title is centred in the title font (18 px); the `PLAY` marker is gone; `SETUP` is still shown, small, at the right in Setup mode. The operator asked for the home title centred, `PLAY` removed and the other titles in the home title's font and size. Version 0.6.3.
- `docs/architecture/ARCHITECTURE.md`, `firmware/README.md`: the mode marker.

## Tests

- No host test: drawing code. On the device: 0.6.3 starts. NOT verified by eye: in particular that "FREQUENCY STUDIO" fits the width at this size, which was estimated, not measured.

**Test-Run:** 20261118T003258-798167

## Regression impact and documentation

- ADR-000204 said the mode is always visible; Play mode is now unmarked by the operator's choice, Setup mode still marked.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-002 -> WORK-000023 -> evidence (pending)

## Transitions

- 2026-11-18T05:31:54.035334+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:33:09.607726+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
