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

struct NoTradeHook {
  constexpr void operator()(OrderId /*maker*/, OrderId /*taker*/, Price, Qty) const noexcept {}
};

template <Price MaxPrice = MAXP, uint32_t PoolSize = POOL, uint32_t MaxIds = MAXID,
          class OnTrade = NoTradeHook>
class Book {
  static_assert(MaxPrice > 1 && PoolSize > 0 && MaxIds >= PoolSize);
  static constexpr uint32_t kEnd = PoolSize;  // dummy slot terminating every list
  static constexpr Price kNoBid = -1, kNoAsk = MaxPrice;

  struct Node { uint32_t prev, next; Qty qty; uint32_t id; Price px; uint32_t side; };
  struct Level { uint32_t head = kEnd, tail = kEnd; };

  std::unique_ptr<Node[]>     pool_ = std::make_unique<Node[]>(PoolSize + 1);
  std::unique_ptr<uint32_t[]> slot_ = std::make_unique<uint32_t[]>(MaxIds);
  std::unique_ptr<Level[]>    lv_   = std::make_unique<Level[]>(2 * static_cast<size_t>(MaxPrice));
  uint32_t free_head_ = 0;
  Price best_[2] = {kNoBid, kNoAsk};
  [[no_unique_address]] OnTrade on_trade_{};

