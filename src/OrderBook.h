#pragma once

#include <cstddef>
#include <span>
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

    OrderBook(Price minPrice, Price maxPrice, std::size_t orderCapacity = DefaultOrderCapacity)
        : bids_{ minPrice, maxPrice }
        , asks_{ minPrice, maxPrice }
        , orderSlab_{ orderCapacity }
    {
    }

    std::span<const Trade> addOrder(Order order);
    bool cancelOrder(OrderId orderId);
    [[nodiscard]] std::size_t size() const { return orders_.size(); }
    [[nodiscard]] TradeId getNextTradeId() const { return nextTradeId_; }
    [[nodiscard]] OrderBookLevelInfos getLevelInfos() const;

private:
    PriceLadder<Side::Buy> bids_;
    PriceLadder<Side::Sell> asks_;

    std::unordered_map<OrderId, Order*> orders_;
    OrderSlab orderSlab_;

    std::vector<Trade> tradeBuffer_;
    TradeId nextTradeId_{ 1 };

    [[nodiscard]] bool acceptsPrice(const Order& order) const noexcept;
    [[nodiscard]] bool canMatch(const Order* order) const;
    [[nodiscard]] bool canFullyFill(Side side, Price price, Quantity qty) const;
    [[nodiscard]] PriceLevel* getBestOppositeLevel(const Order* order);
    Trade createTrade(Order* incoming, Order* resting, Quantity quantity, Timestamp timestamp);
    void cleanupAfterTrade(Order* incoming, Order* resting, PriceLevel& level, Quantity tradeQty);
    void matchOrder(Order* incomingOrder);
    void addToBook(Order* order);
    bool removeOrder(OrderId orderId);

    static void pushBack(PriceLevel& level, Order* order);
    static void unlink(PriceLevel& level, Order* order);
};

}
