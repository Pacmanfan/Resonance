# WORK-000026: Resonance Stage 7 (first part): web interface - status, sound files, upload, Wi-Fi

**Status:** completed
**Type:** feature
**Request:** requests/processing/2026-11-17-resonance-complete-project-dev-req-chat-20261117-001.md
**Requirements:** RES-WEB-001, RES-WEB-004, RES-WEB-005, RES-WEB-006, RES-NET-002, RES-NET-004
**Decision:** ADR-000204
**Evidence:** VER-000027

## Scope

The first part of Stage 7: a working web interface in Setup mode for the things that cannot be done on the touchscreen - getting sound files onto the card and giving the device a Wi-Fi network. Not in scope: renaming, playlists, sessions and presets, settings panels, backup and restore.

## Implementation

- `firmware/lib/resonance_core/src/rc_paths.h/.cpp` (new): rules for names and relative paths, sound-file sniffing from the first bytes, JSON escaping.
- `firmware/src/net/webui.h/.cpp`, `firmware/src/net/webpage.h` (new): web server in Setup mode - Digest authentication on every request, status, file listing, mkdir, delete, upload (temporary file, signature check, rename, overwrite only on request), Wi-Fi settings, restart; one self-contained page.
- `firmware/src/storage/sdcard.*`: file-management functions for the server; leftover `.part` files removed at start.
- `firmware/src/main.cpp`: the server is started and served in Setup mode; `REPORT web` line. `firmware/src/gui/ui.cpp`: the Setup screen shows address, login name and password, and no longer says the page is not built; the brightness control is removed from Setup (the operator asked). Version 0.7.0.
- `docs/architecture/API.md` (new), `ARCHITECTURE.md`, `IMPLEMENTATION_PLAN.md`, `README.md` files.
- The operator tried to open the web page in Setup mode and found nothing; it had not been built.

## Tests

- Added `firmware/tests/test_paths.cpp` (8 cases): safe and unsafe names, relative paths including traversal attempts, sound-file names, content sniffing (WAV, ID3, MPEG frames; executables, HTML, AVI, erased flash and layer I refused), extension against content, JSON escaping. 82 host cases in all.
- On the device (serial): in Setup mode `HEAP web server started free 137352` and `REPORT wifi state 3 network 'Resonance-Setup' address 192.168.4.1`.
- NOT verified: **any request over the network.** No page load, login, listing, upload or Wi-Fi change has been made: this computer is not on the device's network, and joining it would have meant taking the operator's Wi-Fi connection and reading the device password off its screen. The JavaScript of the page has not run in a browser.

**Test-Run:** 20261118T011443-896543

## Regression impact and documentation

- The Stage 1 choice of an asynchronous web server is replaced by the core's blocking one (reason in `docs/architecture/ARCHITECTURE.md`).
- The SD card is mounted with four file slots instead of three.

## Traceability

REQ-CHAT-20261117-001 -> RES-WEB-001, RES-WEB-004, RES-WEB-005, RES-WEB-006, RES-NET-002, RES-NET-004 -> WORK-000026 -> evidence (pending)

## Transitions

- 2026-11-18T06:09:24.765799+00:00 created in active by agent-dev-20261117205050-9ef7a9
- 2026-11-18T06:15:09.798694+00:00 active -> completed by agent-dev-20261117205050-9ef7a9
