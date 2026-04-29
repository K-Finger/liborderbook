#include <chrono>
#include <stdexcept>
#include <string>

#include "Order.h"

void Order::fill(Quantity qty)
{
    if (qty > remainingQuantity_)
        throw std::logic_error(
            "Fill of " + std::to_string(qty.get()) +
            " exceeds remaining quantity " + std::to_string(remainingQuantity_.get()) +
            " on order " + std::to_string(orderId_.get()));
    remainingQuantity_ -= qty;
}

Timestamp Order::now()
{
    auto n = std::chrono::time_point_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now()
    );
    return Timestamp{ n.time_since_epoch() };
}

OrderBuilder& OrderBuilder::id(OrderId id)         { id_ = id;           setFlags_ |= hasId;        return *this; }
OrderBuilder& OrderBuilder::buy()                  { side_ = Side::Buy;  setFlags_ |= hasSide;      return *this; }
OrderBuilder& OrderBuilder::sell()                 { side_ = Side::Sell; setFlags_ |= hasSide;      return *this; }
OrderBuilder& OrderBuilder::goodTillCancel()       { type_ = OrderType::GoodTillCancel;             return *this; }
OrderBuilder& OrderBuilder::market()               { type_ = OrderType::Market;                     return *this; }
OrderBuilder& OrderBuilder::price(Price p)         { price_ = p;         setFlags_ |= hasPrice;     return *this; }
OrderBuilder& OrderBuilder::quantity(Quantity qty) { quantity_ = qty;    setFlags_ |= hasQty;       return *this; }
OrderBuilder& OrderBuilder::timestamp(Timestamp t) { timestamp_ = t;     setFlags_ |= hasTimestamp; return *this; }

Order OrderBuilder::build()
{
    const bool isMarket = (type_ == OrderType::Market);
    const std::uint8_t required = isMarket ? requiredMarketFlags : requiredLimitFlags;
    const std::uint8_t missing = static_cast<std::uint8_t>(required & ~setFlags_);

    if (missing != 0)
    {
        std::string msg = isMarket ? "Market order missing:" : "Limit order missing:";
        if (missing & hasId)    msg += " id";
        if (missing & hasSide)  msg += " side";
        if (missing & hasPrice) msg += " price";
        if (missing & hasQty)   msg += " quantity";
        throw std::logic_error(msg);
    }

    const Price effectivePrice = isMarket ? InvalidPrice : price_;
    const Timestamp ts = (setFlags_ & hasTimestamp) ? timestamp_ : Order::now();

    return Order{ type_, id_, side_, effectivePrice, quantity_, ts };
}
