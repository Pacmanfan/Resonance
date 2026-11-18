# WORK-000025: Resonance: trance and astral session programs

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-SESS-002, RES-PROC-006
**Evidence:** VER-000026

## Scope

Resonance: trance and astral session programs

## Implementation

- `firmware/lib/resonance_core/src/rc_session.cpp`: three more factory programs - "Trance 10-4.5 Hz" (30 min), "Deep trance 3.5 Hz" (40 min), "Astral 6.3-4 Hz" (45 min) - each a descent from alpha into theta with long holds. The operator asked for trance / astral projection settings among the programs. Version 0.6.5.
- The frequency values are the ones commonly quoted for these practices, taken from general knowledge; no source was consulted for this change, and nothing is claimed about their effect.
- `docs/architecture/ARCHITECTURE.md`, `firmware/README.md`.

## Tests

- `firmware/tests/test_session.cpp`: the three programs exist, have their stated lengths, only ever descend, leave the carrier to the user, end in a hold of at least a quarter of the program between 3 and 5 Hz; no program name contains a word that promises an effect. 74 host cases in all.
- On the device: 0.6.5 starts, in play mode (read from its report). NOT verified: the programs have not been selected or run on the device.

**Test-Run:** 20261118T010451-871429

## Regression impact and documentation

- The new programs follow the existing four, so a stored program choice still points at the same program.

## Traceability

REQ-CHAT-20261117-001 -> RES-SESS-002, RES-PROC-006 -> WORK-000025 -> evidence (pending)

## Transitions

- 2026-11-18T06:03:23.057268+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T06:05:14.480003+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
