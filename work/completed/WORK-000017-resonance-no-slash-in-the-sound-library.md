# WORK-000017: Resonance: no slash in the sound library's folder line

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-GUI-002
**Evidence:** VER-000017

## Scope

Resonance: no slash in the sound library's folder line

## Implementation

- `firmware/src/main.cpp`: the folder line of the SND tab is empty at the library root and shows a folder without its leading slash (the operator asked for the "/" to be removed). Version 0.5.7.

## Tests

- No host test: display text. On the device: 0.5.7 starts. NOT verified by eye yet.

**Test-Run:** 20261118T000956-670840

## Regression impact and documentation

- None. The operator's question about a simplified Setup-mode screen was answered with a proposal and is awaiting decisions; nothing was built for it.

## Traceability

REQ-CHAT-20261117-001 -> RES-GUI-002 -> WORK-000017 -> evidence (pending)

## Transitions

- 2026-11-18T05:08:47.782177+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T05:09:56.811447+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
