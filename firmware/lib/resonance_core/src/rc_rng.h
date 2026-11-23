// Resonance portable core: random-number sources for Mindfield (RES-MIND-001).
//
// Mindfield never asks "give me random numbers"; it asks a named source, and
// records which one answered. A source says what kind of thing it is, whether
// it can be used now, and whether its output is established physical entropy
// at this moment. A source that cannot deliver says so: nothing here ever
// falls back to software in its place.
//
// The device's sources (hardware generator, ADC, timing) are in
// firmware/src/mind/; the software control source is here because it is
// arithmetic and the host tests need it.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

// What a source is, as shown to the user. Only HardwareEntropy may ever be
// described as physical entropy, and only while physicalEntropy() is true.
enum class RngClass : uint8_t {
    HardwareEntropy = 0,     // a generator documented by its maker as physical
    ExperimentalNoise = 1,   // a physical signal whose entropy is not established
    ExperimentalTiming = 2,  // timing differences; physical share not established
    External = 3,            // hardware that is not part of the board
    SoftwareControl = 4,     // deterministic arithmetic: a control, never "random"
};

// Stored in experiment files: never renumber.
enum class RngId : uint8_t {
    None = 0,
    Esp32Hardware = 1,
    InternalAdc = 2,
    AdcNoise = 3,
    TimingJitter = 4,
    External = 5,
    Software = 6,
};

const char* rngClassName(RngClass c);

class RngSource {
public:
    virtual ~RngSource() = default;
    virtual RngId id() const = 0;
    virtual RngClass rngClass() const = 0;
    virtual const char* name() const = 0;
    // Prepares the source for collection. False when it cannot be used now;
    // status() then says why.
    virtual bool begin() = 0;
    virtual void end() {}
    // Can be selected and started at this moment.
    virtual bool available() const = 0;
    // Its output is established physical entropy at this moment. False for
    // every experimental and software source, whatever it produces.
    virtual bool physicalEntropy() const = 0;
    // Fit for an experiment whose result is to be read as a measurement. A
    // source can be available for study without being suitable: its own
    // statistics would be mistaken for an effect.
    virtual bool suitable() const = 0;
    // Highest rate, in 32-bit samples a second, it has been validated for.
    virtual uint16_t maxRate() const = 0;
    // Fills `out` with `count` raw 32-bit samples. False when it could not:
    // the caller records a gap, and `out` is not to be used.
    virtual bool collect(uint32_t* out, size_t count) = 0;
    // One short line: state and known limitation.
    virtual const char* status() const = 0;
};

// The software control source: xoshiro128** seeded through splitmix32. The
// same seed gives the same samples on every machine, which is its purpose:
// testing the statistics and the pictures, repeating a run, and showing what
// chance alone looks like through the whole pipeline.
class SoftwareRng final : public RngSource {
public:
    explicit SoftwareRng(uint32_t seed = 1) { setSeed(seed); }
    void setSeed(uint32_t seed);
    uint32_t seed() const { return _seed; }
    uint32_t next();

    RngId id() const override { return RngId::Software; }
    RngClass rngClass() const override { return RngClass::SoftwareControl; }
    const char* name() const override { return "Software PRNG"; }
    bool begin() override;
    bool available() const override { return true; }
    bool physicalEntropy() const override { return false; }
    bool suitable() const override { return true; }  // as a control, which is what it is
    uint16_t maxRate() const override { return 1000; }
    bool collect(uint32_t* out, size_t count) override;
    const char* status() const override { return "Deterministic control, not random"; }

private:
    uint32_t _seed = 1;
    uint32_t _s[4] = {0, 0, 0, 0};
};

// A place for hardware that is not fitted: always unavailable. A real
// external generator replaces this class; see docs/MINDFIELD.md for the
// serial framing it is expected to speak.
class ExternalRngPlaceholder final : public RngSource {
public:
    RngId id() const override { return RngId::External; }
    RngClass rngClass() const override { return RngClass::External; }
    const char* name() const override { return "External RNG"; }
    bool begin() override { return false; }
    bool available() const override { return false; }
    bool physicalEntropy() const override { return false; }
    bool suitable() const override { return false; }
    uint16_t maxRate() const override { return 0; }
    bool collect(uint32_t*, size_t) override { return false; }
    const char* status() const override { return "No external generator fitted"; }
};

}  // namespace rc
