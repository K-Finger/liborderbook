#pragma once

#include <cstddef>

#include "Order.h"
#include "Types.h"

namespace orderbook {

struct PriceLevel
{
    Price price{ Price{ 0 } };
    Quantity totalQuantity{ Quantity{ 0 } };
    Order* head{ nullptr };
    Order* tail{ nullptr };
    std::size_t orderCount{ 0 };
};

}
