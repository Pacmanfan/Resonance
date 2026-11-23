#include "rc_mindviz.h"

#include <math.h>

namespace rc {

float vizScore(const VizInput& in) { return in.directed ? in.directedZ : in.z; }

float vizUnit(float score) {
    if (!(score == score)) return 0.0f;  // not a number
    const float u = score / kVizFullZ;
    return u > 1.0f ? 1.0f : (u < -1.0f ? -1.0f : u);
}

float vizApproach(float shown, float target, uint32_t elapsedMs) {
    if (!(shown == shown)) return target;
    const float keep = expf(-static_cast<float>(elapsedMs) / static_cast<float>(kVizSmoothMs));
    return target + (shown - target) * keep;
}

// ---- tree ----------------------------------------------------------------------

void Tree::begin() {
    _tipCount = 1;
    _tips[0] = {0.0f, 0.0f, 0.0f, 0};
    _steps = 0;
}

bool Tree::step(uint32_t sample, TreeSegment& out) {
    if (_steps >= kTreeSteps || _tipCount == 0) return false;
    // Bits, from the lowest: which tip (4), which way and how far it turns
    // (2), how long the segment is (2), whether the tip forks (2).
    Tip& tip = _tips[(sample & 0x0Fu) % _tipCount];
    static const float kTurn[4] = {-0.52f, -0.21f, 0.21f, 0.52f};  // about 30 and 12 degrees
    const float turn = kTurn[(sample >> 4) & 3u];
    const float length = (9.0f + static_cast<float>((sample >> 6) & 3u) * 3.0f) / (1.0f + 0.12f * tip.depth);
    const bool fork = ((sample >> 8) & 3u) == 0 && _tipCount < kTreeTips;

    float angle = tip.angle + turn;
    // A branch does not grow downwards.
    if (angle > 1.35f) angle = 1.35f;
    if (angle < -1.35f) angle = -1.35f;
    float x = tip.x + sinf(angle) * length;
    float y = tip.y + cosf(angle) * length;
    // At the edge of the field it turns back in rather than leaving it.
    if (x > kTreeHalfWidth) { x = static_cast<float>(kTreeHalfWidth); angle = -fabsf(angle) * 0.5f; }
    if (x < -kTreeHalfWidth) { x = -static_cast<float>(kTreeHalfWidth); angle = fabsf(angle) * 0.5f; }
    if (y > kTreeHeight) y = static_cast<float>(kTreeHeight);

    out.x0 = static_cast<int16_t>(lrintf(tip.x));
    out.y0 = static_cast<int16_t>(lrintf(tip.y));
    out.x1 = static_cast<int16_t>(lrintf(x));
    out.y1 = static_cast<int16_t>(lrintf(y));
    out.depth = tip.depth;

    const Tip before = tip;
    tip.x = x;
    tip.y = y;
    tip.angle = angle;
    if (tip.depth < 250) ++tip.depth;
    if (fork) {
        // The other arm of the fork starts where this segment did, turned
        // the other way.
        _tips[_tipCount++] = {before.x, before.y, before.angle - turn, static_cast<uint8_t>(before.depth + 1)};
    }
    ++_steps;
    return true;
}

// ---- mandala and lotus -----------------------------------------------------------

bool Mandala::step(uint32_t sample, MandalaElement& out) {
    if (_steps >= static_cast<uint16_t>(kMandalaRings) * kMandalaPerRing) return false;
    out.ring = static_cast<uint8_t>(_steps / kMandalaPerRing);
    out.kind = static_cast<uint8_t>(sample & 3u);
    out.angle = static_cast<uint8_t>((sample >> 2) & 0xFFu);
    out.size = static_cast<uint8_t>((sample >> 10) & 3u);
    ++_steps;
    return true;
}

void Lotus::begin() {
    for (uint8_t& layer : _open) layer = 0;
    _opened = 0;
}

bool Lotus::step(uint32_t sample, LotusPetal& out) {
    if (_opened >= kLotusLayers * kLotusPetals) return false;
    const uint8_t layer = _opened / kLotusPetals;
    const uint8_t closed = static_cast<uint8_t>(kLotusPetals - _opened % kLotusPetals);
    uint8_t pick = static_cast<uint8_t>((sample & 0xFFu) % closed);
    uint8_t petal = 0;
    for (;; ++petal) {
        if ((_open[layer] >> petal) & 1u) continue;
        if (pick == 0) break;
        --pick;
    }
    _open[layer] = static_cast<uint8_t>(_open[layer] | (1u << petal));
    out.layer = layer;
    out.petal = petal;
    out.size = static_cast<uint8_t>((sample >> 8) & 3u);
    ++_opened;
    return true;
}

// ---- flow ----------------------------------------------------------------------

bool flowTurn(uint32_t sample, uint8_t level) { return level < kFlowLevels && ((sample >> level) & 1u) != 0; }

uint8_t flowChannel(uint32_t sample) {
    uint8_t channel = 0;
    for (uint8_t level = 0; level < kFlowLevels; ++level) channel = static_cast<uint8_t>((channel << 1) | (flowTurn(sample, level) ? 1 : 0));
    return channel;
}

void FlowCounts::begin() {
    for (uint32_t& c : _counts) c = 0;
    _total = 0;
}

uint8_t FlowCounts::add(uint32_t sample) {
    const uint8_t channel = flowChannel(sample);
    ++_counts[channel];
    ++_total;
    return channel;
}

float FlowCounts::chi2Z() const {
    if (_total == 0) return 0.0f;
    const double expected = static_cast<double>(_total) / kFlowChannels;
    double chi2 = 0.0;
    for (uint32_t c : _counts) {
        const double diff = static_cast<double>(c) - expected;
        chi2 += diff * diff / expected;
    }
    // Wilson-Hilferty, 7 degrees of freedom.
    const double k = kFlowChannels - 1, v = 2.0 / (9.0 * k);
    return static_cast<float>((cbrt(chi2 / k) - (1.0 - v)) / sqrt(v));
}

// ---- labyrinth -------------------------------------------------------------------

namespace {

const int8_t kDx[4] = {0, 1, 0, -1};
const int8_t kDy[4] = {-1, 0, 1, 0};

// The maze is the same on every device and in every replay: carved once by a
// depth-first walk whose choices come from a fixed arithmetic sequence.
const uint8_t* maze() {
    static uint8_t cells[kMazeSize * kMazeSize];
    static bool built = false;
    if (built) return cells;
    for (uint8_t& c : cells) c = 0;
    bool seen[kMazeSize * kMazeSize] = {};
    uint8_t stack[kMazeSize * kMazeSize];
    uint8_t depth = 0;
    uint32_t state = 0x2F6E2B1u;
    stack[depth++] = 0;
    seen[0] = true;
    while (depth != 0) {
        const uint8_t at = stack[depth - 1];
        const int x = at % kMazeSize, y = at / kMazeSize;
        uint8_t options[4], count = 0;
        for (uint8_t d = 0; d < 4; ++d) {
            const int nx = x + kDx[d], ny = y + kDy[d];
            if (nx < 0 || ny < 0 || nx >= kMazeSize || ny >= kMazeSize || seen[ny * kMazeSize + nx]) continue;
            options[count++] = d;
        }
        if (count == 0) {
            --depth;
            continue;
        }
        state = state * 1664525u + 1013904223u;
        const uint8_t d = options[(state >> 24) % count];
        const uint8_t next = static_cast<uint8_t>((y + kDy[d]) * kMazeSize + (x + kDx[d]));
        cells[at] = static_cast<uint8_t>(cells[at] | (1u << d));
        cells[next] = static_cast<uint8_t>(cells[next] | (1u << ((d + 2) & 3)));
        seen[next] = true;
        stack[depth++] = next;
    }
    built = true;
    return cells;
}

}  // namespace

uint8_t Labyrinth::openings(uint8_t x, uint8_t y) {
    if (x >= kMazeSize || y >= kMazeSize) return 0;
    return maze()[y * kMazeSize + x];
}

void Labyrinth::begin() {
    _x = _y = 0;
    _from = 0xFF;
    _exits = _steps = 0;
}

void Labyrinth::step(uint32_t sample) {
    const uint8_t open = openings(_x, _y);
    uint8_t options[4], count = 0;
    for (uint8_t d = 0; d < 4; ++d) {
        if (((open >> d) & 1u) != 0 && d != _from) options[count++] = d;
    }
    // A dead end: the only way is back.
    if (count == 0 && _from != 0xFF) options[count++] = _from;
    if (count == 0) return;
    const uint8_t d = options[(sample & 0xFFu) % count];
    _x = static_cast<uint8_t>(_x + kDx[d]);
    _y = static_cast<uint8_t>(_y + kDy[d]);
    _from = static_cast<uint8_t>((d + 2) & 3);
    ++_steps;
    if (_x == kMazeSize - 1 && _y == kMazeSize - 1) {
        ++_exits;
        _x = _y = 0;
        _from = 0xFF;
    }
}

}  // namespace rc
