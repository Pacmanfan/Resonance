#include "rc_paths.h"

#include <string.h>

namespace rc {

namespace {

bool hasExtension(const char* name, const char* extension) {
    const size_t n = strlen(name), e = strlen(extension);
    if (n <= e) return false;
    for (size_t i = 0; i < e; ++i) {
        char c = name[n - e + i];
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
        if (c != extension[i]) return false;
    }
    return true;
}

bool safeNameN(const char* name, size_t n) {
    if (n == 0 || n > kMaxNameLength) return false;
    if (name[0] == '.' || name[0] == ' ' || name[n - 1] == '.' || name[n - 1] == ' ') return false;
    for (size_t i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        if (c < 0x20 || c > 0x7e) return false;
        if (strchr("/\\:*?\"<>|", c) != nullptr) return false;
    }
    return true;
}

}  // namespace

bool isSafeName(const char* name) { return name != nullptr && safeNameN(name, strlen(name)); }

bool isSafeRelativePath(const char* path) {
    if (path == nullptr) return false;
    const size_t length = strlen(path);
    if (length == 0) return true;
    if (length > kMaxRelativePath) return false;
    const char* segment = path;
    for (;;) {
        const char* slash = strchr(segment, '/');
        const size_t n = slash != nullptr ? static_cast<size_t>(slash - segment) : strlen(segment);
        if (!safeNameN(segment, n)) return false;  // also refuses "", "." and ".."
        if (slash == nullptr) return true;
        segment = slash + 1;
    }
}

bool isSoundFileName(const char* name) {
    return isSafeName(name) && (hasExtension(name, ".wav") || hasExtension(name, ".mp3"));
}

SoundKind sniffSound(const uint8_t* head, size_t length) {
    if (head == nullptr || length < 12) return SoundKind::Unknown;
    if (memcmp(head, "RIFF", 4) == 0 && memcmp(head + 8, "WAVE", 4) == 0) return SoundKind::Wav;
    if (memcmp(head, "ID3", 3) == 0 && head[3] != 0xFF && head[4] != 0xFF) return SoundKind::Mp3;
    // An MPEG audio frame header: eleven sync bits, then a version that is
    // not "reserved", layer III, and a bitrate and sample rate that are not
    // the forbidden values.
    if (head[0] == 0xFF && (head[1] & 0xE0) == 0xE0) {
        const uint8_t version = (head[1] >> 3) & 0x03;
        const uint8_t layer = (head[1] >> 1) & 0x03;
        const uint8_t bitrate = (head[2] >> 4) & 0x0F;
        const uint8_t rate = (head[2] >> 2) & 0x03;
        if (version != 1 && layer == 1 && bitrate != 0 && bitrate != 15 && rate != 3) return SoundKind::Mp3;
    }
    return SoundKind::Unknown;
}

bool nameMatchesKind(const char* name, SoundKind kind) {
    if (kind == SoundKind::Wav) return hasExtension(name, ".wav");
    if (kind == SoundKind::Mp3) return hasExtension(name, ".mp3");
    return false;
}

bool jsonEscape(const char* text, char* out, size_t capacity) {
    if (capacity == 0) return false;
    size_t at = 0;
    for (const char* p = text; *p != '\0'; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        char piece[7];
        size_t n;
        if (c == '"' || c == '\\') {
            piece[0] = '\\'; piece[1] = static_cast<char>(c); n = 2;
        } else if (c < 0x20) {
            static const char hex[] = "0123456789abcdef";
            piece[0] = '\\'; piece[1] = 'u'; piece[2] = '0'; piece[3] = '0';
            piece[4] = hex[c >> 4]; piece[5] = hex[c & 15]; n = 6;
        } else if (c > 0x7e) {
            piece[0] = '?'; n = 1;
        } else {
            piece[0] = static_cast<char>(c); n = 1;
        }
        if (at + n >= capacity) {
            out[at] = '\0';
            return false;
        }
        memcpy(out + at, piece, n);
        at += n;
    }
    out[at] = '\0';
    return true;
}

}  // namespace rc
