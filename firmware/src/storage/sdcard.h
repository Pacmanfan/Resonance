// The microSD card and its SPI bus. This module is the only owner of both
// (SDCardManager in the architecture). The board has no card-detect signal,
// so presence is found out by trying.
#pragma once

#include <stdint.h>

namespace sdcard {

struct Info {
    bool mounted;
    uint32_t totalMb;
    uint32_t freeMb;
};

// Mounts the card and creates the /RESONANCE directory tree if it is missing
// (RES-PLAY-004). Returns false when no usable card is present.
bool begin();

// Unmounts and mounts again; use after a failed operation or a card change.
bool remount();

// Current state. `mounted` is false after a failed mount.
Info info();

}  // namespace sdcard
