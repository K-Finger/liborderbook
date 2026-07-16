#include <gtest/gtest.h>
#include "Constants.h"
#include "Order.h"

using namespace orderbook;

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
