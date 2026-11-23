// The Resonance Lab (RES-AUDIO-017).
#include "host_test.h"
#include "rc_audio.h"
#include "rc_lab.h"
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
    Rig() : storage(4096), ring(storage.data(), 4096), engine(ring) {
        engine.setMasterVolume(100);
        engine.tone().setFrequencies(200.0f, 10.0f);
        engine.tone().setEnabled(true);
        render(44100);
    }
    std::vector<int16_t> render(size_t frames) {
        std::vector<int16_t> out(frames * 2);
        size_t done = 0;
        while (done < frames) {
            const size_t n = frames - done < 367 ? frames - done : 367;
            engine.render(&out[done * 2], n);
            done += n;
        }
        return out;
    }
};

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

// Phase of the component at `hz`, in radians.
double phaseAt(const std::vector<int16_t>& audio, int channel, double hz) {
    const size_t frames = audio.size() / 2;
    double re = 0, im = 0;
    for (size_t i = 0; i < frames; ++i) {
        const double a = 2.0 * M_PI * hz * static_cast<double>(i) / kSampleRate;
        re += audio[2 * i + channel] * cos(a);
        im += audio[2 * i + channel] * sin(a);
    }
    return atan2(im, re);
}

int largestStep(const std::vector<int16_t>& audio, int channel) {
    int step = 0;
    for (size_t i = 1; i < audio.size() / 2; ++i) {
        const int d = abs(audio[2 * i + channel] - audio[2 * (i - 1) + channel]);
        step = d > step ? d : step;
    }
    return step;
}

double rms(const std::vector<int16_t>& audio, int channel, size_t from, size_t to) {
    double sum = 0;
    for (size_t i = from; i < to; ++i) sum += static_cast<double>(audio[2 * i + channel]) * audio[2 * i + channel];
    return sqrt(sum / static_cast<double>(to - from));
}

}  // namespace

TEST(settings_are_brought_into_range) {
    LabSettings s;
    CHECK(labAtRest(s));
    CHECK(!sanitizeLab(s));
    s.widthPercent = 250;
    s.delayUs = 9000;
    s.bilateralPeriodDs = 2;
    s.bilateralDepth = 180;
    CHECK(sanitizeLab(s));
    CHECK(s.widthPercent == 100 && s.delayUs == kLabDelayMaxUs && s.bilateralPeriodDs == kBilateralMinDs && s.bilateralDepth == 100);
    s.delayUs = -9000;
    s.bilateralPeriodDs = 9999;
    sanitizeLab(s);
    CHECK(s.delayUs == -kLabDelayMaxUs && s.bilateralPeriodDs == kBilateralMaxDs);
    CHECK(!labAtRest(s));
    s = LabSettings{};
    s.bilateralPeriodDs = 20;
    s.bilateralDepth = 0;   // an alternation of no depth changes nothing
    CHECK(labAtRest(s));
}

TEST(at_rest_the_lab_is_not_in_the_signal_path) {
    Rig plain, lab;
    lab.engine.lab().set(LabSettings{});
    lab.engine.lab().setTrim(1.0f);
    const auto a = plain.render(22050);
    const auto b = lab.render(22050);
    bool same = true;
    for (size_t i = 0; i < a.size(); ++i) same = same && a[i] == b[i];
    CHECK(same);
    // Used and put back, it leaves the path again: the samples are the plain ones once more.
    LabSettings s;
    s.widthPercent = 40;
    s.delayUs = 500;
    s.bilateralPeriodDs = 10;
    lab.engine.lab().set(s);
    lab.engine.lab().setTrim(0.5f);
    plain.render(44100);
    lab.render(44100);
    lab.engine.lab().set(LabSettings{});
    lab.engine.lab().setTrim(1.0f);
    plain.render(44100);
    lab.render(44100);
    const auto c = plain.render(22050);
    const auto d = lab.render(22050);
    same = true;
    for (size_t i = 0; i < c.size(); ++i) same = same && c[i] == d[i];
    CHECK(same);
}

