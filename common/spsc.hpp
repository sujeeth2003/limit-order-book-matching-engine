#pragma once
// Lock-free single-producer/single-consumer ring. acquire/release only, no CAS.
#include <atomic>
#include <cstddef>
#include <memory>
#include "platform.hpp"

template <class T, size_t N>
class SpscRing {
  static_assert((N & (N - 1)) == 0, "N must be a power of two");
  alignas(64) std::atomic<size_t> head_{0};  // written by the consumer only
  alignas(64) std::atomic<size_t> tail_{0};  // written by the producer only
  alignas(64) std::unique_ptr<T[]> buf_ = std::make_unique<T[]>(N);

 public:
  bool push(const T& v) {
    size_t t = tail_.load(std::memory_order_relaxed);
    if (t - head_.load(std::memory_order_acquire) == N) return false;
    buf_[t & (N - 1)] = v;
    tail_.store(t + 1, std::memory_order_release);
    return true;
  }
  bool pop(T& v) {
    size_t h = head_.load(std::memory_order_relaxed);
    if (h == tail_.load(std::memory_order_acquire)) return false;
    v = buf_[h & (N - 1)];
    head_.store(h + 1, std::memory_order_release);
    return true;
  }
};
