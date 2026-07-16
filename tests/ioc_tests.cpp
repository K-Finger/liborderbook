#include <gtest/gtest.h>
#include "Constants.h"
#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

TEST(OrderBookIOC, FullFillsAtCrossingPrice) {
    OrderBook book;
    Order restingSell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    Order iocBuy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .immediateOrCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell));
    auto trades = book.addOrder(std::move(iocBuy));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades.front().quantity, Quantity{ 10 });
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookIOC, PartialFillDropsRemainder) {
    OrderBook book;
    Order restingSell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .build();

    Order iocBuy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .immediateOrCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell));
    auto trades = book.addOrder(std::move(iocBuy));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades.front().quantity, Quantity{ 5 });
    EXPECT_EQ(book.size(), 0u);

    auto infos = book.getLevelInfos();
    EXPECT_TRUE(infos.asks.empty());
    EXPECT_TRUE(infos.bids.empty());
}

TEST(OrderBookIOC, NoMatchAtPriceDropsEntirely) {
    OrderBook book;
    Order restingSell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 101 })
        .quantity(Quantity{ 10 })
        .build();

    Order iocBuy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .immediateOrCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell));
    auto trades = book.addOrder(std::move(iocBuy));

    EXPECT_EQ(trades.size(), 0u);
    EXPECT_EQ(book.size(), 1u);

    auto infos = book.getLevelInfos();
    EXPECT_TRUE(infos.bids.empty());
    ASSERT_EQ(infos.asks.size(), 1u);
    EXPECT_EQ(infos.asks.front().quantity, Quantity{ 10 });
}

TEST(OrderBookIOC, EmptyOppositeDropsEntirely) {
    OrderBook book;
    Order iocBuy = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .immediateOrCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    auto trades = book.addOrder(std::move(iocBuy));

    EXPECT_EQ(trades.size(), 0u);
    EXPECT_EQ(book.size(), 0u);
}
