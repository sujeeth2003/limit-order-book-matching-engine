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

