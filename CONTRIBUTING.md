# Contributing to Resonance

Thank you for looking. Useful contributions include bug reports with a
serial log, measurements on other boards of the ESP32-2432S0xx family,
listening reports ("this fade clicks"), prints of the enclosure, and code.

## Development environment

- [PlatformIO](https://platformio.org/) Core 6 or later. The platform and
  library versions are pinned in `firmware/platformio.ini`.
- Python 3.9 or later.
- g++ or clang++ with C++17, for the host tests.
- An ESP32-2432S032C board on USB-C, a Bluetooth headset, a FAT32 microSD
  card.
- OpenSCAD, for the enclosure (`hardware/README.md` names the version used).

## Build and test

```bash
python3 firmware/tools/host_tests.py          # all host tests
python3 firmware/tools/host_tests.py tone lab # only test_tone.cpp and test_lab.cpp
cd firmware && pio run                        # build
cd firmware && pio run -t upload              # flash
```

The host tests compile the portable core with `-Wall -Wextra -Werror` and
the address and undefined-behaviour sanitizers. A change to the core needs a
test; a change that cannot be tested on the host needs a note saying how it
was checked on the board. Do not weaken or delete a test to make a change
pass.

Useful on the board (115200 baud): `report`, `heap`, `bench audio`, `bt`,
`player`, `ui`, `tap <x> <y>`. `firmware/README.md` lists every command.

## What the code must keep to

- **The portable core stays portable.** Nothing under
  `firmware/lib/resonance_core/` includes an Arduino, ESP-IDF or display
  header.
- **The audio render path never blocks, allocates, logs or touches a
  file.** Values cross from the interface to the render as single atomics.
- **A new stage in the signal path is exactly neutral at rest.** With a
  feature off, the output is sample for sample what it was; the tests
  compare.
- **Memory is the scarce thing.** No PSRAM; while an MP3 streams over
  Bluetooth at least 16 kB of data memory must stay free. Measure with
  `heap` before and after, and say so in the pull request.
- **No GUI framework, no network.** The interface is a small widget layer
  on LovyanGFX. Wi-Fi was removed on purpose (ADR-000205).
- **No claims.** Preset names say where a number comes from. Nothing in the
  code, the interface or the documents states an effect on health or mind.
- **Say what was verified and what was not.** "Builds" and "passes host
  tests" are not "works on the device", and that is not "sounds right".

Style: C++17, four spaces, about 120 columns, comments that say why. Follow
the file you are in.

## Branches, commits and pull requests

- `master` is the stable branch. Work on a branch and open a pull request
  against `master`.
- One topic per pull request. Commit messages say what changed and why, in
  the imperative ("Fade the recording layer on stop").
- Do not commit build output (`firmware/.pio/`, `build/`, `tmp/`), IDE
  settings, sound files, credentials or anything personal.
- In the pull request say which board you tested on, what you measured and
  what you could not check.

## Issues

For a bug, give the firmware version (SET tab, or `report`), what you did,
what happened, what you expected, and the serial output if you can. For
audio problems say which headset and whether a file was playing.

## Requirements, decisions and the records in this repository

What the device must do is written in `docs/requirements/resonance.md`;
decisions that shape the architecture are in `docs/decisions/`. A change of
behaviour updates the requirement; a change of architecture adds a decision
record.

The directories `requests/`, `work/`, `evidence/` and `scripts/` belong to
the development-control system the project was built with (`AGENTS.md`,
`docs/README.md`). Outside contributors do not need to use it; maintainers
record merged work there.

## Licence of contributions

Resonance is licensed under GPL-3.0-or-later. By submitting a contribution
you agree that it is licensed under the same terms and that you have the
right to submit it. Keep existing copyright and licence notices; if you
bring in third-party code, name its origin and licence in the pull request
and in `THIRD_PARTY_LICENSES.md`, and make sure the licence is compatible
with GPL version 3.
