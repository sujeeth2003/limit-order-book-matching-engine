#pragma once
// v4: v3 plus a per-level aggregate quantity array (lvq) so depth queries
// ("total size in the top N levels") are a sum over contiguous memory, which
// AVX2 can accelerate. Whether SIMD wins depends on N: see simd_bench.cpp.
// A single level with only 1-3 resting orders gains nothing from SIMD.
#include <cpuid.h>
#include <immintrin.h>
#include <memory>
#include "../common/types.hpp"

namespace v4 {
constexpr uint32_t END = POOL;

struct Node { uint32_t prev, next; Qty qty; uint32_t id; Price px; uint32_t side; };
struct Level { uint32_t head = END, tail = END; };

inline uint64_t sum_scalar(const uint32_t* a, int n) {
  uint64_t t = 0;
#if defined(__clang__)
  #pragma clang loop vectorize(disable) interleave(disable)
#endif
  for (int i = 0; i < n; ++i) t += a[i];
  return t;
}

__attribute__((target("avx2")))
inline uint64_t sum_avx2(const uint32_t* a, int n) {
  __m256i acc = _mm256_setzero_si256();
  int i = 0;
  for (; i + 8 <= n; i += 8) {  // widen to 64-bit lanes so large books cannot overflow
    __m256i v = _mm256_loadu_si256((const __m256i*)(a + i));
    acc = _mm256_add_epi64(acc, _mm256_cvtepu32_epi64(_mm256_castsi256_si128(v)));
    acc = _mm256_add_epi64(acc, _mm256_cvtepu32_epi64(_mm256_extracti128_si256(v, 1)));
  }
  alignas(32) uint64_t lanes[4];
  _mm256_store_si256((__m256i*)lanes, acc);
  uint64_t t = lanes[0] + lanes[1] + lanes[2] + lanes[3];
  for (; i < n; ++i) t += a[i];
  return t;
}

// CPUID + XGETBV instead of __builtin_cpu_supports so this links without libgcc/compiler-rt.
inline bool detect_avx2() {
  unsigned a, b, c, d;
  if (!__get_cpuid(1, &a, &b, &c, &d)) return false;
  if (!(c & (1u << 27)) || !(c & (1u << 28))) return false;  // OSXSAVE + AVX
  unsigned lo, hi;
  __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
  if ((lo & 6) != 6) return false;                            // OS saves XMM+YMM
  if (!__get_cpuid_count(7, 0, &a, &b, &c, &d)) return false;
  return b & (1u << 5);                                       // AVX2
}
inline bool has_avx2() { static const bool ok = detect_avx2(); return ok; }

struct Book {
  std::unique_ptr<Node[]>     pool = std::make_unique<Node[]>(POOL + 1);
  std::unique_ptr<uint32_t[]> slot = std::make_unique<uint32_t[]>(MAXID);
  std::unique_ptr<Level[]>    lv   = std::make_unique<Level[]>(2 * MAXP);
  std::unique_ptr<uint32_t[]> lvq  = std::make_unique<uint32_t[]>(2 * MAXP);  // aggregate qty per level
  uint32_t free_head = 0;
  Price best[2] = {NO_BID, NO_ASK};
  uint64_t trades = 0, volume = 0;

  Book() {
    for (uint32_t i = 0; i < POOL; ++i) pool[i].next = i + 1 < POOL ? i + 1 : END;
    for (uint32_t i = 0; i < MAXID; ++i) slot[i] = END;
  }
  Level& L(uint32_t s, Price p) { return lv[(size_t)s * MAXP + p]; }
  uint32_t& Q(uint32_t s, Price p) { return lvq[(size_t)s * MAXP + p]; }

  void advance(uint32_t s) {
    Price dir = (Price)(2 * s) - 1;
    Price lim = s ? MAXP : -1;
    Price p = best[s];
    while (p != lim && L(s, p).head == END) p += dir;
    best[s] = p;
  }
  void unlink(Node& n, uint32_t i) {
    Level& l = L(n.side, n.px);
    uint32_t* fwd  = n.prev == END ? &l.head : &pool[n.prev].next;
    uint32_t* back = n.next == END ? &l.tail : &pool[n.next].prev;
    *fwd = n.next;
    *back = n.prev;
    slot[n.id] = END;
    n.next = free_head;
    free_head = i;
  }
  void add(OrderId id, Side side, Price px, Qty q) {
    const uint32_t s = side, o = s ^ 1u;
    const Price sgn = 1 - 2 * (Price)s;
    while (q) {
      Price bp = best[o];
      if (sgn * (px - bp) < 0) break;
      Level& l = L(o, bp);
      while (q && l.head != END) {
        uint32_t cur = l.head;
        Node& n = pool[cur];
        Qty f = q < n.qty ? q : n.qty;
        q -= f; n.qty -= f; ++trades; volume += f;
        Q(o, bp) -= f;
        if (!n.qty) unlink(n, cur);
      }
      if (l.head == END) advance(o);
    }
    if (!q) return;
    uint32_t i = free_head;
    Node& n = pool[i];
    free_head = n.next;
    Level& l = L(s, px);
    n = {l.tail, END, q, (uint32_t)id, px, s};
    uint32_t* fwd = l.tail == END ? &l.head : &pool[l.tail].next;
    *fwd = i;
    l.tail = i;
    slot[id] = i;
    Q(s, px) += q;
    best[s] = sgn * (px - best[s]) > 0 ? px : best[s];
  }
  bool cancel(OrderId id) {
    uint32_t i = slot[id];
    if (i == END) return false;
    Node& n = pool[i];
    uint32_t s = n.side;
    Price px = n.px;
    Q(s, px) -= n.qty;
    unlink(n, i);
    if (L(s, px).head == END && best[s] == px) advance(s);
    return true;
  }
  Price best_bid() const { return best[Buy]; }
  Price best_ask() const { return best[Sell]; }

  // Total resting quantity in the n price levels nearest the touch on `side`.
  template <bool Simd>
  uint64_t depth(Side side, int n) const {
    Price b = best[side];
    if (b == NO_BID || b == NO_ASK) return 0;
    const uint32_t* base = lvq.get() + (size_t)side * MAXP;
    int lo = side == Buy ? (b - n + 1 < 0 ? 0 : b - n + 1) : b;
    int hi = side == Buy ? b + 1 : (b + n > MAXP ? MAXP : b + n);
    return Simd ? sum_avx2(base + lo, hi - lo) : sum_scalar(base + lo, hi - lo);
  }
};
}  // namespace v4
