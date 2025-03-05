#pragma once
// v1: textbook book. std::map of price -> std::list of orders.
// Profiling target: tree pointer-chasing dominates, not the matching logic.
#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <unordered_map>
#include "../common/types.hpp"

namespace v1 {
