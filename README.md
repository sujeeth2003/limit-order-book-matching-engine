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

- `make asm` (clang 21, `-O2`): v2 has 27 conditional jumps / 1 `cmov`; v3 has 16 jumps / 7 `cmov`. The fill loop's exit condition is still a data-dependent branch.
- v1 -> v2 is the big win (**~7x**). v2 -> v3 is a modest gain and within run-to-run noise on some runs; v4 is slightly slower on this workload because it maintains an extra array on every fill and never calls `depth()` in the benchmark. I am reporting that rather than hiding it.
- **SIMD crossover** (`make simd`, ns per sum): n=1: 1.2 scalar vs 1.5 AVX2; n=8: 2.0 vs 1.6; n=64: 10.0 vs 5.2; n=1024: 157 vs 70. AVX2 does not help with 1-3 elements.
- **v5** on this box with 2 clients (unpinned, 3 us send gap): p50 112 ns queue-to-processed, but p99 ~0.5 ms from OS scheduling on a laptop that is also running other work. Pin cores and isolate them on Linux for a meaningful tail.

## Layout
```
common/   types, workload generator, calibrated clock, SPSC ring, thread pinning
v1..v6/   one directory per version
bench/    bench_all.cpp
tests/    test_book.cpp
```
Each version is a git tag (`v1` ... `v6`) with one commit per version, so the history reads as the optimisation log. The benchmark/test harness and this README arrive with the final commit (they need all versions).

## Limits
Fixed price range (`MAXP` ticks), fixed pool size, and ids must be dense integers below `MAXID` in the index-by-id versions. That is a deliberate trade for speed, not a production design.
