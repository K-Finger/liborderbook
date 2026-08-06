#include <cstdint>

#include <gtest/gtest.h>

#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

namespace {

constexpr std::size_t TinyPool = 4;
constexpr int Cycles = 200;

class PoolFixture : public ::testing::Test
{
protected:
    OrderBook book{ Price{ 1 }, Price{ 1000 }, TinyPool };
    std::uint64_t nextId{ 0 };

    Order limit(Side side, Price price, Quantity qty)
    {
        OrderBuilder builder;
        builder.id(OrderId{ ++nextId });
        (side == Side::Buy) ? builder.buy() : builder.sell();
        return builder.goodTillCancel().price(price).quantity(qty).build();
    }

    Order immediateOrCancel(Side side, Price price, Quantity qty)
    {
        OrderBuilder builder;
        builder.id(OrderId{ ++nextId });
        (side == Side::Buy) ? builder.buy() : builder.sell();
        return builder.immediateOrCancel().price(price).quantity(qty).build();
    }

    Order market(Side side, Quantity qty)
    {
        OrderBuilder builder;
        builder.id(OrderId{ ++nextId });
        (side == Side::Buy) ? builder.buy() : builder.sell();
        return builder.market().quantity(qty).build();
    }
};

}

TEST_F(PoolFixture, FullyFilledAggressorReturnsItsSlot)
{
    for (int cycle = 0; cycle < Cycles; ++cycle)
    {
        book.addOrder(limit(Side::Sell, Price{ 100 }, Quantity{ 10 }));
        auto trades = book.addOrder(limit(Side::Buy, Price{ 100 }, Quantity{ 10 }));

        ASSERT_EQ(trades.size(), 1u) << "cycle " << cycle;
    }

    EXPECT_EQ(book.size(), 0u);
}

TEST_F(PoolFixture, UnmatchedImmediateOrCancelReturnsItsSlot)
{
    for (int cycle = 0; cycle < Cycles; ++cycle)
    {
        auto trades = book.addOrder(immediateOrCancel(Side::Buy, Price{ 100 }, Quantity{ 10 }));

        ASSERT_TRUE(trades.empty()) << "cycle " << cycle;
    }

    EXPECT_EQ(book.size(), 0u);
}

TEST_F(PoolFixture, PartiallyFilledImmediateOrCancelReturnsItsSlot)
{
    for (int cycle = 0; cycle < Cycles; ++cycle)
    {
        book.addOrder(limit(Side::Sell, Price{ 100 }, Quantity{ 4 }));
        auto trades = book.addOrder(immediateOrCancel(Side::Buy, Price{ 100 }, Quantity{ 10 }));

        ASSERT_EQ(trades.size(), 1u) << "cycle " << cycle;
        ASSERT_EQ(trades.front().quantity, Quantity{ 4 }) << "cycle " << cycle;
    }

    EXPECT_EQ(book.size(), 0u);
}

TEST_F(PoolFixture, MarketOrderAgainstEmptyBookReturnsItsSlot)
{
    for (int cycle = 0; cycle < Cycles; ++cycle)
    {
        auto trades = book.addOrder(market(Side::Buy, Quantity{ 10 }));

        ASSERT_TRUE(trades.empty()) << "cycle " << cycle;
    }

    EXPECT_EQ(book.size(), 0u);
}

TEST_F(PoolFixture, RestingOrdersStillReclaimTheirSlotOnCancel)
{
    for (int cycle = 0; cycle < Cycles; ++cycle)
    {
        Order order = limit(Side::Buy, Price{ 100 }, Quantity{ 10 });
        const OrderId id = order.getOrderId();

        book.addOrder(std::move(order));
        ASSERT_TRUE(book.cancelOrder(id)) << "cycle " << cycle;
    }

    EXPECT_EQ(book.size(), 0u);
}
