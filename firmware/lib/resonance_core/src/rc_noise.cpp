#include "rc_noise.h"

namespace rc {

namespace {
// Coloured noise has rare large peaks; they are held inside the range.
float clamp(float x) { return x > 1.0f ? 1.0f : (x < -1.0f ? -1.0f : x); }
}  // namespace

float NoiseGenerator::white() {
    // xorshift32: fast, and long enough in period for audio.
    _state ^= _state << 13;
    _state ^= _state >> 17;
    _state ^= _state << 5;
    return static_cast<float>(static_cast<int32_t>(_state)) * (1.0f / 2147483648.0f);
}

float NoiseGenerator::next() {
    switch (_type) {
        case NoiseType::White:
            return white() * 0.5f;
        case NoiseType::Pink: {
            // Paul Kellet's economy filter: three one-pole sections over
            // white noise approximate a -3 dB per octave slope.
            const float w = white();
            _pink[0] = 0.99765f * _pink[0] + w * 0.0990460f;
            _pink[1] = 0.96300f * _pink[1] + w * 0.2965164f;
            _pink[2] = 0.57000f * _pink[2] + w * 1.0526913f;
            return clamp((_pink[0] + _pink[1] + _pink[2] + w * 0.1848f) * 0.11f);
        }
        case NoiseType::Brown: {
            // Leaky integration of white noise: -6 dB per octave. The leak
            // keeps it from wandering off to the rails.
            _brown = 0.995f * _brown + white() * 0.06f;
            return clamp(_brown * 0.6f);
        }
        default:
            return 0.0f;
    }
}

}  // namespace rc
