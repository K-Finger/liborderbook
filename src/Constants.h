#pragma once
#include <cstddef>

#include "Types.h"

namespace orderbook {

inline constexpr std::size_t DefaultOrderCapacity = 1'000'000;

inline constexpr Price InvalidPrice{ -1 };

inline constexpr Price DefaultMinPrice{ 1 };
inline constexpr Price DefaultMaxPrice{ 100'000 };

}
