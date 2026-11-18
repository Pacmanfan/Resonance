#include "sdcard.h"

#include <SD.h>
#include <SPI.h>

#include "../hal/board_pins.h"

namespace sdcard {

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
    // Two open files at most: one being played, one being written.
    mounted = SD.begin(pins::kSdCs, sdSpi(), 20000000, "/sd", 2, false);
    if (!mounted) return false;
    for (const char* dir : kTree) {
        if (!SD.exists(dir) && !SD.mkdir(dir)) {
            // A card that cannot be written is still usable for playback.
            break;
        }
    }
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

Info info() {
    Info i = {mounted, 0, 0};
    if (!mounted) return i;
    const uint64_t total = SD.totalBytes();
    const uint64_t used = SD.usedBytes();
    if (total == 0) {
        // The card stopped answering: treat it as removed.
        mounted = false;
        i.mounted = false;
        return i;
    }
    i.totalMb = static_cast<uint32_t>(total >> 20);
    i.freeMb = static_cast<uint32_t>((total - used) >> 20);
    return i;
}

}  // namespace sdcard
