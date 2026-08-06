#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "Order.h"
#include "OrderBook.h"

using namespace orderbook;

namespace fixtures {

inline constexpr Price BenchMinPrice{ 1 };
inline constexpr Price BenchMaxPrice{ 12'000 };
inline constexpr std::int64_t BenchMidPrice = 5'000;

inline constexpr std::size_t BenchOrderCapacity = 1'100'000;

inline OrderBook makeBook() {
    return OrderBook{ BenchMinPrice, BenchMaxPrice, BenchOrderCapacity };
}

inline std::uint64_t g_id = 0;

inline void resetIdCounter() noexcept { g_id = 0; }

inline Order makeBuy(Price price, Quantity qty) {
    return OrderBuilder{}
        .id(OrderId{ ++g_id })
        .buy()
        .goodTillCancel()
        .price(price)
        .quantity(qty)
        .build();
}

inline Order makeSell(Price price, Quantity qty) {
    return OrderBuilder{}
        .id(OrderId{ ++g_id })
        .sell()
        .goodTillCancel()
        .price(price)
        .quantity(qty)
        .build();
}

inline std::vector<Order> buildBuyBatch(std::size_t n, Price price = Price{ 100 }) {
    std::vector<Order> orders;
    orders.reserve(n);
    for (std::size_t k = 0; k < n; ++k) {
        orders.push_back(makeBuy(price, Quantity{ 10 }));
    }
    return orders;
}

inline std::vector<Order> buildSellBatch(std::size_t n, Price price = Price{ 100 }) {
    std::vector<Order> orders;
    orders.reserve(n);
    for (std::size_t k = 0; k < n; ++k) {
        orders.push_back(makeSell(price, Quantity{ 10 }));
    }
    return orders;
}

inline std::vector<Order> buildScatterBuyBatch(std::size_t n, std::int64_t priceRange) {
    std::mt19937 rng{ 42 };
    std::uniform_int_distribution<std::int64_t> dist{ 1, priceRange };

    std::vector<Order> orders;
    orders.reserve(n);
    for (std::size_t k = 0; k < n; ++k) {
        orders.push_back(makeBuy(Price{ dist(rng) }, Quantity{ 10 }));
    }
    return orders;
}

inline void populateDepth(OrderBook& book, std::int64_t depth) {
    for (std::int64_t k = 0; k < depth; ++k) {
        book.addOrder(makeBuy(Price{ k + 1 }, Quantity{ 10 }));
    }
}

inline void populateBothSides(OrderBook& book, std::int64_t depth) {
    for (std::int64_t k = 0; k < depth; ++k) {
        book.addOrder(makeBuy(Price{ BenchMidPrice - k }, Quantity{ 10 }));
        book.addOrder(makeSell(Price{ BenchMidPrice + 1 + k }, Quantity{ 10 }));
    }
}

} // namespace fixtures
