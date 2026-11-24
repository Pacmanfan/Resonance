<p align="center"><img src="assets/banner.png" alt="Resonance" width="100%"></p>

# Resonance

**Resonance is an open-source ESP32-based binaural meditation device with a
modular audio engine, customizable meditation sessions, and an experimental
platform for exploring physical randomness and meditation.**

It runs on an inexpensive 3.2-inch touchscreen board (the ESP32-2432S032C,
one of the "Cheap Yellow Display" family) and plays through Bluetooth
headphones: one tone in the left ear, a slightly different one in the right,
with noise and sound files from a microSD card mixed in, and timed sessions
that move the frequencies while you listen. It works offline; there is no
account, no cloud and no network connection.

Resonance is a complete, self-contained instrument: a meditation device you
can hold in one hand, an audio laboratory for people who like to shape
sound, and a research platform for experiments with physical randomness
("Mindfield"). Switch it on, connect your headphones, press ON. Everything
is on the touchscreen, and every screen has its own help page.

Firmware 0.13.1 · [GPL-3.0-or-later](LICENSE) · [Changelog](CHANGELOG.md)

## Why you will like it

- **It just works.** Glitch-free 44.1 kHz stereo over Bluetooth, with tone,
  noise, layers and an MP3 all playing at once: no dropouts in any of the
  seven mixes measured. Frequencies glide without a click, because every change is
  phase-continuous.
- **It is easy.** Five tabs, large buttons, and a "?" on every title bar
  that explains the screen you are on. The Home screen always tells you
  what is sounding. Your settings are there again the next time you switch
  on.
- **It is yours.** No account, no cloud, no network, no subscription. Your
  sessions and your sounds live on your own microSD card as plain files.
- **It goes deep.** Start with one button; when you are curious there is a
  layer mixer, a session builder, a breathing pacer, a sound laboratory and
  a full random-number research suite behind the ADV tab.
- **It is inexpensive and open.** One off-the-shelf board, a printed
  enclosure included in this repository, and every line of the firmware
  under the GPL.

## What it does

| | |
|---|---|
| **Binaural tone** | Two pure sine oscillators, one for each ear: primary 20–1000 Hz, beat 0.5–40 Hz, in steps of 0.1 Hz, changed smoothly while you listen |
| **Presets** | 14 primary and 9 beat frequencies one touch away |
| **Noise** | White, pink and brown, mixed under the tone |
| **Your own sounds** | WAV and MP3 from the microSD card, streamed from files of any length, with folders |
| **Sessions** | Seven ready-made sessions with sweeps, holds and a gentle fade at the end, plus a plain timer; pause, resume and stop at any time |
| **Bluetooth** | Finds your headphones or speaker, connects, and reconnects by itself next time |
| **Help** | 17 help topics on the device, one for every screen |

### The studio behind the ADV tab

For those who want more than a preset, Resonance is a small sound studio:

- **Layer mixer** — tone, golden partials, noise and a recording, each with
  its own level and fade, under a soft limiter that keeps everything clean.
- **Golden partials** — two extra tones in each ear at φ and φ² times the
  primary, for a richer sound.
- **Session builder** — compose your own sessions of up to ten stages on
  the touchscreen: linear, exponential and custom sweeps, holds, a
  progressive descent, a recording for each stage. Store them on the card
  and run them like the built-in ones.
- **Breathing pacer** — inhale, hold, exhale, hold, shown on the screen and,
  if you like, gently carried by the sound itself.
- **Frequency sequences** — step through a series of tones; the Solfeggio
  set is included and every value can be edited.
- **Resonance Lab** — phase offset and inversion for each ear, phase sweep,
  stereo width, interaural delay, bilateral alternation, and an A/B switch
  with loudness matching so you can compare two settings fairly.

Everything you set here is remembered on the card.

### Mindfield: a research instrument in your hand

Mindfield turns Resonance into a rigorous experiment platform. It collects
the raw output of the chip's hardware random-number generator on an exact
schedule, up to 1000 samples a second, records every sample to the card
with checksums, and analyses it with honest statistics: bit balance, runs,
byte distribution, autocorrelation, window scores with a correction for
multiple comparisons, and comparison against your own baseline.

- **Twelve live pictures** — orb, balance, star, flame, pendulum, vortex,
  tree, mandala, lotus, river, waterfall, labyrinth — show the data as it
  arrives.
- **Complete records** — every experiment is stored with its settings, can
  be replayed exactly from its file, and is exported to a summary table for
  your own analysis. An experiment interrupted by a power loss is recovered.
- **It keeps up** — in a ten-minute run it recorded 600,000 of 600,000
  samples without a gap, and it runs alongside an MP3 over Bluetooth.
- **It is honest** — baseline, control and sham conditions are built in,
  and results are reported as what they are. Mindfield is an instrument for
  asking questions; it does not claim an answer.

