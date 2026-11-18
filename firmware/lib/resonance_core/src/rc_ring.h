// Resonance portable core: a ring buffer of stereo 16-bit frames between one
// writer (the decoder task) and one reader (the audio render). Lock-free:
// each side changes only its own index (RES-AUDIO-012, RES-PLAY-003).
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <atomic>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace rc {

class FrameRing {
public:
    // `storage` holds 2 * capacityFrames samples; capacityFrames must be a
    // power of two.
    FrameRing(int16_t* storage, uint32_t capacityFrames)
        : _data(storage), _mask(capacityFrames - 1), _head(0), _tail(0) {}

    uint32_t capacity() const { return _mask + 1; }
    uint32_t available() const {
        return _head.load(std::memory_order_acquire) - _tail.load(std::memory_order_acquire);
    }
    uint32_t space() const { return capacity() - available(); }

    // Writer side. Returns the number of frames stored (may be fewer).
    uint32_t write(const int16_t* frames, uint32_t count) {
        const uint32_t head = _head.load(std::memory_order_relaxed);
        const uint32_t room = capacity() - (head - _tail.load(std::memory_order_acquire));
        if (count > room) count = room;
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t at = ((head + i) & _mask) * 2;
            _data[at] = frames[2 * i];
            _data[at + 1] = frames[2 * i + 1];
        }
        _head.store(head + count, std::memory_order_release);
        return count;
    }

    // Reader side. Returns the number of frames delivered (may be fewer).
    uint32_t read(int16_t* frames, uint32_t count) {
        const uint32_t tail = _tail.load(std::memory_order_relaxed);
        const uint32_t have = _head.load(std::memory_order_acquire) - tail;
        if (count > have) count = have;
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t at = ((tail + i) & _mask) * 2;
            frames[2 * i] = _data[at];
            frames[2 * i + 1] = _data[at + 1];
        }
        _tail.store(tail + count, std::memory_order_release);
        return count;
    }

    // Reader side: throws away everything buffered.
    void discard() { _tail.store(_head.load(std::memory_order_acquire), std::memory_order_release); }

private:
    int16_t* _data;
    uint32_t _mask;
    std::atomic<uint32_t> _head;  // written by the writer only
    std::atomic<uint32_t> _tail;  // written by the reader only
};

}  // namespace rc
