// Single-threaded comparison of v1..v4 and v6, then the v5 multi-client engine.
// usage: bench_all [ops=1000000] [clients=2] [gap_ns=0]
#include <cstdio>
#include <cstdlib>
#include "../common/bench.hpp"
#include "../v1_std_map/book.hpp"
#include "../v2_flat_array/book.hpp"
#include "../v3_branchless/book.hpp"
#include "../v4_avx2/book.hpp"
#include "../v5_single_writer/engine.hpp"
#include "../v6_modern/book.hpp"

int main(int argc, char** argv) {
  size_t n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 1000000;
  unsigned clients = argc > 2 ? (unsigned)std::atoi(argv[2]) : 2;
  uint64_t gap = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 0;
  pin_thread(0);
  auto ops = make_workload(n);
  std::printf("== single thread, %zu ops ==\n", n);
  run_bench<v1::Book>("v1 std::map", ops);
  run_bench<v2::Book>("v2 flat array", ops);
  run_bench<v3::Book>("v3 branchless", ops);
  run_bench<v4::Book>("v4 avx2 (+lvq)", ops);
  run_bench<v6::DefaultBook>("v6 modern", ops);
  std::printf("== v5 single writer, %u clients, gap %llu ns ==\n", clients, (unsigned long long)gap);
  auto r = v5::run<v6::DefaultBook>(clients, n / clients, gap, 0);
  std::printf("v5 engine            %7.2f Mops/s  p50=%uns p99=%uns p99.9=%uns max=%uns (send->processed)\n",
              r.mops, r.lat.p50, r.lat.p99, r.lat.p999, r.lat.max);
}
