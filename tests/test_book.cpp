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

