#pragma once

#include <cstdint>

/// FIX standard for qualifiers. 1 byte for packing.
enum class OrderType : std::uint8_t
{
    GoodTillCancel,
    Market,
    FillOrKill,
    ImmediateOrCancel,
    GoodForDay,
};
