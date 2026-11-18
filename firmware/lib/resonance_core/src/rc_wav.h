// Resonance portable core: WAV files (RES-PLAY-001: standard PCM).
// Reads the header through a small byte-source interface, so the same code
// runs on the device and in the host tests, and converts PCM blocks to the
// engine's format (stereo, 16-bit).
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

class ByteSource {
public:
    virtual ~ByteSource() {}
    virtual size_t read(uint8_t* out, size_t count) = 0;  // returns bytes read
    virtual bool seek(uint32_t position) = 0;             // absolute
};

struct WavInfo {
    uint32_t sampleRate;
    uint16_t channels;       // 1 or 2
    uint16_t bitsPerSample;  // 8, 16, 24 or 32 (integer PCM)
    uint32_t dataOffset;     // file position of the first sample
    uint32_t dataBytes;
    uint32_t frameBytes() const { return static_cast<uint32_t>(channels) * (bitsPerSample / 8u); }
    uint32_t frames() const { return frameBytes() ? dataBytes / frameBytes() : 0; }
};

enum class WavError : uint8_t {
    Ok,
    NotWav,       // no RIFF/WAVE signature
    Unsupported,  // compressed, floating point, more than two channels, odd sample size or rate
    Truncated,    // the file ends before the format or the data
};

// Reads the header of a WAV file from the start of `source`. On success the
// source is positioned at the first sample.
WavError readWavHeader(ByteSource& source, WavInfo& out);

// Converts whole frames of PCM in `in` to stereo 16-bit in `out`. Returns the
// number of frames written; `out` must hold bytes / frameBytes() frames.
size_t wavToStereo16(const uint8_t* in, size_t bytes, const WavInfo& info, int16_t* out);

}  // namespace rc
