#include <benchmark/benchmark.h>
#include "fixtures/book_fixtures.h"

using namespace fixtures;

// Add orders to a book with N existing price levels.
// All adds go to same price (best case for level lookup).
// Isolates map insertion cost as tree grows.
static void BM_Depth_AddAtBest(benchmark::State& state) {
    const std::int64_t depth = state.range(0);
    constexpr std::size_t kBatch = 1'000;

    OrderBook book = makeBook();
    populateDepth(book, depth);
    auto aggressors = buildBuyBatch(kBatch);
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == kBatch) {
            state.PauseTiming();
            book = makeBook();
            populateDepth(book, depth);
            aggressors = buildBuyBatch(kBatch);
            i = 0;
            state.ResumeTiming();
        }

        auto trades = book.addOrder(std::move(aggressors[i++]));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_Depth_AddAtBest)->Arg(0)->Arg(100)->Arg(1000)->Arg(10000);

// Add orders at random prices across the price range.
// Forces tree traversal on every add. Measures worst-case map descent.
static void BM_Depth_AddScatter(benchmark::State& state) {
    const std::int64_t depth = state.range(0);
    constexpr std::size_t kBatch = 1'000;

    OrderBook book = makeBook();
    populateDepth(book, depth);
    auto aggressors = buildScatterBuyBatch(kBatch, depth);
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == kBatch) {
            state.PauseTiming();
            book = makeBook();
            populateDepth(book, depth);
            aggressors = buildScatterBuyBatch(kBatch, depth);
            i = 0;
            state.ResumeTiming();
        }

        auto trades = book.addOrder(std::move(aggressors[i++]));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_Depth_AddScatter)->Arg(100)->Arg(1000)->Arg(10000);

// Matching that sweeps through multiple price levels.
// Aggressive buy crosses spread and walks up the ask ladder.
static void BM_Depth_SweepLevels(benchmark::State& state) {
    const std::int64_t levels = state.range(0);
    constexpr std::size_t kBatch = 500;

    OrderBook book = makeBook();
    std::vector<Order> aggressors;
    aggressors.reserve(kBatch);

    auto refill = [&]() {
        book = makeBook();
        aggressors.clear();
        resetIdCounter();

        for (std::size_t k = 0; k < kBatch; ++k) {
            // Build ask ladder: prices 101, 102, ..., 100+levels
            for (std::int64_t lvl = 0; lvl < levels; ++lvl) {
                book.addOrder(makeSell(Price{ 101 + lvl }, Quantity{ 10 }));
            }
            // Aggressive buy sweeps all levels
            aggressors.push_back(
                OrderBuilder{}
                    .id(OrderId{ ++g_id })
                    .buy()
                    .goodTillCancel()
                    .price(Price{ 100 + levels })
                    .quantity(Quantity{ static_cast<std::uint64_t>(10 * levels) })
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
BENCHMARK(BM_Depth_SweepLevels)->Arg(2)->Arg(5)->Arg(10)->Arg(20);

#ifndef BENCHMARK_COMBINED
BENCHMARK_MAIN();
#endif
