#pragma once
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>
#include "workload.hpp"

// Calibrated TSC clock (~10 ns resolution). std::chrono::steady_clock has
// 100 ns granularity on Windows, which hides sub-100 ns operations entirely.
// Assumes an invariant TSC synchronized across cores (true on current x86).
struct TscClock {
  double ns_per_tick;
  uint64_t base_tsc, base_ns;
  static uint64_t steady() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
  }
  TscClock() {
    uint64_t s0 = steady(), t0 = __builtin_ia32_rdtsc();
    while (steady() - s0 < 50'000'000) {}  // 50 ms calibration
    uint64_t s1 = steady(), t1 = __builtin_ia32_rdtsc();
    ns_per_tick = double(s1 - s0) / double(t1 - t0);
    base_tsc = t1; base_ns = s1;
  }
};
inline uint64_t now_ns() {
  static const TscClock c;
  return c.base_ns + (uint64_t)((double)(__builtin_ia32_rdtsc() - c.base_tsc) * c.ns_per_tick);
}

struct Pct { uint32_t p50, p99, p999, max; };
inline Pct percentiles(std::vector<uint32_t>& v) {
  std::sort(v.begin(), v.end());
  auto at = [&](double q) { return v[std::min(v.size() - 1, (size_t)(q * v.size()))]; };
  return {at(0.50), at(0.99), at(0.999), v.back()};
}

// Runs the workload twice: once without per-op timing (throughput), once with
// per-op timestamps (latency; includes ~20-30 ns of clock overhead per op).
template <class Book>
