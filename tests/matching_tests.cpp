#include <gtest/gtest.h>
#include "Constants.h"
#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

TEST(OrderBookMatch, FullFillRemovesBothOrders) {
    OrderBook book;
    Order buy = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    Order sell = OrderBuilder{}
        .id(OrderId{ 2 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(buy));
    auto trades = book.addOrder(std::move(sell));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades.front().price, Price{ 100 });
    EXPECT_EQ(trades.front().quantity, Quantity{ 10 });
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookMatch, FIFOAtSamePrice) {
    OrderBook book;
    Timestamp t1{ std::chrono::nanoseconds{1} };
    Timestamp t2{ std::chrono::nanoseconds{2} };

    Order buy1 = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .timestamp(t1)
        .build();

    Order buy2 = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .timestamp(t2)
        .build();

    Order sell = OrderBuilder{}
        .id(OrderId{ 3 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .build();

    book.addOrder(std::move(buy1));
    book.addOrder(std::move(buy2));
    auto trades = book.addOrder(std::move(sell));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades.front().buyOrderId, OrderId{ 1 });
}

TEST(OrderBookMatch, TradePriceIsRestingPrice) {
    OrderBook book;
    Order sell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    Order buy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .goodTillCancel()
        .price(Price{ 105 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(sell));
    auto trades = book.addOrder(std::move(buy));

    ASSERT_EQ(trades.front().price, Price{ 100 });
}
