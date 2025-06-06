#pragma once
// Lock-free single-producer/single-consumer ring. acquire/release only, no CAS.
#include <atomic>
#include <cstddef>
#include <memory>
#include "platform.hpp"

template <class T, size_t N>
