# Limit Order Book + Matching Engine (C++20)

A price-time-priority limit order book, rebuilt six times. Each version changes **one idea** and is benchmarked and correctness-checked against the previous ones. The point of the project is the method: *measure, change one thing, measure again.*

| Version | Change | What it teaches |
|---|---|---|
| [v1_std_map](v1_std_map/book.hpp) | `std::map<price, std::list<Order>>` + `unordered_map` id index | Baseline. Profiling shows tree pointer-chasing, not matching, is the cost |
| [v2_flat_array](v2_flat_array/book.hpp) | Flat array indexed by price tick, preallocated order pool, index-based linked lists | Data layout beats clever code; far fewer cache misses |
| [v3_branchless](v3_branchless/book.hpp) | Side as runtime index, sign-multiply crossing test, cmov-friendly min / best updates, sentinel list end | Verify in assembly (`make asm`) and branch-miss counters (`make perf`) |
| [v4_avx2](v4_avx2/book.hpp) | Per-level aggregate quantity + AVX2 depth sum | SIMD only wins past a few vectors (see below) |
| [v5_single_writer](v5_single_writer/engine.hpp) | One matching thread owns the book; each client has its own SPSC ring | No locks, no CAS on the book |
| [v6_modern](v6_modern/book.hpp) | Templates for capacity / trade hook, RAII, move-only, `constexpr`, `[[nodiscard]]`, integer fixed-point prices | Clean code that keeps v3's speed |

Prices are integer ticks everywhere. Orders: limit add (matching against the opposite side first, remainder rests) and cancel by id.

## Build, test, benchmark
```bash
make test     # hand-written scenarios + randomized differential test (v1 is the reference)
make bench    # v1-v4, v6 single-thread, then v5 multi-client
make simd     # scalar vs AVX2 summation crossover
make asm      # cmov vs jcc count, v2 vs v3
make perf     # Linux: perf stat branch / cache counters
```
Needs a C++20 compiler (g++ or clang++) and x86-64 (uses `rdtsc`; v4 uses AVX2 and falls back to scalar at runtime if unavailable).

## Correctness
`tests/test_book.cpp` replays 300,000 random operations through v1, v2, v3, v4 and v6 and compares best bid, best ask, trade count and volume **after every operation**. All versions agree.

## Results
Measured on a 11th-gen Core i5-1135G7 laptop, **Windows 11, clang 21 (`-O2`), unpinned**, 1M ops, ~53% of adds cross the spread. Latency is per operation from a calibrated TSC clock (includes about 10 ns of timer cost). These are dev-machine numbers; run `make bench` on your hardware, ideally Linux with isolated cores, before quoting anything.

| Version | Throughput | p50 | p99 | p99.9 |
|---|---|---|---|---|
| v1 std::map | 6.8 Mops/s | 109 ns | 570 ns | 1770 ns |
| v2 flat array | 46.8 Mops/s | 20 ns | 102 ns | 285 ns |
| v3 branchless | 56.1 Mops/s | 15 ns | 99 ns | 160 ns |
| v4 AVX2 (+aggregate qty) | 48.1 Mops/s | 16 ns | 95 ns | 169 ns |
| v6 modern | 54.2 Mops/s | 14 ns | 96 ns | 231 ns |

