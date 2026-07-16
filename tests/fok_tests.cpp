#include <gtest/gtest.h>
#include "Constants.h"
#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

TEST(OrderBookFOK, FullFillSingleLevel) {
    OrderBook book;
    Order restingSell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    Order fokBuy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .fillOrKill()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell));
    auto trades = book.addOrder(std::move(fokBuy));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades.front().quantity, Quantity{ 10 });
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookFOK, FullFillAcrossLevels) {
    OrderBook book;
    Order restingSell1 = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 99 })
        .quantity(Quantity{ 5 })
        .build();

    Order restingSell2 = OrderBuilder{}
        .id(OrderId{ 2 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .build();

    Order fokBuy = OrderBuilder{}
        .id(OrderId{ 3 })
        .buy()
        .fillOrKill()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell1));
    book.addOrder(std::move(restingSell2));
    auto trades = book.addOrder(std::move(fokBuy));

    EXPECT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].quantity + trades[1].quantity, Quantity{ 10 });
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookFOK, RejectsWhenInsufficientQty) {
    OrderBook book;
    Order restingSell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .build();

    Order fokBuy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .fillOrKill()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell));
    auto trades = book.addOrder(std::move(fokBuy));

    EXPECT_EQ(trades.size(), 0u);
    EXPECT_EQ(book.size(), 1u);

    auto infos = book.getLevelInfos();
    ASSERT_EQ(infos.asks.size(), 1u);
    EXPECT_EQ(infos.asks.front().quantity, Quantity{ 5 });
    EXPECT_TRUE(infos.bids.empty());
}

TEST(OrderBookFOK, RejectsWhenLiquidityBeyondPriceCeiling) {
    OrderBook book;
    Order restingSell1 = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .build();

    Order restingSell2 = OrderBuilder{}
        .id(OrderId{ 2 })
        .sell()
        .goodTillCancel()
        .price(Price{ 101 })
        .quantity(Quantity{ 5 })
        .build();

    Order fokBuy = OrderBuilder{}
        .id(OrderId{ 3 })
        .buy()
        .fillOrKill()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell1));
    book.addOrder(std::move(restingSell2));
    auto trades = book.addOrder(std::move(fokBuy));

    EXPECT_EQ(trades.size(), 0u);
    EXPECT_EQ(book.size(), 2u);

    auto infos = book.getLevelInfos();
    ASSERT_EQ(infos.asks.size(), 2u);
    EXPECT_EQ(infos.asks[0].quantity, Quantity{ 5 });
    EXPECT_EQ(infos.asks[1].quantity, Quantity{ 5 });
}

TEST(OrderBookFOK, RejectsOnEmptyOpposite) {
    OrderBook book;
    Order fokBuy = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .fillOrKill()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    auto trades = book.addOrder(std::move(fokBuy));

    EXPECT_EQ(trades.size(), 0u);
    EXPECT_EQ(book.size(), 0u);
}
