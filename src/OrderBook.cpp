#include "OrderBook.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <vector>

#include "Order.h"
#include "Side.h"
#include "Types.h"

namespace orderbook {

void OrderBook::pushBack(PriceLevel& level, Order* order)
{
    order->prev = level.tail;
    order->next = nullptr;

    if (level.tail)
    {
        level.tail->next = order;
    }
    else
    {
        level.head = order;
    }

    level.tail = order;
    level.orderCount++;
    level.totalQuantity += order->getRemainingQuantity();
}

void OrderBook::unlink(PriceLevel& level, Order* order)
{
    if (order->prev)
    {
        order->prev->next = order->next;
    }
    else
    {
        level.head = order->next;
    }

    if (order->next)
    {
        order->next->prev = order->prev;
    }
    else
    {
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

    PriceLevel& level = (side == Side::Buy) ? bids_.getOrCreate(price) : asks_.getOrCreate(price);

    pushBack(level, order);

    orders_[order->getOrderId()] = OrderEntry{ .side = side, .price = price, .order = order };
}

bool OrderBook::acceptsPrice(const Order& order) const noexcept
{
    if (order.getOrderType() == OrderType::Market)
    {
        return true;
    }

    return bids_.contains(order.getPrice());
}

bool OrderBook::canMatch(Order* order) const
{
    const Price orderPrice = order->getPrice();

    if (order->getOrderType() == OrderType::Market)
    {
        return order->getSide() == Side::Buy ? !asks_.empty() : !bids_.empty();
    }

    if (order->getSide() == Side::Buy)
    {
        const PriceLevel* bestAsk = asks_.best();
        return bestAsk != nullptr && orderPrice >= bestAsk->price;
    }

    const PriceLevel* bestBid = bids_.best();
    return bestBid != nullptr && orderPrice <= bestBid->price;
}

bool OrderBook::canFullyFill(Side side, Price price, Quantity qty) const
{
    Quantity available{ 0 };
    bool sufficient = false;

    const auto accumulate = [&](const PriceLevel& level)
    {
        const bool withinLimit =
            (side == Side::Buy) ? level.price <= price : level.price >= price;
        if (!withinLimit)
        {
            return false;
        }

        available += level.totalQuantity;
        if (available >= qty)
        {
            sufficient = true;
            return false;
        }

        return true;
    };

    if (side == Side::Buy)
    {
        asks_.forEachFromBest(accumulate);
    }
    else
    {
        bids_.forEachFromBest(accumulate);
    }

    return sufficient;
}

PriceLevel* OrderBook::getBestOppositeLevel(Order* order)
{
    return order->getSide() == Side::Buy ? asks_.best() : bids_.best();
}

Trade OrderBook::createTrade(Order* incomingOrder,
                             Order* restingOrder,
                             Quantity quantity,
                             Timestamp timestamp)
{
    OrderId incomingOrderId = incomingOrder->getOrderId();
    OrderId restingOrderId = restingOrder->getOrderId();

    Trade trade;

    trade.id = nextTradeId_;
    ++nextTradeId_;

    if (incomingOrder->getSide() == Side::Buy)
    {
        trade.buyOrderId = incomingOrderId;
        trade.sellOrderId = restingOrderId;
    }
    else
    {
        trade.buyOrderId = restingOrderId;
        trade.sellOrderId = incomingOrderId;
    }

    trade.price = restingOrder->getPrice();
    trade.quantity = quantity;
    trade.timestamp = timestamp;

    return trade;
}

void OrderBook::cleanupAfterTrade(Order* incoming,
                                  Order* resting,
                                  PriceLevel& level,
                                  Quantity tradeQuantity)
{
    const Price restingPrice = resting->getPrice();

    level.totalQuantity -= tradeQuantity;

    if (resting->isFilled())
    {
        orders_.erase(resting->getOrderId());
        unlink(level, resting);
        orderSlab_.release(resting);
    }

    if (level.orderCount == 0)
    {
        if (incoming->getSide() == Side::Buy)
        {
            asks_.erase(restingPrice);
        }
        else
        {
            bids_.erase(restingPrice);
        }
    }
}

std::vector<Trade> OrderBook::matchOrder(Order* incoming)
{
    std::vector<Trade> trades;

    const auto now =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
    const Timestamp arrivalTime{ now.time_since_epoch() };

    while (!incoming->isFilled() && canMatch(incoming))
    {
        PriceLevel* level = getBestOppositeLevel(incoming);

        if (level == nullptr || level->head == nullptr)
        {
            break;
        }

        Order* resting = level->head;

        const Quantity tradeQuantity =
            std::min(incoming->getRemainingQuantity(), resting->getRemainingQuantity());

        Trade trade = createTrade(incoming, resting, tradeQuantity, arrivalTime);

        incoming->fill(tradeQuantity);
        resting->fill(tradeQuantity);

        trades.push_back(trade);

        cleanupAfterTrade(incoming, resting, *level, tradeQuantity);
    }

    return trades;
}

std::vector<Trade> OrderBook::addOrder(Order order)
{
    if (orders_.contains(order.getOrderId()))
    {
        return {};
    }

    if (!acceptsPrice(order))
    {
        return {};
    }

    if (order.getOrderType() == OrderType::FillOrKill &&
        !canFullyFill(order.getSide(), order.getPrice(), order.getRemainingQuantity()))
    {
        return {};
    }

    Order* incoming = orderSlab_.allocate(std::move(order));

    std::vector<Trade> trades;

    if (canMatch(incoming))
    {
        trades = matchOrder(incoming);
    }

    if (!incoming->isFilled())
    {
        const OrderType type = incoming->getOrderType();
        if (type != OrderType::Market && type != OrderType::ImmediateOrCancel)
        {
            addToBook(incoming);
        }
    }

    return trades;
}

bool OrderBook::removeOrder(OrderId orderId)
{
    auto it = orders_.find(orderId);
    if (it == orders_.end())
    {
        return false;
    }

    const OrderEntry entry = it->second;
    Order* order = entry.order;

    PriceLevel* level =
        entry.side == Side::Buy ? bids_.find(entry.price) : asks_.find(entry.price);

    if (level == nullptr)
    {
        return false;
    }

    level->totalQuantity -= order->getRemainingQuantity();

    unlink(*level, order);

    if (level->orderCount == 0)
    {
        if (entry.side == Side::Buy)
        {
            bids_.erase(entry.price);
        }
        else
        {
            asks_.erase(entry.price);
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
    result.bids.reserve(bids_.levelCount());
    result.asks.reserve(asks_.levelCount());

    bids_.forEachFromBest(
        [&result](const PriceLevel& level)
        {
            result.bids.push_back({ level.price, level.totalQuantity, level.orderCount });
            return true;
        });

    asks_.forEachFromBest(
        [&result](const PriceLevel& level)
        {
            result.asks.push_back({ level.price, level.totalQuantity, level.orderCount });
            return true;
        });

    return result;
}

void OrderBook::printBook()
{
    const auto printLevel = [](const PriceLevel& level)
    {
        std::cout << std::format("{} | {} ({})\n", level.price.get(), level.totalQuantity.get(),
                                 level.orderCount);
        return true;
    };

    std::cout << "=== ASKS ===" << std::endl;

    asks_.forEachFromBest(printLevel);

    std::cout << "-------------" << std::endl;

    bids_.forEachFromBest(printLevel);

    std::cout << "=== BIDS ===" << std::endl;
}

}
