# ADR-000205: One operating mode: no Wi-Fi, no web administration

**Status:** ACCEPTED
**Date:** 2026-11-23
**Requirements:** `docs/requirements/resonance.md` RES-ARCH-005 (amended), RES-NET-002..005 and RES-WEB-001..008 (withdrawn), RES-CFG-002 (applied)
**Request:** REQ-CHAT-20261117-001, follow-up of 2026-11-23 (owner's brief "GitHub Migration, Open-Source Release & CAD Publication", section 7: "the Setup/web administration mode is unused and can be removed")
**Replaces:** the operating modes of ADR-000204 (its other decisions stand)

## Context

ADR-000204 gave the firmware two modes entered by a restart: Play (Bluetooth,
audio, sessions) and Setup (Wi-Fi, a web page for files and settings). The
split existed because Wi-Fi and Bluetooth A2DP together leave too little
memory on this board. Since then every setting has gained a place on the
touchscreen, sound files are put on the card in a computer, and the owner
does not use Setup mode. It cost 650 kB of flash, 24 kB of static memory, a
stored Wi-Fi password, and a second way for the device to start.

## Decision

1. **One operating mode.** The device always starts Bluetooth audio, playback
   and sessions. The mode setting, its screen, its button on SET and the
   serial commands `mode`, `wifi` and `scan` are removed.
2. **Wi-Fi and the web interface are removed**, not disabled: `net/webui`,
   `net/webpage.h`, `radio/wifi_link`. No network function replaces them.
3. **Settings record version 4** drops the mode, the device name and the
   Wi-Fi fields. A stored version 3 record is read once and its user values
   carried over (`rc::migrateFromV3`); rewriting it removes the stored
   credentials from the device.
4. The radio start guard (a start that keeps losing power skips the radio
   once) stays: it protects the Bluetooth start as well.

## Consequences

- Measured on the board, 0.9.6 → 0.10.0: image 1,720,931 → 1,065,043 bytes;
  static memory 78,368 → 53,872 bytes; free heap at rest with Bluetooth
  started 71,760 → 97,872 bytes. Heap during MP3 playback was not re-measured.
- Files can no longer be uploaded over the network. They are copied to the
  card in a computer, or sent over the serial line (`firmware/tools/serial_put.py`).
- Firmware update stays over USB.
- `docs/architecture/API.md` becomes `docs/history/web-api-0.9.md`.
- If a network function is wanted again it needs a new decision; the memory
  finding of ADR-000204 still holds.
