#include <benchmark/benchmark.h>
#include "fixtures/book_fixtures.h"

using namespace fixtures;

// Cancel orders from a book with distinct price levels.
// Each cancel requires map lookup + unlink + potential level removal.
static void BM_CancelOrder(benchmark::State& state) {
    constexpr std::size_t kBatch = 10'000;

    OrderBook book = makeBook();
    std::vector<OrderId> ids;
    ids.reserve(kBatch);

    // Every order placed here is cancelled again, so the book is already empty
    // on refill and the slab reuses its free list.
    auto refill = [&]() {
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
    const auto queueDepth = static_cast<std::size_t>(state.range(0));

    OrderBook book = makeBook();
    std::vector<OrderId> cancelOrderIds;

    auto refill = [&]() {
        std::vector<OrderId> queued;
        queued.reserve(queueDepth);
        for (std::size_t q = 0; q < queueDepth; ++q) {
            Order o = makeBuy(Price{ 100 }, Quantity{ 10 });
            queued.push_back(o.getOrderId());
            book.addOrder(std::move(o));
        }

        // Walk outwards from the middle so each unlink hits the interior of
        // the queue rather than its head or tail.
        cancelOrderIds.clear();
        cancelOrderIds.reserve(queueDepth);
        std::size_t low = queueDepth / 2;
        std::size_t high = low + 1;
        cancelOrderIds.push_back(queued[low]);
        while (cancelOrderIds.size() < queueDepth) {
            if (high < queueDepth) {
                cancelOrderIds.push_back(queued[high++]);
            }
            if (cancelOrderIds.size() < queueDepth && low > 0) {
                cancelOrderIds.push_back(queued[--low]);
            }
        }
    };

    refill();
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == cancelOrderIds.size()) {
            state.PauseTiming();
            refill();
            i = 0;
            state.ResumeTiming();
        }

        bool ok = book.cancelOrder(cancelOrderIds[i++]);
        benchmark::DoNotOptimize(ok);
    }
}
BENCHMARK(BM_CancelOrder_MidQueue)->Arg(10)->Arg(100)->Arg(1000);

#ifndef BENCHMARK_COMBINED
BENCHMARK_MAIN();
#endif
