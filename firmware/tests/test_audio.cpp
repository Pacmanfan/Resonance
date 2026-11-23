// Ring buffer, noise, limiter and the mixing engine (RES-AUDIO-007, -008,
// -010; RES-PLAY-003).
#include "host_test.h"
#include "rc_audio.h"
#include "rc_settings.h"

#include <math.h>
#include <stdlib.h>
#include <vector>

using namespace rc;

namespace {

struct Rig {
    std::vector<uint32_t> storage;
    FrameRing ring;
    AudioEngine engine;
    Rig() : storage(4096), ring(storage.data(), 4096), engine(ring) {}
};

std::vector<int16_t> render(AudioEngine& engine, size_t frames) {
    std::vector<int16_t> out(frames * 2);
    size_t done = 0;
    while (done < frames) {
        const size_t n = frames - done < 367 ? frames - done : 367;
        engine.render(&out[done * 2], n);
        done += n;
    }
    return out;
}

int peak(const std::vector<int16_t>& a, int channel = -1) {
    int p = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        if (channel >= 0 && static_cast<int>(i % 2) != channel) continue;
        p = abs(a[i]) > p ? abs(a[i]) : p;
    }
    return p;
}

// Keeps the ring full of a constant value while rendering.
std::vector<int16_t> renderWithBackground(Rig& rig, size_t frames, int16_t left, int16_t right) {
    std::vector<int16_t> out(frames * 2);
    std::vector<int16_t> fill(512 * 2);
    for (size_t i = 0; i < 512; ++i) { fill[2 * i] = left; fill[2 * i + 1] = right; }
    size_t done = 0;
    while (done < frames) {
        while (rig.ring.space() >= 512) rig.ring.write(fill.data(), 512);
        const size_t n = frames - done < 300 ? frames - done : 300;
        rig.engine.render(&out[done * 2], n);
        done += n;
    }
    return out;
}

}  // namespace

TEST(ring_delivers_what_was_written_in_order_across_the_wrap) {
    uint32_t storage[8];
    FrameRing ring(storage, 8);
    CHECK(ring.capacity() == 8 && ring.available() == 0 && ring.space() == 8);
    int16_t in[6 * 2], out[6 * 2];
    int next = 0, expect = 0;
    for (int round = 0; round < 50; ++round) {
        for (int i = 0; i < 6; ++i) { in[2 * i] = static_cast<int16_t>(next); in[2 * i + 1] = static_cast<int16_t>(-next); ++next; }
        const uint32_t wrote = ring.write(in, 6);
        next -= 6 - static_cast<int>(wrote);  // frames that did not fit are offered again
        const uint32_t got = ring.read(out, 5);
        for (uint32_t i = 0; i < got; ++i) {
            CHECK(out[2 * i] == expect && out[2 * i + 1] == -expect);
            ++expect;
        }
    }
    CHECK(expect > 100);
}

TEST(ring_refuses_more_than_it_holds_and_discard_empties_it) {
    uint32_t storage[4];
    FrameRing ring(storage, 4);
    const int16_t in[6 * 2] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
    CHECK(ring.write(in, 6) == 4);
    CHECK(ring.space() == 0);
    CHECK(ring.write(in, 1) == 0);
    ring.discard();
    CHECK(ring.available() == 0 && ring.space() == 4);
    int16_t out[2];
    CHECK(ring.read(out, 1) == 0);
}

TEST(limiter_is_transparent_below_the_knee_and_bounded_above) {
    CHECK_NEAR(limit(0.0f), 0.0, 1e-9);
    CHECK_NEAR(limit(0.5f), 0.5, 1e-9);
    CHECK_NEAR(limit(-kLimiterKnee), -kLimiterKnee, 1e-6);
    float previous = 0.0f;
    for (float x = 0.0f; x < 20.0f; x += 0.01f) {
        const float y = limit(x);
        CHECK(y <= kOutputCeiling + 1e-6f);
        CHECK(y >= previous - 1e-6f);        // never folds back
        CHECK_NEAR(limit(-x), -y, 1e-6);     // symmetric
        previous = y;
    }
    // No corner at the knee: the slope just above it is still close to one.
    CHECK_NEAR((limit(kLimiterKnee + 0.01f) - kLimiterKnee) / 0.01f, 1.0, 0.01);
}

TEST(the_engine_is_silent_by_default) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    CHECK(peak(render(rig.engine, 4410)) == 0);
}

