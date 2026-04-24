#pragma once

#include <chrono>
#include <cstdint>

// Strong typedefs. Tag parameter keeps aliases with the same underlying
// type incompatible (OrderId != ClientId even though both are uint64_t).

template <typename T, typename Tag>
struct StrongType
{
    T value;
    explicit constexpr StrongType(T v) noexcept : value(v) {}

    friend constexpr bool operator==(StrongType a, StrongType b) noexcept
    {
        return a.value == b.value;
    }

    friend constexpr auto operator<=>(StrongType a, StrongType b) noexcept
    {
        return a.value <=> b.value;
    }
};

struct OrderIdTag {};
struct ClientIdTag {};
struct TradeIdTag {};

struct PriceTag {};
struct QuantityTag {};
struct TimestampTag {};

using OrderId = StrongType<std::uint64_t, OrderIdTag>;    // one per submitted order ticket
using ClientId = StrongType<std::uint64_t, ClientIdTag>;   // the submitter; one client, many orders
using TradeId = StrongType<std::uint64_t, TradeIdTag>;    // one per executed fill; one order, many trades

using Price = StrongType<std::int64_t, PriceTag>;      // signed: InvalidPrice sentinel
using Quantity = StrongType<std::uint64_t, QuantityTag>;
using Timestamp = StrongType<std::chrono::nanoseconds, TimestampTag>;
