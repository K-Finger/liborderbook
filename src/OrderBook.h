#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "Constants.h"
#include "LevelInfo.h"
#include "Order.h"
#include "OrderSlab.h"
#include "PriceLadder.h"
#include "PriceLevel.h"
#include "Trade.h"
#include "Types.h"

namespace orderbook {

class OrderBook
{
public:
    OrderBook() : OrderBook(DefaultMinPrice, DefaultMaxPrice) {}

    OrderBook(Price minPrice, Price maxPrice)
        : bids_{ minPrice, maxPrice }
        , asks_{ minPrice, maxPrice }
    {
    }

    std::vector<Trade> addOrder(Order order);
    bool cancelOrder(OrderId orderId);
    std::size_t size() const { return orders_.size(); }
    TradeId getNextTradeId() const { return nextTradeId_; }
    OrderBookLevelInfos getLevelInfos() const;
    void printBook();

private:
    struct OrderEntry
    {
        Side side;
        Price price;
        Order* order;
    };

    PriceLadder<Side::Buy> bids_;
    PriceLadder<Side::Sell> asks_;

    std::unordered_map<OrderId, OrderEntry> orders_;
    OrderSlab orderSlab_;

    TradeId nextTradeId_{ 1 };

    bool acceptsPrice(const Order& order) const noexcept;
    bool canMatch(Order* order) const;
    bool canFullyFill(Side side, Price price, Quantity qty) const;
    PriceLevel* getBestOppositeLevel(Order* order);
    Trade createTrade(Order* incoming, Order* resting, Quantity quantity, Timestamp timestamp);
    void cleanupAfterTrade(Order* incoming, Order* resting, PriceLevel& level, Quantity tradeQty);
    std::vector<Trade> matchOrder(Order* incomingOrder);
    void addToBook(Order* order);
    bool removeOrder(OrderId orderId);

    void pushBack(PriceLevel& level, Order* order);
    void unlink(PriceLevel& level, Order* order);
};

}
