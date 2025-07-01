#pragma once
// v5: single-writer design. Exactly one matching thread owns the book, so the
// book needs no locks and no atomics. Each client thread has its OWN SPSC ring
// into the engine (no shared MPSC queue, no CAS, no ABA). The engine busy-polls
// the rings round-robin.
#include <atomic>
#include <memory>
#include <vector>
#include "../common/bench.hpp"
#include "../common/platform.hpp"
#include "../common/spsc.hpp"

namespace v5 {
struct Msg { Op op; uint64_t t_send_ns; uint8_t stop; };
using Ring = SpscRing<Msg, 1u << 16>;

struct Result {
  double mops = 0;
  Pct lat{};
  uint64_t trades = 0, volume = 0, processed = 0;
};

// gap_ns = spacing between sends per client (0 = as fast as the ring accepts).
template <class Book>