TEST(width_brings_the_ears_together) {
    Rig rig;
    const auto full = rig.render(44100);
    const double own = componentAt(full, 0, 200.0);
    CHECK(componentAt(full, 0, 210.0) < own * 0.01);   // as generated: nothing of the other ear
    LabSettings s;
    s.widthPercent = 0;
    rig.engine.lab().set(s);
    const auto moving = rig.render(8820);
    // The left ear now carries the right's higher tone as well: a slightly steeper slope, no step.
    CHECK(largestStep(moving, 0) <= largestStep(full, 0) * 106 / 100);
    const auto mono = rig.render(44100);
    bool same = true;
    for (size_t i = 0; i < mono.size(); i += 2) same = same && abs(mono[i] - mono[i + 1]) <= 1;
    CHECK(same);                                        // both ears hear the same
    CHECK_NEAR(componentAt(mono, 0, 200.0), own * 0.5, own * 0.01);
    CHECK_NEAR(componentAt(mono, 0, 210.0), own * 0.5, own * 0.01);
    s.widthPercent = 50;
    rig.engine.lab().set(s);
    rig.render(8820);
    const auto half = rig.render(44100);
    CHECK_NEAR(componentAt(half, 0, 200.0), own * 0.75, own * 0.01);
    CHECK_NEAR(componentAt(half, 0, 210.0), own * 0.25, own * 0.01);
    CHECK_NEAR(componentAt(half, 1, 210.0), own * 0.75, own * 0.01);
}

TEST(the_interaural_delay_holds_one_ear_back_by_the_time_set) {
    Rig reference, delayed;
    const int usual = largestStep(reference.render(22050), 1);
    delayed.render(22050);
    LabSettings s;
    s.delayUs = 500;   // the right ear later
    delayed.engine.lab().set(s);
    const auto moving = delayed.render(22050);
    reference.render(22050);
    CHECK(largestStep(moving, 0) <= usual + 2);
    CHECK(largestStep(moving, 1) <= usual + 3);
    auto a = reference.render(44100);
    auto b = delayed.render(44100);
    // 500 us at 210 Hz is 37.8 degrees behind.
    double lag = phaseAt(b, 1, 210.0) - phaseAt(a, 1, 210.0);
    lag = fmod(lag + 3.0 * M_PI, 2.0 * M_PI) - M_PI;
    CHECK_NEAR(lag * 180.0 / M_PI, 37.8, 0.3);
    CHECK_NEAR(componentAt(b, 1, 210.0), componentAt(a, 1, 210.0), 30.0);
    bool same = true;
    for (size_t i = 0; i < a.size(); i += 2) same = same && abs(a[i] - b[i]) <= 1;
    CHECK(same);                                        // the left ear is untouched
    // The other way, at the limit: 2000 us on the left ear.
    s.delayUs = -5000;
    delayed.engine.lab().set(s);
    const auto turning = delayed.render(22050);
    reference.render(22050);
    CHECK(largestStep(turning, 0) <= usual + 3);
    CHECK(largestStep(turning, 1) <= usual + 3);
    a = reference.render(44100);
    b = delayed.render(44100);
    lag = phaseAt(b, 0, 200.0) - phaseAt(a, 0, 200.0);
    lag = fmod(lag + 3.0 * M_PI, 2.0 * M_PI) - M_PI;
    CHECK_NEAR(lag * 180.0 / M_PI, 144.0, 0.5);         // 2000 us at 200 Hz
    for (size_t i = 0; i < a.size(); i += 2) same = same && abs(a[i + 1] - b[i + 1]) <= 1;
    CHECK(same);
}

