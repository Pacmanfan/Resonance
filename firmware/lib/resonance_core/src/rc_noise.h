// Resonance portable core: white, pink and brown noise (RES-AUDIO-010).
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stdint.h>

namespace rc {

enum class NoiseType : uint8_t { Off = 0, White = 1, Pink = 2, Brown = 3 };

class NoiseGenerator {
public:
    void setType(NoiseType type) { _type = type; }
    NoiseType type() const { return _type; }

    // One sample, roughly within -1..1 for every type.
    float next();

private:
    float white();

    NoiseType _type = NoiseType::Off;
    uint32_t _state = 0x2545F491u;
    float _pink[3] = {0.0f, 0.0f, 0.0f};
    float _brown = 0.0f;
};

}  // namespace rc
