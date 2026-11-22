// The binaural tone engine (RES-AUDIO-002, -003, -005, -008).
#include "host_test.h"
#include "rc_settings.h"
#include "rc_tone.h"

#include <limits>
#include <math.h>
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

// Amplitude of the component at `hz` in one channel, by correlation with a
// sine and a cosine over the whole recording.
double componentAt(const std::vector<int16_t>& audio, int channel, double hz) {
    const size_t frames = audio.size() / 2;
    double re = 0, im = 0;
    for (size_t i = 0; i < frames; ++i) {
        const double a = 2.0 * M_PI * hz * static_cast<double>(i) / kSampleRate;
        re += audio[2 * i + channel] * cos(a);
        im += audio[2 * i + channel] * sin(a);
    }
    return 2.0 * sqrt(re * re + im * im) / static_cast<double>(frames);
}

// Time of the first upward zero crossing of one channel, in samples.
double firstCrossing(const std::vector<int16_t>& audio, int channel) {
    for (size_t i = 1; i < audio.size() / 2; ++i) {
        const double a = audio[2 * (i - 1) + channel], b = audio[2 * i + channel];
        if (a < 0 && b >= 0) return (i - 1) + (-a) / (b - a);
    }
    return -1;
}

// Largest step between two neighbouring samples of one channel.
int largestStep(const std::vector<int16_t>& audio, int channel) {
    int step = 0;
    for (size_t i = 1; i < audio.size() / 2; ++i) {
        const int d = abs(audio[2 * i + channel] - audio[2 * (i - 1) + channel]);
        if (d > step) step = d;
    }
    return step;
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

// ---- RES-AUDIO-013: phase offset and inversion --------------------------------

TEST(with_the_new_controls_at_rest_the_output_is_the_plain_engine) {
    ToneEngine plain, touched;
    touched.setPhase(0.0f, 0.0f);
    touched.setInverted(false, false);
    touched.setPartial(0, 0);
    touched.setPartial(1, 0);
    startAndSettle(plain, 200.0f, 10.0f, 70);
    startAndSettle(touched, 200.0f, 10.0f, 70);
    CHECK(renderSeconds(plain, 2.0) == renderSeconds(touched, 2.0));
}

TEST(each_channel_takes_its_own_phase_offset) {
    // 200 Hz left, 210 Hz right: periods of 220.5 and 210 samples.
    ToneEngine reference, shifted;
    startAndSettle(reference, 200.0f, 10.0f, 80);
    startAndSettle(shifted, 200.0f, 10.0f, 80);
    shifted.setPhase(0.0f, 90.0f);
    renderSeconds(reference, 0.5);
    renderSeconds(shifted, 0.5);
    CHECK_NEAR(shifted.phaseDegrees(Channel::Right), 90.0, 1e-3);
    CHECK_NEAR(shifted.phaseDegrees(Channel::Left), 0.0, 1e-3);
    CHECK(!shifted.phaseMoving());
    auto a = renderSeconds(reference, 0.2);
    auto b = renderSeconds(shifted, 0.2);
    // The right channel is a quarter of its period ahead; the left is untouched.
    double lead = fmod(firstCrossing(a, 1) - firstCrossing(b, 1) + 420.0, 210.0);
    CHECK_NEAR(lead, 52.5, 0.2);
    for (size_t i = 0; i < a.size(); i += 2) CHECK(a[i] == b[i]);

    shifted.setPhase(-120.0f, 90.0f);
    renderSeconds(reference, 0.5);
    renderSeconds(shifted, 0.5);
    a = renderSeconds(reference, 0.2);
    b = renderSeconds(shifted, 0.2);
    lead = fmod(firstCrossing(a, 0) - firstCrossing(b, 0) + 441.0, 220.5);
    CHECK_NEAR(lead, 220.5 * (240.0 / 360.0), 0.2);  // 120 degrees behind
}

TEST(each_channel_can_be_inverted_alone) {
    ToneEngine reference, inverted;
    startAndSettle(reference, 200.0f, 10.0f, 80);
    startAndSettle(inverted, 200.0f, 10.0f, 80);
    inverted.setInverted(false, true);
    renderSeconds(reference, 0.3);
    renderSeconds(inverted, 0.3);
    auto a = renderSeconds(reference, 0.5);
    auto b = renderSeconds(inverted, 0.5);
    for (size_t i = 0; i < a.size(); i += 2) {
        CHECK(a[i] == b[i]);                      // left as it was
        CHECK(abs(a[i + 1] + b[i + 1]) <= 3);     // right mirrored, apart from dither and rounding
    }
    inverted.setInverted(true, false);
    renderSeconds(reference, 0.3);
    renderSeconds(inverted, 0.3);
    a = renderSeconds(reference, 0.5);
    b = renderSeconds(inverted, 0.5);
    for (size_t i = 0; i < a.size(); i += 2) {
        CHECK(abs(a[i] + b[i]) <= 3);
        CHECK(a[i + 1] == b[i + 1]);
    }
}

TEST(phase_and_inversion_changes_make_no_step) {
    // The steepest step of a steady 210 Hz sine at this level is about 245.
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 100);
    const int steady = largestStep(renderSeconds(engine, 0.5), 1);
    engine.setPhase(180.0f, -180.0f);
    auto audio = renderSeconds(engine, 0.3);
    CHECK(largestStep(audio, 0) <= steady * 115 / 100);
    CHECK(largestStep(audio, 1) <= steady * 115 / 100);
    engine.setInverted(true, true);
    audio = renderSeconds(engine, 0.3);
    CHECK(largestStep(audio, 0) <= steady * 115 / 100);
    CHECK(largestStep(audio, 1) <= steady * 115 / 100);
    // A frequency change while an offset is held stays continuous too.
    engine.setFrequencies(240.0f, 6.0f);
    audio = renderSeconds(engine, 0.3);
    CHECK(largestStep(audio, 1) <= steady * 130 / 100);
    engine.setPhase(0.0f, 0.0f);
    engine.setInverted(false, false);
    engine.setFrequencies(200.0f, 10.0f);
    audio = renderSeconds(engine, 0.3);
    CHECK(largestStep(audio, 0) <= steady * 130 / 100);
    CHECK(largestStep(audio, 1) <= steady * 130 / 100);
}

