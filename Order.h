#pragma once

#include <stdexcept>
#include <string>

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

    // Seperate static factory method because market orders use InvalidPrice
    static Order makeMarket(OrderId id, Side side, Quantity quantity, Timestamp ts)
    {
        return Order{OrderType::Market, id, side, InvalidPrice, quantity, ts};
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

    [[nodiscard]] Quantity getFilledQuantity() const noexcept { return getInitialQuantity() - getRemainingQuantity(); }
    [[nodiscard]] bool     isFilled()          const noexcept { return getRemainingQuantity() == Quantity{ 0 }; }

    void fill(Quantity qty)
    {
        if (qty > remainingQuantity_)
            throw std::logic_error(
                "Fill of " + std::to_string(qty.get()) +
                " exceeds remaining quantity " + std::to_string(remainingQuantity_.get()) +
                " on order " + std::to_string(orderId_.get()));
        remainingQuantity_ -= qty;
    }
private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
    Timestamp timestamp_;
};