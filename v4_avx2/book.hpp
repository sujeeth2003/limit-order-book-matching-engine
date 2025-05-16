#pragma once
// v4: v3 plus a per-level aggregate quantity array (lvq) so depth queries
// ("total size in the top N levels") are a sum over contiguous memory, which
// AVX2 can accelerate. Whether SIMD wins depends on N: see simd_bench.cpp.
// A single level with only 1-3 resting orders gains nothing from SIMD.
#include <cpuid.h>
#include <immintrin.h>
#include <memory>
#include "../common/types.hpp"

namespace v4 {
constexpr uint32_t END = POOL;

struct Node { uint32_t prev, next; Qty qty; uint32_t id; Price px; uint32_t side; };
struct Level { uint32_t head = END, tail = END; };

inline uint64_t sum_scalar(const uint32_t* a, int n) {
  uint64_t t = 0;
#if defined(__clang__)
  #pragma clang loop vectorize(disable) interleave(disable)
#endif
  for (int i = 0; i < n; ++i) t += a[i];
  return t;
}

__attribute__((target("avx2")))
inline uint64_t sum_avx2(const uint32_t* a, int n) {
  __m256i acc = _mm256_setzero_si256();
  int i = 0;
  for (; i + 8 <= n; i += 8) {  // widen to 64-bit lanes so large books cannot overflow
    __m256i v = _mm256_loadu_si256((const __m256i*)(a + i));
    acc = _mm256_add_epi64(acc, _mm256_cvtepu32_epi64(_mm256_castsi256_si128(v)));
    acc = _mm256_add_epi64(acc, _mm256_cvtepu32_epi64(_mm256_extracti128_si256(v, 1)));
  }
  alignas(32) uint64_t lanes[4];
  _mm256_store_si256((__m256i*)lanes, acc);
  uint64_t t = lanes[0] + lanes[1] + lanes[2] + lanes[3];
  for (; i < n; ++i) t += a[i];
  return t;
}

// CPUID + XGETBV instead of __builtin_cpu_supports so this links without libgcc/compiler-rt.
inline bool detect_avx2() {
  unsigned a, b, c, d;
  if (!__get_cpuid(1, &a, &b, &c, &d)) return false;
  if (!(c & (1u << 27)) || !(c & (1u << 28))) return false;  // OSXSAVE + AVX
  unsigned lo, hi;
  __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
  if ((lo & 6) != 6) return false;                            // OS saves XMM+YMM
  if (!__get_cpuid_count(7, 0, &a, &b, &c, &d)) return false;
  return b & (1u << 5);                                       // AVX2
}
inline bool has_avx2() { static const bool ok = detect_avx2(); return ok; }

