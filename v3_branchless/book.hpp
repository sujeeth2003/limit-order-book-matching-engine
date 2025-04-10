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

struct Node { uint32_t prev, next; Qty qty; uint32_t id; Price px; uint32_t side; };
struct Level { uint32_t head = END, tail = END; };

struct Book {
  std::unique_ptr<Node[]>     pool = std::make_unique<Node[]>(POOL + 1);
  std::unique_ptr<uint32_t[]> slot = std::make_unique<uint32_t[]>(MAXID);
  std::unique_ptr<Level[]>    lv   = std::make_unique<Level[]>(2 * MAXP);
  uint32_t free_head = 0;
  Price best[2] = {NO_BID, NO_ASK};
  uint64_t trades = 0, volume = 0;

  Book() {
    for (uint32_t i = 0; i < POOL; ++i) pool[i].next = i + 1 < POOL ? i + 1 : END;
    for (uint32_t i = 0; i < MAXID; ++i) slot[i] = END;
  }
  Level& L(uint32_t s, Price p) { return lv[(size_t)s * MAXP + p]; }

