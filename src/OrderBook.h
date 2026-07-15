#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

#include "LevelInfo.h"
#include "Order.h"
#include "OrderSlab.h"
#include "Trade.h"
#include "Types.h"

namespace orderbook {

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
    struct PriceLevel
    {
        Price price{ Price{ 0 } };
        Quantity totalQuantity{ Quantity{ 0 } };
        Order* head{ nullptr };
        Order* tail{ nullptr };
        std::size_t orderCount{ 0 };
    };

    struct OrderEntry
    {
        Side side;
        Price price;
        Order* order;
    };

    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel, std::less<Price>> asks_;
    PriceLevel* bestBid_ = nullptr;
    PriceLevel* bestAsk_ = nullptr;

    std::unordered_map<OrderId, OrderEntry> orders_;
    OrderSlab orderSlab_;

    std::vector<Trade> trades_;
    TradeId nextTradeId_{ 1 };

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