TEST(the_tone_passes_through_at_master_volume) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.tone().setFrequencies(200.0f, 10.0f);
    rig.engine.tone().setEnabled(true);
    render(rig.engine, 44100);
    CHECK_NEAR(peak(render(rig.engine, 22050)), kToneCeiling * 32767.0, 40.0);
    rig.engine.setMasterVolume(50);  // square law: a quarter
    render(rig.engine, 8820);
    CHECK_NEAR(peak(render(rig.engine, 22050)), 0.25 * kToneCeiling * 32767.0, 20.0);
}

TEST(background_keeps_its_channels_and_follows_its_volume) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true);
    renderWithBackground(rig, 8820, 20000, -10000);
    const auto audio = renderWithBackground(rig, 4410, 20000, -10000);
    CHECK_NEAR(audio[audio.size() - 2], 20000.0 * kBackgroundCeiling, 4.0);
    CHECK_NEAR(audio[audio.size() - 1], -10000.0 * kBackgroundCeiling, 4.0);
    CHECK(rig.engine.underruns() == 0);
}

TEST(pausing_fades_out_and_keeps_the_place) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true);
    renderWithBackground(rig, 8820, 20000, 20000);
    CHECK(!rig.engine.backgroundIdle());
    rig.engine.setBackgroundPlaying(false);
    const auto fade = renderWithBackground(rig, 4410, 20000, 20000);
    int worst = 0;
    for (size_t i = 2; i < fade.size(); ++i) worst = abs(fade[i] - fade[i - 2]) > worst ? abs(fade[i] - fade[i - 2]) : worst;
    CHECK(worst < 40);                 // a ramp, not a step
    CHECK(fade[fade.size() - 1] == 0);
    CHECK(rig.engine.backgroundIdle());
    const uint32_t held = rig.ring.available();
    render(rig.engine, 4410);
    CHECK(rig.ring.available() == held);  // nothing consumed while paused
}

TEST(an_empty_ring_is_counted_and_does_not_disturb_the_tone) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.tone().setEnabled(true);
    render(rig.engine, 44100);
    const int before = peak(render(rig.engine, 22050));
    rig.engine.setBackgroundPlaying(true);  // nothing in the ring
    const int during = peak(render(rig.engine, 22050));
    CHECK(rig.engine.underruns() > 0);
    CHECK_NEAR(during, before, 4.0);
}

TEST(the_end_of_a_track_is_not_an_underrun) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true);
    renderWithBackground(rig, 4410, 1000, 1000);
    rig.engine.setBackgroundEnding();
    render(rig.engine, 44100);  // the ring empties
    CHECK(rig.engine.underruns() == 0);
    rig.engine.setBackgroundPlaying(false);  // the track is over
    render(rig.engine, 4410);
    rig.engine.setBackgroundPlaying(true);   // the next one starts, with nothing buffered
    render(rig.engine, 4410);
    CHECK(rig.engine.underruns() > 0);       // and that is counted again
}

TEST(all_layers_at_maximum_stay_under_the_ceiling) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true);
    rig.engine.setNoise(NoiseType::White, 100);
    rig.engine.tone().setFrequencies(1000.0f, 40.0f);
    rig.engine.tone().setEnabled(true);
    renderWithBackground(rig, 44100, 32767, 32767);
    const auto audio = renderWithBackground(rig, 44100, 32767, 32767);
    const int ceiling = static_cast<int>(kOutputCeiling * 32767.0f) + 1;
    CHECK(peak(audio) <= ceiling);
    CHECK(peak(audio) > 30000);  // it is loud, and it is limited, not wrapped
    // No wrap-around: a wrapped sum would show as a sign flip of huge size.
    int worst = 0;
    for (size_t i = 2; i < audio.size(); ++i) worst = abs(audio[i] - audio[i - 2]) > worst ? abs(audio[i] - audio[i - 2]) : worst;
    CHECK(worst < 30000);
}

namespace {
struct NoiseStats { double mean, rms, lag1; float peak; };
NoiseStats measure(NoiseType type) {
    NoiseGenerator g;
    g.setType(type);
    const int n = 400000;
    double sum = 0, sq = 0, cross = 0;
    float previous = 0, peak = 0;
    for (int i = 0; i < n; ++i) {
        const float x = g.next();
        sum += x; sq += x * x; cross += x * previous;
        previous = x;
        peak = fabsf(x) > peak ? fabsf(x) : peak;
    }
    return {sum / n, sqrt(sq / n), cross / sq, peak};
}
}  // namespace

