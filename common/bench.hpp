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
