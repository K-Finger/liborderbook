#pragma once

#include <format>
#include <ostream>

#include "LevelInfo.h"
#include "OrderBook.h"

namespace orderbook {

inline void printBook(std::ostream& out, const OrderBook& book)
{
    const OrderBookLevelInfos levels = book.getLevelInfos();

    out << "=== ASKS ===\n";

    for (const LevelInfo& level : levels.asks)
    {
        out << std::format("{} | {} ({})\n", level.price.get(), level.quantity.get(),
                           level.orderCount);
    }

    out << "-------------\n";

    for (const LevelInfo& level : levels.bids)
    {
        out << std::format("{} | {} ({})\n", level.price.get(), level.quantity.get(),
                           level.orderCount);
    }

    out << "=== BIDS ===\n";
}

}
