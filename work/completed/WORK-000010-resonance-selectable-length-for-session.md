# WORK-000010: Resonance: selectable length for session programs

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-SESS-002, RES-SESS-003
**Evidence:** VER-000010

## Scope

Resonance: selectable length for session programs

## Implementation

- `firmware/lib/resonance_core/src/rc_session.*`: `scaledSession()` stretches or shortens a session to a given total, each stage keeping its share; factory program names no longer carry a length.
- `firmware/src/main.cpp`, `firmware/src/gui/ui.cpp`: the Length button is shown for programs too and cycles the program's own length and 10 to 60 minutes. Version 0.5.1.
- The operator asked for a way to set a program's duration.

## Tests

- `firmware/tests/test_session.cpp`: scaling gives the exact total for every factory program at eight lengths, keeps each stage's share and its frequencies, leaves endless sessions alone, and a scaled sweep reaches its midpoint at the scaled time. 60 host cases in all.
- On the device: 0.5.1 starts in Play mode. The Length button itself has not been seen or tapped yet.

**Test-Run:** 20261117T233842-590212

## Regression impact and documentation

- None beyond the SESS screen.

## Traceability

REQ-CHAT-20261117-001 -> RES-SESS-002, RES-SESS-003 -> WORK-000010 -> evidence (pending)

## Transitions

- 2026-11-18T04:37:02.528701+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T04:38:52.817437+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
