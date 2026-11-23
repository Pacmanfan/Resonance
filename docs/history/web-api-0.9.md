# Resonance Web API

> **Historical:** the web interface described here existed up to firmware
> 0.9.6 and was removed in 0.10.0 (ADR-000205). Kept for reference. The
> requirements it names are withdrawn, and the source files `net/webui.cpp`
> and `net/webpage.h` no longer exist. The text below is as it stood at 0.9.6.

**Requirement:** RES-WEB-001, RES-WEB-004..006, RES-NET-002, RES-NET-004 (`docs/requirements/resonance.md`)
**Code:** `firmware/src/net/webui.cpp`, page in `firmware/src/net/webpage.h`, rules in `firmware/lib/resonance_core/src/rc_paths.h`
**Status:** first part of the web interface (firmware 0.7.0). The server starts on the device; **no request has been made to it over the network yet.**

The web interface exists in Setup mode only (ADR-000204). The device serves
one page at `/` and the endpoints below on port 80, at the address shown on
its screen (`http://192.168.4.1` on its own access point, or the address and
`<name>.local` on a home network).

## Authentication

Every request, the page included, needs HTTP Digest authentication:

| | |
|---|---|
| User | `resonance` |
| Password | The device password shown on the Setup screen (also the password of the device's own Wi-Fi network) |

The password is generated on the device from its hardware random generator
and is never sent by the device in any answer. There is no unauthenticated
endpoint.

## Conventions

- `GET` endpoints take query arguments; `POST` endpoints take form fields
  (`application/x-www-form-urlencoded`), except the upload.
- Answers are JSON. An error is `{"error":"<text>"}` with a 4xx or 5xx status.
- `dir` is a folder of the sound library, relative to `RESONANCE/AUDIO` on
  the card: empty for the library itself, otherwise names joined by `/`
  (`OCEAN`, `NATURE/Birds`). `..`, `.`, a leading or trailing `/`,
  backslashes and empty parts are refused with 400.
- A file or folder name is 1 to 60 printable ASCII characters without
  `/ \ : * ? " < > |`, not starting with a dot or space, not ending with a
  dot or space.

## Endpoints

| Method and path | Arguments | Answer |
|---|---|---|
| `GET /` | — | The page |
| `GET /api/status` | — | `{"name","version","mode":"setup","sd":bool,"totalMb","freeMb","network","ssid","heap"}`. `ssid` is the saved network's name; its password is never included |
| `GET /api/files` | `dir` | `{"entries":[{"n":name,"d":0 or 1,"s":bytes}]}`: the folder's sub-folders and `.wav` / `.mp3` files |
| `POST /api/mkdir` | `dir`, `name` | `{"ok":true}`; 409 if it exists |
| `POST /api/delete` | `dir`, `name` | `{"ok":true}`; 409 for a folder that is not empty |
| `POST /api/upload` | query: `dir`, `size` (bytes), `overwrite=1` (optional); body: `multipart/form-data` with one file | `{"ok":true}` |
| `POST /api/wifi` | `ssid`, `password`, `hostname` | `{"ok":true,"restarting":true}`; an empty `ssid` forgets the network. The device restarts in Setup mode |
| `POST /api/restart` | `mode` = `play` or `setup` | `{"ok":true,"restarting":true}` |

## Uploads

- One file per request. The page sends several files one after another.
- The name must end in `.mp3` or `.wav`. The type the browser declares is
  ignored: the first bytes of the file must be a WAV header, an ID3 tag or
  an MPEG audio frame, and must agree with the extension (415 otherwise).
- The file is written as `<name>.part` and renamed when it has arrived
  complete. An interrupted upload leaves no file under the real name; a
  leftover `.part` is removed at once, or at the next start of Setup mode.
- An existing file is not replaced unless `overwrite=1` is given (409
  otherwise). The page asks before it sends that.
- 507 when the card has too little free space for `size`.

## Not built yet

Renaming files, playlists, sessions and presets, audio and system settings,
configuration backup and restore, live status over WebSocket. Control of
playback from the browser is not possible in this design: nothing plays in
Setup mode.
