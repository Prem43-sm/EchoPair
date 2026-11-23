#include <echopair/audio/spsc_ring_buffer.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <thread>

namespace {

bool test_wraparound()
{
    echopair::audio::SpscRingBuffer<int> ring(3);
    const std::array<int, 4> first{1, 2, 3, 4};
    std::array<int, 2> first_read{};
    const std::array<int, 2> second{4, 5};
    std::array<int, 3> second_read{};

    return ring.write(first) == 3 &&
           ring.read(first_read) == 2 &&
           first_read == std::array<int, 2>{1, 2} &&
           ring.write(second) == 2 &&
           ring.read(second_read) == 3 &&
           second_read == std::array<int, 3>{3, 4, 5};
}

bool test_concurrent_sequence()
{
    constexpr std::size_t item_count = 200'000;
    echopair::audio::SpscRingBuffer<std::uint32_t> ring(127);
    std::atomic<bool> sequence_valid{true};

    std::jthread producer([&] {
        std::array<std::uint32_t, 64> batch{};
        std::size_t next = 0;
        while (next < item_count) {
            const auto count = std::min(batch.size(), item_count - next);
            for (std::size_t index = 0; index < count; ++index) {
                batch[index] = static_cast<std::uint32_t>(next + index);
            }
            const auto written = ring.write(std::span<const std::uint32_t>(
                batch.data(), count));
            next += written;
            if (written == 0) {
                std::this_thread::yield();
            }
        }
    });

    std::jthread consumer([&] {
        std::array<std::uint32_t, 53> batch{};
        std::size_t next = 0;
        while (next < item_count) {
            const auto count = ring.read(batch);
            for (std::size_t index = 0; index < count; ++index) {
                if (batch[index] != next + index) {
                    sequence_valid.store(false, std::memory_order_relaxed);
                }
            }
            next += count;
            if (count == 0) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();
    return sequence_valid.load(std::memory_order_relaxed);
}

}

int main()
{
    if (!test_wraparound() || !test_concurrent_sequence()) {
        std::cerr << "SPSC ring buffer test failed.\n";
        return 1;
    }
    return 0;
}