TEST(noise_types_have_the_expected_colour) {
    const NoiseStats white = measure(NoiseType::White);
    const NoiseStats pink = measure(NoiseType::Pink);
    const NoiseStats brown = measure(NoiseType::Brown);
    for (const NoiseStats& s : {white, pink, brown}) {
        CHECK(fabs(s.mean) < 0.02);  // no offset
        CHECK(s.peak <= 1.0f);       // within range
        CHECK(s.rms > 0.05);         // and audible
    }
    // Correlation between neighbouring samples rises with the weight of the
    // low frequencies: none for white, some for pink, nearly one for brown.
    CHECK(fabs(white.lag1) < 0.02);
    CHECK(pink.lag1 > 0.5 && pink.lag1 < 0.97);
    CHECK(brown.lag1 > 0.98);
}

TEST(noise_off_is_silence_and_noise_fades_in) {
    NoiseGenerator g;
    for (int i = 0; i < 100; ++i) CHECK(g.next() == 0.0f);

    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setNoise(NoiseType::Brown, 100);
    const auto start = render(rig.engine, 441);  // first 10 ms of a 200 ms fade
    CHECK(peak(start) < 1500);
    render(rig.engine, 22050);
    CHECK(peak(render(rig.engine, 22050)) > 2000);
    rig.engine.setNoise(NoiseType::Off, 0);
    render(rig.engine, 22050);
    CHECK(peak(render(rig.engine, 4410)) == 0);
}

TEST(the_session_level_scales_everything_smoothly) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.tone().setEnabled(true);
    render(rig.engine, 44100);
    const int full = peak(render(rig.engine, 22050));
    rig.engine.setSessionGain(0.5f);
    const auto step = render(rig.engine, 22050);
    int worst = 0;
    for (size_t i = 2; i < step.size(); ++i) worst = abs(step[i] - step[i - 2]) > worst ? abs(step[i] - step[i - 2]) : worst;
    CHECK(worst < 600);  // no jump: the sine's own steps are about 500 at this level
    CHECK_NEAR(peak(render(rig.engine, 22050)), full * 0.5, 30.0);
    rig.engine.setSessionGain(0.0f);
    render(rig.engine, 22050);
    CHECK(peak(render(rig.engine, 4410)) <= 1);
    rig.engine.setSessionGain(7.0f);  // out of range: treated as 1
    render(rig.engine, 22050);
    CHECK_NEAR(peak(render(rig.engine, 22050)), full, 30.0);
}

// ---- The layer mixer (RES-AUDIO-016) and the recording's fades (RES-PLAY-007)

namespace {

// Amplitude of the component at `hz` in one channel.
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

int largestStep(const std::vector<int16_t>& audio, int channel) {
    int step = 0;
    for (size_t i = 1; i < audio.size() / 2; ++i) {
        const int d = abs(audio[2 * i + channel] - audio[2 * (i - 1) + channel]);
        step = d > step ? d : step;
    }
    return step;
}

void startTone(Rig& rig) {
    rig.engine.setMasterVolume(100);
    rig.engine.tone().setFrequencies(200.0f, 10.0f);
    rig.engine.tone().setEnabled(true);
    render(rig.engine, 44100);
}

}  // namespace

TEST(the_layers_start_enabled_and_the_mixer_changes_nothing_at_rest) {
    Rig plain, mixed;
    startTone(plain);
    startTone(mixed);
    CHECK(mixed.engine.layerEnabled(Layer::Tone) && mixed.engine.layerEnabled(Layer::Partials));
    CHECK(mixed.engine.layerEnabled(Layer::Noise) && mixed.engine.layerEnabled(Layer::Voice));
    CHECK(mixed.engine.layerLevel(Layer::Tone) == 100 && mixed.engine.layerLevel(Layer::Partials) == 100);
    // Setting every layer to what it already is leaves the samples alone.
    mixed.engine.setLayerEnabled(Layer::Tone, true);
    mixed.engine.setLayerLevel(Layer::Tone, 100);
    mixed.engine.setLayerLevel(Layer::Partials, 100);
    mixed.engine.setLayerFade(Layer::Tone, 5.0f);
    render(plain.engine, 4410);
    render(mixed.engine, 4410);
    const auto a = render(plain.engine, 22050);
    const auto b = render(mixed.engine, 22050);
    bool same = true;
    for (size_t i = 0; i < a.size(); ++i) same = same && a[i] == b[i];
    CHECK(same);
}