TEST(bad_phase_values_are_ignored_or_brought_into_range) {
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 50);
    engine.setPhase(30.0f, 60.0f);
    renderSeconds(engine, 0.2);
    engine.setPhase(std::numeric_limits<float>::quiet_NaN(), 10.0f);
    engine.setPhase(10.0f, std::numeric_limits<float>::infinity());
    engine.setPhase(10.0f, 10.0f, std::numeric_limits<float>::quiet_NaN());
    renderSeconds(engine, 0.2);
    CHECK_NEAR(engine.phaseDegrees(Channel::Left), 30.0, 1e-3);
    CHECK_NEAR(engine.phaseDegrees(Channel::Right), 60.0, 1e-3);
    engine.setPhase(1e9f, -1e9f, -5.0f);  // clamped; a negative time is immediate
    renderSeconds(engine, 0.05);
    CHECK_NEAR(engine.phaseDegrees(Channel::Left), kPhaseMaxDegrees, 1e-2);
    CHECK_NEAR(engine.phaseDegrees(Channel::Right), -kPhaseMaxDegrees, 1e-2);
}

// ---- RES-AUDIO-015: phase sweep ----------------------------------------------

TEST(a_phase_sweep_moves_in_a_straight_line_and_reports_itself) {
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 50);
    engine.setPhase(0.0f, 90.0f, 2.0f);
    renderSeconds(engine, 1.0);
    CHECK(engine.phaseMoving());
    CHECK_NEAR(engine.phaseDegrees(Channel::Right), 45.0, 0.5);
    CHECK_NEAR(engine.phaseDegrees(Channel::Left), 0.0, 1e-3);
    // While it sweeps, the right channel runs 90 degrees in 2 s faster: 0.125 Hz.
    ToneEngine sweeping;
    startAndSettle(sweeping, 200.0f, 10.0f, 80);
    sweeping.setPhase(0.0f, 3600.0f, 80.0f);  // 45 degrees a second = 0.125 Hz
    renderSeconds(sweeping, 0.1);
    CHECK_NEAR(measuredHz(renderSeconds(sweeping, 20.0), 1), 210.125, 0.005);
    renderSeconds(engine, 1.2);
    CHECK(!engine.phaseMoving());
    CHECK_NEAR(engine.phaseDegrees(Channel::Right), 90.0, 1e-3);
}

TEST(an_hour_long_phase_sweep_keeps_its_precision) {
    PhaseRamp ramp;
    const uint32_t samples = 3600u * kSampleRate;
    ramp.moveTo(360.0f, samples);
    for (uint32_t i = 0; i < samples / 2; ++i) ramp.next();
    CHECK_NEAR(ramp.inDegrees(), 180.0, 1e-3);
    CHECK(!ramp.settled());
    uint32_t last = 0;
    for (uint32_t i = samples / 2; i < samples; ++i) last = ramp.next();
    CHECK(ramp.settled());
    CHECK_NEAR(ramp.inDegrees(), 360.0, 1e-6);
    CHECK(last == 0);  // a whole turn is the same offset as none
    // A quarter turn is a quarter of the 32-bit phase.
    ramp.jump(90.0f);
    CHECK(ramp.next() == 0x40000000u);
    ramp.jump(-90.0f);
    CHECK(ramp.next() == 0xC0000000u);
}

