#pragma once
// v6: modern C++ cleanup of the v3 design.
//   * capacity and trade callback are template parameters (no globals, no virtuals)
//   * RAII storage (unique_ptr arrays) -> move-only, no manual new/delete
//   * strong-ish types, integer fixed-point prices (see FixedPoint)
//   * constexpr helpers, [[nodiscard]], noexcept on the hot path
//   * throws only in the constructor; the hot path never allocates or throws
#include <cstddef>
#include <memory>
#include <type_traits>
#include "../common/types.hpp"

namespace v6 {

// Prices are integers of the smallest tick; no floating point on the hot path.
// Convert from an external decimal representation exactly once, at the edge.
struct FixedPoint {
  static constexpr int64_t kScale = 10'000;  // 4 implied decimals on the wire
  [[nodiscard]] static constexpr Price to_ticks(int64_t px_e4, int64_t tick_e4) noexcept {
    return static_cast<Price>(px_e4 / tick_e4);
  }
  [[nodiscard]] static constexpr int64_t to_e4(Price ticks, int64_t tick_e4) noexcept {
    return static_cast<int64_t>(ticks) * tick_e4;
  }
};
static_assert(FixedPoint::to_ticks(1'234'500, 100) == 12'345);  // 123.4500 @ 0.01 tick

