// Where does AVX2 quantity summation actually win?
// Sums n contiguous uint32 quantities, scalar vs AVX2, for growing n.
// Expect scalar to be as fast or faster for n = 1..3 (the common case of very
// few orders/levels), with AVX2 pulling ahead once n reaches a few vectors.
#include <chrono>
#include <cstdio>
#include <vector>
#include "book.hpp"

