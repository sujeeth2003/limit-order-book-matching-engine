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
