#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <functional>

// CRTP helper: skills reach into the concrete type without repeating the cast.
// https://youtu.be/fWcnp7Bulc8

template <typename Strong, template<typename> class Skill>
struct crtp
{
    constexpr Strong& underlying()       noexcept { return static_cast<Strong&>(*this); }
    constexpr Strong const& underlying() const noexcept { return static_cast<Strong const&>(*this); }
};

// Skills are opt-in per alias so nonsense ops (OrderId + OrderId) don't compile.

template <typename Strong>
struct Addable : crtp<Strong, Addable>
{
    constexpr Strong operator+(Strong const& other) const noexcept
    {
        return Strong{ this->underlying().get() + other.get() };
    }

    constexpr Strong& operator+=(Strong const& other) noexcept
    {
        this->underlying().get() += other.get(); return this->underlying();
    }
};

template <typename Strong>
struct Subtractable : crtp<Strong, Subtractable>
{
    constexpr Strong operator-(Strong const& other) const noexcept
    {
        return Strong{ this->underlying().get() - other.get() };
    }

    constexpr Strong& operator-=(Strong const& other) noexcept
    {
        this->underlying().get() -= other.get(); return this->underlying();
    }
};

template <typename Strong>
struct Incrementable : crtp<Strong, Incrementable>
{
    constexpr Strong& operator++() noexcept
    {
        ++this->underlying().get(); return this->underlying();
    }

    constexpr Strong operator++(int) noexcept
    {
        Strong tmp = this->underlying();
        ++this->underlying().get();
        return tmp;
    }
};

// Tag kills cross-alias assignment.

template <typename T, typename Tag, template<typename> class... Skills>
struct StrongType : Skills<StrongType<T, Tag, Skills...>>...
{
    constexpr StrongType() noexcept : value_{} {}
    explicit constexpr StrongType(T v) noexcept : value_(v) {}

    constexpr T& get()       noexcept { return value_; }
    constexpr T const& get() const noexcept { return value_; }

    friend constexpr bool operator==(StrongType a, StrongType b) noexcept
    {
        return a.value_ == b.value_;
    }

    friend constexpr auto operator<=>(StrongType a, StrongType b) noexcept
    {
        return a.value_ <=> b.value_;
    }

private:
    T value_;
};

// -----------------------------------------

struct OrderIdTag {};
struct ClientIdTag {};
struct TradeIdTag {};

struct PriceTag {};
struct QuantityTag {};
struct TimestampTag {};

using OrderId = StrongType<std::uint64_t, OrderIdTag>;
using ClientId = StrongType<std::uint64_t, ClientIdTag>;
using TradeId = StrongType<std::uint64_t, TradeIdTag, Incrementable>;

using Price = StrongType<std::int64_t, PriceTag>;      // signed for InvalidPrice sentinel
using Quantity = StrongType<std::uint64_t, QuantityTag, Addable, Subtractable>;
using Timestamp = StrongType<std::chrono::nanoseconds, TimestampTag>;

// std::hash specialization for any StrongType — placed after StrongType
// is defined so the compiler knows what we're specializing.
template <typename T, typename Tag, template<typename> class... Skills>
struct std::hash<StrongType<T, Tag, Skills...>>
{
    std::size_t operator()(StrongType<T, Tag, Skills...> const& s) const noexcept
    {
        return std::hash<T>{}(s.get());
    }
};
