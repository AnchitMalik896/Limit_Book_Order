#pragma once
#include <atomic>
#include <array>
#include <bit>
#include <cstddef>

template<typename T, std::size_t N>
class SPSCQueue {
    static_assert(std::has_single_bit(N), "N must be a power of two");
    static constexpr std::size_t MASK = N - 1;

    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    alignas(64) std::array<T, N>         buf_;

public:
    [[nodiscard]] bool push(T val) noexcept {
        const auto t    = tail_.load(std::memory_order_relaxed);
        const auto next = (t + 1) & MASK;
        if (next == head_.load(std::memory_order_acquire)) return false;
        buf_[t] = std::move(val);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T& out) noexcept {
        const auto h = head_.load(std::memory_order_relaxed);
        if (h == tail_.load(std::memory_order_acquire)) return false;
        out = std::move(buf_[h]);
        head_.store((h + 1) & MASK, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool   empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }
    [[nodiscard]] size_t size()  const noexcept {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_acquire);
        return (t - h) & MASK;
    }
};
