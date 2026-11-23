// Mindfield statistics (RES-MIND-003), against data with known answers.
#include "host_test.h"
#include "rc_mindstats.h"

#include <initializer_list>
#include "rc_rng.h"

using namespace rc;

namespace {

MindSummary run(uint32_t (*next)(uint32_t), uint32_t count, uint32_t window = 100,
                MindTarget target = MindTarget::None) {
    MindStats stats;
    stats.begin(window, target);
    for (uint32_t i = 0; i < count; ++i) stats.add(next(i));
    MindSummary s;
    stats.summary(s);
    return s;
}

}  // namespace

TEST(normal_distribution_helpers) {
    CHECK_NEAR(normalUpperTail(0.0), 0.5, 1e-12);
    CHECK_NEAR(normalUpperTail(1.6448536), 0.05, 1e-6);
    CHECK_NEAR(normalUpperTail(-1.959964), 0.975, 1e-6);
    CHECK_NEAR(normalQuantile(0.975), 1.959964, 1e-5);
    CHECK_NEAR(normalQuantile(0.5), 0.0, 1e-9);
    CHECK_NEAR(normalQuantile(0.001), -3.090232, 1e-5);
    for (double p : {0.0001, 0.01, 0.3, 0.9, 0.9999}) CHECK_NEAR(1.0 - normalUpperTail(normalQuantile(p)), p, 1e-7);
}

TEST(more_comparisons_need_a_higher_threshold) {
    CHECK_NEAR(sidakCritZ(1), 1.959964, 1e-5);
    CHECK_NEAR(sidakCritZ(10), 2.800, 2e-3);    // each at 0.00512
    CHECK_NEAR(sidakCritZ(100), 3.4740, 2e-3);  // each at 0.000513
    CHECK(sidakCritZ(1000) > sidakCritZ(100));
    CHECK_NEAR(sidakCritZ(0), sidakCritZ(1), 1e-12);
    CHECK(sidakCritZ(3600000) < 7.0);  // an hour of windows: still finite
}

TEST(nothing_collected_claims_nothing) {
    MindStats stats;
    stats.begin(10, MindTarget::MoreOnes);
    MindSummary s;
    stats.summary(s);
    CHECK(s.samples == 0 && s.bits == 0 && s.windows == 0);
    CHECK(s.z == 0.0 && s.directedZ == 0.0 && s.pDirected == 1.0 && s.pTwoSided == 1.0);
    CHECK(stats.z() == 0.0 && stats.directedZ() == 0.0);
}

TEST(perfectly_balanced_alternating_bits) {
    const MindSummary s = run([](uint32_t) { return 0x55555555u; }, 1000);
    CHECK(s.bits == 32000 && s.ones == 16000);
    CHECK_NEAR(s.proportion, 0.5, 1e-12);
    CHECK_NEAR(s.z, 0.0, 1e-12);
    CHECK_NEAR(s.pTwoSided, 1.0, 1e-12);
    CHECK(s.ciLow < 0.5 && s.ciHigh > 0.5);
    CHECK_NEAR(s.ciHigh - 0.5, 1.96 * 0.5 / sqrt(32000.0), 1e-4);
    // Balance alone says "fine"; the other measures say what this really is.
    CHECK(s.runs == 32000);  // every bit starts a new run
    CHECK(s.runLength[0] == 32000);
    CHECK(s.runsZ > 100.0);
    CHECK_NEAR(s.autocorr[0], -1.0, 1e-12);  // lag 1: always different
    CHECK_NEAR(s.autocorr[1], 1.0, 1e-12);   // lag 2: always the same
    CHECK_NEAR(s.autocorr[3], 1.0, 1e-12);   // lag 32
    CHECK(s.chi2Z > 100.0);                  // one byte value only
    CHECK(s.windows == 10 && s.windowMaxAbsZ == 0.0 && s.windowsBeyond95 == 0);
}

TEST(all_ones) {
    const MindSummary s = run([](uint32_t) { return 0xFFFFFFFFu; }, 100);
    CHECK(s.ones == 3200);
    CHECK_NEAR(s.z, sqrt(3200.0), 1e-9);
    CHECK(s.runs == 1 && s.runLength[kRunBins - 1] == 1);
    CHECK(s.runsZ < -50.0);
    CHECK_NEAR(s.autocorr[0], 1.0, 1e-12);
    CHECK(s.pTwoSided < 1e-100);
}

