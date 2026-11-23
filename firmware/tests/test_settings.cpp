// Settings validation (RES-CFG-002, RES-CFG-003, RES-AUDIO-005).
#include "host_test.h"
#include "rc_settings.h"

#include <limits>

using namespace rc;

TEST(defaults_are_already_valid) {
    Settings s;
    setDefaults(s);
    CHECK(!sanitize(s));
    CHECK_NEAR(s.primaryHz, 200.0, 1e-6);
    CHECK_NEAR(s.beatHz, 10.0, 1e-6);
    CHECK(s.startVolume <= kStartVolumeMax);
}

TEST(another_version_is_replaced_by_defaults) {
    Settings s;
    setDefaults(s);
    s.version = 99;
    s.primaryHz = 440.0f;
    CHECK(sanitize(s));
    CHECK(s.version == kSettingsVersion);
    CHECK_NEAR(s.primaryHz, 200.0, 1e-6);
}

TEST(a_record_of_garbage_becomes_valid) {
    Settings s;
    memset(&s, 0xff, sizeof(s));
    s.version = kSettingsVersion;
    CHECK(sanitize(s));
    CHECK(!sanitize(s));  // stable after one pass
    CHECK(s.startVolume <= kStartVolumeMax);
    CHECK(s.brightness >= kBrightnessMin && s.brightness <= kBrightnessMax);
    CHECK(s.screenTimeoutS <= kScreenTimeoutMaxS);
    CHECK(s.primaryHz >= kPrimaryMinHz && s.primaryHz <= kPrimaryMaxHz);
    CHECK(s.beatHz >= kBeatMinHz && s.beatHz <= kBeatMaxHz);
}

TEST(frequencies_are_clamped_to_their_ranges) {
    CHECK_NEAR(sanitizeFrequency(5.0f, kPrimaryMinHz, kPrimaryMaxHz, 200.0f), 20.0, 1e-4);
    CHECK_NEAR(sanitizeFrequency(5000.0f, kPrimaryMinHz, kPrimaryMaxHz, 200.0f), 1000.0, 1e-4);
    CHECK_NEAR(sanitizeFrequency(0.0f, kBeatMinHz, kBeatMaxHz, 10.0f), 0.5, 1e-4);
    CHECK_NEAR(sanitizeFrequency(-3.0f, kBeatMinHz, kBeatMaxHz, 10.0f), 0.5, 1e-4);
    CHECK_NEAR(sanitizeFrequency(41.0f, kBeatMinHz, kBeatMaxHz, 10.0f), 40.0, 1e-4);
}

TEST(frequencies_are_rounded_to_a_tenth_of_a_hertz) {
    CHECK_NEAR(sanitizeFrequency(200.04f, kPrimaryMinHz, kPrimaryMaxHz, 200.0f), 200.0, 1e-3);
    CHECK_NEAR(sanitizeFrequency(200.06f, kPrimaryMinHz, kPrimaryMaxHz, 200.0f), 200.1, 1e-3);
    CHECK_NEAR(sanitizeFrequency(7.83f, kBeatMinHz, kBeatMaxHz, 10.0f), 7.8, 1e-3);
}

TEST(a_value_that_is_not_finite_is_replaced) {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    CHECK_NEAR(sanitizeFrequency(nan, kPrimaryMinHz, kPrimaryMaxHz, 200.0f), 200.0, 1e-6);
    CHECK_NEAR(sanitizeFrequency(inf, kPrimaryMinHz, kPrimaryMaxHz, 200.0f), 200.0, 1e-6);
    CHECK_NEAR(sanitizeFrequency(-inf, kBeatMinHz, kBeatMaxHz, 10.0f), 10.0, 1e-6);

    Settings s;
    setDefaults(s);
    s.primaryHz = nan;
    CHECK(sanitize(s));
    CHECK_NEAR(s.primaryHz, 200.0, 1e-6);
}

TEST(start_volume_can_never_be_loud) {
    Settings s;
    setDefaults(s);
    s.startVolume = 100;
    CHECK(sanitize(s));
    CHECK(s.startVolume == kStartVolumeMax);
}

