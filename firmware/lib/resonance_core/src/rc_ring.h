// Resonance portable core: a ring buffer of stereo 16-bit frames between one
// writer (the decoder task) and one reader (the audio render). Lock-free:
// each side changes only its own index (RES-AUDIO-012, RES-PLAY-003).
//
// A frame is kept as one 32-bit word, left sample in the low half, and the
// storage is only ever read and written as whole words. On the ESP32 it can
// therefore live in the memory region that allows nothing but 32-bit
// access, which ordinary data cannot use; the data memory it would
// otherwise take is needed by the Bluetooth stack (docs/architecture/HARDWARE.md).
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
    // `storage` holds capacityFrames words; capacityFrames must be a power
    // of two.
    FrameRing(uint32_t* storage, uint32_t capacityFrames)
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
            _data[(head + i) & _mask] = static_cast<uint32_t>(static_cast<uint16_t>(frames[2 * i])) |
                                        (static_cast<uint32_t>(static_cast<uint16_t>(frames[2 * i + 1])) << 16);
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
            const uint32_t word = _data[(tail + i) & _mask];
            frames[2 * i] = static_cast<int16_t>(word & 0xFFFFu);
            frames[2 * i + 1] = static_cast<int16_t>(word >> 16);
        }
        _tail.store(tail + count, std::memory_order_release);
        return count;
    }

    // Reader side: throws away everything buffered.
    void discard() { _tail.store(_head.load(std::memory_order_acquire), std::memory_order_release); }

private:
    uint32_t* _data;
    uint32_t _mask;
    std::atomic<uint32_t> _head;  // written by the writer only
    std::atomic<uint32_t> _tail;  // written by the reader only
};

}  // namespace rc
