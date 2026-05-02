#pragma once

#include <chrono>
#include <cstdint>

#include "Side.h"
#include "OrderType.h"
#include "Types.h"
#include "Constants.h"

class Order
{
public:
    Order(OrderType type, OrderId id, Side side, Price price, Quantity quantity, Timestamp ts)
        : orderType_{ type }
        , orderId_{ id }
        , side_{ side }
        , price_{ price }
        , initialQuantity_{ quantity }
        , remainingQuantity_{ quantity }
        , timestamp_{ ts }
    {
    }

    // Copies forbidden (OrderId identity)
    // Moves allowed for ownership transfer
    Order(const Order&) = delete;
    Order& operator=(const Order&) = delete;
    Order(Order&&) = default;
    Order& operator=(Order&&) = default;

    OrderType getOrderType() const noexcept { return orderType_; }
    OrderId   getOrderId()   const noexcept { return orderId_; }
    Side      getSide()      const noexcept { return side_; }
    Price     getPrice()     const noexcept { return price_; }
    Quantity  getInitialQuantity()   const noexcept { return initialQuantity_; }
    Quantity  getRemainingQuantity() const noexcept { return remainingQuantity_; }
    Timestamp getTimestamp() const noexcept { return timestamp_; }

    [[nodiscard]] Quantity getFilledQuantity() const noexcept { return initialQuantity_ - remainingQuantity_; }
    [[nodiscard]] bool     isFilled()          const noexcept { return remainingQuantity_ == Quantity{ 0 }; }

    void fill(Quantity qty);

    static Timestamp now();

private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
    Timestamp timestamp_;
};

class OrderBuilder
{
public:
    OrderBuilder& id(OrderId id);

    OrderBuilder& buy();
    OrderBuilder& sell();

    OrderBuilder& goodTillCancel();
    OrderBuilder& market();
    OrderBuilder& fillOrKill();
    OrderBuilder& immediateOrCancel();

    OrderBuilder& price(Price price);
    OrderBuilder& quantity(Quantity qty);
    OrderBuilder& timestamp(Timestamp ts);

    Order build();

private:
    OrderType type_{ OrderType::GoodTillCancel };
    OrderId   id_{};
    Side      side_{};
    Price     price_{};
    Quantity  quantity_{};
    Timestamp timestamp_{};
    std::uint8_t setFlags_{ 0 };

    static constexpr std::uint8_t hasId        = 1 << 0;
    static constexpr std::uint8_t hasSide      = 1 << 1;
    static constexpr std::uint8_t hasPrice     = 1 << 2;
    static constexpr std::uint8_t hasQty       = 1 << 3;
    static constexpr std::uint8_t hasTimestamp = 1 << 4;

    static constexpr std::uint8_t requiredLimitFlags  = hasId | hasSide | hasPrice | hasQty;
    static constexpr std::uint8_t requiredMarketFlags = hasId | hasSide | hasQty;
};
