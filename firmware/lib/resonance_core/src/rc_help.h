// Resonance portable core: the help texts shown on the device (RES-GUI-007).
//
// One topic for each screen, plus a first topic that says what can sound and
// where each sound is controlled. The texts are plain ASCII; the device wraps
// them to the width of its screen. A line that starts with "# " is a heading.
// They describe how to use the device and make no statement about effects on
// a listener (RES-PROC-006); the host tests check both.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stdint.h>

namespace rc {

enum class HelpTopic : uint8_t {
    Start = 0, Home, Frequency, Sound, Sessions, AdvTab, Mixer, Breathing, Builder, Sequence, Lab,
    Mindfield, MindSource, MindSetup, MindPicture, MindResult, MindHistory, Count,
};

constexpr uint8_t kHelpTopics = static_cast<uint8_t>(HelpTopic::Count);

// Short name for the list of topics and the screen's title.
const char* helpTitle(uint8_t topic);
// The text; empty for a topic that does not exist.
const char* helpText(uint8_t topic);

}  // namespace rc
