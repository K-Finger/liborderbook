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
        Quantity totalQuantity{ Quantity{0} };
        Order* head{ nullptr };
        Order* tail{ nullptr };
        std::size_t orderCount{ 0 };
    };

    // Record for locating an order
    struct OrderEntry
    {
        Side side;
        Price price;
        Order* order;
    };

    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel, std::less<Price>> asks_;

    std::unordered_map<OrderId, OrderEntry> orders_;
    std::vector<std::unique_ptr<Order>> orderStorage_;

    std::vector<Trade> trades_;
    TradeId nextTradeId_{ 1 };

    bool canMatch(Order* order) const;
    bool canFullyFill(Side side, Price price, Quantity qty) const;
    PriceLevel* getBestOppositeLevel(Order* order);
    Trade createTrade(
        Order* incoming,
        Order* resting,
        Quantity quantity,
        Timestamp timestamp
    );
    void cleanupAfterTrade(
        Order* incoming,
        Order* resting,
        PriceLevel& level,
        Quantity tradeQuantity
    );
    std::vector<Trade> matchOrder(Order* incomingOrder);
    void addToBook(Order* order);
    bool removeOrder(OrderId orderId);

    void pushBack(PriceLevel& level, Order* order);
    void unlink(PriceLevel& level, Order* order);
};