// GPIO assignment of the ESP32-2432S032C, from docs/architecture/GPIO_MAP.md
// (manufacturer schematic V1.0 and the C-variant board-definition files).
// This is the only place where pin numbers appear.
#pragma once

#include <stdint.h>

namespace pins {

// Display: ST7789 on the HSPI pins. The panel reset is tied to EN.
constexpr int8_t kLcdSclk = 14;
constexpr int8_t kLcdMosi = 13;
constexpr int8_t kLcdMiso = 12;
constexpr int8_t kLcdCs = 15;
constexpr int8_t kLcdDc = 2;
constexpr int8_t kLcdBacklight = 27;  // N-FET gate, active high

// Capacitive touch: GT911 on its own I2C bus.
constexpr int8_t kTouchSda = 33;
constexpr int8_t kTouchScl = 32;
constexpr int8_t kTouchRst = 25;
constexpr int8_t kTouchInt = 21;  // also on connectors P3 and CN1

// microSD: SPI on the VSPI pins. No card-detect signal.
constexpr int8_t kSdSclk = 18;
constexpr int8_t kSdMiso = 19;
constexpr int8_t kSdMosi = 23;
constexpr int8_t kSdCs = 5;

// Onboard mono amplifier input. The amplifier cannot be disabled, so this
// pin is held low (docs/architecture/HARDWARE.md section 5).
constexpr int8_t kSpeaker = 26;

// RGB status LED, common anode: a low level lights the colour.
constexpr int8_t kLedRed = 4;
constexpr int8_t kLedGreen = 16;
constexpr int8_t kLedBlue = 17;

}  // namespace pins
