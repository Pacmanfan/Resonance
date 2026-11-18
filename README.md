# Resonance

Resonance is a standalone binaural meditation and ambient audio device on the
ESP32-2432S032C-I board (3.2-inch capacitive touchscreen): two independently
tuned tones, one per ear, over Bluetooth headphones, with sound files from a
microSD card to follow. Tagline: *Find Your Frequency.*

This file is an orientation only. `docs/README.md` is the canonical
documentation entry point.

## The product

* `docs/requirements/resonance.md` — what the device must do.
* `docs/architecture/IMPLEMENTATION_PLAN.md` — where the work stands, stage by
  stage, and what still needs checking on the bench.
* `docs/architecture/HARDWARE.md` — the board as audited and measured.
* `firmware/README.md` — building, flashing, serial diagnostics, unit tests.

## How work is done here

The repository is run by a development-control system for autonomous coding
agents:

* **Requests** — every piece of work is a persistent record under `requests/`.
* **Claims** — an agent claims one request atomically and registers every
  path before editing it; only its own changes are committed.
* **Gated completion** — work completes only with a requirement, tests,
  evidence, documentation and a PASS record of the completion gate.
* **Workers** — unattended agent sessions that take requests from the queue.
* **Agent coordination** — a roster of agents, messages between agents and
  the operator, and live panels.

Python 3.9 or later and git are required; commands run from the repository
root.

```bash
python3 scripts/devsys.py doctor      # environment, configuration, what is not configured yet
python3 scripts/devsys.py status      # workers, claims, inbox, processing, work by state
python3 scripts/create_request.py --source chat --text "what you want" [--priority HIGH]
python3 scripts/devsys.py workers start         # needs an agent command in the configuration
python3 scripts/devsys.py workers status
```

An agent working interactively follows `AGENTS.md` "Request Processing".

## Panels

The first agent that joins the roster starts the station, which opens the
panels; they close after the last agent is gone.

```bash
python3 scripts/msg.py panels                   # where the panels and the push hub are right now
python3 tools/panel/web_panel.py --open         # web control panel (loopback)
python3 tools/panel/dash.py                     # terminal dashboard
tools/worker-panel                              # Qt control panel (needs PySide6)
python3 scripts/msg.py who                      # the agents, what each holds and where it works
```

## Where to read more

* `docs/README.md` — the documentation map.
* `AGENTS.md` — the operating rules every agent follows.
* `docs/process/agent-coordination.md` — the roster, messages, the station
  and the panels.
* `CI_CD.md` — technical map of the development-control system.