TEST(the_alternation_shape_is_smooth_and_splits_the_time_as_asked) {
    for (bool golden : {true, false}) {
        int left = 0, right = 0;
        float last = bilateralSide(0.0f, golden), biggest = 0.0f;
        for (int i = 1; i <= 100000; ++i) {
            const float side = bilateralSide(static_cast<float>(i) / 100000.0f, golden);
            CHECK(side >= -1.0f && side <= 1.0f);
            biggest = fabsf(side - last) > biggest ? fabsf(side - last) : biggest;
            last = side;
            if (side < 0.0f) ++left; else if (side > 0.0f) ++right;
        }
        CHECK(biggest < 0.0005f);                        // no jump anywhere, the wrap included
        const double share = static_cast<double>(left) / (left + right);
        CHECK_NEAR(share, golden ? 0.618 : 0.5, 0.002);
    }
    CHECK(bilateralSide(0.3f, true) == -1.0f);
    CHECK(bilateralSide(0.8f, true) == 1.0f);
    CHECK(bilateralSide(NAN, true) == bilateralSide(0.0f, true));
    CHECK_NEAR(bilateralSide(7.3f, false), bilateralSide(0.3f, false), 1e-4);
}

TEST(the_alternation_moves_the_sound_between_the_ears_without_a_step) {
    Rig rig;
    const auto before = rig.render(22050);
    const int usual = largestStep(before, 0), usualRight = largestStep(before, 1);
    LabSettings s;
    s.bilateralPeriodDs = 20;   // two seconds
    s.bilateralDepth = 100;
    s.bilateralGolden = true;
    rig.engine.lab().set(s);
    rig.render(44100);                                   // the depth has come in
    const auto audio = rig.render(10 * 44100);           // five periods
    CHECK(largestStep(audio, 0) <= usual + 2);
    CHECK(largestStep(audio, 1) <= usualRight + 2);
    // In tenths of a second: how long each ear is the louder one.
    int left = 0, right = 0;
    double quietest = 1e9, loudest = 0;
    for (size_t t = 0; t + 441 <= audio.size() / 2; t += 441) {
        const double l = rms(audio, 0, t, t + 441), r = rms(audio, 1, t, t + 441);
        if (l > r) ++left; else ++right;
        quietest = l < quietest ? l : quietest;
        loudest = l > loudest ? l : loudest;
    }
    CHECK_NEAR(static_cast<double>(left) / (left + right), 0.618, 0.02);
    CHECK(quietest < loudest * 0.02);                    // at full depth the far ear is silent
    // Half the depth: the far ear keeps half its level.
    s.bilateralDepth = 50;
    s.bilateralGolden = false;
    rig.engine.lab().set(s);
    rig.render(44100);
    const auto half = rig.render(4 * 44100);
    quietest = 1e9;
    loudest = 0;
    left = right = 0;
    for (size_t t = 0; t + 441 <= half.size() / 2; t += 441) {
        const double l = rms(half, 0, t, t + 441), r = rms(half, 1, t, t + 441);
        if (l > r) ++left; else ++right;
        quietest = l < quietest ? l : quietest;
        loudest = l > loudest ? l : loudest;
    }
    CHECK_NEAR(quietest / loudest, 0.5, 0.02);
    CHECK_NEAR(static_cast<double>(left) / (left + right), 0.5, 0.02);
    // Switched off it fades out of the path.
    s.bilateralPeriodDs = 0;
    rig.engine.lab().set(s);
    const auto leaving = rig.render(44100);
    CHECK(largestStep(leaving, 0) <= usual + 2);
    const auto after = rig.render(44100);
    CHECK_NEAR(rms(after, 0, 0, 44100), rms(after, 1, 0, 44100), 20.0);
}

TEST(a_recording_is_not_touched_by_the_lab) {
    Rig rig;
    rig.engine.tone().setEnabled(false);
    rig.engine.setBackgroundVolume(100);
    rig.engine.setBackgroundPlaying(true);
    LabSettings s;
    s.widthPercent = 0;
    s.delayUs = 1000;
    s.bilateralPeriodDs = 10;
    rig.engine.lab().set(s);
    rig.engine.lab().setTrim(0.3f);
    std::vector<int16_t> fill(512 * 2);
    for (size_t i = 0; i < 512; ++i) { fill[2 * i] = 8000; fill[2 * i + 1] = -4000; }
    std::vector<int16_t> out;
    for (int pass = 0; pass < 300; ++pass) {
        while (rig.ring.space() >= 512) rig.ring.write(fill.data(), 512);
        out = rig.render(300);
    }
    CHECK_NEAR(out[out.size() - 2], 8000.0 * kBackgroundCeiling, 4.0);
    CHECK_NEAR(out[out.size() - 1], -4000.0 * kBackgroundCeiling, 4.0);
}

