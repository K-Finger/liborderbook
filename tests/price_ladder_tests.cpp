#include <vector>

#include <gtest/gtest.h>

#include "Order.h"
#include "OrderBook.h"
#include "PriceLadder.h"

using namespace orderbook;

namespace {

template<Side S>
std::vector<std::int64_t> walkPrices(const PriceLadder<S>& ladder) {
    std::vector<std::int64_t> prices;
    ladder.forEachFromBest([&prices](const PriceLevel& level) {
        prices.push_back(level.price.get());
        return true;
    });
    return prices;
}

}

TEST(PriceLadder, EmptyLadderHasNoBest) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    EXPECT_TRUE(ladder.empty());
    EXPECT_EQ(ladder.levelCount(), 0u);
    EXPECT_EQ(ladder.best(), nullptr);
    EXPECT_EQ(ladder.bandSize(), 101u);
}

TEST(PriceLadder, ContainsCoversInclusiveBand) {
    PriceLadder<Side::Buy> ladder{ Price{ -5 }, Price{ 5 } };

    EXPECT_TRUE(ladder.contains(Price{ -5 }));
    EXPECT_TRUE(ladder.contains(Price{ 0 }));
    EXPECT_TRUE(ladder.contains(Price{ 5 }));
    EXPECT_FALSE(ladder.contains(Price{ -6 }));
    EXPECT_FALSE(ladder.contains(Price{ 6 }));
}

TEST(PriceLadder, RejectsInvertedBand) {
    EXPECT_THROW((PriceLadder<Side::Buy>{ Price{ 10 }, Price{ 9 } }), std::invalid_argument);
}

TEST(PriceLadder, RejectsExcessivelyWideBand) {
    EXPECT_THROW((PriceLadder<Side::Buy>{ Price{ 0 }, Price{ 5'000'000'000 } }),
                 std::length_error);
}

TEST(PriceLadder, GetOrCreateStampsPriceAndIsIdempotent) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    PriceLevel& first = ladder.getOrCreate(Price{ 42 });
    EXPECT_EQ(first.price, Price{ 42 });
    EXPECT_EQ(ladder.levelCount(), 1u);

    first.totalQuantity = Quantity{ 7 };

    PriceLevel& again = ladder.getOrCreate(Price{ 42 });
    EXPECT_EQ(&again, &first);
    EXPECT_EQ(again.totalQuantity, Quantity{ 7 });
    EXPECT_EQ(ladder.levelCount(), 1u);
}

TEST(PriceLadder, FindReturnsNullForVacantAndOutOfBandPrices) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };
    ladder.getOrCreate(Price{ 50 });

    EXPECT_NE(ladder.find(Price{ 50 }), nullptr);
    EXPECT_EQ(ladder.find(Price{ 51 }), nullptr);
    EXPECT_EQ(ladder.find(Price{ 1000 }), nullptr);
    EXPECT_EQ(ladder.find(Price{ -1 }), nullptr);
}

TEST(PriceLadder, BuySideBestIsHighestPrice) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 30 });
    ladder.getOrCreate(Price{ 70 });
    ladder.getOrCreate(Price{ 50 });

    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 70 });
}

TEST(PriceLadder, SellSideBestIsLowestPrice) {
    PriceLadder<Side::Sell> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 30 });
    ladder.getOrCreate(Price{ 70 });
    ladder.getOrCreate(Price{ 50 });

    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 30 });
}

TEST(PriceLadder, ErasingBestPromotesNextLevel) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 30 });
    ladder.getOrCreate(Price{ 50 });
    ladder.getOrCreate(Price{ 70 });

    ladder.erase(Price{ 70 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 50 });

    ladder.erase(Price{ 50 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 30 });

    ladder.erase(Price{ 30 });
    EXPECT_EQ(ladder.best(), nullptr);
    EXPECT_TRUE(ladder.empty());
}

