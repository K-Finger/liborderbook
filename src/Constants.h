#pragma once
#include "Types.h"

namespace orderbook {

inline constexpr Price InvalidPrice{ -1 };

inline constexpr Price DefaultMinPrice{ -10'000 };
inline constexpr Price DefaultMaxPrice{ 100'000 };

}
