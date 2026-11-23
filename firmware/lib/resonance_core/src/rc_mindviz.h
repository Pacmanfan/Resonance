// Resonance portable core: what the Mindfield pictures show (RES-MIND-005).
//
// A picture is a reading of the statistics, never a part of them. Everything
// here takes numbers that rc_mindstats has already computed, or raw samples
// that have already been recorded, and turns them into positions and shapes.
// Nothing here can change a sample or a score, and the same recorded data
// always gives the same picture: a run can be played again from its file.
//
// Two kinds of picture:
//  - Score pictures (Orb, Balance, Star, Flame, Pendulum, Vortex) show one
//    number, the standard score of the bit balance: towards the target in a
//    directed experiment, towards "more ones" in an open one. Zero is the
//    neutral state, and chance keeps the picture near it: a score is not a
//    sum that wanders off, so nothing drifts outwards by itself.
//  - Pattern pictures (Tree, Mandala, Lotus, River, Waterfall, Labyrinth)
//    are built step by step from recorded samples, one a window. They show
//    what randomness looks like; they are not a measure of deviation, and
//    the screen says so.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

// The score at which a score picture reaches its limit.
constexpr float kVizFullZ = 3.0f;

// What a picture is given.
struct VizInput {
    float z = 0.0f;           // bit balance so far, positive for more ones
    float directedZ = 0.0f;   // the same towards the target
    bool directed = false;
    uint32_t windows = 0;     // completed windows
    uint32_t windowSample = 0;  // the sample that ended the last window
};

// The one number a score picture shows.
float vizScore(const VizInput& in);
// The score as a share of the picture's range: -1..1, 0 for a score of 0 and
// for anything that is not a number.
float vizUnit(float score);
// Moves what is shown towards the measured value, for a calm picture: the
// distance left shrinks by 1/e every `kVizSmoothMs`. It never passes the
// target and never changes it.
constexpr uint32_t kVizSmoothMs = 600;
float vizApproach(float shown, float target, uint32_t elapsedMs);

// ---- growth: tree --------------------------------------------------------------

constexpr uint8_t kTreeTips = 10;
constexpr uint16_t kTreeSteps = 150;   // a tree is full grown after this many
constexpr int16_t kTreeHalfWidth = 100;  // it stays inside -100..100 across
constexpr int16_t kTreeHeight = 170;     // and 0..170 upwards from the root

struct TreeSegment {
    int16_t x0, y0, x1, y1;  // x across from the root, y upwards
    uint8_t depth;           // 0 = trunk
};

class Tree {
public:
    void begin();
    // Grows one segment from the sample. False once the tree is full grown.
    bool step(uint32_t sample, TreeSegment& out);
    uint16_t steps() const { return _steps; }
    uint8_t tips() const { return _tipCount; }

private:
    struct Tip {
        float x, y, angle;  // angle in radians from straight up
        uint8_t depth;
    };
    Tip _tips[kTreeTips];
    uint8_t _tipCount = 0;
    uint16_t _steps = 0;
};

// ---- growth: mandala and lotus ---------------------------------------------------

constexpr uint8_t kMandalaRings = 6;
constexpr uint8_t kMandalaPerRing = 6;  // elements a ring takes before the next begins
constexpr uint8_t kMandalaFolds[] = {6, 8, 12};  // "complexity": how many times an element repeats round

struct MandalaElement {
    uint8_t ring;    // 0 innermost
    uint8_t kind;    // 0 dot, 1 petal, 2 spoke, 3 arc
    uint8_t angle;   // position within one fold, 0..255
    uint8_t size;    // 0..3
};

class Mandala {
public:
    void begin() { _steps = 0; }
    bool step(uint32_t sample, MandalaElement& out);
    uint16_t steps() const { return _steps; }

private:
    uint16_t _steps = 0;
};

constexpr uint8_t kLotusLayers = 3;
constexpr uint8_t kLotusPetals = 8;  // in a layer

struct LotusPetal {
    uint8_t layer;   // 0 = outermost, opening first
    uint8_t petal;   // 0..kLotusPetals-1
    uint8_t size;    // 0..3
};

class Lotus {
public:
    void begin();
    // Opens one petal, chosen by the sample among those still closed in the
    // layer. False once every petal is open.
    bool step(uint32_t sample, LotusPetal& out);
    uint8_t opened() const { return _opened; }

private:
    uint8_t _open[kLotusLayers] = {};  // bit per petal
    uint8_t _opened = 0;
};

// ---- flow ----------------------------------------------------------------------

constexpr uint8_t kFlowChannels = 8;
constexpr uint8_t kFlowLevels = 3;  // left-or-right decisions on the way down

// The channel a particle ends in: three decisions, one bit each, the first
// decision being the most significant. Every channel is equally likely.
uint8_t flowChannel(uint32_t sample);
// The decision at `level` (0 first): false left, true right.
bool flowTurn(uint32_t sample, uint8_t level);

class FlowCounts {
public:
    void begin();
    uint8_t add(uint32_t sample);  // returns the channel
    uint32_t count(uint8_t channel) const { return channel < kFlowChannels ? _counts[channel] : 0; }
    uint32_t total() const { return _total; }
    // Chi-squared of the counts against equal shares (7 degrees of freedom),
    // as a standard score; 0 with nothing counted.
    float chi2Z() const;

private:
    uint32_t _counts[kFlowChannels] = {};
    uint32_t _total = 0;
};

// A fixed maze of 7 x 7 cells. A point starts at the top left and at each
// step moves to a neighbouring cell through an opening, chosen by the sample
// among the openings that do not lead straight back (unless it is a dead
// end). Reaching the bottom right cell is an exit; the point starts again.
constexpr uint8_t kMazeSize = 7;

class Labyrinth {
public:
    void begin();
    void step(uint32_t sample);
    uint8_t x() const { return _x; }
    uint8_t y() const { return _y; }
    uint32_t exits() const { return _exits; }
    uint32_t steps() const { return _steps; }
    // Openings of a cell: bit 0 up, 1 right, 2 down, 3 left.
    static uint8_t openings(uint8_t x, uint8_t y);

private:
    uint8_t _x = 0, _y = 0;
    uint8_t _from = 0xFF;  // direction the point came by; 0xFF = none
    uint32_t _exits = 0, _steps = 0;
};

}  // namespace rc
