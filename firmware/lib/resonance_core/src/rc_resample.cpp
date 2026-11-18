#include "rc_resample.h"

namespace rc {

void Resampler::begin(uint32_t inputRate, uint32_t outputRate) {
    _step = outputRate ? static_cast<uint32_t>((static_cast<uint64_t>(inputRate) << 16) / outputRate) : kOne;
    if (_step == 0) _step = 1;
    _position = kOne;  // the first output frame is the first input frame
    _last[0] = _last[1] = 0;
}

size_t Resampler::maxOutput(size_t inFrames) const {
    return static_cast<size_t>((static_cast<uint64_t>(inFrames) << 16) / _step) + 2;
}

size_t Resampler::process(const int16_t* in, size_t inFrames, int16_t* out) {
    if (inFrames == 0) return 0;
    if (passthrough()) {
        for (size_t i = 0; i < inFrames * 2; ++i) out[i] = in[i];
        return inFrames;
    }
    // Positions run over a virtual array: index 0 is the last frame of the
    // previous block, index i + 1 is in[i].
    size_t written = 0;
    const uint64_t end = static_cast<uint64_t>(inFrames) << 16;
    uint64_t position = _position;
    while (position <= end) {
        const size_t index = static_cast<size_t>(position >> 16);
        const int32_t fraction = static_cast<int32_t>(position & 0xFFFF);
        for (int c = 0; c < 2; ++c) {
            const int32_t a = index == 0 ? _last[c] : in[2 * (index - 1) + c];
            const int32_t b = index < inFrames ? in[2 * index + c] : a;
            out[2 * written + c] = static_cast<int16_t>(a + (((b - a) * fraction) >> 16));
        }
        ++written;
        position += _step;
    }
    _position = static_cast<uint32_t>(position - end);
    _last[0] = in[2 * (inFrames - 1)];
    _last[1] = in[2 * (inFrames - 1) + 1];
    return written;
}

}  // namespace rc