TEST(PriceLadder, ErasingSellBestPromotesNextLevel) {
    PriceLadder<Side::Sell> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 30 });
    ladder.getOrCreate(Price{ 50 });
    ladder.getOrCreate(Price{ 70 });

    ladder.erase(Price{ 30 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 50 });

    ladder.erase(Price{ 50 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 70 });
}

TEST(PriceLadder, ErasingInteriorLevelLeavesBestIntact) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 30 });
    ladder.getOrCreate(Price{ 50 });
    ladder.getOrCreate(Price{ 70 });

    ladder.erase(Price{ 50 });

    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 70 });
    EXPECT_EQ(ladder.levelCount(), 2u);
    EXPECT_EQ(walkPrices(ladder), (std::vector<std::int64_t>{ 70, 30 }));
}

TEST(PriceLadder, ErasingWorstLevelLeavesBestIntact) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 30 });
    ladder.getOrCreate(Price{ 50 });
    ladder.getOrCreate(Price{ 70 });

    ladder.erase(Price{ 30 });

    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 70 });
    EXPECT_EQ(walkPrices(ladder), (std::vector<std::int64_t>{ 70, 50 }));
}

TEST(PriceLadder, EraseIsNoOpForVacantAndOutOfBandPrices) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };
    ladder.getOrCreate(Price{ 50 });

    ladder.erase(Price{ 51 });
    ladder.erase(Price{ 9999 });
    ladder.erase(Price{ -9999 });

    EXPECT_EQ(ladder.levelCount(), 1u);
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 50 });
}

TEST(PriceLadder, ErasedLevelIsResetOnReuse) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    PriceLevel& level = ladder.getOrCreate(Price{ 50 });
    level.totalQuantity = Quantity{ 99 };
    level.orderCount = 3;

    ladder.erase(Price{ 50 });

    PriceLevel& reused = ladder.getOrCreate(Price{ 50 });
    EXPECT_EQ(reused.price, Price{ 50 });
    EXPECT_EQ(reused.totalQuantity, Quantity{ 0 });
    EXPECT_EQ(reused.orderCount, 0u);
    EXPECT_EQ(reused.head, nullptr);
}

TEST(PriceLadder, HandlesBandBoundaries) {
    PriceLadder<Side::Buy> ladder{ Price{ -10 }, Price{ 10 } };

    ladder.getOrCreate(Price{ -10 });
    ladder.getOrCreate(Price{ 10 });

    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 10 });
    EXPECT_EQ(walkPrices(ladder), (std::vector<std::int64_t>{ 10, -10 }));

    ladder.erase(Price{ 10 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ -10 });

    ladder.erase(Price{ -10 });
    EXPECT_TRUE(ladder.empty());
}

TEST(PriceLadder, WalksNegativePricesInOrder) {
    PriceLadder<Side::Sell> ladder{ Price{ -100 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 5 });
    ladder.getOrCreate(Price{ -60 });
    ladder.getOrCreate(Price{ -20 });

    EXPECT_EQ(walkPrices(ladder), (std::vector<std::int64_t>{ -60, -20, 5 }));
}

TEST(PriceLadder, ScansAcrossBitsetWords) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 1000 } };

    ladder.getOrCreate(Price{ 1 });
    ladder.getOrCreate(Price{ 300 });
    ladder.getOrCreate(Price{ 999 });

    EXPECT_EQ(walkPrices(ladder), (std::vector<std::int64_t>{ 999, 300, 1 }));

    ladder.erase(Price{ 999 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 300 });

    ladder.erase(Price{ 300 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 1 });
}

TEST(PriceLadder, ScansAcrossBitsetWordsOnSellSide) {
    PriceLadder<Side::Sell> ladder{ Price{ 0 }, Price{ 1000 } };

    ladder.getOrCreate(Price{ 1 });
    ladder.getOrCreate(Price{ 300 });
    ladder.getOrCreate(Price{ 999 });

    EXPECT_EQ(walkPrices(ladder), (std::vector<std::int64_t>{ 1, 300, 999 }));

    ladder.erase(Price{ 1 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 300 });

    ladder.erase(Price{ 300 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 999 });
}

