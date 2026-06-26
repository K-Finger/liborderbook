#include <gtest/gtest.h>
#include "Constants.h"
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

TEST(OrderFill, ThrowsWhenExceedsRemaining) {
    Order buy = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 5 })
        .build();

    EXPECT_THROW(buy.fill(Quantity{ 6 }), std::logic_error);
}

TEST(OrderFill, DecrementsRemainingOnPartial) {
    Order o = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    o.fill(Quantity{ 4 });
    EXPECT_EQ(o.getRemainingQuantity(), Quantity{ 6 });
    EXPECT_EQ(o.getFilledQuantity(), Quantity{ 4 });
    EXPECT_FALSE(o.isFilled());

    o.fill(Quantity{ 6 });
    EXPECT_TRUE(o.isFilled());
}

TEST(OrderBuilderMarket, PriceDefaultsToInvalid) {
    // Market order's price overrides any manual .price() call to InvalidPrice.
    // Arguably better to throw if price set on market — leaving as-is for now.
    Order marketWithPrice = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .market()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    EXPECT_EQ(marketWithPrice.getPrice(), InvalidPrice);
}

TEST(OrderBuilderBuild, ThrowsWhenMissingParameters) {
    OrderBuilder missingId = OrderBuilder{}
    .buy().goodTillCancel().price(Price{ 100 }).quantity(Quantity{ 10 });

    OrderBuilder missingSide = OrderBuilder{}
    .id(OrderId{ 1 }).goodTillCancel().price(Price{ 100 }).quantity(Quantity{ 10 });

    OrderBuilder missingPrice = OrderBuilder{}
    .id(OrderId{ 1 }).buy().goodTillCancel().quantity(Quantity{ 10 });

    OrderBuilder missingQuantity = OrderBuilder{}
    .id(OrderId{ 1 }).buy().goodTillCancel().price(Price{ 100 });

    OrderBuilder marketMissingQuantity = OrderBuilder{}
    .id(OrderId{ 1 }).buy().market();

    EXPECT_THROW(missingId.build(), std::logic_error);
    EXPECT_THROW(missingSide.build(), std::logic_error);
    EXPECT_THROW(missingPrice.build(), std::logic_error);
    EXPECT_THROW(missingQuantity.build(), std::logic_error);
    EXPECT_THROW(marketMissingQuantity.build(), std::logic_error);
}

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
