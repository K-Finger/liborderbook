#include <gtest/gtest.h>
#include "Constants.h"
#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

TEST(OrderBookEmpty, SizeIsZero) {
    OrderBook book;
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookCancel, ReturnsFalseForUnknownId) {
    OrderBook book;
    EXPECT_FALSE(book.cancelOrder(OrderId{ 42 }));
}

TEST(OrderBookAddOrder, NoMatchForRestingBuy) {
    OrderBook book;
    Order buy = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    auto trades = book.addOrder(std::move(buy));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.size(), 1u);
}

TEST(OrderBookCancel, RemoveRestingOrder) {
    OrderBook book;
    Order buy = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(buy));
    EXPECT_TRUE(book.cancelOrder(OrderId{ 1 }));
    EXPECT_EQ(book.size(), 0u);
    EXPECT_FALSE(book.cancelOrder(OrderId{ 1 }));
}
