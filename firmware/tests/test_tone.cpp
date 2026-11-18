// The binaural tone engine (RES-AUDIO-002, -003, -005, -008).
#include "host_test.h"
#include "rc_settings.h"
#include "rc_tone.h"

#include <limits>
#include <stdlib.h>
#include <vector>

using namespace rc;

namespace {

// Renders `seconds` of audio in blocks of an awkward size, as an output
// driver would ask for it.
std::vector<int16_t> renderSeconds(ToneEngine& engine, double seconds) {
    const size_t frames = static_cast<size_t>(seconds * kSampleRate);
    std::vector<int16_t> out(frames * 2);
    size_t done = 0;
    while (done < frames) {
        const size_t block = frames - done < 367 ? frames - done : 367;
        engine.render(&out[done * 2], block);
        done += block;
    }
    return out;
}

// Frequency of one channel from its upward zero crossings, with the first
// and last crossing located between samples.
double measuredHz(const std::vector<int16_t>& audio, int channel) {
    const size_t frames = audio.size() / 2;
    double first = -1, last = -1;
    int crossings = 0;
    for (size_t i = 1; i < frames; ++i) {
        const double a = audio[2 * (i - 1) + channel], b = audio[2 * i + channel];
        if (a < 0 && b >= 0) {
            const double t = (i - 1) + (-a) / (b - a);
            if (first < 0) first = t;
            last = t;
            ++crossings;
        }
    }
    if (crossings < 2) return 0;
    return (crossings - 1) * static_cast<double>(kSampleRate) / (last - first);
}

int peak(const std::vector<int16_t>& audio, int channel) {
    int p = 0;
    for (size_t i = channel; i < audio.size(); i += 2) p = abs(audio[i]) > p ? abs(audio[i]) : p;
    return p;
}

// A running engine, past its start fade.
void startAndSettle(ToneEngine& engine, float primary, float beat, uint8_t volume) {
    engine.setFrequencies(primary, beat);
    engine.setVolume(volume);
    engine.setEnabled(true);
    renderSeconds(engine, 1.0);
}

}  // namespace

TEST(it_is_silent_until_enabled) {
    ToneEngine engine;
    engine.setVolume(100);
    const auto audio = renderSeconds(engine, 0.2);
    CHECK(peak(audio, 0) == 0);
    CHECK(peak(audio, 1) == 0);
    CHECK(!engine.enabled());
}

TEST(left_is_the_primary_and_right_is_primary_plus_beat) {
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 80);
    const auto audio = renderSeconds(engine, 10.0);
    CHECK_NEAR(measuredHz(audio, 0), 200.0, 0.01);
    CHECK_NEAR(measuredHz(audio, 1), 210.0, 0.01);
}

TEST(frequencies_are_accurate_across_the_range) {
    const float cases[][2] = {{20.0f, 0.5f}, {100.3f, 7.8f}, {432.0f, 40.0f}, {1000.0f, 4.0f}};
    for (const auto& c : cases) {
        ToneEngine engine;
        startAndSettle(engine, c[0], c[1], 80);
        const auto audio = renderSeconds(engine, 20.0);
        CHECK_NEAR(measuredHz(audio, 0), c[0], 0.01);
        CHECK_NEAR(measuredHz(audio, 1), c[0] + c[1], 0.01);
    }
}

TEST(the_channels_are_independent) {
    // If any of the right channel leaked into the left, the left's amplitude
    // would beat at the difference frequency. Its envelope must be flat.
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 100);
    const auto audio = renderSeconds(engine, 2.0);
    const size_t window = kSampleRate / 100;  // 10 ms: two periods of 200 Hz
    int lowest = 1 << 20, highest = 0;
    for (size_t start = 0; start + window <= audio.size() / 2; start += window) {
        int p = 0;
        for (size_t i = start; i < start + window; ++i) p = abs(audio[2 * i]) > p ? abs(audio[2 * i]) : p;
        lowest = p < lowest ? p : lowest;
        highest = p > highest ? p : highest;
    }
    CHECK(highest - lowest <= 8);  // dither and sampling of the peak only
}

