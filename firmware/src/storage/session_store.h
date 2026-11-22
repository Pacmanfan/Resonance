// The user's sessions on the card (RES-SESS-009): text files
// /RESONANCE/SESSIONS/USER01.TXT .. USER12.TXT in the form of
// rc_session_text.h. The card is reached only through storage/sdcard.
//
// A file is read and written a line at a time; nothing here keeps a session
// in memory.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rc_session.h"

namespace sessionstore {

constexpr uint8_t kSlots = 12;  // slots are numbered 1..kSlots

// Reads and validates the session in `slot`. On failure `out` is untouched
// and error() says why.
bool load(uint8_t slot, rc::Session& out);

// Writes `session` to `slot`, replacing what was there. The file is written
// under another name first and renamed, so that an interrupted write never
// leaves half a session under the slot's name. Refuses an invalid session.
bool save(uint8_t slot, const rc::Session& session);

bool remove(uint8_t slot);

// True when the slot holds a file (which may still fail to load).
bool used(uint8_t slot);

// Name of the session in `slot`, for a list; false when the slot is empty
// or its file has no readable name.
bool name(uint8_t slot, char* out, size_t capacity);

// Why the last load() or save() failed; empty after a success.
const char* error();

}  // namespace sessionstore