TEST(layer_volumes_and_session_choices_are_kept_within_bounds) {
    Settings s;
    setDefaults(s);
    CHECK(s.backgroundVolume == kBackgroundVolumeDefault && s.noiseVolume == kNoiseVolumeDefault);
    s.backgroundVolume = 85;
    s.noiseVolume = 15;
    s.sessionProgram = 3;
    s.timerChoice = 5;
    s.programLength = 2;
    s.fadeChoice = 3;
    CHECK(!sanitize(s));  // plausible values pass untouched
    CHECK(s.backgroundVolume == 85 && s.noiseVolume == 15 && s.sessionProgram == 3 && s.timerChoice == 5);
    s.backgroundVolume = 200;
    s.noiseVolume = 255;
    s.sessionProgram = 250;
    s.fadeChoice = 99;
    CHECK(sanitize(s));
    CHECK(s.backgroundVolume == kVolumeMax && s.noiseVolume == kVolumeMax);
    CHECK(s.sessionProgram <= 31 && s.fadeChoice <= 31);
}

TEST(brightness_never_reaches_zero) {
    Settings s;
    setDefaults(s);
    s.brightness = 0;
    CHECK(sanitize(s));
    CHECK(s.brightness == kBrightnessMin);
}

// A version 3 record, byte for byte as the device stored it: offsets 0 version,
// 1 mode, 2 start volume, 3 brightness, 4 screen timeout, 8 and 12 the two
// frequencies, 16 name, 40 network, 73 its password, 138 the access-point
// password, 150..155 the six values added in version 3.
static void v3Record(unsigned char* r) {
    memset(r, 0, kSettingsV3Size);
    r[0] = 3;
    r[1] = 1;  // Setup mode
    r[2] = 45;
    r[3] = 70;
    const uint16_t timeout = 300;
    memcpy(r + 4, &timeout, 2);
    const float primary = 136.1f, beat = 7.8f;
    memcpy(r + 8, &primary, 4);
    memcpy(r + 12, &beat, 4);
    strcpy(reinterpret_cast<char*>(r + 40), "home");
    strcpy(reinterpret_cast<char*>(r + 73), "secret-password");
    r[150] = 55;
    r[151] = 25;
    r[152] = 2;
    r[153] = 4;
    r[154] = 3;
    r[155] = 2;
}

TEST(a_version_3_record_keeps_what_the_user_had_set) {
    unsigned char record[kSettingsV3Size];
    v3Record(record);
    Settings s;
    CHECK(migrateFromV3(record, sizeof(record), s));
    CHECK(s.version == kSettingsVersion);
    CHECK(s.startVolume == 45 && s.brightness == 70 && s.screenTimeoutS == 300);
    CHECK_NEAR(s.primaryHz, 136.1, 1e-3);
    CHECK_NEAR(s.beatHz, 7.8, 1e-3);
    CHECK(s.backgroundVolume == 55 && s.noiseVolume == 25);
    CHECK(s.sessionProgram == 2 && s.timerChoice == 4 && s.programLength == 3 && s.fadeChoice == 2);
    CHECK(!sanitize(s));
    // Nothing of the old network credentials is in the new record.
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&s);
    for (size_t i = 0; i + 4 <= sizeof(s); ++i) CHECK(memcmp(bytes + i, "secr", 4) != 0 && memcmp(bytes + i, "home", 4) != 0);
}

TEST(a_version_3_record_is_validated_on_the_way) {
    unsigned char record[kSettingsV3Size];
    v3Record(record);
    record[2] = 100;  // louder than a start may be
    record[3] = 0;
    const float bad = std::numeric_limits<float>::quiet_NaN();
    memcpy(record + 8, &bad, 4);
    Settings s;
    CHECK(migrateFromV3(record, sizeof(record), s));
    CHECK(s.startVolume == kStartVolumeMax && s.brightness == kBrightnessMin);
    CHECK_NEAR(s.primaryHz, 200.0, 1e-6);
}

TEST(only_a_version_3_record_is_migrated) {
    unsigned char record[kSettingsV3Size];
    v3Record(record);
    Settings s;
    setDefaults(s);
    s.brightness = 33;
    CHECK(!migrateFromV3(record, sizeof(record) - 1, s));
    CHECK(!migrateFromV3(nullptr, sizeof(record), s));
    record[0] = 2;
    CHECK(!migrateFromV3(record, sizeof(record), s));
    CHECK(s.brightness == 33);
    CHECK(sizeof(Settings) != kSettingsV3Size);
}

int main() { return host_test::runAll(); }
