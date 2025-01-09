#pragma once
#include <cstdint>

using OrderId = uint64_t;
using Price   = int32_t;   // integer ticks, never floating point
using Qty     = uint32_t;

enum Side : uint8_t { Buy = 0, Sell = 1 };

constexpr Price    MAXP   = 4096;      // price ticks 0..MAXP-1 (flat-array versions)
constexpr Price    NO_BID = -1;        // best_bid() when the bid side is empty
constexpr Price    NO_ASK = MAXP;      // best_ask() when the ask side is empty
constexpr uint32_t MAXID  = 1u << 22;  // id space for index-by-id versions
constexpr uint32_t POOL   = 1u << 20;  // max simultaneously resting orders
constexpr uint32_t NIL    = 0xFFFFFFFFu;
