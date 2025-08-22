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

