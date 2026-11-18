// Resonance portable core: sample-rate conversion to the engine's 44.1 kHz
// by linear interpolation, block by block. Good enough for ambient material;
// files already at 44.1 kHz pass through untouched.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

class Resampler {
public:
    // Sets the input rate and restarts. The output rate is fixed.
    void begin(uint32_t inputRate, uint32_t outputRate);

    bool passthrough() const { return _step == kOne; }

    // Converts stereo frames from `in` to `out`. Consumes all `inFrames` and
    // returns the frames written; `out` must hold maxOutput(inFrames).
    size_t process(const int16_t* in, size_t inFrames, int16_t* out);

    // Upper bound of the frames process() writes for `inFrames` of input.
    size_t maxOutput(size_t inFrames) const;

private:
    static constexpr uint32_t kOne = 1u << 16;
    uint32_t _step = kOne;      // input frames per output frame, 16.16
    uint32_t _position = 0;     // 16.16, relative to the previous block's last frame
    int16_t _last[2] = {0, 0};  // last frame of the previous block
};

}  // namespace rc
