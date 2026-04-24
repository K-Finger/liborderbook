#pragma once

#include "Side.h"
#include "OrderType.h"
#include "Types.h"

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

    OrderType GetOrderType() const { return orderType_; }
    OrderId   GetOrderId()   const { return orderId_; }
    Side      GetSide()      const { return side_; }
    Price     GetPrice()     const { return price_; }
    Quantity  GetInitialQuantity()   const { return initialQuantity_; }
    Quantity  GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity  getFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
    Timestamp GetTimestamp() const { return timestamp_; }

    bool      isFilled() const { return GetRemainingQuantity() == 0; }

private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
    Timestamp timestamp_;
};