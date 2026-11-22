#include "session_store.h"

#include <Arduino.h>

#include "rc_session_text.h"
#include "sdcard.h"

namespace sessionstore {

namespace {

const char* lastError = "";

bool pathOf(uint8_t slot, const char* extension, char* out, size_t capacity) {
    if (slot < 1 || slot > kSlots) return false;
    snprintf(out, capacity, "/RESONANCE/SESSIONS/USER%02u.%s", static_cast<unsigned>(slot), extension);
    return true;
}

// Reads one line into `out`, without its line end. Returns false at the end
// of the file; sets `tooLong` when the line did not fit (the rest of it is
// skipped).
bool readLine(fs::File& file, char* out, size_t capacity, bool& tooLong) {
    size_t n = 0;
    bool any = false;
    tooLong = false;
    while (file.available()) {
        const int c = file.read();
        if (c < 0) break;
        any = true;
        if (c == '\n') break;
        if (n + 1 < capacity) out[n++] = static_cast<char>(c);
        else tooLong = true;
    }
    out[n] = '\0';
    return any;
}

}  // namespace

const char* error() { return lastError; }

bool used(uint8_t slot) {
    char path[40];
    return pathOf(slot, "TXT", path, sizeof(path)) && sdcard::exists(path);
}

bool load(uint8_t slot, rc::Session& out) {
    char path[40];
    if (!pathOf(slot, "TXT", path, sizeof(path))) return (lastError = "no such slot"), false;
    fs::File file = sdcard::openForRead(path);
    if (!file) return (lastError = "no session stored here"), false;
    rc::SessionParser parser;
    parser.begin();
    char line[rc::kSessionLineMax];
    bool tooLong = false;
    // A session file has at most 3 + kMaxStages lines; allow for comments.
    for (int lines = 0; lines < 64 && readLine(file, line, sizeof(line), tooLong); ++lines) {
        if (tooLong) {
            file.close();
            return (lastError = "line too long"), false;
        }
        if (!parser.line(line)) break;
    }
    file.close();
    if (!parser.finish(out)) {
        lastError = parser.error()[0] != '\0' ? parser.error() : "values out of range";
        return false;
    }
    lastError = "";
    return true;
}

bool save(uint8_t slot, const rc::Session& session) {
    char path[40], temporary[40];
    if (!pathOf(slot, "TXT", path, sizeof(path)) || !pathOf(slot, "TMP", temporary, sizeof(temporary))) {
        return (lastError = "no such slot"), false;
    }
    if (!rc::isValidSession(session)) return (lastError = "values out of range"), false;
    fs::File file = sdcard::openForWrite(temporary);
    if (!file) return (lastError = "cannot write to the card"), false;
    char line[rc::kSessionLineMax];
    bool ok = true;
    for (uint8_t i = 0; ok && rc::sessionLine(session, i, line, sizeof(line)); ++i) {
        const size_t length = strlen(line);
        ok = file.write(reinterpret_cast<const uint8_t*>(line), length) == length && file.write('\n') == 1;
    }
    file.close();
    if (!ok) {
        sdcard::remove(temporary);
        return (lastError = "the card did not take the file"), false;
    }
    if (sdcard::exists(path)) sdcard::remove(path);
    if (!sdcard::rename(temporary, path)) return (lastError = "cannot rename the file"), false;
    lastError = "";
    return true;
}

bool remove(uint8_t slot) {
    char path[40];
    return pathOf(slot, "TXT", path, sizeof(path)) && sdcard::exists(path) && sdcard::remove(path);
}

bool name(uint8_t slot, char* out, size_t capacity) {
    char path[40];
    if (capacity == 0 || !pathOf(slot, "TXT", path, sizeof(path))) return false;
    fs::File file = sdcard::openForRead(path);
    if (!file) return false;
    char line[rc::kSessionLineMax];
    bool tooLong = false, found = false;
    for (int lines = 0; lines < 8 && !found && readLine(file, line, sizeof(line), tooLong); ++lines) {
        if (!tooLong && strncmp(line, "name=", 5) == 0) {
            size_t length = strlen(line + 5);
            while (length > 0 && (line[5 + length - 1] == '\r' || line[5 + length - 1] == ' ')) --length;
            line[5 + length] = '\0';
            strlcpy(out, line + 5, capacity);
            found = out[0] != '\0';
        }
    }
    file.close();
    return found;
}

}  // namespace sessionstore
