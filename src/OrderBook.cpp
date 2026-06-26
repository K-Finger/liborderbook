#include <iostream>
#include <format>
#include <algorithm>
#include <chrono>
#include <vector>

#include "OrderBook.h"
#include "Order.h"
#include "Types.h"
#include "Side.h"

void OrderBook::pushBack(PriceLevel& level, Order* order)
{
    order->prev = level.tail;
    order->next = nullptr;

    if (level.tail) {
        level.tail->next = order;
    }
    else {
        level.head = order;
    }

    level.tail = order;
    level.orderCount++;
    level.totalQuantity += order->getRemainingQuantity();
}

void OrderBook::unlink(PriceLevel& level, Order* order)
{
    if (order->prev) {
        order->prev->next = order->next;
    }
    else {
        level.head = order->next;
    }

    if (order->next) {
        order->next->prev = order->prev;
    }
    else {
        level.tail = order->prev;
    }

    order->prev = nullptr;
    order->next = nullptr;
    level.orderCount--;
}

void OrderBook::addToBook(Order* order)
{
    const Price price = order->getPrice();
    const Side side = order->getSide();

    PriceLevel& level = (side == Side::Buy)
        ? bids_.try_emplace(price).first->second
        : asks_.try_emplace(price).first->second;
    level.price = price;

    pushBack(level, order);

    if (side == Side::Buy) {
        if (!bestBid_ || price > bestBid_->price) {
            bestBid_ = &level;
        }
    }
    else {
        if (!bestAsk_ || price < bestAsk_->price) {
            bestAsk_ = &level;
        }
    }

    orders_[order->getOrderId()] = OrderEntry{
        .side = side,
        .price = price,
        .order = order
    };
}

bool OrderBook::canMatch(Order* order) const
{

    Price orderPrice = order->getPrice();

    if (order->getOrderType() == OrderType::Market) {
        return order->getSide() == Side::Buy ? bestAsk_ != nullptr : bestBid_ != nullptr;
    }

    if (order->getSide() == Side::Buy) {
        if (!bestAsk_) {
            return false;
        }

        return orderPrice >= bestAsk_->price;
    }
    else {
        if (!bestBid_) {
            return false;
        }

        return orderPrice <= bestBid_->price;
    }
}

bool OrderBook::canFullyFill(Side side, Price price, Quantity qty) const
{
    Quantity available{ 0 };

    if (side == Side::Buy) {
        for (const auto& [askPrice, level] : asks_) {
            if (askPrice > price) break;
            available += level.totalQuantity;
            if (available >= qty) return true;
        }
    }
    else {
        for (const auto& [bidPrice, level] : bids_) {
            if (bidPrice < price) break;
            available += level.totalQuantity;
            if (available >= qty) return true;
        }
    }
    return false;
}

OrderBook::PriceLevel* OrderBook::getBestOppositeLevel(Order* order)
{
    return order->getSide() == Side::Buy ? bestAsk_ : bestBid_;
}

Trade OrderBook::createTrade(
    Order* incomingOrder,
    Order* restingOrder,
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
    Order* incoming,
    Order* resting,
    PriceLevel& level,
    Quantity tradeQuantity
)
{
    const Price restingPrice = resting->getPrice(); // cache before release

    level.totalQuantity -= tradeQuantity;

    if (resting->isFilled()) {
        orders_.erase(resting->getOrderId());
        unlink(level, resting);
        orderSlab_.release(resting);
    }

    if (level.orderCount == 0) {
        if (incoming->getSide() == Side::Buy) {
            asks_.erase(restingPrice);
            bestAsk_ = asks_.empty() ? nullptr : &asks_.begin()->second;
        }
        else {
            bids_.erase(restingPrice);
            bestBid_ = bids_.empty() ? nullptr : &bids_.begin()->second;
        }
    }
}