TEST(a_frequency_change_is_phase_continuous) {
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 100);
    engine.setFrequencies(800.0f, 40.0f);
    const auto audio = renderSeconds(engine, 0.5);
    // The steepest step a full-level 840 Hz sine can take between samples.
    const double amplitude = kToneCeiling * 32767.0;
    const double limit = amplitude * 2.0 * M_PI * 840.0 / kSampleRate + 4.0;
    int worst = 0;
    for (size_t i = 2; i < audio.size(); ++i) {
        const int step = abs(audio[i] - audio[i - 2]);
        worst = step > worst ? step : worst;
    }
    CHECK(worst <= limit);
    CHECK_NEAR(measuredHz(renderSeconds(engine, 5.0), 0), 800.0, 0.01);
}

TEST(start_and_stop_are_faded) {
    ToneEngine engine;
    engine.setFrequencies(200.0f, 10.0f);
    engine.setVolume(100);
    engine.setEnabled(true);
    const auto start = renderSeconds(engine, kStartStopFadeS + 0.1);
    // The first 5 ms stay near zero: no click.
    int early = 0;
    for (size_t i = 0; i < kSampleRate / 200 * 2; ++i) early = abs(start[i]) > early ? abs(start[i]) : early;
    CHECK(early < 400);
    const int full = static_cast<int>(kToneCeiling * 32767.0f);
    CHECK(peak(renderSeconds(engine, 0.2), 0) > full - 40);

    engine.setEnabled(false);
    const auto stop = renderSeconds(engine, kStartStopFadeS + 0.1);
    // No sample-to-sample jump larger than the running sine makes.
    const double limit = full * 2.0 * M_PI * 210.0 / kSampleRate + 4.0;
    int worst = 0;
    for (size_t i = 2; i < stop.size(); ++i) worst = abs(stop[i] - stop[i - 2]) > worst ? abs(stop[i] - stop[i - 2]) : worst;
    CHECK(worst <= limit);
    const auto after = renderSeconds(engine, 0.2);
    CHECK(peak(after, 0) == 0 && peak(after, 1) == 0);
}

TEST(the_output_never_exceeds_the_ceiling) {
    ToneEngine engine;
    startAndSettle(engine, 1000.0f, 40.0f, 100);
    const auto audio = renderSeconds(engine, 2.0);
    const int ceiling = static_cast<int>(kToneCeiling * 32767.0f) + 2;
    CHECK(peak(audio, 0) <= ceiling);
    CHECK(peak(audio, 1) <= ceiling);
}

TEST(volume_follows_a_square_law_and_zero_is_silent_apart_from_dither) {
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 50);
    const int half = peak(renderSeconds(engine, 0.5), 0);
    CHECK_NEAR(half, 0.25 * kToneCeiling * 32767.0, 6.0);
    engine.setVolume(0);
    renderSeconds(engine, 0.2);
    CHECK(peak(renderSeconds(engine, 0.5), 0) <= 1);
}

TEST(a_volume_step_is_ramped) {
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 10);
    engine.setVolume(100);
    const auto audio = renderSeconds(engine, 0.3);
    const double limit = kToneCeiling * 32767.0 * 2.0 * M_PI * 210.0 / kSampleRate + 16.0;
    int worst = 0;
    for (size_t i = 2; i < audio.size(); ++i) worst = abs(audio[i] - audio[i - 2]) > worst ? abs(audio[i] - audio[i - 2]) : worst;
    CHECK(worst <= limit);
}

TEST(out_of_range_and_non_finite_frequencies_are_handled) {
    ToneEngine engine;
    startAndSettle(engine, 5000.0f, 100.0f, 80);  // clamped to 1000 and 40
    const auto audio = renderSeconds(engine, 5.0);
    CHECK_NEAR(measuredHz(audio, 0), 1000.0, 0.01);
    CHECK_NEAR(measuredHz(audio, 1), 1040.0, 0.01);

    engine.setFrequencies(std::numeric_limits<float>::quiet_NaN(), 10.0f);  // ignored
    engine.setFrequencies(300.0f, std::numeric_limits<float>::infinity());  // ignored
    CHECK_NEAR(measuredHz(renderSeconds(engine, 5.0), 0), 1000.0, 0.01);
}

int main() { return host_test::runAll(); }