TEST(a_known_bias) {
    // Three ones in every four bits: 0x77777777.
    const MindSummary s = run([](uint32_t) { return 0x77777777u; }, 500, 50, MindTarget::MoreOnes);
    CHECK(s.bits == 16000 && s.ones == 12000);
    CHECK_NEAR(s.proportion, 0.75, 1e-12);
    CHECK_NEAR(s.z, (24000.0 - 16000.0) / sqrt(16000.0), 1e-9);
    CHECK_NEAR(s.directedZ, s.z, 1e-12);
    CHECK(s.pDirected < 1e-100);
    CHECK(s.ciLow > 0.74 && s.ciHigh < 0.76 && s.ciLow < 0.75 && s.ciHigh > 0.75);
    // Each window of 50 samples: z = (2*1200 - 1600)/40 = 20.
    CHECK(s.windows == 10);
    CHECK_NEAR(s.windowMeanZ, 20.0, 1e-9);
    CHECK_NEAR(s.windowMaxAbsZ, 20.0, 1e-9);
    CHECK(s.windowsBeyond95 == 10 && s.windowExceedsCrit);
}

TEST(the_target_decides_the_sign_and_nothing_else) {
    auto biased = [](uint32_t) { return 0x77777777u; };
    const MindSummary ones = run(biased, 200, 50, MindTarget::MoreOnes);
    const MindSummary zeros = run(biased, 200, 50, MindTarget::MoreZeros);
    const MindSummary open = run(biased, 200, 50, MindTarget::None);
    CHECK_NEAR(zeros.directedZ, -ones.directedZ, 1e-12);
    CHECK(ones.pDirected < 1e-50);
    CHECK(zeros.pDirected > 1.0 - 1e-12);  // against the target: no success to report
    CHECK(open.directedZ == 0.0 && open.pDirected == 1.0);
    // What was measured is the same in all three.
    CHECK(ones.ones == zeros.ones && ones.ones == open.ones);
    CHECK(ones.z == zeros.z && ones.z == open.z);
    CHECK(ones.chi2Bytes == open.chi2Bytes && ones.runs == zeros.runs);
}

TEST(run_lengths_of_a_repeating_pattern) {
    // From the least significant bit: sixteen ones, then sixteen zeros.
    const MindSummary s = run([](uint32_t) { return 0x0000FFFFu; }, 50);
    CHECK(s.runs == 100);
    CHECK(s.runLength[kRunBins - 1] == 100);  // all of length 16 or more
    CHECK_NEAR(s.z, 0.0, 1e-12);
    // 1 one, 2 zeros, 3 ones, 2 zeros, then 24 ones: across the boundary the
    // 24 join the next sample's single one.
    const MindSummary t = run([](uint32_t) { return 0xFFFFFF39u; }, 10);
    CHECK(t.runLength[0] == 1);                 // only the very first one stands alone
    CHECK(t.runLength[1] == 20);                // two runs of two zeros a sample
    CHECK(t.runLength[2] == 10);
    CHECK(t.runLength[kRunBins - 1] == 10);     // 25 nine times, and 24 at the end
    CHECK(t.runs == 41);
}

TEST(every_byte_value_equally_often) {
    // Bytes 0..255 in order, four a sample: a perfectly flat distribution.
    const MindSummary s = run([](uint32_t i) {
        const uint32_t b = i * 4u;
        return (b & 0xFFu) | (((b + 1) & 0xFFu) << 8) | (((b + 2) & 0xFFu) << 16) | (((b + 3) & 0xFFu) << 24);
    }, 6400);
    CHECK_NEAR(s.chi2Bytes, 0.0, 1e-9);
    CHECK(s.chi2Z < -10.0);  // too flat to be chance: also a finding
    CHECK_NEAR(s.z, 0.0, 1e-12);
}

TEST(a_sequence_with_nothing_in_it_shows_nothing) {
    for (uint32_t seed : {1u, 2u, 3u, 77u, 2026u}) {
        SoftwareRng rng(seed);
        rng.begin();
        MindStats stats;
        stats.begin(100, MindTarget::MoreOnes);
        for (int i = 0; i < 200000; ++i) stats.add(rng.next());
        MindSummary s;
        stats.summary(s);
        CHECK(s.windows == 2000);
        CHECK(fabs(s.z) < 4.0 && fabs(s.runsZ) < 4.0 && fabs(s.chi2Z) < 4.0);
        for (uint8_t i = 0; i < kLagCount; ++i) CHECK(fabs(s.autocorrZ[i]) < 4.0 && fabs(s.autocorr[i]) < 0.01);
        CHECK((s.ciLow < 0.5 && s.ciHigh > 0.5) || fabs(s.z) > 1.9);
        // Window scores behave like independent standard normal values.
        CHECK(fabs(s.windowMeanZ) < 0.1);
        CHECK(s.windowVarZ > 0.85 && s.windowVarZ < 1.15);
        CHECK(s.windowsBeyond95 > 60 && s.windowsBeyond95 < 140);  // 100 expected
        CHECK(fabs(s.windowLag1) < 0.1);
        // Among 2000 windows some pass 1.96 - and that is exactly why the
        // corrected threshold exists: none passes it here.
        CHECK(s.windowMaxAbsZ > 1.96);
        CHECK(s.windowCritZ > 4.0 && !s.windowExceedsCrit);
        // Half of all runs have length one, a quarter length two.
        const double runs = static_cast<double>(s.runs);
        CHECK_NEAR(s.runLength[0] / runs, 0.5, 0.005);
        CHECK_NEAR(s.runLength[1] / runs, 0.25, 0.005);
    }
}

