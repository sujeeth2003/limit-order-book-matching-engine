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

