// Ring buffer, noise, limiter and the mixing engine (RES-AUDIO-007, -008,
// -010; RES-PLAY-003).
#include "host_test.h"
#include "rc_audio.h"
#include "rc_settings.h"

#include <stdlib.h>
#include <vector>

using namespace rc;

namespace {

struct Rig {
    std::vector<int16_t> storage;
    FrameRing ring;
    AudioEngine engine;
    Rig() : storage(4096 * 2), ring(storage.data(), 4096), engine(ring) {}
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
    int16_t storage[8 * 2];
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
    int16_t storage[4 * 2];
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

int main() { return host_test::runAll(); }
