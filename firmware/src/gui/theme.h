// The RESONANCE visual identity (RES-GUI-002): dark charcoal and midnight
// blue, cyan and teal highlights, violet accents. Colours are RGB565.
#pragma once

#include <stdint.h>

namespace theme {

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr uint16_t kBackground = rgb(0x0B, 0x10, 0x20);
constexpr uint16_t kPanel = rgb(0x16, 0x1D, 0x33);
constexpr uint16_t kPanelPressed = rgb(0x24, 0x30, 0x52);
constexpr uint16_t kOutline = rgb(0x2A, 0x35, 0x55);
constexpr uint16_t kCyan = rgb(0x22, 0xD3, 0xEE);
constexpr uint16_t kTeal = rgb(0x14, 0xB8, 0xA6);
constexpr uint16_t kViolet = rgb(0x8B, 0x5C, 0xF6);
constexpr uint16_t kText = rgb(0xE5, 0xE7, 0xEB);
constexpr uint16_t kTextDim = rgb(0x8A, 0x93, 0xA6);
constexpr uint16_t kWarning = rgb(0xF5, 0x9E, 0x0B);

// Layout of the 240 x 320 portrait screen.
constexpr int16_t kScreenW = 240;
constexpr int16_t kScreenH = 320;
constexpr int16_t kHeaderH = 34;
constexpr int16_t kTabBarH = 46;
constexpr int16_t kContentTop = kHeaderH;
constexpr int16_t kContentBottom = kScreenH - kTabBarH;

}  // namespace theme
