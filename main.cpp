#include "OrderBook.h"
#include "Order.h"

int main()
{
    OrderBook book;

    Order buy1{
        OrderType::GoodTillCancel,
        OrderId{1},
        Side::Buy,
        Price{100},
        Quantity{10},
        Timestamp{ std::chrono::nanoseconds{0} }
    };

    Order sell1{
        OrderType::GoodTillCancel,
        OrderId{2},
        Side::Sell,
        Price{101},
        Quantity{20},
        Timestamp{ std::chrono::nanoseconds{0} }
    };

    Order buy2{
        OrderType::GoodTillCancel,
        OrderId{3},
        Side::Buy,
        Price{102},
        Quantity{5},
        Timestamp{ std::chrono::nanoseconds{0} }
    };

    book.addOrder(std::move(buy1));
    book.addOrder(std::move(sell1));
    book.addOrder(std::move(buy2));
    book.cancelOrder(OrderId{ 3 });
    book.printBook();

    return 0;
}