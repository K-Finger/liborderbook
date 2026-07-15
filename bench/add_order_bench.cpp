#include <benchmark/benchmark.h>
#include "fixtures/book_fixtures.h"

using namespace fixtures;

// Measures raw add latency: pre-built orders, no matching, single price level.
// Best-case scenario for cache locality.
static void BM_AddOrder_NoMatch(benchmark::State& state) {
    constexpr std::size_t kBatch = 10'000;

    OrderBook book;
    auto orders = buildBuyBatch(kBatch);
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == kBatch) {
            state.PauseTiming();
            book = OrderBook{};
            orders = buildBuyBatch(kBatch);
            i = 0;
            state.ResumeTiming();
        }

        auto trades = book.addOrder(std::move(orders[i++]));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_AddOrder_NoMatch);

// Add + immediate single match. Measures full crossing-spread latency.
static void BM_AddOrder_SingleMatch(benchmark::State& state) {
    constexpr std::size_t kBatch = 10'000;

    OrderBook book;
    auto buys = buildBuyBatch(kBatch);
    auto sells = buildSellBatch(kBatch);
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == kBatch) {
            state.PauseTiming();
            book = OrderBook{};
            buys = buildBuyBatch(kBatch);
            sells = buildSellBatch(kBatch);
            i = 0;
            state.ResumeTiming();
        }

        auto buyTrades = book.addOrder(std::move(buys[i]));
        auto sellTrades = book.addOrder(std::move(sells[i]));
        ++i;
        benchmark::DoNotOptimize(buyTrades);
        benchmark::DoNotOptimize(sellTrades);
    }
}
BENCHMARK(BM_AddOrder_SingleMatch);

// Aggressive order sweeps multiple resting orders at same price.
static void BM_AddOrder_MultiMatch(benchmark::State& state) {
    const std::int64_t matchCount = state.range(0);
    constexpr std::size_t kBatch = 1'000;

    OrderBook book;
    std::vector<Order> aggressors;
    aggressors.reserve(kBatch);

    auto refill = [&]() {
        book = OrderBook{};
        aggressors.clear();
        for (std::size_t k = 0; k < kBatch; ++k) {
            // Place matchCount resting sells
            for (std::int64_t m = 0; m < matchCount; ++m) {
                book.addOrder(makeSell(Price{ 100 }, Quantity{ 10 }));
            }
            // Aggressive buy sweeps them all
            aggressors.push_back(
                OrderBuilder{}
                    .id(OrderId{ ++g_id })
                    .buy()
                    .goodTillCancel()
                    .price(Price{ 100 })
                    .quantity(Quantity{ static_cast<std::uint64_t>(10 * matchCount) })
                    .build()
            );
        }
    };

    refill();
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == kBatch) {
            state.PauseTiming();
            refill();
            i = 0;
            state.ResumeTiming();
        }

        auto trades = book.addOrder(std::move(aggressors[i++]));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_AddOrder_MultiMatch)->Arg(2)->Arg(5)->Arg(10);

#ifndef BENCHMARK_COMBINED
BENCHMARK_MAIN();
#endif
