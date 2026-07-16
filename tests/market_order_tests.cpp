#include <gtest/gtest.h>
#include "Constants.h"
#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

TEST(OrderBookAddOrder, MarketTradesNothingForEmptySide) {
    OrderBook book;
    Order market = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .market()
        .quantity(Quantity{ 10 })
        .build();

    auto trades = book.addOrder(std::move(market));

    EXPECT_EQ(trades.size(), 0u);
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookAddOrder, MarketFullFills) {
    OrderBook book;
    Order restingSell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 6767 })
        .quantity(Quantity{ 100 })
        .build();

    Order marketBuy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .market()
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell));
    auto trades = book.addOrder(std::move(marketBuy));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades.front().price, Price{ 6767 });
    EXPECT_EQ(trades.front().quantity, Quantity{ 10 });
    EXPECT_EQ(book.size(), 1u);

    auto infos = book.getLevelInfos();
    ASSERT_EQ(infos.asks.size(), 1u);
    EXPECT_EQ(infos.asks.front().price, Price{ 6767 });
    EXPECT_EQ(infos.asks.front().quantity, Quantity{ 90 });
    EXPECT_EQ(infos.asks.front().orderCount, 1u);
    EXPECT_TRUE(infos.bids.empty());
}
