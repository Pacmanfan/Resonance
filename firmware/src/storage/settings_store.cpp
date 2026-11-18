#include "settings_store.h"

#include <Preferences.h>
#include <esp_random.h>

namespace storage {

namespace {
const char kNamespace[] = "resonance";
const char kKey[] = "settings";

// The set-up access point needs a password that cannot be derived from
// anything the device broadcasts, so it is drawn from the hardware random
// generator once and then kept.
void ensureApPassword(rc::Settings& s) {
    if (s.apPass[0] != '\0') return;
    static const char kAlphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";  // no look-alikes
    for (size_t i = 0; i < 10; ++i) {
        s.apPass[i] = kAlphabet[esp_random() % (sizeof(kAlphabet) - 1)];
    }
    s.apPass[10] = '\0';
}
}  // namespace

LoadResult load(rc::Settings& out) {
    Preferences prefs;
    LoadResult result = LoadResult::Defaults;
    rc::setDefaults(out);
    if (prefs.begin(kNamespace, true)) {
        if (prefs.getBytesLength(kKey) == sizeof(rc::Settings)) {
            rc::Settings stored;
            if (prefs.getBytes(kKey, &stored, sizeof(stored)) == sizeof(stored)) {
                const bool repaired = rc::sanitize(stored);
                out = stored;
                result = repaired ? LoadResult::Repaired : LoadResult::Ok;
            }
        }
        prefs.end();
    }
    if (out.apPass[0] == '\0' || result != LoadResult::Ok) {
        ensureApPassword(out);
        save(out);
    }
    return result;
}

bool save(rc::Settings& s) {
    rc::sanitize(s);
    ensureApPassword(s);
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    const bool ok = prefs.putBytes(kKey, &s, sizeof(s)) == sizeof(s);
    prefs.end();
    return ok;
}

bool saveMode(rc::Mode mode) {
    rc::Settings s;
    load(s);
    s.mode = mode;
    return save(s);
}

namespace {
const char kGuardKey[] = "radio_tries";

uint8_t readGuard() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return 0;
    const uint8_t tries = prefs.getUChar(kGuardKey, 0);
    prefs.end();
    return tries;
}

void writeGuard(uint8_t tries) {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return;
    if (prefs.getUChar(kGuardKey, 0) != tries) prefs.putUChar(kGuardKey, tries);
    prefs.end();
}
}  // namespace

namespace {
// NVS keys are at most 15 characters: "bt" and the twelve hex digits of the
// address make fourteen.
void deviceKey(const uint8_t address[6], char key[16]) {
    snprintf(key, 16, "bt%02x%02x%02x%02x%02x%02x", address[0], address[1], address[2], address[3], address[4], address[5]);
}
}  // namespace

void saveDeviceName(const uint8_t address[6], const char* name) {
    if (name == nullptr || name[0] == '\0') return;
    char key[16];
    deviceKey(address, key);
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return;
    // Written only when it differs, to spare the flash.
    if (prefs.getString(key, "") != name) prefs.putString(key, name);
    prefs.end();
}

bool loadDeviceName(const uint8_t address[6], char* out, size_t capacity) {
    out[0] = '\0';
    char key[16];
    deviceKey(address, key);
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return false;
    const size_t length = prefs.getString(key, out, capacity);
    prefs.end();
    return length > 0 && out[0] != '\0';
}

bool radioStartKeepsFailing() { return readGuard() >= kRadioStartAttempts; }

void markRadioStarting() {
    const uint8_t tries = readGuard();
    if (tries < 255) writeGuard(static_cast<uint8_t>(tries + 1));
}

void markRadioStable() { writeGuard(0); }

}  // namespace storage