TEST(windows_that_depend_on_each_other_are_seen) {
    // Slow drift: a hundred samples heavy in ones, a hundred heavy in zeros.
    const MindSummary s = run([](uint32_t i) { return (i / 100) % 2 == 0 ? 0xFFFF7FFFu : 0x00008000u; }, 4000, 10);
    CHECK(s.windows == 400);
    CHECK(s.windowLag1 > 0.5);    // neighbours agree: not independent observations
    CHECK_NEAR(s.z, 0.0, 1e-12);  // while the total balance is perfect
}

TEST(live_scores_agree_with_the_summary) {
    SoftwareRng rng(5);
    rng.begin();
    MindStats stats;
    stats.begin(64, MindTarget::MoreZeros);
    for (int i = 0; i < 1000; ++i) stats.add(rng.next());
    MindSummary s;
    stats.summary(s);
    CHECK_NEAR(stats.z(), s.z, 1e-12);
    CHECK_NEAR(stats.directedZ(), -s.z, 1e-12);
    CHECK(stats.windows() == 15 && s.windows == 15);  // the unfinished window is not scored
    CHECK(stats.samples() == 1000);
    uint64_t bytes = 0;
    for (int v = 0; v < 256; ++v) bytes += stats.byteCount(static_cast<uint8_t>(v));
    CHECK(bytes == 4000);
    // Asking for a summary changes nothing.
    MindSummary again;
    stats.summary(again);
    CHECK(again.z == s.z && again.runs == s.runs && again.runLength[0] == s.runLength[0]);
}

TEST(two_conditions_are_compared_with_each_other) {
    // Same proportion in both: no difference, whatever its distance from 0.5.
    MindComparison c = compareProportions(510000, 1000000, 510000, 1000000);
    CHECK(c.valid);
    CHECK_NEAR(c.difference, 0.0, 1e-12);
    CHECK_NEAR(c.z, 0.0, 1e-12);
    CHECK_NEAR(c.pTwoSided, 1.0, 1e-12);
    // Each of these alone is far from 0.5 (z = 20); that is the source's own
    // bias and says nothing about the condition.
    CHECK(balanceZ(510000, 1000000) > 19.0);
    // A real difference: 0.500 against 0.503 in a million bits each.
    c = compareProportions(500000, 1000000, 503000, 1000000);
    CHECK_NEAR(c.difference, 0.003, 1e-12);
    CHECK_NEAR(c.z, 0.003 / sqrt(0.5015 * 0.4985 * 2e-6), 1e-6);
    CHECK(c.z > 4.2 && c.z < 4.3);
    CHECK(c.ciLow > 0.0 && c.ciLow < 0.003 && c.ciHigh > 0.003);
    CHECK(c.pTwoSided < 1e-4);
    // Order decides the sign.
    CHECK_NEAR(compareProportions(503000, 1000000, 500000, 1000000).z, -c.z, 1e-9);
    // Nothing to compare.
    CHECK(!compareProportions(0, 0, 5, 10).valid);
    CHECK(!compareProportions(5, 10, 0, 0).valid);
}

TEST(begin_starts_from_nothing) {
    MindStats stats;
    stats.begin(10, MindTarget::MoreOnes);
    for (int i = 0; i < 55; ++i) stats.add(0xFFFFFFFFu);
    stats.begin(0, MindTarget::None);  // a window of nothing is a window of one
    CHECK(stats.samples() == 0 && stats.windows() == 0 && stats.target() == MindTarget::None);
    stats.add(0x0000FFFFu);
    MindSummary s;
    stats.summary(s);
    CHECK(s.windows == 1 && s.ones == 16 && s.runs == 2);
    CHECK(s.autocorr[0] == 0.0);  // one sample has nothing before it
}

int main() { return host_test::runAll(); }
