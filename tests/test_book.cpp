// Correctness: hand-written scenarios + a randomized differential test that
// replays the same order flow through every version and compares best bid/ask,
// trade count and traded volume after every single operation. v1 (std::map)
// is the reference.
#include <cstdio>
#include <cstdlib>
#include "../common/workload.hpp"
#include "../v1_std_map/book.hpp"
#include "../v2_flat_array/book.hpp"
#include "../v3_branchless/book.hpp"
#include "../v4_avx2/book.hpp"
#include "../v6_modern/book.hpp"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

template <class B> void scenarios(const char* name) {
  { // price-time priority: earlier order at the same price fills first
    B b; b.add(1, Sell, 100, 5); b.add(2, Sell, 100, 5); b.add(3, Buy, 100, 6);
    CHECK(b.trades == 2 && b.volume == 6);
    CHECK(b.best_ask() == 100 && !b.cancel(1) && b.cancel(2));
    CHECK(b.best_ask() == NO_ASK);
  }
  { // sweep several levels, remainder rests
    B b; b.add(1, Sell, 101, 3); b.add(2, Sell, 102, 3); b.add(3, Buy, 105, 10);
    CHECK(b.trades == 2 && b.volume == 6);
    CHECK(b.best_bid() == 105 && b.best_ask() == NO_ASK);
  }
  { // no cross: both sides rest, spread visible
    B b; b.add(1, Buy, 99, 1); b.add(2, Sell, 101, 1);
    CHECK(b.best_bid() == 99 && b.best_ask() == 101 && b.trades == 0);
    CHECK(b.cancel(1) && b.best_bid() == NO_BID);
  }
  { // best level advances past emptied levels after a cancel
    B b; b.add(1, Buy, 100, 1); b.add(2, Buy, 98, 1); b.add(3, Buy, 90, 1);
    CHECK(b.cancel(1) && b.best_bid() == 98);
    CHECK(b.cancel(2) && b.best_bid() == 90);
  }
  std::printf("scenarios ok: %s\n", name);
}

template <class A, class B> void same(A& a, B& b, size_t i) {
  if (a.best_bid() != b.best_bid() || a.best_ask() != b.best_ask() || a.trades != b.trades ||
      a.volume != b.volume) {
    std::printf("DIVERGED at op %zu\n", i);
    ++failures;
  }
}

int main() {
  scenarios<v1::Book>("v1"); scenarios<v2::Book>("v2"); scenarios<v3::Book>("v3");
  scenarios<v4::Book>("v4"); scenarios<v6::DefaultBook>("v6");

  auto ops = make_workload(300000, 7);
  v1::Book r; v2::Book b2; v3::Book b3; v4::Book b4; v6::DefaultBook b6;
  for (size_t i = 0; i < ops.size() && !failures; ++i) {
    const Op& o = ops[i];
    if (o.cancel) {
      bool x = r.cancel(o.id);
      CHECK(b2.cancel(o.id) == x); CHECK(b3.cancel(o.id) == x);
      CHECK(b4.cancel(o.id) == x); CHECK(b6.cancel(o.id) == x);
    } else {
      r.add(o.id, o.side, o.px, o.qty); b2.add(o.id, o.side, o.px, o.qty); b3.add(o.id, o.side, o.px, o.qty);
      b4.add(o.id, o.side, o.px, o.qty); b6.add(o.id, o.side, o.px, o.qty);
    }
    same(r, b2, i); same(r, b3, i); same(r, b4, i); same(r, b6, i);
  }
  // v4 aggregate quantity: SIMD and scalar depth must agree
  if (v4::has_avx2()) for (int n : {1, 3, 8, 20, 100}) {
    CHECK(b4.depth<true>(Buy, n) == b4.depth<false>(Buy, n));
    CHECK(b4.depth<true>(Sell, n) == b4.depth<false>(Sell, n));
  }
  std::printf(failures ? "FAILED (%d)\n" : "differential test ok: 300000 ops x 5 versions\n", failures);
  return failures ? 1 : 0;
}