TEST(a_layer_switched_off_fades_over_its_time_and_comes_back_at_its_level) {
    Rig rig;
    startTone(rig);
    rig.engine.setLayerLevel(Layer::Tone, 50);
    render(rig.engine, 8820);
    const int usual = largestStep(render(rig.engine, 22050), 0);
    const double level = componentAt(render(rig.engine, 44100), 0, 200.0);
    CHECK_NEAR(level, 0.25 * kToneCeiling * 32767.0, 30.0);

    rig.engine.setLayerFade(Layer::Tone, 2.0f);
    rig.engine.setLayerEnabled(Layer::Tone, false);
    CHECK(!rig.engine.layerEnabled(Layer::Tone));
    CHECK(rig.engine.layerLevel(Layer::Tone) == 50);
    const auto first = render(rig.engine, 44100);
    CHECK(largestStep(first, 0) <= usual + 2);
    // After one of the two seconds about half is left.
    CHECK_NEAR(peak(std::vector<int16_t>(first.end() - 882, first.end())), 0.5 * level, 0.03 * level + 4);
    render(rig.engine, 44100);
    CHECK(peak(render(rig.engine, 4410)) <= 1);

    rig.engine.setLayerEnabled(Layer::Tone, true);
    const auto back = render(rig.engine, 88200);
    CHECK(largestStep(back, 0) <= usual + 2);
    CHECK_NEAR(componentAt(render(rig.engine, 44100), 0, 200.0), level, 2.0);
}

TEST(each_layer_has_a_level_of_its_own) {
    Rig rig;
    startTone(rig);
    rig.engine.tone().setPartial(0, 100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true);
    renderWithBackground(rig, 22050, 6000, 6000);
    auto audio = renderWithBackground(rig, 44100, 6000, 6000);
    const double tone = componentAt(audio, 0, 200.0);
    const double partial = componentAt(audio, 0, 200.0 * kPartialRatio[0]);
    double dc = 0;
    for (size_t i = 0; i < audio.size(); i += 2) dc += audio[i];
    dc /= static_cast<double>(audio.size() / 2);
    CHECK_NEAR(dc, 6000.0 * kBackgroundCeiling, 12.0);  // the partial does not fit the window a whole number of times

    // The partials alone go down: tone and recording stay where they were.
    rig.engine.setLayerLevel(Layer::Partials, 50);
    renderWithBackground(rig, 8820, 6000, 6000);
    audio = renderWithBackground(rig, 44100, 6000, 6000);
    CHECK_NEAR(componentAt(audio, 0, 200.0 * kPartialRatio[0]), partial * 0.25, partial * 0.005);
    CHECK_NEAR(componentAt(audio, 0, 200.0), tone, tone * 0.003);

    // The tone goes off: partials and recording stay.
    rig.engine.setLayerFade(Layer::Tone, 0.1f);
    rig.engine.setLayerEnabled(Layer::Tone, false);
    renderWithBackground(rig, 8820, 6000, 6000);
    audio = renderWithBackground(rig, 44100, 6000, 6000);
    CHECK(componentAt(audio, 0, 200.0) < tone * 0.005);
    CHECK_NEAR(componentAt(audio, 0, 200.0 * kPartialRatio[0]), partial * 0.25, partial * 0.005);
    dc = 0;
    for (size_t i = 0; i < audio.size(); i += 2) dc += audio[i];
    CHECK_NEAR(dc / static_cast<double>(audio.size() / 2), 6000.0 * kBackgroundCeiling, 4.0);

    // The recording's level alone.
    rig.engine.setLayerLevel(Layer::Voice, 50);
    renderWithBackground(rig, 8820, 6000, 6000);
    audio = renderWithBackground(rig, 44100, 6000, 6000);
    dc = 0;
    for (size_t i = 0; i < audio.size(); i += 2) dc += audio[i];
    CHECK_NEAR(dc / static_cast<double>(audio.size() / 2), 1500.0 * kBackgroundCeiling, 4.0);
    CHECK_NEAR(componentAt(audio, 0, 200.0 * kPartialRatio[0]), partial * 0.25, partial * 0.005);
}

