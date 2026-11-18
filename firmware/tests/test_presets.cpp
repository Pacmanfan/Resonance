// Frequency presets (RES-AUDIO-006, RES-PROC-006).
#include "host_test.h"
#include "rc_presets.h"
#include "rc_settings.h"

#include <ctype.h>

using namespace rc;

TEST(presets_are_in_range_on_the_step_and_rising) {
    CHECK(primaryPresetCount() >= 8);
    CHECK(beatPresetCount() >= 5);
    float previous = 0;
    for (uint8_t i = 0; i < primaryPresetCount(); ++i) {
        const FrequencyPreset& p = primaryPreset(i);
        CHECK(p.hz > previous);
        CHECK(p.hz >= kPrimaryMinHz && p.hz <= kPrimaryMaxHz);
        // Exactly what the device would store: nothing is rounded away.
        CHECK_NEAR(sanitizeFrequency(p.hz, kPrimaryMinHz, kPrimaryMaxHz, 0), p.hz, 1e-3);
        CHECK(strlen(p.name) <= 14);
        previous = p.hz;
    }
    previous = 0;
    for (uint8_t i = 0; i < beatPresetCount(); ++i) {
        const FrequencyPreset& p = beatPreset(i);
        CHECK(p.hz > previous);
        CHECK(p.hz >= kBeatMinHz && p.hz <= kBeatMaxHz);
        CHECK_NEAR(sanitizeFrequency(p.hz, kBeatMinHz, kBeatMaxHz, 0), p.hz, 1e-3);
        CHECK(strlen(p.name) <= 14);
        previous = p.hz;
    }
}

TEST(every_band_of_the_requirement_has_a_beat_preset) {
    const float bands[][2] = {{0.5f, 4.0f}, {4.0f, 8.0f}, {8.0f, 12.0f}, {12.0f, 30.0f}, {30.0f, 40.0f}};
    for (const auto& band : bands) {
        bool found = false;
        for (uint8_t i = 0; i < beatPresetCount(); ++i) {
            if (beatPreset(i).hz >= band[0] && beatPreset(i).hz <= band[1]) found = true;
        }
        CHECK(found);
    }
}

TEST(cycling_visits_every_preset_and_wraps) {
    float hz = 0;
    for (uint8_t i = 0; i < primaryPresetCount(); ++i) {
        hz = nextPrimaryPreset(hz).hz;
        CHECK_NEAR(hz, primaryPreset(i).hz, 1e-6);
    }
    CHECK_NEAR(nextPrimaryPreset(hz).hz, primaryPreset(0).hz, 1e-6);  // wraps to the lowest
    hz = 0;
    for (uint8_t i = 0; i < beatPresetCount(); ++i) {
        hz = nextBeatPreset(hz).hz;
        CHECK_NEAR(hz, beatPreset(i).hz, 1e-6);
    }
    CHECK_NEAR(nextBeatPreset(hz).hz, beatPreset(0).hz, 1e-6);
}

TEST(cycling_from_a_value_set_by_hand_goes_to_the_next_one_up) {
    CHECK_NEAR(nextPrimaryPreset(430.0f).hz, 432.0, 1e-6);
    CHECK_NEAR(nextPrimaryPreset(432.0f).hz, 440.0, 1e-6);
    CHECK_NEAR(nextPrimaryPreset(1000.0f).hz, primaryPreset(0).hz, 1e-6);
    CHECK_NEAR(nextBeatPreset(7.7f).hz, 7.8, 1e-6);
    CHECK_NEAR(nextBeatPreset(7.8f).hz, 10.0, 1e-6);
}

TEST(a_value_on_a_preset_is_named_and_one_beside_it_is_not) {
    CHECK(primaryPresetName(528.0f) != nullptr && strstr(primaryPresetName(528.0f), "528") != nullptr);
    CHECK(primaryPresetName(528.1f) == nullptr);
    CHECK(primaryPresetName(136.1f) != nullptr);
    CHECK(beatPresetName(7.8f) != nullptr);
    CHECK(beatPresetName(7.9f) == nullptr);
    CHECK(beatPresetName(10.0f) != nullptr);
}

TEST(names_make_no_claims) {
    // RES-PROC-006: a name identifies a frequency; it does not promise an
    // effect. These words would.
    const char* const forbidden[] = {"heal", "cure", "sleep", "focus", "relax", "pain", "anxiety", "dna",
                                     "miracle", "love", "repair", "detox", "therapy"};
    auto check = [&](const char* name) {
        char lower[32];
        size_t n = 0;
        for (; name[n] != '\0' && n < sizeof(lower) - 1; ++n) lower[n] = static_cast<char>(tolower(name[n]));
        lower[n] = '\0';
        for (const char* word : forbidden) CHECK(strstr(lower, word) == nullptr);
    };
    for (uint8_t i = 0; i < primaryPresetCount(); ++i) check(primaryPreset(i).name);
    for (uint8_t i = 0; i < beatPresetCount(); ++i) check(beatPreset(i).name);
}

int main() { return host_test::runAll(); }
