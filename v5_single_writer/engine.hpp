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
Result run(unsigned clients, size_t ops_per_client, uint64_t gap_ns, int first_cpu = -1) {
  std::vector<std::vector<Op>> work;
  std::vector<std::unique_ptr<Ring>> rings;
  for (unsigned c = 0; c < clients; ++c) {
    work.push_back(make_workload(ops_per_client, 100 + c, (OrderId)c << 20));
    rings.push_back(std::make_unique<Ring>());
  }
  std::atomic<bool> go{false};
  std::vector<std::thread> threads;
  for (unsigned c = 0; c < clients; ++c) {
    threads.emplace_back([&, c] {
      if (first_cpu >= 0) pin_thread(first_cpu + 1 + c);
      while (!go.load(std::memory_order_acquire)) cpu_relax();
      uint64_t next = now_ns();
      for (const Op& o : work[c]) {
        if (gap_ns) { while (now_ns() < next) cpu_relax(); next += gap_ns; }
        Msg m{o, now_ns(), 0};
        while (!rings[c]->push(m)) cpu_relax();
      }
      Msg m{}; m.stop = 1;
      while (!rings[c]->push(m)) cpu_relax();
    });
  }

  Result r;
  Book book;
  std::vector<uint32_t> lat;
  lat.reserve(clients * ops_per_client);
  if (first_cpu >= 0) pin_thread(first_cpu);
  go.store(true, std::memory_order_release);
  uint64_t t0 = now_ns();
  unsigned stopped = 0;
  Msg m;
  while (stopped < clients) {
    bool idle = true;
    for (unsigned c = 0; c < clients; ++c) {
      while (rings[c]->pop(m)) {
        idle = false;
        if (m.stop) { ++stopped; break; }
        if (m.op.cancel) (void)book.cancel(m.op.id);
        else book.add(m.op.id, m.op.side, m.op.px, m.op.qty);
        lat.push_back((uint32_t)(now_ns() - m.t_send_ns));
      }
    }
    if (idle) cpu_relax();
  }
  uint64_t dt = now_ns() - t0;
  for (auto& t : threads) t.join();
  r.processed = lat.size();
  r.mops = r.processed / (dt / 1e3);
  r.lat = percentiles(lat);
  r.trades = book.trades;
  r.volume = book.volume;
  return r;
}
}  // namespace v5
