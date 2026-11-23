// Help on the device (RES-GUI-007). A "?" at the top right of every screen
// opens the help topic of that screen; ALL TOPICS lists every topic. The
// texts are in the portable core (rc_help.h) and are wrapped here to the
// width of the screen, so that no line can run off it.
#include <Arduino.h>

#include "theme.h"
#include "ui.h"

namespace gui {

namespace {

using namespace theme;

const lgfx::IFont* const kFontSmall = &fonts::DejaVu12;

constexpr uint8_t kHelpPrev = 240, kHelpNext = 241, kHelpAll = 242, kHelpTopicBase = 200;
constexpr int16_t kTop = kContentTop;
constexpr int16_t kLineH = 16;
constexpr uint8_t kLines = 12;       // lines of text on a page
constexpr uint8_t kListRows = 9;     // topics on a page of the list, in two columns

}  // namespace

// The topic that belongs to what is on the screen.
uint8_t Ui::helpTopicHere() const {
    using rc::HelpTopic;
    HelpTopic topic = HelpTopic::Start;
    switch (_sub) {
        case Sub::None:
            if (_tab == Tab::Home) topic = HelpTopic::Home;
            else if (_tab == Tab::Frequency) topic = HelpTopic::Frequency;
            else if (_tab == Tab::Sound) topic = HelpTopic::Sound;
            else if (_tab == Tab::Sessions) topic = HelpTopic::Sessions;
            else topic = HelpTopic::AdvTab;
            break;
        case Sub::Menu: topic = HelpTopic::AdvTab; break;
        case Sub::Mixer: topic = HelpTopic::Mixer; break;
        case Sub::Breath: topic = HelpTopic::Breathing; break;
        case Sub::Builder:
        case Sub::Stage:
        case Sub::Slots: topic = _slotKind == 1 && _sub == Sub::Slots ? HelpTopic::Sequence : HelpTopic::Builder; break;
        case Sub::Sequence: topic = HelpTopic::Sequence; break;
        case Sub::Lab: topic = HelpTopic::Lab; break;
        case Sub::Mind: topic = HelpTopic::Mindfield; break;
        case Sub::MindSource: topic = HelpTopic::MindSource; break;
        case Sub::MindSetup: topic = HelpTopic::MindSetup; break;
        case Sub::MindRun: topic = HelpTopic::MindPicture; break;
        case Sub::MindResult: topic = HelpTopic::MindResult; break;
        case Sub::MindHistory: topic = HelpTopic::MindHistory; break;
    }
    return static_cast<uint8_t>(topic);
}

void Ui::openHelp(uint8_t topic) {
    _helpOpen = true;
    _helpList = false;
    _helpTopic = topic < rc::kHelpTopics ? topic : 0;
    _helpPage = 0;
    drawHeader();
    drawContent();
}

// Back to the screen help was opened from, as it is now.
void Ui::closeHelp() {
    _helpOpen = false;
    drawHeader();
    drawContent();
}

uint16_t Ui::wrapped(const char* text, uint16_t first, uint8_t count, int16_t x, int16_t y, int16_t width, int16_t lineH,
                     uint16_t colour) {
    _lcd.setFont(kFontSmall);
    char line[48];
    size_t length = 0;
    uint16_t lines = 0;
    bool heading = false;
    auto flush = [&]() {
        line[length] = '\0';
        if (lines >= first && lines < first + count) {
            label(line, x, static_cast<int16_t>(y + (lines - first) * lineH), heading ? kCyan : colour, kFontSmall);
            _lcd.setFont(kFontSmall);
        }
        ++lines;
        length = 0;
    };
    const char* p = text;
    while (*p != '\0') {
        if (*p == '\n') {  // end of a paragraph; an empty one is a gap
            flush();
            heading = false;
            ++p;
            continue;
        }
        if (length == 0 && p[0] == '#' && p[1] == ' ') {
            heading = true;
            p += 2;
            continue;
        }
        // The next word, with the space before it if the line has begun.
        const char* end = p;
        while (*end != '\0' && *end != ' ' && *end != '\n') ++end;
        const size_t word = static_cast<size_t>(end - p);
        const size_t need = length + (length != 0 ? 1 : 0) + word;
        bool fits = need < sizeof(line);
        if (fits) {
            char trial[48];
            memcpy(trial, line, length);
            size_t n = length;
            if (length != 0) trial[n++] = ' ';
            memcpy(trial + n, p, word);
            trial[n + word] = '\0';
            fits = _lcd.textWidth(trial) <= width;
        }
        if (!fits && length != 0) {
            flush();  // the word starts the next line
            continue;
        }
        if (length != 0) line[length++] = ' ';
        const size_t take = word < sizeof(line) - 1 - length ? word : sizeof(line) - 1 - length;
        memcpy(line + length, p, take);
        length += take;
        p = end;
        while (*p == ' ') ++p;
    }
    if (length != 0) flush();
    return lines;
}

void Ui::drawHelp() {
    if (_helpList) {
        // Every topic, in two columns.
        const uint8_t pages = static_cast<uint8_t>((rc::kHelpTopics + 2 * kListRows - 1) / (2 * kListRows));
        _helpPages = pages;
        if (_helpPage >= pages) _helpPage = 0;
        for (uint8_t i = 0; i < 2 * kListRows; ++i) {
            const uint8_t topic = static_cast<uint8_t>(_helpPage * 2 * kListRows + i);
            if (topic >= rc::kHelpTopics) break;
            addButton(static_cast<int16_t>(i < kListRows ? 4 : 122), static_cast<int16_t>(kTop + 3 + (i % kListRows) * 26), 114, 24,
                      rc::helpTitle(topic), static_cast<uint8_t>(kHelpTopicBase + topic));
        }
        return;
    }
    const uint16_t lines = wrapped(rc::helpText(_helpTopic), static_cast<uint16_t>(_helpPage * kLines), kLines, 8, kTop + 4,
                                   kScreenW - 16, kLineH);
    _helpPages = static_cast<uint8_t>((lines + kLines - 1) / kLines);
    if (_helpPages == 0) _helpPages = 1;
    snprintf(_helpLabel[0], sizeof(_helpLabel[0]), "%u / %u", static_cast<unsigned>(_helpPage + 1), static_cast<unsigned>(_helpPages));
    if (_helpPage > 0) addButton(4, kTop + 210, 50, 28, "<", kHelpPrev);
    addButton(58, kTop + 210, 124, 28, "ALL TOPICS", kHelpAll);
    if (_helpPage + 1 < _helpPages) addButton(186, kTop + 210, 50, 28, ">", kHelpNext);
    centered(_helpLabel[0], kScreenW / 2, kTop + 202, kTextDim, kBackground, kFontSmall);
}

void Ui::onHelpButton(uint8_t id) {
    if (id == kHelpPrev && _helpPage > 0) {
        --_helpPage;
    } else if (id == kHelpNext && _helpPage + 1 < _helpPages) {
        ++_helpPage;
    } else if (id == kHelpAll) {
        _helpList = true;
        _helpPage = 0;
        drawHeader();
    } else if (id >= kHelpTopicBase && id < kHelpTopicBase + rc::kHelpTopics) {
        _helpList = false;
        _helpTopic = static_cast<uint8_t>(id - kHelpTopicBase);
        _helpPage = 0;
        drawHeader();
    } else {
        return;
    }
    drawContent();
}

}  // namespace gui
