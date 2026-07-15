#include <benchmark/benchmark.h>
#include "fixtures/book_fixtures.h"

using namespace fixtures;

// Cancel orders from a book with distinct price levels.
// Each cancel requires map lookup + unlink + potential level removal.
static void BM_CancelOrder(benchmark::State& state) {
    constexpr std::size_t kBatch = 10'000;

    OrderBook book;
    std::vector<OrderId> ids;
    ids.reserve(kBatch);

    auto refill = [&]() {
        book = OrderBook{};
        ids.clear();
        for (std::size_t k = 0; k < kBatch; ++k) {
            Order o = makeBuy(Price{ static_cast<std::int64_t>(k + 1) }, Quantity{ 10 });
            ids.push_back(o.getOrderId());
            book.addOrder(std::move(o));
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

        bool ok = book.cancelOrder(ids[i++]);
        benchmark::DoNotOptimize(ok);
    }
}
BENCHMARK(BM_CancelOrder);

// Cancel from middle of a price level's queue (not head/tail).
// Tests intrusive list unlink performance.
static void BM_CancelOrder_MidQueue(benchmark::State& state) {
    const std::int64_t queueDepth = state.range(0);
    constexpr std::size_t kBatch = 1'000;

    OrderBook book;
    std::vector<OrderId> midIds;
    midIds.reserve(kBatch);

    auto refill = [&]() {
        book = OrderBook{};
        midIds.clear();
        for (std::size_t k = 0; k < kBatch; ++k) {
            // Place queueDepth orders at same price
            for (std::int64_t q = 0; q < queueDepth; ++q) {
                Order o = makeBuy(Price{ 100 }, Quantity{ 10 });
                // Capture middle order's ID
                if (q == queueDepth / 2) {
                    midIds.push_back(o.getOrderId());
                }
                book.addOrder(std::move(o));
            }
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

        bool ok = book.cancelOrder(midIds[i++]);
        benchmark::DoNotOptimize(ok);
    }
}
BENCHMARK(BM_CancelOrder_MidQueue)->Arg(10)->Arg(100)->Arg(1000);

#ifndef BENCHMARK_COMBINED
BENCHMARK_MAIN();
#endif
