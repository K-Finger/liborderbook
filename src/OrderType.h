#pragma once
#include <cstdint>

namespace orderbook {

enum class OrderType : std::uint8_t
{
    GoodTillCancel,
    Market,
    FillOrKill,
    ImmediateOrCancel,
    GoodForDay,
};

}
