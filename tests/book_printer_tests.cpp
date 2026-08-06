#include <sstream>

#include <gtest/gtest.h>

#include "BookPrinter.h"
#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

namespace {

Order limit(std::uint64_t id, Side side, Price price, Quantity qty)
{
    OrderBuilder builder;
    builder.id(OrderId{ id });
    (side == Side::Buy) ? builder.buy() : builder.sell();
    return builder.goodTillCancel().price(price).quantity(qty).build();
}

}

TEST(BookPrinter, RendersAsksAscendingThenBidsDescending)
{
    OrderBook book;
    book.addOrder(limit(1, Side::Buy, Price{ 99 }, Quantity{ 5 }));
    book.addOrder(limit(2, Side::Buy, Price{ 98 }, Quantity{ 7 }));
    book.addOrder(limit(3, Side::Sell, Price{ 101 }, Quantity{ 3 }));
    book.addOrder(limit(4, Side::Sell, Price{ 102 }, Quantity{ 4 }));

    std::ostringstream out;
    printBook(out, book);

    EXPECT_EQ(out.str(),
              "=== ASKS ===\n"
              "101 | 3 (1)\n"
              "102 | 4 (1)\n"
              "-------------\n"
              "99 | 5 (1)\n"
              "98 | 7 (1)\n"
              "=== BIDS ===\n");
}

TEST(BookPrinter, RendersEmptyBook)
{
    const OrderBook book;

    std::ostringstream out;
    printBook(out, book);

    EXPECT_EQ(out.str(), "=== ASKS ===\n-------------\n=== BIDS ===\n");
}
