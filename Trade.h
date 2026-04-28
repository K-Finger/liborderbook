#pragma once

#include "Types.h"

/// @brief  Data record for an executed trade
struct Trade 
{
    TradeId id;
    OrderId buyOrderId;
    OrderId sellOrderId;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};