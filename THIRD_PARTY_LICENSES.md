# Third-party software and licences

Resonance's own source, documentation and hardware design files are licensed
under GPL-3.0-or-later ([LICENSE](LICENSE)). **Nothing below is relicensed by
that.** These components are not copied into this repository; PlatformIO
fetches them at build time, and each keeps its own licence and notices. A
firmware image built from this repository contains all of them.

Reviewed on 2026-11-23 against the copies PlatformIO downloaded for
firmware 0.10.0 (`firmware/platformio.ini`).

| Component | Version | Licence | Used for | Compatible with GPL-3.0 distribution |
|---|---|---|---|---|
| [arduino-esp32](https://github.com/espressif/arduino-esp32) | 3.3.13 | LGPL-2.1-or-later | Arduino framework | Yes |
| [ESP-IDF](https://github.com/espressif/esp-idf) and its components (FreeRTOS: MIT; Bluedroid: Apache-2.0; newlib, mbedTLS, FatFs and others under their own permissive licences) | 5.5.6 | Apache-2.0 for Espressif's code | System libraries, Bluetooth stack, file system | Yes (Apache-2.0 is compatible with GPL version 3) |
| [LovyanGFX](https://github.com/lovyan03/LovyanGFX) | 1.3.0 | FreeBSD (BSD-2-Clause), with parts under MIT and BSD from Adafruit GFX and TFT_eSPI; notices in its `license.txt` | Display and touch driver | Yes |
| [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) | 1.8.12 | Apache-2.0 | Bluetooth A2DP source | Yes |
| [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix) — wrapper | commit `f6e55a3` (0.9.4) | GPL-3.0 (its `License.txt`) | C++ wrapper around the decoder | Yes |
| **Helix MP3 decoder** (inside arduino-libhelix, `src/libhelix-mp3`) | as above | **RealNetworks Public Source License 1.0 (RPSL)**, or RCSL for those who hold one; © 1995–2002 RealNetworks | MP3 decoding | **Open question — see below** |

The host tests and tools use only the C++ and Python standard libraries.

## Open question: the Helix MP3 decoder (RPSL)

The Helix decoder's source files carry the RPSL 1.0 licence block. The RPSL
is a free-software licence, but the Free Software Foundation lists it as
**incompatible with the GNU GPL**. Publishing Resonance's source under
GPL-3.0 is unaffected, because the decoder is not part of this repository.
What is unsettled is **distributing a compiled firmware image**, which links
Resonance (GPL) and the decoder (RPSL) into one program.

For that reason **this release publishes source only; no firmware binary is
attached.** Building the firmware for your own device is not distribution.

Ways to settle it, which are the copyright holder's decision and have not
been taken:

1. Add an explicit additional permission (GPL-3.0 section 7) to Resonance's
   licence allowing it to be combined with the Helix decoder.
2. Replace the decoder with one under a GPL-compatible licence.
3. Build without MP3 support (WAV only) for distributed binaries.

The AAC decoder in the same library is not compiled into Resonance. MP3
patents have expired.

## Development-control system

The directories `scripts/`, `tools/`, `docs/process/` and the files
`AGENTS.md`, `CI_CD.md` and `devsys.json` are a development-control system
for coding agents that was installed into this repository at its first
commit. Its files carry no copyright or licence notice of their own. It is
published here as part of the repository's history under the repository's
licence on the understanding that it is the owner's own tooling; **if that
is not so, its licence must be clarified.**

## Sound files

No sound files are included in this repository. Recordings a user puts on
the card keep whatever licence they came with.

## Fonts

The interface uses fonts built into LovyanGFX, under that library's licence.
The project images in `assets/` were drawn by a script in this repository
with the DejaVu Sans font (Bitstream Vera licence, public-domain changes).
