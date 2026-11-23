// What the Mindfield pictures show (RES-MIND-005).
#include "host_test.h"
#include "rc_mindstats.h"
#include "rc_mindviz.h"
#include "rc_rng.h"

#include <initializer_list>
#include <vector>

using namespace rc;

TEST(a_score_of_zero_is_the_centre) {
    VizInput in;
    CHECK(vizScore(in) == 0.0f);
    CHECK(vizUnit(0.0f) == 0.0f);
    CHECK(vizUnit(kVizFullZ) == 1.0f && vizUnit(-kVizFullZ) == -1.0f);
    CHECK(vizUnit(100.0f) == 1.0f && vizUnit(-100.0f) == -1.0f);  // never beyond the field
    CHECK_NEAR(vizUnit(1.5f), 0.5, 1e-6);
    CHECK_NEAR(vizUnit(-0.75f), -0.25, 1e-6);
    CHECK(vizUnit(NAN) == 0.0f);
    for (float z = -4.0f; z <= 4.0f; z += 0.25f) CHECK_NEAR(vizUnit(-z), -vizUnit(z), 1e-6);  // no side is favoured
}

TEST(a_directed_picture_shows_the_score_towards_the_target) {
    VizInput in;
    in.z = 1.2f;
    in.directedZ = -1.2f;  // the target was "more zeros"
    in.directed = true;
    CHECK(vizScore(in) == -1.2f);  // against the target: the picture moves away from it
    in.directed = false;
    CHECK(vizScore(in) == 1.2f);
}

TEST(smoothing_approaches_and_never_passes_or_changes_the_measure) {
    const float target = 0.8f;
    float shown = 0.0f;
    float previous = shown;
    for (int i = 0; i < 200; ++i) {
        shown = vizApproach(shown, target, 50);
        CHECK(shown >= previous && shown <= target);
        previous = shown;
    }
    CHECK_NEAR(shown, target, 1e-4);
    CHECK(target == 0.8f);
    // One time constant leaves 1/e of the distance.
    CHECK_NEAR(vizApproach(0.0f, 1.0f, kVizSmoothMs), 1.0 - exp(-1.0), 1e-5);
    CHECK(vizApproach(0.3f, 0.3f, 1000) == 0.3f);
    CHECK(vizApproach(0.5f, 0.2f, 0) == 0.5f);
    CHECK(vizApproach(NAN, 0.4f, 10) == 0.4f);
    // Downwards as well, and to the centre when the score returns there.
    CHECK(vizApproach(1.0f, 0.0f, 100) < 1.0f && vizApproach(1.0f, 0.0f, 100) > 0.0f);
}

TEST(chance_keeps_a_score_picture_near_the_centre) {
    // The position is a standard score, not a running sum: over long runs of
    // the control source it stays in the field's inner part and does not
    // creep outwards with time.
    for (uint32_t seed : {11u, 12u, 13u, 14u}) {
        SoftwareRng rng(seed);
        rng.begin();
        MindStats stats;
        stats.begin(100, MindTarget::MoreOnes);
        float early = 0.0f, late = 0.0f;
        int beyondHalf = 0, checks = 0;
        for (int i = 1; i <= 400000; ++i) {
            stats.add(rng.next());
            if (i % 1000 != 0) continue;
            VizInput in;
            in.directed = true;
            in.directedZ = static_cast<float>(stats.directedZ());
            const float u = fabsf(vizUnit(vizScore(in)));
            (i <= 200000 ? early : late) += u;
            if (u > 0.5f) ++beyondHalf;
            ++checks;
            CHECK(u < 1.0f);  // a score of 3 would be a one-in-700 event at any one moment
        }
        CHECK(late < early * 3.0f + 20.0f);     // no growth with time
        CHECK(beyondHalf < checks / 3);         // |z| > 1.5 is the exception
    }
}

TEST(a_real_bias_moves_the_picture_and_only_in_its_direction) {
    MindStats ones, zeros;
    ones.begin(100, MindTarget::MoreOnes);
    zeros.begin(100, MindTarget::MoreZeros);
    SoftwareRng rng(3);
    rng.begin();
    for (int i = 0; i < 20000; ++i) {
        const uint32_t v = rng.next() | (i % 8 == 0 ? 0x00000101u : 0u);  // a few extra ones
        ones.add(v);
        zeros.add(v);
    }
    VizInput a, b;
    a.directed = b.directed = true;
    a.directedZ = static_cast<float>(ones.directedZ());
    b.directedZ = static_cast<float>(zeros.directedZ());
    CHECK(vizUnit(vizScore(a)) > 0.9f);
    CHECK(vizUnit(vizScore(b)) < -0.9f);  // the same data against the other target
}

