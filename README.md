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

