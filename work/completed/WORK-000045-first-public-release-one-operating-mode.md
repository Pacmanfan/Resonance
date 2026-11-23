# WORK-000045: First public release: one operating mode, GPL-3.0, documentation, CAD

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-ARCH-005, RES-CFG-002, RES-DOC-001
**Evidence:** VER-000046

## Scope

Owner's brief of 2026-11-23, "GitHub Migration, Open-Source Release & CAD
Publication": remove the Setup boot mode with Wi-Fi and web administration
(ADR-000205), license the project under GPL-3.0-or-later, write the public
documentation, publish the enclosure CAD with STL exports and previews, and
prepare the repository for its first push. The brief names 0.8.2, 39 commits
and nine test programs as baseline; the repository stood at 0.9.6, 46
commits and 14 programs, and the repository was taken as the truth, as the
brief orders. No Mindfield code.

## Implementation

- Firmware 0.10.0: `net/webui*`, `net/webpage.h`, `radio/wifi_link*`,
  `radio/bt_stack.h` deleted; mode, Wi-Fi and web code removed from
  `main.cpp`, `gui/ui*`, `storage/sdcard*`, `storage/settings_store*`;
  settings record version 4 with `rc::migrateFromV3` in `rc_settings`.
- `LICENSE` (GPL-3.0, official text), `THIRD_PARTY_LICENSES.md`,
  `README.md`, `CHANGELOG.md`, `CONTRIBUTING.md`, `docs/ARCHITECTURE.md`,
  `docs/DSP.md`, `docs/HARDWARE.md`, `docs/ROADMAP.md`, `docs/MINDFIELD.md`,
  `assets/` (banner, social preview and the script that draws them).
- `hardware/`: sources moved unchanged to `openscad/`, 16 binary STL parts
  in `stl/`, 30 previews in `previews/`, the development pictures in
  `previews/original/`, the design log as `revisions/HISTORY.md`, new
  `README.md`, `tools/check_stl.py`.
- Existing documents brought in line; `docs/architecture/API.md` moved to
  `docs/history/web-api-0.9.md`. ADR-000205; RES-ARCH-005 amended,
  RES-NET-002..005 and RES-WEB-001..008 withdrawn.

## Tests

- `test_settings`: mode, name and Wi-Fi cases removed with the fields; three
  cases for the version 3 carry-over added. 15 programs, 167 cases, pass.
- Board, 0.10.0: starts; reports "settings repaired" once with the user's
  values kept; SET shows brightness and ADVANCED; `bench audio` 18.0 % of a
  core (unchanged); free heap at rest 97,872 bytes (71,760 before); image
  1,065,043 bytes (1,720,931 before).
- CAD: every part rendered with OpenSCAD 2025.02.11 without warning; every
  STL is one closed shell with the bounding box its source declares.
- Not verified: sound, touch and screens by a person on 0.10.0; heap during
  MP3 playback over Bluetooth on 0.10.0; no print of any enclosure revision
  was made for this work.

**Test-Run:** 20261123T124024-989245

## Regression impact and documentation

- A device that was in Setup mode starts normally. Stored Wi-Fi credentials
  are dropped from the settings record when it is rewritten.
- Files can no longer be uploaded over the network.
- Open: the Helix MP3 decoder is under the RPSL, which is not
  GPL-compatible; no firmware binary is published until the owner decides
  (`THIRD_PARTY_LICENSES.md`). The development-control system's files carry
  no licence notice of their own.

## Traceability

REQ-CHAT-20261117-001 -> RES-ARCH-005, RES-CFG-002, RES-DOC-001 -> WORK-000045 -> evidence (pending)

## Transitions

- 2026-11-23T16:27:47.661853+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-23T17:42:05.368935+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