TEST(a_tree_grows_the_same_from_the_same_samples_and_stays_in_its_field) {
    for (uint32_t seed : {1u, 2u, 3u, 4u, 5u, 6u}) {
        SoftwareRng rng(seed), again(seed);
        rng.begin();
        again.begin();
        Tree a, b;
        a.begin();
        b.begin();
        TreeSegment s, t;
        int segments = 0;
        while (a.step(rng.next(), s)) {
            CHECK(b.step(again.next(), t));
            CHECK(s.x0 == t.x0 && s.y0 == t.y0 && s.x1 == t.x1 && s.y1 == t.y1 && s.depth == t.depth);
            for (int16_t x : {s.x0, s.x1}) CHECK(x >= -kTreeHalfWidth && x <= kTreeHalfWidth);
            for (int16_t y : {s.y0, s.y1}) CHECK(y >= 0 && y <= kTreeHeight);
            ++segments;
        }
        CHECK(segments == kTreeSteps && a.steps() == kTreeSteps);
        CHECK(a.tips() >= 2 && a.tips() <= kTreeTips);
        CHECK(!a.step(1, s));  // full grown
    }
    // Different samples, a different tree.
    Tree a, b;
    a.begin();
    b.begin();
    TreeSegment s, t;
    bool differs = false;
    SoftwareRng r1(1), r2(2);
    r1.begin();
    r2.begin();
    for (int i = 0; i < 20; ++i) {
        a.step(r1.next(), s);
        b.step(r2.next(), t);
        differs = differs || s.x1 != t.x1 || s.y1 != t.y1;
    }
    CHECK(differs);
    // The trunk starts at the root and goes up.
    a.begin();
    a.step(0, s);
    CHECK(s.x0 == 0 && s.y0 == 0 && s.y1 > 0 && s.depth == 0);
}

TEST(a_mandala_is_built_ring_by_ring) {
    Mandala m;
    m.begin();
    MandalaElement e;
    SoftwareRng rng(8);
    rng.begin();
    int count = 0;
    uint8_t lastRing = 0;
    while (m.step(rng.next(), e)) {
        CHECK(e.ring < kMandalaRings && e.kind < 4 && e.size < 4);
        CHECK(e.ring >= lastRing);
        lastRing = e.ring;
        ++count;
    }
    CHECK(count == kMandalaRings * kMandalaPerRing && lastRing == kMandalaRings - 1);
    // Bits to shape: kind from the lowest two, then the angle, then the size.
    m.begin();
    m.step(0x00000C00u | (0xABu << 2) | 2u, e);
    CHECK(e.kind == 2 && e.angle == 0xAB && e.size == 3 && e.ring == 0);
}

TEST(a_lotus_opens_every_petal_once_outermost_first) {
    for (uint32_t seed : {1u, 9u, 123u}) {
        Lotus lotus;
        lotus.begin();
        SoftwareRng rng(seed);
        rng.begin();
        bool seen[kLotusLayers][kLotusPetals] = {};
        LotusPetal p;
        int count = 0;
        uint8_t lastLayer = 0;
        while (lotus.step(rng.next(), p)) {
            CHECK(p.layer < kLotusLayers && p.petal < kLotusPetals && p.size < 4);
            CHECK(!seen[p.layer][p.petal]);
            seen[p.layer][p.petal] = true;
            CHECK(p.layer >= lastLayer);
            lastLayer = p.layer;
            ++count;
        }
        CHECK(count == kLotusLayers * kLotusPetals && lotus.opened() == count);
    }
}

