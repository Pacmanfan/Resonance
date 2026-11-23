// The microSD card and its SPI bus. This module is the only owner of both
// (SDCardManager in the architecture). The board has no card-detect signal,
// so presence is found out by trying.
#pragma once

#include <stdint.h>

#include <FS.h>

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

// Current state. `mounted` is false after a failed mount. The sizes are
// read from the card on begin(), remount() and refresh(), not on every call.
Info info();

// Reads the sizes again. Scans the card's allocation table the first time,
// which can take seconds on a large card: do not call it during playback.
void refresh();

// Root of the sound library on the card.
extern const char kAudioRoot[];  // "/RESONANCE/AUDIO"

constexpr uint8_t kNameLength = 64;

struct Entry {
    char name[kNameLength];  // file or directory name, without its path
    bool directory;
};

// Lists the sub-directories and sound files (.wav, .mp3) of `directory`,
// skipping the first `skip` of them, into `out` (at most `capacity`).
// Returns the number written; `more` tells whether further entries follow.
// Hidden entries and names too long for an Entry are left out.
uint8_t list(const char* directory, uint16_t skip, Entry* out, uint8_t capacity, bool* more);

// Finds the sound file that follows `current` in `directory` (the first one
// when `current` is the last or is not found). Returns false when the
// directory holds no sound file.
bool nextFile(const char* directory, const char* current, char* out, uint8_t capacity);

// Files other than sounds (stored sessions, sequences, settings, uploads
// over the serial line). Every path is absolute on the card; this module is
// the only one that touches the card (RES-ARCH-002).
// Calls `visitor` for every file (not directory) of `directory`, of any
// kind, with its size, until it returns false. False when the directory
// cannot be opened.
typedef bool (*FileVisitor)(const char* name, uint32_t size, void* context);
bool visitFiles(const char* directory, FileVisitor visitor, void* context);

bool exists(const char* path);
// Removes a file, or a directory that is empty.
bool remove(const char* path);
bool rename(const char* from, const char* to);
// Opens a file for reading; an empty File when it is not there.
fs::File openForRead(const char* path);
// Opens a file for writing, replacing any file of that name.
fs::File openForWrite(const char* path);

// True for a name ending in .wav or .mp3, in any letter case.
bool isSoundFile(const char* name);
bool isMp3(const char* name);

}  // namespace sdcard
