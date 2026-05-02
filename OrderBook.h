#pragma once

#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <memory>
#include <functional>
#include "Types.h"
#include "Order.h"
#include "Trade.h"
#include "LevelInfo.h"

using OrderPointer = std::shared_ptr<Order>;

class OrderBook
{
public:
    std::vector<Trade> addOrder(Order order);
    bool cancelOrder(OrderId orderId);
    std::size_t size() const { return orders_.size(); }
    TradeId getNextTradeId() const { return nextTradeId_; }
    const std::vector<Trade>& getTrades() const { return trades_; }
    OrderBookLevelInfos getLevelInfos() const;
    void printBook();

private:

    // stores a FIFO linked list of all orders at a certain price
    // bids_ and asks_ stores the price in a map
    struct PriceLevel
    {
        Quantity totalQuantity;
        std::list<OrderPointer> orders;
    };

    // Record for locating an order
    struct OrderEntry
    {
        Side side;
        Price price;
        std::list<OrderPointer>::iterator iterator;
    };

    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel, std::less<Price>> asks_;

    std::unordered_map<OrderId, OrderEntry> orders_;

    std::vector<Trade> trades_;
    TradeId nextTradeId_{ 1 };

    bool canMatch(OrderPointer order) const;
    bool canFullyFill(Side side, Price price, Quantity qty) const;
    PriceLevel* getBestOppositeLevel(OrderPointer order);
    Trade createTrade(
        OrderPointer incomingOrder,
        OrderPointer restingOrder,
        Quantity quantity,
        Timestamp timestamp
    );
    void cleanupAfterTrade(
        OrderPointer incomingOrder,
        OrderPointer restingOrder,
        PriceLevel* bestPriceLevel,
        Quantity tradeQuantity
    );
    std::vector<Trade> matchOrder(OrderPointer incomingOrder);
    void addToBook(OrderPointer order);
    bool removeOrder(OrderId orderId);
};