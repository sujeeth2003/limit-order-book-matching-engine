// Where does AVX2 quantity summation actually win?
// Sums n contiguous uint32 quantities, scalar vs AVX2, for growing n.
// Expect scalar to be as fast or faster for n = 1..3 (the common case of very
// few orders/levels), with AVX2 pulling ahead once n reaches a few vectors.
#include <chrono>
#include <cstdio>
#include <vector>
#include "book.hpp"

int main() {
  if (!v4::has_avx2()) { std::puts("AVX2 not available on this CPU"); return 0; }
  std::vector<uint32_t> a(4096);
  for (size_t i = 0; i < a.size(); ++i) a[i] = (uint32_t)(i % 97 + 1);
  std::printf("%6s %12s %12s   (ns per call, lower is better)\n", "n", "scalar", "avx2");
  for (int n : {1, 2, 3, 4, 8, 16, 32, 64, 256, 1024}) {
    const int reps = 2000000;
    volatile uint64_t sink = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) sink = sink + v4::sum_scalar(a.data() + (r & 7), n);
    auto t1 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) sink = sink + v4::sum_avx2(a.data() + (r & 7), n);
    auto t2 = std::chrono::steady_clock::now();
    auto ns = [&](auto x, auto y) { return std::chrono::duration<double, std::nano>(y - x).count() / reps; };
    std::printf("%6d %12.2f %12.2f\n", n, ns(t0, t1), ns(t1, t2));
  }
}
