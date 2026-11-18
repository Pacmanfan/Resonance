// The web administration interface (RES-WEB-*). Runs in Setup mode only
// (ADR-000204), where no audio plays, so the core's small blocking web
// server is enough and nothing can be starved by it.
//
// Handlers never touch hardware: files go through storage/sdcard, settings
// through storage/settings_store, and every name, path and uploaded file is
// checked by the rules of rc_paths.h first. The API is described in
// docs/architecture/API.md.
#pragma once

#include "rc_settings.h"

namespace webui {

// Login name of the web interface. The password is the device password
// shown on the Setup screen.
extern const char kUser[];

// Starts the server on port 80. `settings` must outlive it.
void begin(rc::Settings& settings);

// Serves waiting requests. Call from loop().
void tick();

// True once after a request asked for a restart; `toSetup` says into which
// mode. The caller restarts, after the answer has been sent.
bool takeRestartRequest(bool* toSetup);

// Requests answered since start, and uploads completed (diagnostics).
uint32_t requestCount();
uint32_t uploadCount();

}  // namespace webui