TEST(a_weighting_follows_the_standard_curve) {
    CHECK_NEAR(aWeight(1000.0f), 1.0, 0.002);
    CHECK_NEAR(20.0 * log10(aWeight(100.0f)), -19.1, 0.15);
    CHECK_NEAR(20.0 * log10(aWeight(200.0f)), -10.9, 0.15);
    CHECK_NEAR(20.0 * log10(aWeight(500.0f)), -3.2, 0.15);
    CHECK(aWeight(0.0f) == 0.0f && aWeight(-5.0f) == 0.0f && aWeight(NAN) == 0.0f);
}

TEST(two_settings_are_matched_by_turning_the_louder_one_down) {
    LabVoice a, b;
    float ta = 0, tb = 0;
    matchLoudness(a, b, ta, tb);
    CHECK(ta == 1.0f && tb == 1.0f);                     // the same setting: nothing to do
    b.partial[0] = 100;
    b.partial[1] = 100;
    matchLoudness(a, b, ta, tb);
    CHECK(ta == 1.0f && tb < 1.0f && tb > 0.2f);         // partials make B louder: B comes down
    CHECK_NEAR(labLoudness(a), labLoudness(b) * tb * tb, labLoudness(a) * 1e-4);
    // A higher tone is heard louder at the same amplitude.
    b = LabVoice{};
    b.primaryHz = 400.0f;
    matchLoudness(a, b, ta, tb);
    CHECK(ta == 1.0f && tb < 0.6f);
    // Width and the alternation lower the level a little, and are made up for on the other side.
    b = LabVoice{};
    b.lab.widthPercent = 0;
    matchLoudness(a, b, ta, tb);
    CHECK(tb == 1.0f && ta < 1.0f);
    CHECK_NEAR(ta, sqrt(0.5), 0.01);
    b = LabVoice{};
    b.lab.bilateralPeriodDs = 20;
    b.lab.bilateralDepth = 100;
    matchLoudness(a, b, ta, tb);
    CHECK_NEAR(ta, sqrt(0.5), 0.01);
    // Silence on one side, or a value that is no number: no trim, and never above one.
    b = LabVoice{};
    b.toneGain = 0.0f;
    matchLoudness(a, b, ta, tb);
    CHECK(ta == 1.0f && tb == 1.0f);
    b.toneGain = NAN;
    matchLoudness(a, b, ta, tb);
    CHECK(ta == 1.0f && tb == 1.0f);
}

TEST(switching_between_two_settings_makes_no_step_and_matches_the_level) {
    Rig rig;
    const int usual = largestStep(rig.render(22050), 0);
    LabVoice a, b;
    a.primaryHz = 200.0f;
    a.beatHz = 10.0f;
    b = a;
    b.partial[0] = 80;
    b.lab.delayUs = 300;
    float ta = 0, tb = 0;
    matchLoudness(a, b, ta, tb);
    auto apply = [&rig](const LabVoice& v, float trim) {
        rig.engine.tone().setFrequencies(v.primaryHz, v.beatHz);
        for (uint8_t p = 0; p < kPartialCount; ++p) rig.engine.tone().setPartial(p, v.partial[p]);
        rig.engine.lab().set(v.lab);
        rig.engine.lab().setTrim(trim);
    };
    int worst = 0;
    for (int pass = 0; pass < 6; ++pass) {
        apply(pass % 2 == 0 ? b : a, pass % 2 == 0 ? tb : ta);
        const auto audio = rig.render(22050);
        worst = largestStep(audio, 0) > worst ? largestStep(audio, 0) : worst;
        worst = largestStep(audio, 1) > worst ? largestStep(audio, 1) : worst;
    }
    // B carries a partial at 324 Hz, so its own slope is steeper than the plain tone's.
    CHECK(worst <= usual * 2);
    // Unmatched, B would be clearly louder; matched, B is turned down.
    CHECK(tb < 0.95f && ta == 1.0f);
}

int main() { return host_test::runAll(); }
