#pragma once
#include "Types.h"

namespace orderbook {

struct Trade
{
    TradeId id;
    OrderId buyOrderId;
    OrderId sellOrderId;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};

}
