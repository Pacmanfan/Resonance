#include "rc_rng.h"

namespace rc {

const char* rngClassName(RngClass c) {
    switch (c) {
        case RngClass::HardwareEntropy: return "Hardware entropy";
        case RngClass::ExperimentalNoise: return "Experimental noise";
        case RngClass::ExperimentalTiming: return "Experimental timing";
        case RngClass::External: return "External hardware";
        case RngClass::SoftwareControl: return "Software control";
    }
    return "?";
}

namespace {
uint32_t rotl(uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }
}  // namespace

void SoftwareRng::setSeed(uint32_t seed) {
    _seed = seed;
    // splitmix32 spreads any seed, including 0, over the whole state.
    uint32_t z = seed;
    for (uint32_t& s : _s) {
        z += 0x9E3779B9u;
        uint32_t x = z;
        x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
        x = (x ^ (x >> 13)) * 0xC2B2AE35u;
        s = x ^ (x >> 16);
    }
    if ((_s[0] | _s[1] | _s[2] | _s[3]) == 0) _s[0] = 1;  // the one state xoshiro cannot leave
}

bool SoftwareRng::begin() {
    setSeed(_seed);  // every run from the same seed is the same run
    return true;
}

uint32_t SoftwareRng::next() {
    const uint32_t result = rotl(_s[1] * 5u, 7) * 9u;
    const uint32_t t = _s[1] << 9;
    _s[2] ^= _s[0];
    _s[3] ^= _s[1];
    _s[1] ^= _s[2];
    _s[0] ^= _s[3];
    _s[2] ^= t;
    _s[3] = rotl(_s[3], 11);
    return result;
}

bool SoftwareRng::collect(uint32_t* out, size_t count) {
    for (size_t i = 0; i < count; ++i) out[i] = next();
    return true;
}

}  // namespace rc
