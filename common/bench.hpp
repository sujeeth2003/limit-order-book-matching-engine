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
void run_bench(const char* name, const std::vector<Op>& ops) {
  {
    Book b;
    uint64_t t0 = now_ns();
    for (const Op& o : ops) {
      if (o.cancel) (void)b.cancel(o.id); else b.add(o.id, o.side, o.px, o.qty);
    }
    uint64_t dt = now_ns() - t0;
    std::printf("%-20s %7.2f Mops/s  trades=%llu vol=%llu\n", name, ops.size() / (dt / 1e3),
                (unsigned long long)b.trades, (unsigned long long)b.volume);
  }
  {
    Book b;
    std::vector<uint32_t> lat;
    lat.reserve(ops.size());
    for (const Op& o : ops) {
      uint64_t t0 = now_ns();
      if (o.cancel) (void)b.cancel(o.id); else b.add(o.id, o.side, o.px, o.qty);
      lat.push_back((uint32_t)(now_ns() - t0));
    }
    Pct p = percentiles(lat);
    std::printf("%-20s p50=%uns p99=%uns p99.9=%uns max=%uns\n", "", p.p50, p.p99, p.p999, p.max);
  }
}
