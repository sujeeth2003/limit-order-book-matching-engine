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

  void advance(uint32_t s) {  // s = book side whose best level emptied
    Price dir = (Price)(2 * s) - 1;             // Buy: -1, Sell: +1
    Price lim = s ? MAXP : -1;
    Price p = best[s];
    while (p != lim && L(s, p).head == END) p += dir;
    best[s] = p;
  }
  void unlink(Node& n, uint32_t i) {
    Level& l = L(n.side, n.px);
    uint32_t* fwd  = n.prev == END ? &l.head : &pool[n.prev].next;  // pointer select
    uint32_t* back = n.next == END ? &l.tail : &pool[n.next].prev;
    *fwd = n.next;
    *back = n.prev;
    slot[n.id] = END;
    n.next = free_head;
    free_head = i;
  }
  void add(OrderId id, Side side, Price px, Qty q) {
    const uint32_t s = side, o = s ^ 1u;
    const Price sgn = 1 - 2 * (Price)s;  // Buy +1, Sell -1
    while (q) {
      Price bp = best[o];
      if (sgn * (px - bp) < 0) break;  // opposite best does not cross
      Level& l = L(o, bp);
      while (q && l.head != END) {
        uint32_t cur = l.head;
        Node& n = pool[cur];
        Qty f = q < n.qty ? q : n.qty;
        q -= f; n.qty -= f; ++trades; volume += f;
        if (!n.qty) unlink(n, cur);
      }
      if (l.head == END) advance(o);
    }
    if (!q) return;
    uint32_t i = free_head;
    Node& n = pool[i];
    free_head = n.next;
    Level& l = L(s, px);
    n = {l.tail, END, q, (uint32_t)id, px, s};
    uint32_t* fwd = l.tail == END ? &l.head : &pool[l.tail].next;
    *fwd = i;
    l.tail = i;
    slot[id] = i;
    best[s] = sgn * (px - best[s]) > 0 ? px : best[s];
  }
  bool cancel(OrderId id) {
    uint32_t i = slot[id];
    if (i == END) return false;
    Node& n = pool[i];
    uint32_t s = n.side;
    Price px = n.px;
    unlink(n, i);
    if (L(s, px).head == END && best[s] == px) advance(s);
    return true;
  }
  Price best_bid() const { return best[Buy]; }
  Price best_ask() const { return best[Sell]; }
};
}  // namespace v3
