#pragma once
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>
#include "workload.hpp"

// Calibrated TSC clock (~10 ns resolution). std::chrono::steady_clock has
// 100 ns granularity on Windows, which hides sub-100 ns operations entirely.
// Assumes an invariant TSC synchronized across cores (true on current x86).
