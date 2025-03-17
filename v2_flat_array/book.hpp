#pragma once
// v2: flat array indexed by price tick + preallocated order pool with
// index-based doubly linked lists. No allocation and no tree walk on the hot
// path; all resting orders live in one contiguous pool.
#include <algorithm>
#include <memory>
#include "../common/types.hpp"

namespace v2 {
struct Node { uint32_t prev, next; Qty qty; uint32_t id; Price px; Side side; };
struct Level { uint32_t head = NIL, tail = NIL; };

struct Book {
  std::unique_ptr<Node[]>     pool = std::make_unique<Node[]>(POOL);
  std::unique_ptr<uint32_t[]> slot = std::make_unique<uint32_t[]>(MAXID);   // id -> pool index
  std::unique_ptr<Level[]>    lv   = std::make_unique<Level[]>(2 * MAXP);   // [side][price]
  uint32_t free_head = 0;
  Price best[2] = {NO_BID, NO_ASK};
  uint64_t trades = 0, volume = 0;

  Book() {
    for (uint32_t i = 0; i < POOL; ++i) pool[i].next = i + 1 < POOL ? i + 1 : NIL;
    std::fill(slot.get(), slot.get() + MAXID, NIL);
  }
  Level& L(Side s, Price p) { return lv[(size_t)s * MAXP + p]; }

