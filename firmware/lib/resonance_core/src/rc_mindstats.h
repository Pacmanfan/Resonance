// Resonance portable core: the statistics of a Mindfield experiment
// (RES-MIND-003).
//
// One engine, fed raw 32-bit samples in the order they were collected. It
// knows nothing of sources, files or pictures, and nothing can change what
// it has counted. A picture reads its numbers; it never writes any.
//
// The bit stream is each sample from its least significant bit upwards.
//
// What the numbers mean, and do not mean:
//  - z scores are against the null hypothesis of independent fair bits. A
//    real source has a small bias of its own, and with millions of bits a
//    tiny bias gives a large z. Only a comparison with baseline data from
//    the same source (compareProportions) says anything about a condition.
//  - The window scores of one run are many comparisons. windowCritZ is the
//    threshold corrected for their number; windowLag1 shows whether they
//    can be treated as independent at all.
//  - A small p value is not evidence of a cause.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

constexpr uint8_t kRunBins = 16;  // run lengths 1..15, and 16 or more
constexpr uint8_t kLagCount = 4;
constexpr uint8_t kLags[kLagCount] = {1, 2, 8, 32};  // in bits
constexpr double kZ95 = 1.959963984540054;

// The direction chosen before a directed experiment. Stored in files.
enum class MindTarget : uint8_t { None = 0, MoreOnes = 1, MoreZeros = 2 };

struct MindSummary {
    uint64_t samples = 0;
    uint64_t bits = 0;
    uint64_t ones = 0;
    // Bit balance.
    double proportion = 0.5;  // ones / bits
    double z = 0.0;           // positive: more ones than zeros
    double ciLow = 0.0;       // 95 % Wilson interval of the proportion
    double ciHigh = 1.0;
    double pTwoSided = 1.0;
    // Directed experiments: the same score counted towards the target, and
    // the one-sided probability of a score at least this large by chance.
    double directedZ = 0.0;
    double pDirected = 1.0;
    // Runs of identical bits (Wald-Wolfowitz).
    uint64_t runs = 0;
    double runsExpected = 0.0;
    double runsZ = 0.0;
    uint32_t runLength[kRunBins] = {};
    // Byte values against a uniform distribution: chi-squared with 255
    // degrees of freedom, and the same as a standard score.
    double chi2Bytes = 0.0;
    double chi2Z = 0.0;
    // Agreement of each bit with the bit `kLags[i]` before it.
    double autocorr[kLagCount] = {};   // -1..1
    double autocorrZ[kLagCount] = {};
    // Fixed windows.
    uint32_t windows = 0;
    double windowMeanZ = 0.0;
    double windowVarZ = 0.0;         // 1 is expected
    double windowMaxAbsZ = 0.0;
    uint32_t windowsBeyond95 = 0;    // |z| >= 1.96; 5 % of the windows are expected
    double windowCritZ = 0.0;        // 5 % threshold corrected for `windows` comparisons
    bool windowExceedsCrit = false;  // some window passed that threshold
    double windowLag1 = 0.0;         // correlation of successive window scores
};

class MindStats {
public:
    // `windowSamples` is the length of a measurement window in samples (at
    // least 1). The target cannot be changed afterwards.
    void begin(uint32_t windowSamples, MindTarget target);
    void add(uint32_t sample);

    uint64_t samples() const { return _samples; }
    uint64_t ones() const { return _ones; }
    MindTarget target() const { return _target; }
    uint32_t windows() const { return _windows; }
    // Bit balance of everything so far; 0 before the first sample.
    double z() const;
    // The same counted towards the target; 0 without a target.
    double directedZ() const;
    // Score of the last completed window; 0 before the first.
    double lastWindowZ() const { return _lastWindowZ; }
    uint32_t byteCount(uint8_t value) const { return _bytes[value]; }

    void summary(MindSummary& out) const;

private:
    MindTarget _target = MindTarget::None;
    uint32_t _windowSamples = 1;
    uint64_t _samples = 0;
    uint64_t _ones = 0;
    uint32_t _bytes[256] = {};
    // Runs.
    uint64_t _runs = 0;
    uint32_t _runLength[kRunBins] = {};
    uint32_t _currentRun = 0;
    uint8_t _lastBit = 0;
    // Lags.
    uint32_t _previous = 0;
    uint64_t _agree[kLagCount] = {};
    // Windows.
    uint32_t _windowFill = 0;
    uint32_t _windowOnes = 0;
    uint32_t _windows = 0;
    double _lastWindowZ = 0.0;
    double _sumZ = 0.0, _sumZ2 = 0.0, _sumLag = 0.0, _maxAbsZ = 0.0;
    uint32_t _beyond = 0;
};

// P(Z >= z) for a standard normal variable.
double normalUpperTail(double z);
// The z with P(Z <= z) = p, for 0 < p < 1.
double normalQuantile(double p);
// Two-sided threshold at level `alpha` for one of `comparisons` independent
// comparisons (Sidak), so that the chance of any of them passing by chance is
// `alpha`.
double sidakCritZ(uint32_t comparisons, double alpha = 0.05);

// Bit balance for a count: z, positive for more ones.
double balanceZ(uint64_t ones, uint64_t bits);

// Two sets of bits from the same source under two conditions: the difference
// of the proportions of ones (second minus first), its standard score and
// 95 % interval. This, not either z alone, is the comparison of a
// meditation run with its baseline.
struct MindComparison {
    double difference = 0.0;
    double z = 0.0;
    double pTwoSided = 1.0;
    double ciLow = 0.0;
    double ciHigh = 0.0;
    bool valid = false;  // both sets hold bits
};
MindComparison compareProportions(uint64_t ones1, uint64_t bits1, uint64_t ones2, uint64_t bits2);

}  // namespace rc