std::vector<Trade> OrderBook::matchOrder(Order* incoming)
{
    std::vector<Trade> trades;

    const auto now = std::chrono::time_point_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now()
    );
    const Timestamp arrivalTime{ now.time_since_epoch() };

    while (!incoming->isFilled() && canMatch(incoming)) {
        PriceLevel* level = getBestOppositeLevel(incoming);

        if (level == nullptr || level->head == nullptr) {
            break;
        }

        Order* resting = level->head;

        const Quantity tradeQuantity = std::min(
            incoming->getRemainingQuantity(),
            resting->getRemainingQuantity()
        );

        Trade trade = createTrade(incoming, resting, tradeQuantity, arrivalTime);

        incoming->fill(tradeQuantity);
        resting->fill(tradeQuantity);

        trades.push_back(trade);
        trades_.push_back(trade);

        cleanupAfterTrade(incoming, resting, *level, tradeQuantity);
    }

    return trades;
}

std::vector<Trade> OrderBook::addOrder(Order order)
{
    if (orders_.contains(order.getOrderId())) {
        return {};   // TODO: return rejection enum once that exists
    }

    if (order.getOrderType() == OrderType::FillOrKill &&
        !canFullyFill(order.getSide(), order.getPrice(), order.getRemainingQuantity())) {
        return {};
    }

    Order* incoming = orderSlab_.allocate(std::move(order));

    std::vector<Trade> trades;

    if (canMatch(incoming)) {
        trades = matchOrder(incoming);
    }

    // Market and IOC drop their remainder. GTC and FOK rest if still unfilled
    // FOK reaches here only after passing canFullyFill check.
    if (!incoming->isFilled()) {
        const OrderType type = incoming->getOrderType();
        if (type != OrderType::Market &&
            type != OrderType::ImmediateOrCancel) {
            addToBook(incoming);
        }
    }

    return trades;
}
bool OrderBook::removeOrder(OrderId orderId)
{
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return false;
    }

    OrderEntry entry = it->second;
    Order* order = entry.order;

    PriceLevel& level = entry.side == Side::Buy
        ? bids_.at(entry.price)
        : asks_.at(entry.price);


    level.totalQuantity -= order->getRemainingQuantity();

    unlink(level, order);

    if (level.orderCount == 0) {
        PriceLevel* erased = &level;
        if (entry.side == Side::Buy) {
            bids_.erase(entry.price);
            if (bestBid_ == erased) {
                bestBid_ = bids_.empty() ? nullptr : &bids_.begin()->second;
            }
        }
        else {
            asks_.erase(entry.price);
            if (bestAsk_ == erased) {
                bestAsk_ = asks_.empty() ? nullptr : &asks_.begin()->second;
            }
        }
    }

    orders_.erase(orderId);
    orderSlab_.release(order);

    return true;
}

bool OrderBook::cancelOrder(OrderId orderId)
{
    return removeOrder(orderId);
}

OrderBookLevelInfos OrderBook::getLevelInfos() const
{
    OrderBookLevelInfos result;
    result.bids.reserve(bids_.size());
    result.asks.reserve(asks_.size());

    for (const auto& [price, level] : bids_) {
        result.bids.push_back({ price, level.totalQuantity, level.orderCount });
    }
    for (const auto& [price, level] : asks_) {
        result.asks.push_back({ price, level.totalQuantity, level.orderCount });
    }

    return result;
}


void OrderBook::printBook()
{
    std::cout << "=== ASKS ===" << std::endl;

    for (auto const& [key, value] : asks_) {
        std::cout << std::format("{} | {} ({})\n", key.get(), value.totalQuantity.get(), value.orderCount);
    }

    std::cout << "-------------" << std::endl;

    for (auto const& [key, value] : bids_) {
        std::cout << std::format("{} | {} ({})\n", key.get(), value.totalQuantity.get(), value.orderCount);
    }

    std::cout << "=== BIDS ===" << std::endl;
}