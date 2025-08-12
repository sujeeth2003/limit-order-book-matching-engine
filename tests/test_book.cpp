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