TEST(the_noise_layer_switches_with_a_fade_and_keeps_its_type_and_level) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setNoise(NoiseType::Pink, 80);
    render(rig.engine, 22050);
    const int loud = peak(render(rig.engine, 44100));
    CHECK(loud > 1000);
    rig.engine.setLayerFade(Layer::Noise, 1.0f);
    rig.engine.setLayerEnabled(Layer::Noise, false);
    const auto fade = render(rig.engine, 44100);
    // Still sounding half way, silent at the end.
    CHECK(peak(std::vector<int16_t>(fade.begin() + 40000, fade.begin() + 48000)) > loud / 8);
    CHECK(peak(render(rig.engine, 4410)) == 0);
    // A level set while off is kept for the return, and stays silent now.
    rig.engine.setNoise(NoiseType::Pink, 40);
    CHECK(peak(render(rig.engine, 22050)) == 0);
    rig.engine.setLayerEnabled(Layer::Noise, true);
    render(rig.engine, 66150);
    const int quiet = peak(render(rig.engine, 44100));
    CHECK(quiet > loud / 8 && quiet < loud / 2);  // (40/80)^2 = a quarter
}

TEST(a_recording_layer_switched_off_goes_on_playing_unheard) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true);
    renderWithBackground(rig, 8820, 20000, 20000);
    rig.engine.setLayerFade(Layer::Voice, 0.5f);
    rig.engine.setLayerEnabled(Layer::Voice, false);
    const auto fade = renderWithBackground(rig, 22050, 20000, 20000);
    CHECK(largestStep(fade, 0) < 4);
    renderWithBackground(rig, 4410, 20000, 20000);
    const uint32_t before = rig.engine.backgroundFrames();
    const auto silent = renderWithBackground(rig, 4410, 20000, 20000);
    CHECK(peak(silent) == 0);
    CHECK(rig.engine.backgroundFrames() == before + 4410);  // the track moved on
    CHECK(!rig.engine.backgroundIdle());
    CHECK(rig.engine.underruns() == 0);
}

TEST(a_recording_comes_in_and_goes_out_over_the_time_given) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    const double full = 20000.0 * kBackgroundCeiling;
    rig.engine.setBackgroundPlaying(true, 2.0f);
    const auto in = renderWithBackground(rig, 88200, 20000, 20000);
    CHECK(largestStep(in, 0) < 3);
    CHECK_NEAR(in[2 * 22050], full * 0.25, full * 0.01);
    CHECK_NEAR(in[2 * 44100], full * 0.5, full * 0.01);
    CHECK_NEAR(in[in.size() - 2], full, 4.0);

    rig.engine.setBackgroundPlaying(false, 1.0f);
    const auto out = renderWithBackground(rig, 22050, 20000, 20000);
    CHECK(largestStep(out, 0) < 3);
    CHECK_NEAR(out[out.size() - 2], full * 0.5, full * 0.01);
    // Until the fade is over the track is still being played, so the ring
    // may not be refilled from another file yet.
    CHECK(!rig.engine.backgroundIdle());
    const uint32_t during = rig.engine.backgroundFrames();
    renderWithBackground(rig, 22050 + 300, 20000, 20000);
    CHECK(rig.engine.backgroundFrames() > during + 20000);
    renderWithBackground(rig, 600, 20000, 20000);
    CHECK(rig.engine.backgroundIdle());
    CHECK(rig.engine.underruns() == 0);
}

TEST(one_recording_gives_way_to_the_next_through_silence) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true, 0.5f);
    renderWithBackground(rig, 44100, 12000, 12000);
    // The first goes out while it keeps playing...
    rig.engine.setBackgroundPlaying(false, 0.5f);
    std::vector<int16_t> all = renderWithBackground(rig, 22050 + 600, 12000, 12000);
    CHECK(rig.engine.backgroundIdle());
    CHECK(all[all.size() - 2] == 0);
    // ...the ring is emptied and filled from the second...
    rig.ring.discard();
    rig.engine.setBackgroundPlaying(true, 0.5f);
    const auto second = renderWithBackground(rig, 44100, -9000, -9000);
    all.insert(all.end(), second.begin(), second.end());
    // ...which comes in from silence: nowhere a step.
    CHECK(largestStep(all, 0) < 3);
    CHECK_NEAR(second[second.size() - 2], -9000.0 * kBackgroundCeiling, 4.0);
    CHECK(rig.engine.underruns() == 0);
}

