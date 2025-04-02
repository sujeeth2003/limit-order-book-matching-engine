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

  void advance(Side s) {  // best level emptied: walk to the next non-empty one
    if (s == Buy) { Price b = best[Buy];  while (b >= 0 && L(Buy, b).head == NIL) --b;  best[Buy] = b; }
    else          { Price a = best[Sell]; while (a < MAXP && L(Sell, a).head == NIL) ++a; best[Sell] = a; }
  }
  void unlink(Node& n, uint32_t i) {
    Level& l = L(n.side, n.px);
    if (n.prev != NIL) pool[n.prev].next = n.next; else l.head = n.next;
    if (n.next != NIL) pool[n.next].prev = n.prev; else l.tail = n.prev;
    slot[n.id] = NIL;
    n.next = free_head;
    free_head = i;
  }
  template <Side S> void add_impl(OrderId id, Price px, Qty q) {
    constexpr Side O = S == Buy ? Sell : Buy;
    while (q) {
      Price bp = best[O];
      if constexpr (S == Buy) { if (bp > px) break; } else { if (bp < px) break; }
      Level& l = L(O, bp);
      while (q && l.head != NIL) {
        Node& n = pool[l.head];
        Qty f = std::min(q, n.qty);
        q -= f; n.qty -= f; ++trades; volume += f;
        if (!n.qty) unlink(n, l.head);
      }
      if (l.head == NIL) advance(O);
    }
    if (!q) return;
    uint32_t i = free_head;
    Node& n = pool[i];
    free_head = n.next;
    Level& l = L(S, px);
    n = {l.tail, NIL, q, (uint32_t)id, px, S};
    if (l.tail != NIL) pool[l.tail].next = i; else l.head = i;
    l.tail = i;
    slot[id] = i;
    if constexpr (S == Buy) { if (px > best[Buy]) best[Buy] = px; }
    else                    { if (px < best[Sell]) best[Sell] = px; }
  }
  void add(OrderId id, Side s, Price px, Qty q) {
    if (s == Buy) add_impl<Buy>(id, px, q); else add_impl<Sell>(id, px, q);
  }
  bool cancel(OrderId id) {
    uint32_t i = slot[id];
    if (i == NIL) return false;
    Node& n = pool[i];
    Side s = n.side;
    Price px = n.px;
    unlink(n, i);
    if (L(s, px).head == NIL && best[s] == px) advance(s);
    return true;
  }
  Price best_bid() const { return best[Buy]; }
  Price best_ask() const { return best[Sell]; }
};
}  // namespace v2
