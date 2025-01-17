#pragma once
// Deterministic synthetic order flow: 70% limit orders around a fixed mid
// (about half cross the spread), 30% cancels of previously issued ids.
#include <cstddef>
#include <vector>
#include "types.hpp"

struct Op { uint8_t cancel; Side side; Price px; Qty qty; OrderId id; };

struct Rng {
  uint64_t s;
  explicit Rng(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ull + 1) {}
  uint64_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
};

inline std::vector<Op> make_workload(size_t n, uint64_t seed = 1, OrderId id_base = 0) {
  std::vector<Op> ops;
  ops.reserve(n);
  Rng r(seed);
  constexpr Price mid = MAXP / 2;
  OrderId issued = 0;
  for (size_t i = 0; i < n; ++i) {
    uint64_t x = r.next();
    if (issued > 0 && (x % 100) < 30) {
      ops.push_back({1, Buy, 0, 0, id_base + (r.next() % issued)});
    } else {
      Side s = (x >> 8) & 1 ? Sell : Buy;
      Price off = (Price)((x >> 16) % 40);
      Price px = s == Buy ? mid - 20 + off : mid + 20 - off;
      Qty q = 1 + (Qty)((x >> 32) % 100);
      ops.push_back({0, s, px, q, id_base + issued++});
    }
  }
  return ops;
}
