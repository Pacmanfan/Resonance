// The help texts shown on the device (RES-GUI-007, RES-PROC-006).
#include "host_test.h"
#include "rc_help.h"

#include <ctype.h>
#include <initializer_list>
#include <string>

using namespace rc;

TEST(every_topic_has_a_title_and_a_text) {
    CHECK(kHelpTopics == 17);
    for (uint8_t i = 0; i < kHelpTopics; ++i) {
        CHECK(strlen(helpTitle(i)) >= 4 && strlen(helpTitle(i)) <= 15);  // fits the header and a list button
        CHECK(strlen(helpText(i)) > 150);
        CHECK(helpText(i)[0] == '#');  // starts with a heading
    }
    CHECK(helpTitle(kHelpTopics)[0] == '\0' && helpText(200)[0] == '\0');
}

TEST(texts_are_plain_ascii_with_words_the_screen_can_hold) {
    for (uint8_t i = 0; i < kHelpTopics; ++i) {
        size_t word = 0;
        for (const char* c = helpText(i); *c != '\0'; ++c) {
            const unsigned char u = static_cast<unsigned char>(*c);
            CHECK(u == '\n' || (u >= 0x20 && u <= 0x7E));  // the device's font has nothing else
            word = (u == ' ' || u == '\n') ? 0 : word + 1;
            if (word > 24) {
                printf("  topic %u: a word longer than a line\n", i);
                CHECK(false);
                break;
            }
        }
    }
}

TEST(nothing_is_promised_to_the_listener) {
    static const char* const kNot[] = {"heal", "cure", "treat", "therap", "medical", "anxiety", "depress", "relax",
                                       "proven", "improve", "boost", "benefit", "stress", "sleep better", "brainwave"};
    for (uint8_t i = 0; i < kHelpTopics; ++i) {
        std::string text = helpText(i);
        for (char& c : text) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        for (const char* word : kNot) {
            if (text.find(word) != std::string::npos) {
                printf("  topic %u says \"%s\"\n", i, word);
                CHECK(false);
            }
        }
    }
}

TEST(mindfield_help_says_what_it_is_not) {
    const std::string text = helpText(static_cast<uint8_t>(HelpTopic::Mindfield));
    CHECK(text.find("Nothing here shows that the mind can change random numbers") != std::string::npos);
    CHECK(text.find("Baseline") != std::string::npos);
    const std::string source = helpText(static_cast<uint8_t>(HelpTopic::MindSource));
    CHECK(source.find("not random") != std::string::npos);
    const std::string result = helpText(static_cast<uint8_t>(HelpTopic::MindResult));
    CHECK(result.find("Chance gives scores like these") != std::string::npos);
}

TEST(the_first_topic_names_every_sound_and_where_it_is_controlled) {
    const std::string text = helpText(0);
    for (const char* needle : {"TONE", "GOLD", "NOISE", "FILE", "HOME", "FREQ", "SND", "ADV", "Silent", "?"}) {
        CHECK(text.find(needle) != std::string::npos);
    }
}

int main() { return host_test::runAll(); }
