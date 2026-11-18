# ADR-000204: Bluetooth-only audio output, no battery measurement, and a space-first firmware with operating modes

**Status:** ACCEPTED
**Date:** 2026-11-17
**Requirements:** `docs/requirements/resonance.md` RES-HW-003, RES-HW-005, RES-HW-006, RES-AUDIO-011, RES-ARCH-004, RES-ARCH-005 (new), RES-GUI-002, RES-WEB-008 (amended or applied)
**Request:** REQ-CHAT-20261117-001, follow-up of 2026-11-17 (operator: "drop battery measurement ... put on hold the external stereo jack connected to i2s - we will rely on BT audio. Code conservatively for space. We may need to page swap in code.")
**Builds on:** the Stage 1 audit (`docs/architecture/`, WORK-000001)

## Context

The Stage 1 audit found that internal RAM, and then flash, limit what the
ESP32-2432S032C can run at once: Wi-Fi with Bluetooth A2DP leaves too little
heap on stock libraries for a GUI framework, an MP3 decoder and a web server
(`docs/architecture/LIBRARY_EVALUATION.md` sections 6 and 7). It also found
that the wired DAC needs soldering to the RGB LED pins and that the board has
no battery measurement. The operator then decided the points below.

## Decision

1. **Bluetooth A2DP is the audio output.** The external PCM5102A, headphone
   amplifier and jack are on hold. The `AudioOutput` interface stays, with
   one implementation, so the wired output can return without touching the
   engine. IO4, IO16, IO17 and IO22 keep their board function; the RGB LED
   remains a status light.
2. **No battery measurement.** No divider is fitted, IO35 stays unused, and
   the firmware shows no battery status. Low-battery detection is therefore
   not available.
3. **Space first.** Where a requirement can be met by a smaller means, the
   smaller means is used:
   - no GUI framework: a small widget layer of our own on LovyanGFX instead
     of LVGL (saves the LVGL heap, its draw buffer and its code);
   - one 3 MB application slot (`huge_app`), so no OTA: firmware is updated
     over USB;
   - no general audio framework; decoders deliver PCM to our mixer;
   - fixed buffers sized in one header, allocated once.
4. **Operating modes ("page swapping").** A subsystem that is not needed in
   the current mode is not started at all, so it costs no RAM. The mode is
   stored in non-volatile memory and changed by a restart, because the
   Bluetooth and Wi-Fi stacks do not return their memory reliably when
   stopped. Two modes exist:
   - **Play:** Bluetooth, audio engine, SD playback, sessions, GUI. Wi-Fi is
     not started.
   - **Setup:** Wi-Fi, web administration, SD file management, GUI.
     Bluetooth and audio are not started.
   Whether a combined mode is offered as well is decided from the heap
   measured on the board (gate G1/G2); the mechanism allows it.

## Consequences

- The wired-output acceptance tests and the stereo-separation measurement at
  a DAC are deferred with the hardware. Channel separation is tested in the
  portable core, and by ear over Bluetooth.
- The sample rate is 44.1 kHz, 16-bit at the output, as A2DP fixes it.
- In Setup mode the web interface manages files, presets, sessions and
  settings but cannot control live playback, and the browser does not mirror
  the touchscreen while audio plays. This limits RES-WEB-003 and the
  dashboard's playback controls unless a combined mode proves possible. It
  is a reported limitation (RES-PROC-005), not a silent omission.
- A mode change takes a restart of a few seconds and stops audio. Audio
  never starts by itself afterwards (RES-CFG-003).
- OTA (RES-WEB-008) is not provided; it was conditional on the flash layout.
- The GUI is plainer than an LVGL one could be. The visual identity
  (RES-GUI-002) is kept with our own drawing code.

## Rejected

- **LVGL** — 24-48 kB of heap, a 10-15 kB draw buffer and several hundred
  kilobytes of flash, for a five-screen interface.
- **Stopping and restarting the radio stacks at run time** — releasing
  Bluetooth memory is one-way in ESP-IDF, and repeated start/stop fragments
  the heap.
- **Keeping the wired output in the first build** — it would hold three pins
  and an I2S driver for hardware that is not connected.
