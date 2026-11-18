#include "sdcard.h"

#include <SD.h>
#include <SPI.h>

#include "../hal/board_pins.h"

namespace sdcard {

void refresh();

namespace {
bool mounted = false;

// Built on first use, not before the scheduler runs.
SPIClass& sdSpi() {
    static SPIClass bus(VSPI);
    return bus;
}

const char* const kTree[] = {
    "/RESONANCE",
    "/RESONANCE/AUDIO",
    "/RESONANCE/AUDIO/OCEAN",
    "/RESONANCE/AUDIO/NATURE",
    "/RESONANCE/AUDIO/RAIN",
    "/RESONANCE/AUDIO/WIND",
    "/RESONANCE/AUDIO/AMBIENT",
    "/RESONANCE/AUDIO/MUSIC",
    "/RESONANCE/AUDIO/CUSTOM",
    "/RESONANCE/PLAYLISTS",
    "/RESONANCE/PRESETS",
    "/RESONANCE/SESSIONS",
    "/RESONANCE/CONFIG",
};

bool mount() {
    // Three open files at most: one being played, one directory being
    // listed, one being written.
    mounted = SD.begin(pins::kSdCs, sdSpi(), 20000000, "/sd", 4, false);
    if (!mounted) {
        refresh();
        return false;
    }
    for (const char* dir : kTree) {
        if (!SD.exists(dir) && !SD.mkdir(dir)) {
            // A card that cannot be written is still usable for playback.
            break;
        }
    }
    refresh();
    return true;
}
}  // namespace

bool begin() {
    sdSpi().begin(pins::kSdSclk, pins::kSdMiso, pins::kSdMosi, pins::kSdCs);
    return mount();
}

bool remount() {
    SD.end();
    return mount();
}

namespace {
Info cached = {false, 0, 0};

bool hasExtension(const char* name, const char* extension) {
    const size_t n = strlen(name), e = strlen(extension);
    return n > e && strcasecmp(name + n - e, extension) == 0;
}

// Calls `visit` for every listable entry of `directory` until it returns
// false. Returns false when the directory cannot be opened.
template <typename Visit>
bool forEachEntry(const char* directory, Visit visit) {
    if (!mounted) return false;
    File dir = SD.open(directory);
    if (!dir || !dir.isDirectory()) return false;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        const char* name = f.name();
        const bool isDir = f.isDirectory();
        f.close();
        if (name == nullptr || name[0] == '.' || name[0] == '\0') continue;
        if (strlen(name) >= kNameLength) continue;
        if (!isDir && !isSoundFile(name)) continue;
        if (!visit(name, isDir)) break;
    }
    dir.close();
    return true;
}
}  // namespace

const char kAudioRoot[] = "/RESONANCE/AUDIO";

bool isMp3(const char* name) { return hasExtension(name, ".mp3"); }
bool isSoundFile(const char* name) { return hasExtension(name, ".wav") || isMp3(name); }

void refresh() {
    cached = {mounted, 0, 0};
    if (!mounted) return;
    const uint64_t total = SD.totalBytes();
    const uint64_t used = SD.usedBytes();
    if (total == 0) {
        // The card stopped answering: treat it as removed.
        mounted = false;
        cached.mounted = false;
        return;
    }
    cached.totalMb = static_cast<uint32_t>(total >> 20);
    cached.freeMb = static_cast<uint32_t>((total - used) >> 20);
}

Info info() {
    cached.mounted = mounted;
    return cached;
}

uint8_t list(const char* directory, uint16_t skip, Entry* out, uint8_t capacity, bool* more) {
    uint8_t count = 0;
    uint16_t seen = 0;
    *more = false;
    forEachEntry(directory, [&](const char* name, bool isDir) {
        if (seen++ < skip) return true;
        if (count == capacity) {
            *more = true;
            return false;
        }
        strlcpy(out[count].name, name, kNameLength);
        out[count].directory = isDir;
        ++count;
        return true;
    });
    return count;
}

bool nextFile(const char* directory, const char* current, char* out, uint8_t capacity) {
    bool haveFirst = false, takeNext = false, found = false;
    char first[kNameLength];
    forEachEntry(directory, [&](const char* name, bool isDir) {
        if (isDir) return true;
        if (!haveFirst) {
            strlcpy(first, name, sizeof(first));
            haveFirst = true;
        }
        if (takeNext) {
            strlcpy(out, name, capacity);
            found = true;
            return false;
        }
        if (strcmp(name, current) == 0) takeNext = true;
        return true;
    });
    if (!found && haveFirst) {
        strlcpy(out, first, capacity);
        found = true;
    }
    return found;
}

bool visit(const char* directory, EntryVisitor visitor, void* context) {
    if (!mounted) return false;
    File dir = SD.open(directory);
    if (!dir || !dir.isDirectory()) return false;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        const char* name = f.name();
        const bool isDir = f.isDirectory();
        const uint32_t size = isDir ? 0 : static_cast<uint32_t>(f.size());
        if (name != nullptr && name[0] != '.' && name[0] != '\0' && (isDir || isSoundFile(name))) {
            visitor(name, isDir, size, context);
        }
        f.close();
    }
    dir.close();
    return true;
}

bool exists(const char* path) { return mounted && SD.exists(path); }

bool makeDirectory(const char* path) { return mounted && SD.mkdir(path); }

bool remove(const char* path) {
    if (!mounted) return false;
    File f = SD.open(path);
    if (!f) return false;
    const bool isDir = f.isDirectory();
    f.close();
    return isDir ? SD.rmdir(path) : SD.remove(path);
}

bool rename(const char* from, const char* to) { return mounted && SD.rename(from, to); }

fs::File openForWrite(const char* path) { return mounted ? SD.open(path, FILE_WRITE) : fs::File(); }

void removeLeftovers(const char* directory, uint8_t depth) {
    if (!mounted) return;
    File dir = SD.open(directory);
    if (!dir || !dir.isDirectory()) return;
    char path[200];
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        const char* name = f.name();
        const bool isDir = f.isDirectory();
        snprintf(path, sizeof(path), "%s/%s", directory, name != nullptr ? name : "");
        f.close();
        if (name == nullptr || name[0] == '\0') continue;
        if (isDir) {
            if (depth > 0) removeLeftovers(path, static_cast<uint8_t>(depth - 1));
        } else if (hasExtension(name, ".part")) {
            SD.remove(path);
        }
    }
    dir.close();
}

}  // namespace sdcard
