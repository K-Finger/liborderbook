#include "Order.h"

#include <chrono>
#include <stdexcept>
#include <string>

namespace orderbook {

void Order::fill(Quantity qty)
{
    if (qty > remainingQuantity_)
    {
        throw std::logic_error("Fill of " + std::to_string(qty.get()) +
                               " exceeds remaining quantity " +
                               std::to_string(remainingQuantity_.get()) + " on order " +
                               std::to_string(orderId_.get()));
    }
    remainingQuantity_ -= qty;
}

Timestamp Order::now()
{
    auto n =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
    return Timestamp{ n.time_since_epoch() };
}

OrderBuilder& OrderBuilder::id(OrderId id)
{
    id_ = id;
    setFlags_ |= HasId;
    return *this;
}

OrderBuilder& OrderBuilder::buy()
{
    side_ = Side::Buy;
    setFlags_ |= HasSide;
    return *this;
}

OrderBuilder& OrderBuilder::sell()
{
    side_ = Side::Sell;
    setFlags_ |= HasSide;
    return *this;
}

OrderBuilder& OrderBuilder::goodTillCancel()
{
    type_ = OrderType::GoodTillCancel;
    return *this;
}

OrderBuilder& OrderBuilder::market()
{
    type_ = OrderType::Market;
    return *this;
}

OrderBuilder& OrderBuilder::fillOrKill()
{
    type_ = OrderType::FillOrKill;
    return *this;
}

OrderBuilder& OrderBuilder::immediateOrCancel()
{
    type_ = OrderType::ImmediateOrCancel;
    return *this;
}

OrderBuilder& OrderBuilder::price(Price p)
{
    price_ = p;
    setFlags_ |= HasPrice;
    return *this;
}

OrderBuilder& OrderBuilder::quantity(Quantity qty)
{
    quantity_ = qty;
    setFlags_ |= HasQty;
    return *this;
}

OrderBuilder& OrderBuilder::timestamp(Timestamp t)
{
    timestamp_ = t;
    setFlags_ |= HasTimestamp;
    return *this;
}

Order OrderBuilder::build()
{
    const bool isMarket = (type_ == OrderType::Market);
    const std::uint8_t required = isMarket ? RequiredMarketFlags : RequiredLimitFlags;
    const auto missing = static_cast<std::uint8_t>(required & ~setFlags_);

    if (missing != 0)
    {
        std::string msg = isMarket ? "Market order missing:" : "Limit order missing:";
        if ((missing & HasId) != 0)
        {
            msg += " id";
        }
        if ((missing & HasSide) != 0)
        {
            msg += " side";
        }
        if ((missing & HasPrice) != 0)
        {
            msg += " price";
        }
        if ((missing & HasQty) != 0)
        {
            msg += " quantity";
        }
        throw std::logic_error(msg);
    }

    const Price effectivePrice = isMarket ? InvalidPrice : price_;
    const Timestamp ts = ((setFlags_ & HasTimestamp) != 0) ? timestamp_ : Order::now();

    return Order{ type_, id_, side_, effectivePrice, quantity_, ts };
}

}
