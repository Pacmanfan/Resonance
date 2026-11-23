#include "rng_sources.h"

#include <Arduino.h>
#include <esp_bt.h>
#include <esp_cpu.h>
#include <esp_random.h>
#include <esp_rtc_time.h>

namespace {

// ---- A: the chip's hardware random-number generator --------------------------
//
// esp_random() reads the hardware generator. Espressif's header for the
// framework in use (ESP-IDF 5.5) states: "If Wi-Fi or Bluetooth are enabled,
// this function returns true random numbers"; without a radio the same call
// returns pseudo-random numbers unless another entropy source is switched on.
// It also busy-waits so that the register is not read faster than entropy
// arrives. Resonance has no Wi-Fi, so the condition is: the Bluetooth
// controller is enabled. That is checked every time, and when it is not
// true the source is unavailable - it is never replaced by software.
class HardwareRng final : public rc::RngSource {
public:
    rc::RngId id() const override { return rc::RngId::Esp32Hardware; }
    rc::RngClass rngClass() const override { return rc::RngClass::HardwareEntropy; }
    const char* name() const override { return "ESP32 hardware RNG"; }
    bool begin() override { return available(); }
    bool available() const override { return esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED; }
    bool physicalEntropy() const override { return available(); }
    bool suitable() const override { return available(); }
    uint16_t maxRate() const override { return 1000; }
    bool collect(uint32_t* out, size_t count) override {
        if (!available()) return false;
        for (size_t i = 0; i < count; ++i) out[i] = esp_random();
        return true;
    }
    const char* status() const override {
        return available() ? "Radio on: physical entropy" : "Bluetooth off: no entropy";
    }
};

// ---- B: the internal ADC -----------------------------------------------------
//
// The least significant bit of a reading of IO34, where the board's light
// sensor sits at the middle of a high-impedance divider. This is a physical
// signal, but not established entropy: the bit also carries supply ripple,
// the backlight's switching and the light in the room, and it is biased. It
// is recorded raw, one bit a reading and thirty-two readings a sample, with
// no conditioning, so that its statistics show what it is. The entropy mode
// of the SAR ADC that the bootloader uses is not touched: it cannot run
// beside the radio.
constexpr uint8_t kAdcPin = 34;

class AdcRng final : public rc::RngSource {
public:
    rc::RngId id() const override { return rc::RngId::InternalAdc; }
    rc::RngClass rngClass() const override { return rc::RngClass::ExperimentalNoise; }
    const char* name() const override { return "Internal ADC"; }
    bool begin() override {
        probe();
        return _usable;
    }
    // Looked at once: a reading that does not move has no noise in it, and
    // the source is then not offered at all.
    bool available() const override {
        if (!_probed) const_cast<AdcRng*>(this)->probe();
        return _usable;
    }
    bool physicalEntropy() const override { return false; }
    bool suitable() const override { return false; }
    uint16_t maxRate() const override { return 100; }
    bool collect(uint32_t* out, size_t count) override {
        if (!_usable) return false;
        for (size_t i = 0; i < count; ++i) {
            uint32_t word = 0;
            for (uint8_t bit = 0; bit < 32; ++bit) word |= static_cast<uint32_t>(analogRead(kAdcPin) & 1u) << bit;
            out[i] = word;
        }
        return true;
    }
    const char* status() const override {
        if (!_probed) const_cast<AdcRng*>(this)->probe();
        return _status;
    }

private:
    void probe() {
        analogReadResolution(12);
        analogSetPinAttenuation(kAdcPin, ADC_11db);
        uint16_t low = 4095, high = 0;
        for (int i = 0; i < 256; ++i) {
            const uint16_t v = analogRead(kAdcPin);
            if (v < low) low = v;
            if (v > high) high = v;
        }
        _probed = true;
        _usable = high != low;
        if (_usable) snprintf(_status, sizeof(_status), "Raw low bit: entropy unproven");
        else snprintf(_status, sizeof(_status), "no signal");
        (void)low;
    }
    bool _probed = false;
    bool _usable = false;
    char _status[48] = "";
};

// ---- C: a dedicated noise input ----------------------------------------------
//
// IO35 is free and goes only to connector P3 - where nothing is connected. A
// floating pin is not a noise source, so this is not offered until a noise
// circuit (for example an avalanche or zener stage with an amplifier) is
// fitted there.
class AdcNoiseRng final : public rc::RngSource {
public:
    rc::RngId id() const override { return rc::RngId::AdcNoise; }
    rc::RngClass rngClass() const override { return rc::RngClass::ExperimentalNoise; }
    const char* name() const override { return "ADC noise input"; }
    bool begin() override { return false; }
    bool available() const override { return false; }
    bool physicalEntropy() const override { return false; }
    bool suitable() const override { return false; }
    uint16_t maxRate() const override { return 0; }
    bool collect(uint32_t*, size_t) override { return false; }
    const char* status() const override { return "not fitted"; }
};

// ---- D: timing jitter --------------------------------------------------------
//
// Two oscillators: the 150 kHz RC oscillator of the RTC and the 40 MHz
// crystal behind the CPU's cycle counter. Reading the RTC time waits for an
// edge of the RC oscillator; the low bit of the cycle counter at that moment
// depends on where the edge fell. The RC oscillator's jitter is physical,
// but interrupts and the other core also move that moment, and this code
// cannot tell the two apart. It is an experiment, to be compared with the
// hardware generator, and nothing more.
class JitterRng final : public rc::RngSource {
public:
    rc::RngId id() const override { return rc::RngId::TimingJitter; }
    rc::RngClass rngClass() const override { return rc::RngClass::ExperimentalTiming; }
    const char* name() const override { return "Timing jitter"; }
    bool begin() override { return true; }
    bool available() const override { return true; }
    bool physicalEntropy() const override { return false; }
    // Measured on the board: successive bits are correlated (lag 1 about
    // -0.08). For studying the source, not for an experiment.
    bool suitable() const override { return false; }
    uint16_t maxRate() const override { return 100; }
    bool collect(uint32_t* out, size_t count) override {
        for (size_t i = 0; i < count; ++i) {
            uint32_t word = 0;
            for (uint8_t bit = 0; bit < 32; ++bit) {
                (void)esp_rtc_get_time_us();
                word |= (esp_cpu_get_cycle_count() & 1u) << bit;
            }
            out[i] = word;
        }
        return true;
    }
    const char* status() const override { return "Bits correlated: study only"; }
};

HardwareRng hardware;
AdcRng adc;
AdcNoiseRng adcNoise;
JitterRng jitter;
rc::ExternalRngPlaceholder external;
rc::SoftwareRng softwareRng(1);

}  // namespace

namespace rngsources {

rc::RngSource& at(uint8_t index) {
    switch (index) {
        case 0: return hardware;
        case 1: return adc;
        case 2: return adcNoise;
        case 3: return jitter;
        case 4: return external;
        default: return softwareRng;
    }
}

uint8_t indexOf(rc::RngId id) {
    for (uint8_t i = 0; i < kCount; ++i) {
        if (at(i).id() == id) return i;
    }
    return kCount;
}

rc::SoftwareRng& software() { return softwareRng; }

}  // namespace rngsources
