#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

namespace echopair::audio {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324)
#endif

template <typename T>
class SpscRingBuffer final {
public:
    explicit SpscRingBuffer(std::size_t capacity)
        : storage_(capacity), capacity_(capacity)
    {
        if (capacity == 0) {
            throw std::invalid_argument("Ring buffer capacity must be positive");
        }
    }

    std::size_t write(std::span<const T> input)
    {
        const auto write_index = write_index_.load(std::memory_order_relaxed);
        const auto read_index = read_index_.load(std::memory_order_acquire);
        const auto queued = write_index - read_index;
        const auto count = std::min(input.size(), capacity_ - queued);

        for (std::size_t index = 0; index < count; ++index) {
            storage_[(write_index + index) % capacity_] = input[index];
        }
        write_index_.store(write_index + count, std::memory_order_release);
        return count;
    }

    std::size_t read(std::span<T> output)
    {
        const auto read_index = read_index_.load(std::memory_order_relaxed);
        const auto write_index = write_index_.load(std::memory_order_acquire);
        const auto count = std::min(output.size(), write_index - read_index);

        for (std::size_t index = 0; index < count; ++index) {
            output[index] = storage_[(read_index + index) % capacity_];
        }
        read_index_.store(read_index + count, std::memory_order_release);
        return count;
    }

    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

private:
    std::vector<T> storage_;
    const std::size_t capacity_;
    // Keep producer and consumer counters on separate cache lines.
    alignas(64) std::atomic<std::size_t> write_index_{0};
    alignas(64) std::atomic<std::size_t> read_index_{0};
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}
