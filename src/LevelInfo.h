#pragma once
#include <cstddef>
#include <vector>
#include "Types.h"

namespace orderbook {

struct LevelInfo
{
    Price price;
    Quantity quantity;
    std::size_t orderCount;
};

struct OrderBookLevelInfos
{
    std::vector<LevelInfo> bids;
    std::vector<LevelInfo> asks;
};

}
