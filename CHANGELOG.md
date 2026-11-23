# Changelog

Firmware versions of Resonance, newest first. Dates and contents are taken
from the Git history (`git log -- firmware/src/version.h`); enclosure changes
are listed under the firmware version that was current at the time.
Versions before 0.10.0 were never published as releases.

"Not verified by a person" means: built, host-tested and exercised on the
board through serial commands, but not yet judged by ear or eye.

## 0.11.0 — 2026-11-23

- Mindfield, first part: random-number sources behind one interface
  (hardware generator, timing jitter, software control; ADC and external
  sources reported as not available on this board), collection of raw
  samples on a fixed schedule to the card, a statistics engine, history,
  recovery of interrupted runs and CSV export. Serial commands only; no
  screens or visualizations yet.

## 0.10.0 — 2026-11-23 — first public release

- **Removed** the separate Setup boot mode, the Wi-Fi link and the web
  administration interface (ADR-000205). The device has one operating mode
  and no network function; every setting is on the touchscreen.
- **Removed** the serial commands `mode`, `wifi` and `scan`.
- Settings record version 4 without mode, device name and Wi-Fi fields. A
  stored version 3 record is carried over once; rewriting it removes stored
  Wi-Fi credentials from the device.
- Firmware image 1,720,931 → 1,065,043 bytes; free heap at rest with
  Bluetooth started 71,760 → 97,872 bytes (measured on the board).
- GPL-3.0-or-later licence, public documentation, enclosure STL exports and
  previews for every revision.

## 0.9.6 — 2026-11-23

- Layer mixer, breathing and Resonance Lab settings are kept in
  `/RESONANCE/ADVANCED.TXT` on the card and restored at start.

## 0.9.5 — 2026-11-22

- Resonance Lab on the generated sound: stereo width, interaural delay,
  bilateral alternation with golden or even timing, A/B comparison with
  A-weighted loudness matching. Not verified by a person.

## 0.9.4 — 2026-11-22

- Advanced screens behind one ADVANCED entry on SET: layer mixer, breathing,
  session builder with stage editor and slots, sequence. Not verified by a
  person.

## 0.9.3 — 2026-11-22

- Breathing pacer with optional holds and optional modulation of tone and
  noise.
- Frequency sequences, with the Solfeggio set as an editable default, stored
  on the card.

## 0.9.2 — 2026-11-22

- Four mixer layers (tone, partials, noise, recording) with enable, level
  and fade.
- Fade-in, fade-out and fade-through for recordings; a session stage can
  name a recording. Measured with a headset: no dropouts, lowest free data
  memory 22.2 kB with MP3, tone, partials and noise together.

## 0.9.1 — 2026-11-22

- Stages that move to their values and then hold; layers per stage;
  progressive descent.
- User sessions of up to ten stages as validated text files on the card.

## 0.9.0 — 2026-11-22

- Phase offset and inversion per channel, phase sweep.
- Golden partials at φ and φ² times the primary frequency.
- Exponential and custom sweeps.

## 0.8.2 — 2026-11-18

- Sound list: every entry in the same font, trimmed by width.
- Enclosure, 2026-11-21: version 4 slim (back shell for a 5.75 mm cell).

## 0.8.1 — 2026-11-18

- Fixed stuttering while an MP3 was loaded: the decoder's ring buffer moved
  to memory the Bluetooth stack cannot use, giving the stack its data memory
  back.

## 0.8.0 — 2026-11-18

- Home rings pulse at the beat frequency; display clock raised to 40 MHz.
- Enclosure: version 1 with board and cell reference models; version 2 with
  two snap-fit shells; version 3 (window cut to the measured glass,
  round-ended USB-C hole); version 4 (2 mm side walls).

## 0.7.1 — 2026-11-18

- Display refresh audit with measurements; `bench` serial command.

## 0.7.0 — 2026-11-18

- Web interface in Setup mode: files, upload, Wi-Fi (removed again in
  0.10.0).

## 0.6.5 — 2026-11-18

- Trance, deep trance and astral session programs (seven factory programs
  in all).

## 0.6.1 – 0.6.4 — 2026-11-18

- 0.6.4: single-screen interface in Setup mode.
- 0.6.3: centred screen titles in one font.
- 0.6.2: tap-and-hold repeat on adjustment buttons.
- 0.6.1: Bluetooth device names kept; user values saved across restarts.
- Enclosure: LED and light-sensor windows, lipped button.

## 0.5.0 – 0.5.7 — 2026-11-17 / 2026-11-18

- 0.5.0: sessions — timers, factory programs, linear sweeps, end fades.
- 0.5.1: selectable length for session programs.
- 0.5.2: frequency presets.
- 0.5.3 – 0.5.7: interface corrections (fonts, preset buttons, labels).
- Enclosure: first parametric case draft.

## 0.4.0 – 0.4.1 — 2026-11-17

- 0.4.0: WAV and MP3 playback from SD, mixer, white / pink / brown noise,
  Bluetooth-only ESP-IDF libraries (57 kB more free heap).
- 0.4.1: Bluetooth list by signal strength, rescan after disconnect.

## 0.3.0 – 0.3.2 — 2026-11-17

- 0.3.0: binaural tone over Bluetooth A2DP; scan and connect on screen.
- 0.3.1: tab bar drawn as buttons; SD card verified.
- 0.3.2: Sound tab labelled SND.

## 0.2.0 – 0.2.1 — 2026-11-17

- 0.2.0: board foundation, portable core, operating modes.
- 0.2.1: touch by polling, high-contrast interface.

## Before 0.2.0 — 2026-11-17

- Product requirements and the hardware and software audit.