TEST(a_pause_during_a_long_fade_in_is_quick_and_bad_fade_times_are_safe) {
    Rig rig;
    rig.engine.setMasterVolume(100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true, 10.0f);
    renderWithBackground(rig, 44100, 20000, 20000);
    rig.engine.setBackgroundPlaying(false);  // the usual quick fade
    const auto out = renderWithBackground(rig, 4410, 20000, 20000);
    CHECK(out[out.size() - 2] == 0);
    CHECK(rig.engine.backgroundIdle());
    // Not finite, negative, far too long: none of them hangs or steps.
    rig.engine.setBackgroundPlaying(true, NAN);
    renderWithBackground(rig, 441, 20000, 20000);
    rig.engine.setBackgroundPlaying(false, -4.0f);
    renderWithBackground(rig, 441, 20000, 20000);
    CHECK(rig.engine.backgroundIdle());
    rig.engine.setLayerFade(Layer::Voice, NAN);
    CHECK(rig.engine.layerFade(Layer::Voice) == kLayerFadeDefaultS);
    rig.engine.setLayerFade(Layer::Voice, 1e9f);
    CHECK(rig.engine.layerFade(Layer::Voice) == kLayerFadeMaxS);
    rig.engine.setLayerLevel(Layer::Voice, 250);
    CHECK(rig.engine.layerLevel(Layer::Voice) == 100);
}

TEST(every_layer_at_once_stays_under_the_ceiling_while_layers_switch) {
    Rig rig;
    startTone(rig);
    rig.engine.tone().setPartial(0, 100);
    rig.engine.tone().setPartial(1, 100);
    rig.engine.setNoise(NoiseType::White, 100);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true, 0.3f);
    for (uint8_t i = 0; i < kLayerCount; ++i) rig.engine.setLayerFade(static_cast<Layer>(i), 0.2f);
    int worst = 0;
    for (int pass = 0; pass < 16; ++pass) {
        rig.engine.setLayerEnabled(static_cast<Layer>(pass % kLayerCount), (pass / kLayerCount) % 2 == 1);
        const auto audio = renderWithBackground(rig, 13230, 32767, -32768);
        worst = peak(audio) > worst ? peak(audio) : worst;
    }
    CHECK(worst <= static_cast<int>(kOutputCeiling * 32767.0f) + 1);
    CHECK(rig.engine.underruns() == 0);
}

// The breathing modulation (RES-SESS-011).
TEST(the_breathing_modulation_moves_tone_and_noise_but_not_the_recording) {
    Rig plain, breathing;
    for (Rig* rig : {&plain, &breathing}) {
        startTone(*rig);
        rig->engine.setBackgroundVolume(100);
        rig->engine.setBackgroundPlaying(true);
        renderWithBackground(*rig, 8820, 6000, 6000);
    }
    // At rest, and set to one, nothing changes: sample for sample.
    breathing.engine.setBreathGain(1.0f);
    auto a = renderWithBackground(plain, 22050, 6000, 6000);
    auto b = renderWithBackground(breathing, 22050, 6000, 6000);
    bool same = true;
    for (size_t i = 0; i < a.size(); ++i) same = same && a[i] == b[i];
    CHECK(same);

    const double tone = componentAt(renderWithBackground(plain, 44100, 6000, 6000), 0, 200.0);
    breathing.engine.setBreathGain(0.5f);
    const auto moving = renderWithBackground(breathing, 22050, 6000, 6000);
    CHECK(largestStep(moving, 0) < 1300);  // no more than the tone's own slope
    b = renderWithBackground(breathing, 44100, 6000, 6000);
    CHECK_NEAR(componentAt(b, 0, 200.0), tone * 0.5, tone * 0.005);
    double dc = 0;
    for (size_t i = 0; i < b.size(); i += 2) dc += b[i];
    CHECK_NEAR(dc / static_cast<double>(b.size() / 2), 6000.0 * kBackgroundCeiling, 4.0);  // the recording is untouched

    // Noise follows; bad values are brought into range.
    Rig noisy;
    noisy.engine.setMasterVolume(100);
    noisy.engine.setNoise(NoiseType::White, 100);
    render(noisy.engine, 22050);
    const int loud = peak(render(noisy.engine, 44100));
    noisy.engine.setBreathGain(NAN);   // the floor
    render(noisy.engine, 22050);
    const int quiet = peak(render(noisy.engine, 44100));
    CHECK(quiet > loud / 6 && quiet < loud / 4);  // 0.2
    noisy.engine.setBreathGain(9.0f);
    render(noisy.engine, 22050);
    CHECK(abs(peak(render(noisy.engine, 44100)) - loud) < loud / 20);
}

int main() { return host_test::runAll(); }
