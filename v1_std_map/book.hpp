#pragma once
// v1: textbook book. std::map of price -> std::list of orders.
// Profiling target: tree pointer-chasing dominates, not the matching logic.
#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <unordered_map>
#include "../common/types.hpp"

namespace v1 {
struct Order { OrderId id; Qty qty; Side side; Price px; };
using Level = std::list<Order>;

struct Book {
  std::map<Price, Level, std::greater<Price>> bids;
  std::map<Price, Level> asks;
  std::unordered_map<OrderId, Level::iterator> idx;
  uint64_t trades = 0, volume = 0;

  template <class Opp, class Crosses>
  void sweep(Opp& opp, Price px, Qty& q, Crosses crosses) {
    while (q && !opp.empty()) {
      auto lv = opp.begin();
      if (!crosses(lv->first, px)) break;
      Level& l = lv->second;
      while (q && !l.empty()) {
        Order& o = l.front();
        Qty f = std::min(q, o.qty);
        q -= f; o.qty -= f; ++trades; volume += f;
        if (!o.qty) { idx.erase(o.id); l.pop_front(); }
      }
      if (l.empty()) opp.erase(lv);
    }
  }
  void add(OrderId id, Side s, Price px, Qty q) {
    if (s == Buy) sweep(asks, px, q, [](Price a, Price p) { return a <= p; });
    else          sweep(bids, px, q, [](Price b, Price p) { return b >= p; });
    if (!q) return;
    Level& l = s == Buy ? bids[px] : asks[px];
    l.push_back({id, q, s, px});
    idx[id] = std::prev(l.end());
  }
  bool cancel(OrderId id) {
    auto it = idx.find(id);
    if (it == idx.end()) return false;
    Order o = *it->second;
    if (o.side == Buy) {
      auto l = bids.find(o.px);
      l->second.erase(it->second);
      if (l->second.empty()) bids.erase(l);
    } else {
      auto l = asks.find(o.px);
      l->second.erase(it->second);
      if (l->second.empty()) asks.erase(l);
    }
    idx.erase(it);
    return true;
  }
  Price best_bid() const { return bids.empty() ? NO_BID : bids.begin()->first; }
  Price best_ask() const { return asks.empty() ? NO_ASK : asks.begin()->first; }
};
}  // namespace v1
