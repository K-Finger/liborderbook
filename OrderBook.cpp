#include <iostream>
#include <format>
#include <algorithm>
#include <chrono>
#include <vector>
#include <iterator>

#include "OrderBook.h"
#include "Order.h"
#include "Types.h"
#include "Side.h"

void OrderBook::addToBook(OrderPointer order)
{
    Price orderPrice = order->getPrice();
    Side orderSide = order->getSide();

    PriceLevel* level = nullptr;

    if (orderSide == Side::Buy) {
        auto [it, inserted] = bids_.try_emplace(
            orderPrice,
            PriceLevel{ .totalQuantity = Quantity{0}, .orders = {} }
        );
        level = &it->second;
    }
    else {
        auto [it, inserted] = asks_.try_emplace(
            orderPrice,
            PriceLevel{ .totalQuantity = Quantity{0}, .orders = {} }
        );
        level = &it->second;
    }

    level->orders.push_back(order);
    level->totalQuantity += order->getRemainingQuantity(); // remaining, since order may have partially filled before resting

    auto orderIterator = std::prev(level->orders.end());

    orders_[order->getOrderId()] = OrderEntry{
        .side = orderSide,
        .price = orderPrice,
        .iterator = orderIterator
    };
}

bool OrderBook::canMatch(OrderPointer order) const
{
    Price orderPrice = order->getPrice();

    if (order->getSide() == Side::Buy) {
        if (asks_.empty()) {
            return false;
        }

        return orderPrice >= asks_.begin()->first; // get price from iterator
    }
    else {
        if (bids_.empty()) {
            return false;
        }

        return orderPrice <= bids_.begin()->first;
    }
}

OrderBook::PriceLevel* OrderBook::getBestOppositeLevel(OrderPointer order)
{
    if (order->getSide() == Side::Buy) {
        if (asks_.empty()) {
            return nullptr;
        }

        return &asks_.begin()->second;
    }

    if (bids_.empty()) {
        return nullptr;
    }

    return &bids_.begin()->second;
}

Trade OrderBook::createTrade(
    OrderPointer incomingOrder,
    OrderPointer restingOrder,
    Quantity quantity,
    Timestamp timestamp
)
{
    OrderId incomingOrderId = incomingOrder->getOrderId();
    OrderId restingOrderId = restingOrder->getOrderId();

    Trade trade;

    trade.id = nextTradeId_;
    ++nextTradeId_;

    if (incomingOrder->getSide() == Side::Buy) {
        trade.buyOrderId = incomingOrderId;
        trade.sellOrderId = restingOrderId;
    }
    else {
        trade.buyOrderId = restingOrderId;
        trade.sellOrderId = incomingOrderId;
    }

    trade.price = restingOrder->getPrice();
    trade.quantity = quantity;
    trade.timestamp = timestamp;

    return trade;
}

void OrderBook::cleanupAfterTrade(
    OrderPointer incomingOrder,
    OrderPointer restingOrder,
    PriceLevel* bestPriceLevel,
    Quantity tradeQuantity
)
{
    bestPriceLevel->totalQuantity -= tradeQuantity;

    if (restingOrder->isFilled()) {
        orders_.erase(restingOrder->getOrderId());
        bestPriceLevel->orders.pop_front();
    }

    if (bestPriceLevel->orders.empty()) {
        Price price = restingOrder->getPrice();
        if (incomingOrder->getSide() == Side::Buy) {
            asks_.erase(price);
        }
        else {
            bids_.erase(price);
        }
    }
}

std::vector<Trade> OrderBook::matchOrder(OrderPointer incomingOrder)
{
    std::vector<Trade> trades;

    auto now = std::chrono::time_point_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now()
    );
    Timestamp arrivalTime{ now.time_since_epoch() };

    while (!incomingOrder->isFilled() && canMatch(incomingOrder)) {
        PriceLevel* bestPriceLevel = getBestOppositeLevel(incomingOrder);

        OrderPointer restingOrder = bestPriceLevel->orders.front();
        Quantity tradeQuantity = std::min(
            restingOrder->getRemainingQuantity(),
            incomingOrder->getRemainingQuantity()
        );

        Trade trade = createTrade(incomingOrder, restingOrder, tradeQuantity, arrivalTime);

        incomingOrder->fill(tradeQuantity);
        restingOrder->fill(tradeQuantity);

        trades.push_back(trade);
        trades_.push_back(trade);

        cleanupAfterTrade(
            incomingOrder,
            restingOrder,
            bestPriceLevel,
            tradeQuantity
        );
    }

    return trades;
}

std::vector<Trade> OrderBook::addOrder(OrderPointer order)
{
    std::vector<Trade> trades;

    if (canMatch(order)) {
        trades = matchOrder(order);

    }

    if (!order->isFilled()) {
        addToBook(order);
    }

    return trades;
}
bool OrderBook::removeOrder(OrderId orderId)
{
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return false;
    }

    OrderEntry orderEntry = it->second;
    OrderPointer order = *(orderEntry.iterator);

    PriceLevel* priceLevel = nullptr;

    if (orderEntry.side == Side::Buy) {
        priceLevel = &bids_.at(orderEntry.price);
    }
    else {
        priceLevel = &asks_.at(orderEntry.price);
    }

    priceLevel->totalQuantity -= order->getRemainingQuantity();
    priceLevel->orders.erase(orderEntry.iterator);

    if (priceLevel->orders.empty()) {
        if (orderEntry.side == Side::Buy) {
            bids_.erase(orderEntry.price);
        }
        else {
            asks_.erase(orderEntry.price);
        }
    }

    orders_.erase(orderId);

    return true;
}

bool OrderBook::cancelOrder(OrderId orderId)
{
    return removeOrder(orderId);
}


void OrderBook::printBook()
{
    std::cout << "=== ASKS ===" << std::endl;

    for (auto const& [key, value] : asks_) {
        std::cout << std::format("{} | {} ({})\n", key.get(), value.totalQuantity.get(), value.orders.size());
    }

    std::cout << "-------------" << std::endl;

    for (auto const& [key, value] : bids_) {
        std::cout << std::format("{} | {} ({})\n", key.get(), value.totalQuantity.get(), value.orders.size());
    }

    std::cout << "=== BIDS ===" << std::endl;
}