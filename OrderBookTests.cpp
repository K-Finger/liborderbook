#include <gtest/gtest.h>
#include "OrderBook.h"
#include "Order.h"

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