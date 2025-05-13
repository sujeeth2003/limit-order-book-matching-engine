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
