#include "rc_wav.h"

#include <string.h>

namespace rc {

namespace {

uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

constexpr uint16_t kFormatPcm = 1;
constexpr uint16_t kFormatExtensible = 0xFFFE;

}  // namespace

WavError readWavHeader(ByteSource& source, WavInfo& out) {
    uint8_t buf[40];
    if (!source.seek(0) || source.read(buf, 12) != 12) return WavError::NotWav;
    if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) return WavError::NotWav;

    bool haveFormat = false;
    uint32_t position = 12;
    // Walk the chunks. A sane file has a handful; the bound stops a corrupt
    // one from being walked for ever.
    for (int guard = 0; guard < 64; ++guard) {
        if (!source.seek(position) || source.read(buf, 8) != 8) return WavError::Truncated;
        const uint32_t size = le32(buf + 4);
        const uint32_t body = position + 8;
        if (memcmp(buf, "fmt ", 4) == 0) {
            if (size < 16) return WavError::Unsupported;
            const size_t want = size < sizeof(buf) ? size : sizeof(buf);
            if (source.read(buf, want) != want) return WavError::Truncated;
            uint16_t format = le16(buf);
            out.channels = le16(buf + 2);
            out.sampleRate = le32(buf + 4);
            out.bitsPerSample = le16(buf + 14);
            // WAVE_FORMAT_EXTENSIBLE carries the real format in the first two
            // bytes of its sub-format field.
            if (format == kFormatExtensible && want >= 26) format = le16(buf + 24);
            if (format != kFormatPcm) return WavError::Unsupported;
            if (out.channels < 1 || out.channels > 2) return WavError::Unsupported;
            if (out.bitsPerSample != 8 && out.bitsPerSample != 16 && out.bitsPerSample != 24 &&
                out.bitsPerSample != 32) {
                return WavError::Unsupported;
            }
            if (out.sampleRate < 8000 || out.sampleRate > 96000) return WavError::Unsupported;
            haveFormat = true;
        } else if (memcmp(buf, "data", 4) == 0) {
            if (!haveFormat) return WavError::Unsupported;
            out.dataOffset = body;
            out.dataBytes = size - size % out.frameBytes();
            return source.seek(body) ? WavError::Ok : WavError::Truncated;
        }
        // Chunks are padded to an even length.
        const uint32_t next = body + size + (size & 1u);
        if (next <= position) return WavError::Truncated;  // overflow
        position = next;
    }
    return WavError::Truncated;
}

size_t wavToStereo16(const uint8_t* in, size_t bytes, const WavInfo& info, int16_t* out) {
    const uint32_t sampleBytes = info.bitsPerSample / 8u;
    const uint32_t frameBytes = info.frameBytes();
    if (frameBytes == 0) return 0;
    const size_t frames = bytes / frameBytes;
    for (size_t f = 0; f < frames; ++f) {
        int16_t sample[2] = {0, 0};
        for (uint16_t c = 0; c < info.channels; ++c) {
            const uint8_t* p = in + f * frameBytes + c * sampleBytes;
            switch (sampleBytes) {
                case 1: sample[c] = static_cast<int16_t>((static_cast<int>(p[0]) - 128) * 256); break;  // unsigned
                case 2: sample[c] = static_cast<int16_t>(le16(p)); break;
                case 3: sample[c] = static_cast<int16_t>(le16(p + 1)); break;  // top 16 of 24 bits
                default: sample[c] = static_cast<int16_t>(le16(p + 2)); break; // top 16 of 32 bits
            }
        }
        out[2 * f] = sample[0];
        out[2 * f + 1] = info.channels == 2 ? sample[1] : sample[0];
    }
    return frames;
}

}  // namespace rc
