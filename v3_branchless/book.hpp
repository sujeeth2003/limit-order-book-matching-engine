#pragma once
// v3: branch-free hot path. Same layout as v2, but
//   * side is a runtime index (no per-side templates / if-else on side)
//   * "does it cross?" is one multiply-compare using a +1/-1 sign
//   * min(), best-price updates and list-end handling are selects (cmov)
//     because the list terminator is a real dummy slot (END) instead of NIL,
//     so unlinking can write unconditionally.
// The fill loop's exit condition is still data dependent; check the generated
// assembly with scripts/asm_check.sh and the branch-miss counters with perf.
#include <memory>
#include "../common/types.hpp"

namespace v3 {
constexpr uint32_t END = POOL;  // dummy pool slot used as list terminator

