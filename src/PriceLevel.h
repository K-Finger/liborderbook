#pragma once

#include <cstddef>

#include "Order.h"
#include "Types.h"

namespace orderbook {

struct PriceLevel
{
    Price price;
    Quantity totalQuantity;
    Order* head{ nullptr };
    Order* tail{ nullptr };
    std::size_t orderCount{ 0 };
};

}