TEST(PriceLadder, TraversalStopsWhenCallbackReturnsFalse) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 10 });
    ladder.getOrCreate(Price{ 20 });
    ladder.getOrCreate(Price{ 30 });

    std::vector<std::int64_t> visited;
    ladder.forEachFromBest([&visited](const PriceLevel& level) {
        visited.push_back(level.price.get());
        return level.price != Price{ 20 };
    });

    EXPECT_EQ(visited, (std::vector<std::int64_t>{ 30, 20 }));
}

TEST(PriceLadder, TraversalOnEmptyLadderVisitsNothing) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    int visits = 0;
    ladder.forEachFromBest([&visits](const PriceLevel&) {
        ++visits;
        return true;
    });

    EXPECT_EQ(visits, 0);
}

TEST(PriceLadder, RefillingAfterFullDrainRestoresBest) {
    PriceLadder<Side::Buy> ladder{ Price{ 0 }, Price{ 100 } };

    ladder.getOrCreate(Price{ 40 });
    ladder.erase(Price{ 40 });
    ASSERT_TRUE(ladder.empty());

    ladder.getOrCreate(Price{ 60 });
    ASSERT_NE(ladder.best(), nullptr);
    EXPECT_EQ(ladder.best()->price, Price{ 60 });
    EXPECT_EQ(ladder.levelCount(), 1u);
}

TEST(OrderBookPriceBand, RejectsLimitOrderAboveBand) {
    OrderBook book{ Price{ 1 }, Price{ 100 } };
    Order tooHigh = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 101 })
        .quantity(Quantity{ 10 })
        .build();

    auto trades = book.addOrder(std::move(tooHigh));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookPriceBand, RejectsLimitOrderBelowBand) {
    OrderBook book{ Price{ 1 }, Price{ 100 } };
    Order tooLow = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 0 })
        .quantity(Quantity{ 10 })
        .build();

    auto trades = book.addOrder(std::move(tooLow));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookPriceBand, AcceptsOrdersAtBandEdges) {
    OrderBook book{ Price{ 1 }, Price{ 100 } };

    Order atFloor = OrderBuilder{}
        .id(OrderId{ 1 })
        .buy()
        .goodTillCancel()
        .price(Price{ 1 })
        .quantity(Quantity{ 10 })
        .build();

    Order atCeiling = OrderBuilder{}
        .id(OrderId{ 2 })
        .sell()
        .goodTillCancel()
        .price(Price{ 100 })
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(atFloor));
    book.addOrder(std::move(atCeiling));

    EXPECT_EQ(book.size(), 2u);

    auto infos = book.getLevelInfos();
    ASSERT_EQ(infos.bids.size(), 1u);
    ASSERT_EQ(infos.asks.size(), 1u);
    EXPECT_EQ(infos.bids.front().price, Price{ 1 });
    EXPECT_EQ(infos.asks.front().price, Price{ 100 });
}

TEST(OrderBookPriceBand, MarketOrderIsExemptFromBand) {
    OrderBook book{ Price{ 1 }, Price{ 100 } };

    Order restingSell = OrderBuilder{}
        .id(OrderId{ 1 })
        .sell()
        .goodTillCancel()
        .price(Price{ 50 })
        .quantity(Quantity{ 10 })
        .build();

    Order marketBuy = OrderBuilder{}
        .id(OrderId{ 2 })
        .buy()
        .market()
        .quantity(Quantity{ 10 })
        .build();

    book.addOrder(std::move(restingSell));
    auto trades = book.addOrder(std::move(marketBuy));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades.front().quantity, Quantity{ 10 });
    EXPECT_EQ(book.size(), 0u);
}
