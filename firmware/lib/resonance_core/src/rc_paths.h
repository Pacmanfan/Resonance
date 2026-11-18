// Resonance portable core: rules for the paths and files that reach the SD
// card from outside (RES-WEB-005). Everything the web interface is given -
// a folder, a file name, the first bytes of an upload - passes through here
// before the card is touched.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

// Longest file or folder name accepted, without its path.
constexpr size_t kMaxNameLength = 60;
// Longest folder path accepted, relative to the sound library.
constexpr size_t kMaxRelativePath = 100;

// A single file or folder name: 1..kMaxNameLength characters, printable
// ASCII, none of  / \ : * ? " < > |  , not starting with a dot or a space
// and not ending with a dot or a space. This is what FAT accepts and what
// cannot leave its folder.
bool isSafeName(const char* name);

// A folder path relative to the sound library: empty (the library itself)
// or safe names joined by single '/', with no leading or trailing '/'.
// "..", ".", absolute paths, backslashes and empty segments are refused.
bool isSafeRelativePath(const char* path);

// True for a safe name ending in .wav or .mp3 (any letter case).
bool isSoundFileName(const char* name);

enum class SoundKind : uint8_t { Unknown, Wav, Mp3 };

// What the first bytes of a file say it is, whatever its name claims:
// RIFF....WAVE for WAV; an ID3 tag or an MPEG audio frame header for MP3.
// At least 12 bytes are needed to tell.
SoundKind sniffSound(const uint8_t* head, size_t length);

// True when the name's extension and the content agree.
bool nameMatchesKind(const char* name, SoundKind kind);

// Writes `text` into `out` as the inside of a JSON string: quotes,
// backslashes and control characters escaped, bytes above 0x7E replaced by
// '?'. Always NUL-terminated; returns false when it did not fit.
bool jsonEscape(const char* text, char* out, size_t capacity);

}  // namespace rc
