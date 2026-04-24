#pragma once

#include <cstdint>

/// Drives match/rest behavior. 1 byte for packing.
enum class OrderType : std::uint8_t
{
    /// Rests until filled or cancelled.
    GoodTillCancel,

    /// Crosses at any price. No rest.
    Market,

    /// All-or-nothing on entry, else reject. No rest.
    FillOrKill,

    /// Fill what's available now, cancel rest. No rest.
    ImmediateOrCancel,

    /// Rests intraday, auto-cancel at session close.
    GoodForDay,
};