TEST(flow_channels_follow_the_three_decisions) {
    CHECK(flowChannel(0) == 0);
    CHECK(flowChannel(7) == 7);
    CHECK(flowChannel(1) == 4);  // first decision right, then left, left
    CHECK(flowChannel(4) == 1);  // left, left, right
    CHECK(flowChannel(0xFFFFFFF8u) == 0);  // only the low three bits decide
    CHECK(!flowTurn(2, 0) && flowTurn(2, 1) && !flowTurn(2, 2) && !flowTurn(0xFF, 3));
    FlowCounts flow;
    flow.begin();
    CHECK(flow.chi2Z() == 0.0f && flow.total() == 0);
    // Perfectly even: flatter than chance.
    for (uint32_t i = 0; i < 800; ++i) flow.add(i);
    for (uint8_t c = 0; c < kFlowChannels; ++c) CHECK(flow.count(c) == 100);
    CHECK(flow.chi2Z() < -3.0f);
    CHECK(flow.count(kFlowChannels) == 0);
    // Everything down one channel.
    flow.begin();
    for (int i = 0; i < 800; ++i) CHECK(flow.add(5) == flowChannel(5));
    CHECK(flow.chi2Z() > 10.0f);
    // Chance.
    flow.begin();
    SoftwareRng rng(21);
    rng.begin();
    for (int i = 0; i < 80000; ++i) flow.add(rng.next());
    CHECK(flow.total() == 80000 && fabsf(flow.chi2Z()) < 3.5f);
    for (uint8_t c = 0; c < kFlowChannels; ++c) CHECK(flow.count(c) > 9500 && flow.count(c) < 10500);
}

TEST(the_maze_is_one_connected_maze_with_walls_that_agree) {
    static const int8_t dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};
    int passages = 0;
    for (uint8_t y = 0; y < kMazeSize; ++y) {
        for (uint8_t x = 0; x < kMazeSize; ++x) {
            const uint8_t open = Labyrinth::openings(x, y);
            CHECK(open != 0 && open < 16);
            for (uint8_t d = 0; d < 4; ++d) {
                if (((open >> d) & 1u) == 0) continue;
                const int nx = x + dx[d], ny = y + dy[d];
                CHECK(nx >= 0 && ny >= 0 && nx < kMazeSize && ny < kMazeSize);  // no opening to outside
                CHECK((Labyrinth::openings(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny)) >> ((d + 2) & 3)) & 1u);
                ++passages;
            }
        }
    }
    CHECK(passages == 2 * (kMazeSize * kMazeSize - 1));  // a tree: exactly one way between any two cells
    CHECK(Labyrinth::openings(kMazeSize, 0) == 0);
    // Flood from the start reaches every cell.
    bool seen[kMazeSize][kMazeSize] = {};
    std::vector<std::pair<int, int>> todo = {{0, 0}};
    seen[0][0] = true;
    int reached = 1;
    while (!todo.empty()) {
        const auto at = todo.back();
        todo.pop_back();
        const uint8_t open = Labyrinth::openings(static_cast<uint8_t>(at.first), static_cast<uint8_t>(at.second));
        for (uint8_t d = 0; d < 4; ++d) {
            const int nx = at.first + dx[d], ny = at.second + dy[d];
            if (((open >> d) & 1u) == 0 || seen[ny][nx]) continue;
            seen[ny][nx] = true;
            ++reached;
            todo.push_back({nx, ny});
        }
    }
    CHECK(reached == kMazeSize * kMazeSize);
}

TEST(the_point_moves_through_openings_only_and_finds_the_exit_by_chance) {
    Labyrinth a, b;
    a.begin();
    b.begin();
    SoftwareRng rng(4), again(4);
    rng.begin();
    again.begin();
    for (int i = 0; i < 20000; ++i) {
        const uint8_t x = a.x(), y = a.y();
        const uint32_t exits = a.exits();
        a.step(rng.next());
        b.step(again.next());
        CHECK(a.x() == b.x() && a.y() == b.y());  // the same samples, the same path
        if (a.exits() != exits) {
            CHECK(a.x() == 0 && a.y() == 0);  // back to the start after an exit
            continue;
        }
        const int mx = a.x() - x, my = a.y() - y;
        CHECK(abs(mx) + abs(my) == 1);
        const uint8_t d = mx == 1 ? 1 : (mx == -1 ? 3 : (my == 1 ? 2 : 0));
        CHECK((Labyrinth::openings(x, y) >> d) & 1u);
    }
    CHECK(a.steps() == 20000);
    CHECK(a.exits() > 5);
}

int main() { return host_test::runAll(); }
