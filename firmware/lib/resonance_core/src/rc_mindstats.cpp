#include "rc_mindstats.h"

#include <math.h>

namespace rc {

namespace {

uint32_t popcount(uint32_t x) {
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;
    return (x * 0x01010101u) >> 24;
}

}  // namespace

double normalUpperTail(double z) { return 0.5 * erfc(z * 0.7071067811865476); }

// Peter Acklam's rational approximation; relative error below 1.2e-9.
double normalQuantile(double p) {
    if (!(p > 0.0)) return -HUGE_VAL;
    if (!(p < 1.0)) return HUGE_VAL;
    static const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02,
                               1.383577518672690e+02,  -3.066479806614716e+01, 2.506628277459239e+00};
    static const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02,
                               6.680131188771972e+01,  -1.328068155288572e+01};
    static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00,
                               -2.549732539343734e+00, 4.374664141464968e+00,  2.938163982698783e+00};
    static const double d[] = {7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00,
                               3.754408661907416e+00};
    const double low = 0.02425;
    if (p < low) {
        const double q = sqrt(-2.0 * log(p));
        return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
               ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    if (p > 1.0 - low) {
        const double q = sqrt(-2.0 * log(1.0 - p));
        return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
               ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    const double q = p - 0.5, r = q * q;
    return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
           (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
}

double sidakCritZ(uint32_t comparisons, double alpha) {
    if (comparisons == 0) comparisons = 1;
    // 1 - (1 - alpha)^(1/k), computed so that it stays exact for a large k.
    const double each = -expm1(log1p(-alpha) / static_cast<double>(comparisons));
    return normalQuantile(1.0 - each * 0.5);
}

double balanceZ(uint64_t ones, uint64_t bits) {
    if (bits == 0) return 0.0;
    const double n = static_cast<double>(bits);
    return (2.0 * static_cast<double>(ones) - n) / sqrt(n);
}

MindComparison compareProportions(uint64_t ones1, uint64_t bits1, uint64_t ones2, uint64_t bits2) {
    MindComparison out;
    if (bits1 == 0 || bits2 == 0) return out;
    const double n1 = static_cast<double>(bits1), n2 = static_cast<double>(bits2);
    const double p1 = static_cast<double>(ones1) / n1, p2 = static_cast<double>(ones2) / n2;
    out.valid = true;
    out.difference = p2 - p1;
    // Pooled for the test, unpooled for the interval.
    const double pooled = static_cast<double>(ones1 + ones2) / (n1 + n2);
    const double sePooled = sqrt(pooled * (1.0 - pooled) * (1.0 / n1 + 1.0 / n2));
    out.z = sePooled > 0.0 ? out.difference / sePooled : 0.0;
    out.pTwoSided = 2.0 * normalUpperTail(fabs(out.z));
    const double se = sqrt(p1 * (1.0 - p1) / n1 + p2 * (1.0 - p2) / n2);
    out.ciLow = out.difference - kZ95 * se;
    out.ciHigh = out.difference + kZ95 * se;
    return out;
}

void MindStats::begin(uint32_t windowSamples, MindTarget target) {
    *this = MindStats();
    _windowSamples = windowSamples == 0 ? 1 : windowSamples;
    _target = target;
}

void MindStats::add(uint32_t x) {
    const uint32_t ones = popcount(x);
    // Lags: bit i against bit i - lag, across the boundary with the sample
    // before. The very first sample has nothing before it.
    if (_samples != 0) {
        for (uint8_t i = 0; i < kLagCount; ++i) {
            const uint8_t lag = kLags[i];
            const uint32_t earlier = lag >= 32 ? _previous : ((x << lag) | (_previous >> (32 - lag)));
            _agree[i] += 32u - popcount(x ^ earlier);
        }
    }
    // Runs, bit by bit from the least significant.
    for (uint8_t i = 0; i < 32; ++i) {
        const uint8_t bit = (x >> i) & 1u;
        if (_currentRun != 0 && bit == _lastBit) {
            ++_currentRun;
        } else {
            if (_currentRun != 0) ++_runLength[(_currentRun < kRunBins ? _currentRun : kRunBins) - 1];
            ++_runs;
            _currentRun = 1;
            _lastBit = bit;
        }
    }
    ++_bytes[x & 0xFFu];
    ++_bytes[(x >> 8) & 0xFFu];
    ++_bytes[(x >> 16) & 0xFFu];
    ++_bytes[x >> 24];
    _previous = x;
    _ones += ones;
    ++_samples;

    _windowOnes += ones;
    if (++_windowFill >= _windowSamples) {
        const double z = balanceZ(_windowOnes, static_cast<uint64_t>(_windowFill) * 32u);
        if (_windows != 0) _sumLag += z * _lastWindowZ;
        _sumZ += z;
        _sumZ2 += z * z;
        if (fabs(z) > _maxAbsZ) _maxAbsZ = fabs(z);
        if (fabs(z) >= kZ95) ++_beyond;
        _lastWindowZ = z;
        ++_windows;
        _windowFill = 0;
        _windowOnes = 0;
    }
}

double MindStats::z() const { return balanceZ(_ones, _samples * 32u); }

double MindStats::directedZ() const {
    if (_target == MindTarget::MoreOnes) return z();
    if (_target == MindTarget::MoreZeros) return -z();
    return 0.0;
}

void MindStats::summary(MindSummary& out) const {
    out = MindSummary();
    out.samples = _samples;
    out.bits = _samples * 32u;
    out.ones = _ones;
    if (out.bits == 0) return;
    const double n = static_cast<double>(out.bits);
    const double p = static_cast<double>(_ones) / n;
    out.proportion = p;
    out.z = z();
    out.pTwoSided = 2.0 * normalUpperTail(fabs(out.z));
    // Wilson score interval.
    const double zz = kZ95 * kZ95;
    const double centre = (p + zz / (2.0 * n)) / (1.0 + zz / n);
    const double half = kZ95 * sqrt(p * (1.0 - p) / n + zz / (4.0 * n * n)) / (1.0 + zz / n);
    out.ciLow = centre - half;
    out.ciHigh = centre + half;
    out.directedZ = directedZ();
    out.pDirected = _target == MindTarget::None ? 1.0 : normalUpperTail(out.directedZ);

    // Runs. The run still open counts, as it does in the test's definition.
    out.runs = _runs;
    for (uint8_t i = 0; i < kRunBins; ++i) out.runLength[i] = _runLength[i];
    if (_currentRun != 0) ++out.runLength[(_currentRun < kRunBins ? _currentRun : kRunBins) - 1];
    const double n1 = static_cast<double>(_ones), n0 = n - n1;
    if (n1 > 0.0 && n0 > 0.0 && n > 1.0) {
        out.runsExpected = 2.0 * n1 * n0 / n + 1.0;
        const double variance = 2.0 * n1 * n0 * (2.0 * n1 * n0 - n) / (n * n * (n - 1.0));
        out.runsZ = variance > 0.0 ? (static_cast<double>(_runs) - out.runsExpected) / sqrt(variance) : 0.0;
    } else {
        out.runsExpected = 1.0;
        // Every bit the same: one run where about n/2 are expected.
        out.runsZ = n > 1.0 ? -sqrt(n) : 0.0;
    }

    // Bytes.
    const double expected = static_cast<double>(_samples) * 4.0 / 256.0;
    double chi2 = 0.0;
    for (uint32_t count : _bytes) {
        const double diff = static_cast<double>(count) - expected;
        chi2 += diff * diff / expected;
    }
    out.chi2Bytes = chi2;
    // Wilson-Hilferty: the cube root of chi-squared over its degrees of
    // freedom is close to normal.
    const double k = 255.0, v = 2.0 / (9.0 * k);
    out.chi2Z = (cbrt(chi2 / k) - (1.0 - v)) / sqrt(v);

    // Lags.
    if (_samples > 1) {
        const double pairs = static_cast<double>(_samples - 1) * 32.0;
        for (uint8_t i = 0; i < kLagCount; ++i) {
            const double agree = static_cast<double>(_agree[i]);
            out.autocorr[i] = 2.0 * agree / pairs - 1.0;
            out.autocorrZ[i] = (2.0 * agree - pairs) / sqrt(pairs);
        }
    }

    // Windows.
    out.windows = _windows;
    if (_windows != 0) {
        const double k2 = static_cast<double>(_windows);
        out.windowMeanZ = _sumZ / k2;
        out.windowVarZ = _windows > 1 ? (_sumZ2 - _sumZ * _sumZ / k2) / (k2 - 1.0) : 0.0;
        out.windowMaxAbsZ = _maxAbsZ;
        out.windowsBeyond95 = _beyond;
        out.windowCritZ = sidakCritZ(_windows);
        out.windowExceedsCrit = _maxAbsZ >= out.windowCritZ;
        if (_windows > 2 && out.windowVarZ > 0.0) {
            const double mean = out.windowMeanZ;
            // Lag-1 covariance of the scores about their mean, over the
            // variance. The end terms are left out of the mean correction:
            // with more than a few windows they do not matter.
            const double cov = _sumLag / (k2 - 1.0) - mean * mean;
            out.windowLag1 = cov / out.windowVarZ;
        }
    }
}

}  // namespace rc
