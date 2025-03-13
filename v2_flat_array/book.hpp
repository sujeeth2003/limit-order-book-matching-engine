#pragma once
// v2: flat array indexed by price tick + preallocated order pool with
// index-based doubly linked lists. No allocation and no tree walk on the hot
// path; all resting orders live in one contiguous pool.
#include <algorithm>
#include <memory>
#include "../common/types.hpp"

namespace v2 {
