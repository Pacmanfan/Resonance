// The RESONANCE visual identity (RES-GUI-002): dark charcoal and midnight
// blue, cyan and teal highlights, violet accents. Colours are RGB565.
#pragma once

#include <stdint.h>

namespace theme {

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Contrast comes first on this panel: a near-black background, white text,
// and saturated highlights. Secondary text is light grey, not dim grey, and
// every control has a bright outline.
constexpr uint16_t kBackground = rgb(0x02, 0x04, 0x0C);
constexpr uint16_t kPanel = rgb(0x12, 0x2A, 0x5C);
constexpr uint16_t kPanelPressed = rgb(0x22, 0xD3, 0xEE);
constexpr uint16_t kOutline = rgb(0x5C, 0xE1, 0xF5);
constexpr uint16_t kCyan = rgb(0x40, 0xF0, 0xFF);
constexpr uint16_t kTeal = rgb(0x2D, 0xF0, 0xC8);
constexpr uint16_t kViolet = rgb(0xC4, 0xA0, 0xFF);
constexpr uint16_t kText = rgb(0xFF, 0xFF, 0xFF);
constexpr uint16_t kTextDim = rgb(0xD0, 0xD8, 0xE8);
constexpr uint16_t kWarning = rgb(0xFF, 0xC0, 0x20);

// Layout of the 240 x 320 portrait screen.
constexpr int16_t kScreenW = 240;
constexpr int16_t kScreenH = 320;
constexpr int16_t kHeaderH = 34;
constexpr int16_t kTabBarH = 46;
constexpr int16_t kContentTop = kHeaderH;
constexpr int16_t kContentBottom = kScreenH - kTabBarH;

}  // namespace theme