See [docs/MINDFIELD.md](docs/MINDFIELD.md).

### A note on claims

Resonance makes sound and measures randomness, and does both well. It makes
no claim that any frequency, preset or setting has an effect on health or
mind, and no ability to influence randomness has been demonstrated or is
claimed. Try it and judge for yourself.

## Hardware

| Part | Detail |
|---|---|
| Board | ESP32-2432S032C (Sunton), USB-C, capacitive touch |
| Processor | ESP32-WROOM-32 module (ESP32-D0WD-V3, dual core), 4 MB flash, **no PSRAM** |
| Display | 240 × 320, ST7789, SPI |
| Touch | GT911, I2C, polled |
| Storage | microSD on SPI (FAT32) |
| Audio out | Bluetooth Classic A2DP to headphones or a speaker. The board's small amplifier and speaker connector are not used |
| Power | USB-C, or a lithium cell on the board's battery connector |

Limits that shape the firmware: 4 MB of flash with one 3 MB application slot
and no over-the-air update; about 98 kB of free heap at rest and far less
while Bluetooth streams an MP3, so the decoder's buffers live in memory the
Bluetooth stack cannot use, nothing on the audio path allocates, and there
is no GUI framework. Details: [docs/HARDWARE.md](docs/HARDWARE.md).

## Software architecture

```
firmware/lib/resonance_core/   portable core: tone engine, mixer, limiter, noise,
                               resampler, WAV parser, sessions, presets, settings,
                               breathing pacer, sequences, Lab — no device headers
firmware/src/                  the device: Bluetooth output, file player, touchscreen
                               interface, SD card and settings storage, board pins
firmware/tests/                host tests of the core, built with the PC's compiler
firmware/tools/                host test runner, serial capture, serial file upload
hardware/                      enclosure: OpenSCAD sources, STL exports, previews
docs/                          requirements, decisions, architecture, DSP, hardware
```

The audio is pulled by the Bluetooth stack: its callback asks the engine for
a block, and the engine renders tone, partials, noise and whatever the file
player has decoded into a ring buffer. The render path never blocks,
allocates, logs or touches a file.

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — modules, tasks, data flow.
- [docs/DSP.md](docs/DSP.md) — oscillators, noise, mixer, limiter, resampling.
- [docs/requirements/resonance.md](docs/requirements/resonance.md) — what the device must do, requirement by requirement.

## Get it running

A board, a USB-C cable and a few minutes are all it takes. You need [PlatformIO](https://platformio.org/) (Core 6 or later), Python 3.9
or later, a C++17 compiler on the PC for the host tests (g++ or clang++),
and the board on a USB-C cable.

```bash
# host tests of the portable core (no board needed): 20 programs
python3 firmware/tools/host_tests.py

# build the firmware (the first build compiles the ESP-IDF libraries and
# takes several minutes)
cd firmware
pio run

# build and flash over USB
pio run -t upload

# talk to the device (115200 baud); type "help" for the command list
pio device monitor
```

Put sound files under `/RESONANCE/AUDIO/` on a FAT32 microSD card (WAV or
MP3; folders are shown as folders). No sound files are included in this
repository. More in [firmware/README.md](firmware/README.md).

## Enclosure and CAD

The repository includes the enclosure as parametric OpenSCAD source, STL
exports of every part, and preview images rendered from those sources, for
each revision. **Only version 1 is recorded as printed and fitted to the
board; v3, v4 and v4 slim render as valid solids but have not been printed,
and for v2 no print is recorded.** Status per
revision, dimensions' sources and how to regenerate the files:
[hardware/README.md](hardware/README.md).

<p align="center"><img src="hardware/previews/v4_slim_exploded.png" alt="Enclosure v4 slim, exploded view rendered from the OpenSCAD source" width="60%"></p>

## Roadmap

Resonance is complete and in use as it stands. Ideas for what could come
next — wired audio output, battery display, more Mindfield experiment
designs — are collected in [docs/ROADMAP.md](docs/ROADMAP.md).

## Contributing

Bug reports, measurements on other boards of the family, prints of the
enclosure and code are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Copyright © 2026 Pacmanfan and the Resonance contributors.

Resonance — firmware, tools, documentation and the hardware design files in
this repository — is free software under the **GNU General Public License,
version 3 or (at your option) any later version**. See [LICENSE](LICENSE).

In short: you may use, study, change and share it, including commercially.
If you distribute it or something built from it — for example a device with
this firmware — you must pass on the same freedoms: provide the complete
corresponding source under the GPL and keep the notices. There is no
warranty.

The firmware is built with third-party libraries that keep their own
licences; one of them, the Helix MP3 decoder, needs attention before
binaries are distributed. See
[THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
