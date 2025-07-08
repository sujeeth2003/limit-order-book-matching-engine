#pragma once
// v6: modern C++ cleanup of the v3 design.
//   * capacity and trade callback are template parameters (no globals, no virtuals)
//   * RAII storage (unique_ptr arrays) -> move-only, no manual new/delete
//   * strong-ish types, integer fixed-point prices (see FixedPoint)
//   * constexpr helpers, [[nodiscard]], noexcept on the hot path
//   * throws only in the constructor; the hot path never allocates or throws
#include <cstddef>
#include <memory>
#include <type_traits>
#include "../common/types.hpp"

namespace v6 {

