// Settings validation (RES-CFG-002, RES-CFG-003, RES-AUDIO-005, RES-ARCH-005).
#include "host_test.h"
#include "rc_settings.h"

#include <limits>

using namespace rc;

TEST(defaults_are_already_valid) {
    Settings s;
    setDefaults(s);
    CHECK(!sanitize(s));
    CHECK(s.mode == Mode::Play);
    CHECK_NEAR(s.primaryHz, 200.0, 1e-6);
    CHECK_NEAR(s.beatHz, 10.0, 1e-6);
    CHECK(strcmp(s.hostname, "resonance") == 0);
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
    CHECK(s.mode == Mode::Play);
    CHECK(s.startVolume <= kStartVolumeMax);
    CHECK(s.brightness >= kBrightnessMin && s.brightness <= kBrightnessMax);
    CHECK(s.screenTimeoutS <= kScreenTimeoutMaxS);
    CHECK(s.primaryHz >= kPrimaryMinHz && s.primaryHz <= kPrimaryMaxHz);
    CHECK(s.beatHz >= kBeatMinHz && s.beatHz <= kBeatMaxHz);
    CHECK(strcmp(s.hostname, "resonance") == 0);
    CHECK(s.wifiSsid[0] == '\0' && s.wifiPass[0] == '\0' && s.apPass[0] == '\0');
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

TEST(brightness_never_reaches_zero) {
    Settings s;
    setDefaults(s);
    s.brightness = 0;
    CHECK(sanitize(s));
    CHECK(s.brightness == kBrightnessMin);
}

TEST(an_unknown_mode_falls_back_to_play) {
    Settings s;
    setDefaults(s);
    s.mode = static_cast<Mode>(7);
    CHECK(sanitize(s));
    CHECK(s.mode == Mode::Play);
    s.mode = Mode::Setup;
    CHECK(!sanitize(s));
    CHECK(s.mode == Mode::Setup);
}

TEST(hostname_rules) {
    CHECK(isValidHostname("resonance"));
    CHECK(isValidHostname("Resonance-2"));
    CHECK(!isValidHostname(""));
    CHECK(!isValidHostname("-resonance"));
    CHECK(!isValidHostname("resonance-"));
    CHECK(!isValidHostname("res onance"));
    CHECK(!isValidHostname("res.local"));
    CHECK(!isValidHostname("../etc"));
    CHECK(isValidHostname("abcdefghijklmnopqrstuvw"));    // 23 characters
    CHECK(!isValidHostname("abcdefghijklmnopqrstuvwx"));  // 24 characters
}

TEST(an_invalid_hostname_is_replaced) {
    Settings s;
    setDefaults(s);
    strcpy(s.hostname, "bad name!");
    CHECK(sanitize(s));
    CHECK(strcmp(s.hostname, "resonance") == 0);
}

TEST(an_unterminated_text_field_is_emptied) {
    Settings s;
    setDefaults(s);
    memset(s.wifiSsid, 'A', sizeof(s.wifiSsid));
    CHECK(sanitize(s));
    CHECK(s.wifiSsid[0] == '\0');
}

TEST(a_control_character_empties_the_field) {
    Settings s;
    setDefaults(s);
    strcpy(s.wifiSsid, "home\nnet");
    CHECK(sanitize(s));
    CHECK(s.wifiSsid[0] == '\0');
}

TEST(a_password_without_a_network_is_dropped) {
    Settings s;
    setDefaults(s);
    strcpy(s.wifiPass, "secret-password");
    CHECK(sanitize(s));
    CHECK(s.wifiPass[0] == '\0');

    strcpy(s.wifiSsid, "home");
    strcpy(s.wifiPass, "secret-password");
    CHECK(!sanitize(s));
    CHECK(strcmp(s.wifiPass, "secret-password") == 0);
}

TEST(a_short_access_point_password_is_dropped) {
    Settings s;
    setDefaults(s);
    strcpy(s.apPass, "1234567");
    CHECK(sanitize(s));
    CHECK(s.apPass[0] == '\0');
    strcpy(s.apPass, "12345678");
    CHECK(!sanitize(s));
}

int main() { return host_test::runAll(); }
