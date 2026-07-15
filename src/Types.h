#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <functional>

namespace orderbook {

struct OrderId
{
    std::uint64_t v{};

    constexpr OrderId() = default;
    explicit constexpr OrderId(std::uint64_t val) : v(val) {}

    constexpr std::uint64_t get() const noexcept { return v; }
    auto operator<=>(OrderId const&) const = default;
};

struct ClientId
{
    std::uint64_t v{};

    constexpr ClientId() = default;
    explicit constexpr ClientId(std::uint64_t val) : v(val) {}

    constexpr std::uint64_t get() const noexcept { return v; }
    auto operator<=>(ClientId const&) const = default;
};

struct TradeId
{
    std::uint64_t v{};

    constexpr TradeId() = default;
    explicit constexpr TradeId(std::uint64_t val) : v(val) {}

    constexpr std::uint64_t get() const noexcept { return v; }
    auto operator<=>(TradeId const&) const = default;

    constexpr TradeId& operator++() noexcept { ++v; return *this; }
    constexpr TradeId operator++(int) noexcept { auto tmp = *this; ++v; return tmp; }
};

struct Price
{
    std::int64_t v{};

    constexpr Price() = default;
    explicit constexpr Price(std::int64_t val) : v(val) {}

    constexpr std::int64_t get() const noexcept { return v; }
    auto operator<=>(Price const&) const = default;
};

struct Quantity
{
    std::uint64_t v{};

    constexpr Quantity() = default;
    explicit constexpr Quantity(std::uint64_t val) : v(val) {}

    constexpr std::uint64_t get() const noexcept { return v; }
    auto operator<=>(Quantity const&) const = default;

    constexpr Quantity operator+(Quantity o) const noexcept { return Quantity{ v + o.v }; }
    constexpr Quantity operator-(Quantity o) const noexcept { return Quantity{ v - o.v }; }
    constexpr Quantity& operator+=(Quantity o) noexcept { v += o.v; return *this; }
    constexpr Quantity& operator-=(Quantity o) noexcept { v -= o.v; return *this; }
};

struct Timestamp
{
    std::chrono::nanoseconds v{};

    constexpr Timestamp() = default;
    explicit constexpr Timestamp(std::chrono::nanoseconds val) : v(val) {}

    constexpr std::chrono::nanoseconds get() const noexcept { return v; }
    auto operator<=>(Timestamp const&) const = default;
};

}

template<>
struct std::hash<orderbook::OrderId>
{
    std::size_t operator()(orderbook::OrderId const& id) const noexcept
    {
        return std::hash<std::uint64_t>{}(id.v);
    }
};

template<>
struct std::hash<orderbook::ClientId>
{
    std::size_t operator()(orderbook::ClientId const& id) const noexcept
    {
        return std::hash<std::uint64_t>{}(id.v);
    }
};

template<>
struct std::hash<orderbook::TradeId>
{
    std::size_t operator()(orderbook::TradeId const& id) const noexcept
    {
        return std::hash<std::uint64_t>{}(id.v);
    }
};

template<>
struct std::hash<orderbook::Price>
{
    std::size_t operator()(orderbook::Price const& p) const noexcept
    {
        return std::hash<std::int64_t>{}(p.v);
    }
};

template<>
struct std::hash<orderbook::Quantity>
{
    std::size_t operator()(orderbook::Quantity const& q) const noexcept
    {
        return std::hash<std::uint64_t>{}(q.v);
    }
};