TEST(while_the_audio_is_off_an_offset_is_taken_at_once) {
    ToneEngine engine;
    engine.setPhase(0.0f, 120.0f, 600.0f);
    renderSeconds(engine, 0.01);
    CHECK_NEAR(engine.phaseDegrees(Channel::Right), 120.0, 1e-3);
    CHECK(!engine.phaseMoving());
}

// ---- RES-AUDIO-014: golden partials ------------------------------------------

TEST(golden_partials_sit_at_phi_and_phi_squared_and_keep_the_beat) {
    ToneEngine engine;
    startAndSettle(engine, 200.0f, 10.0f, 80);
    const auto before = renderSeconds(engine, 4.0);
    const double fundamental = componentAt(before, 0, 200.0);
    CHECK(componentAt(before, 0, 200.0 * 1.6180339887) < fundamental * 0.002);

    engine.setPartial(0, 50);
    engine.setPartial(1, 100);
    renderSeconds(engine, 0.2);
    const auto audio = renderSeconds(engine, 4.0);
    const double phi = 1.6180339887, phi2 = 2.6180339887;
    // Left: phi and phi squared times the primary.
    CHECK_NEAR(componentAt(audio, 0, 200.0 * phi) / fundamental, 0.25, 0.005);
    CHECK_NEAR(componentAt(audio, 0, 200.0 * phi2) / fundamental, 0.5, 0.005);
    // Right: the same partials raised by the beat itself...
    CHECK_NEAR(componentAt(audio, 1, 200.0 * phi + 10.0) / fundamental, 0.25, 0.005);
    CHECK_NEAR(componentAt(audio, 1, 200.0 * phi2 + 10.0) / fundamental, 0.5, 0.005);
    // ...and not by a multiple of it, which a partial of the right ear's own tone would give.
    CHECK(componentAt(audio, 1, 210.0 * phi) < fundamental * 0.005);
    CHECK(componentAt(audio, 1, 210.0 * phi2) < fundamental * 0.005);
    // The fundamental is as it was, in both ears.
    CHECK_NEAR(componentAt(audio, 0, 200.0) / fundamental, 1.0, 0.002);
    CHECK_NEAR(componentAt(audio, 1, 210.0) / componentAt(before, 1, 210.0), 1.0, 0.002);
    CHECK_NEAR(measuredHz(before, 0), 200.0, 0.01);
}

TEST(partials_switch_without_a_step_and_follow_a_frequency_change) {
    ToneEngine engine;
    startAndSettle(engine, 300.0f, 8.0f, 60);
    const int steady = largestStep(renderSeconds(engine, 0.3), 0);
    engine.setPartial(0, 100);
    engine.setPartial(1, 100);
    auto audio = renderSeconds(engine, 0.3);
    // Two partials at half level and up to 2.6 times the frequency: at most
    // 1 + 0.5 * 1.62 + 0.5 * 2.62 times the fundamental's steepest step.
    CHECK(largestStep(audio, 0) <= steady * 320 / 100);
    engine.setFrequencies(400.0f, 8.0f);
    renderSeconds(engine, 0.3);
    audio = renderSeconds(engine, 4.0);
    const double fundamental = componentAt(audio, 0, 400.0);
    CHECK_NEAR(componentAt(audio, 0, 400.0 * 1.6180339887) / fundamental, 0.5, 0.01);
    CHECK_NEAR(componentAt(audio, 1, 400.0 * 2.6180339887 + 8.0) / fundamental, 0.5, 0.01);
    engine.setPartial(0, 0);
    engine.setPartial(1, 0);
    audio = renderSeconds(engine, 0.3);
    CHECK(largestStep(audio, 0) <= steady * 450 / 100);
    audio = renderSeconds(engine, 2.0);
    CHECK(componentAt(audio, 0, 400.0 * 1.6180339887) < fundamental * 0.003);
}

TEST(partials_respect_their_limits) {
    ToneEngine engine;
    startAndSettle(engine, 1000.0f, 40.0f, 100);
    engine.setPartial(0, 250);   // taken as 100
    engine.setPartial(1, 100);
    engine.setPartial(7, 100);   // no such partial: ignored
    engine.setInverted(true, false);
    const auto audio = renderSeconds(engine, 2.0);
    CHECK(peak(audio, 0) <= 32768);
    CHECK(peak(audio, 0) > 16384);  // louder than the fundamental alone
    CHECK_NEAR(componentAt(audio, 0, 1618.0339887) / componentAt(audio, 0, 1000.0), 0.5, 0.01);
}

int main() { return host_test::runAll(); }
