#include <benchmark/benchmark.h>
#include <cstdint>
#include <vector>
#include "OrderBook.h"
#include "Order.h"

// Run on (20 X 2995 MHz CPU s)
// CPU Caches:
//   L1 Data 48 KiB (x10)
//   L1 Instruction 32 KiB (x10)
//   L2 Unified 1280 KiB (x10)
//   L3 Unified 24576 KiB (x1)
// --------------------------------------------------------------------
// Benchmark                          Time             CPU   Iterations
// --------------------------------------------------------------------
// BM_AddOrder_PreBuilt             170 ns          163 ns      3446154
// BM_AddOrder_SingleMatch          357 ns          369 ns      1947826
// BM_AddOrder_AtDepth/0            115 ns          105 ns     11200000
// BM_AddOrder_AtDepth/100          118 ns          114 ns      6690133
// BM_AddOrder_AtDepth/1000         114 ns          119 ns      8960000
// BM_AddOrder_AtDepth/10000        121 ns          115 ns      8960000
// BM_CancelOrder                   115 ns          136 ns      8960000
// BM_GetLevelInfos/10             66.1 ns         67.0 ns     11200000
// BM_GetLevelInfos/100             452 ns          449 ns      1600000
// BM_GetLevelInfos/1000           4440 ns         4332 ns       165926
// BM_GetLevelInfos/10000         45313 ns        45200 ns        16593

// Want to get down to sub-100ns for addOrder for single threaded
// Look into chunked bitmaps and intrusive linkedlists
namespace {
    std::uint64_t g_id = 0; // not thread safe

    Order makeBuy(Price price, Quantity qty) {
        return OrderBuilder{}
            .id(OrderId{ ++g_id })
            .buy()
            .goodTillCancel()
            .price(price)
            .quantity(qty)
            .build();
    }

    Order makeSell(Price price, Quantity qty) {
        return OrderBuilder{}
            .id(OrderId{ ++g_id })
            .sell()
            .goodTillCancel()
            .price(price)
            .quantity(qty)
            .build();
    }

    std::vector<Order> buildBuyBatch(std::size_t n) {
        std::vector<Order> orders;
        orders.reserve(n);
        for (std::size_t k = 0; k < n; ++k) {
            orders.push_back(makeBuy(Price{ 100 }, Quantity{ 10 }));
        }
        return orders;
    }

    std::vector<Order> buildSellBatch(std::size_t n) {
        std::vector<Order> orders;
        orders.reserve(n);
        for (std::size_t k = 0; k < n; ++k) {
            orders.push_back(makeSell(Price{ 100 }, Quantity{ 10 }));
        }
        return orders;
    }
}

static void BM_AddOrder_PreBuilt(benchmark::State& state) {
    const std::size_t batch = 10'000;

    OrderBook book;
    std::vector<Order> orders = buildBuyBatch(batch);
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == batch) {
            state.PauseTiming();
            book = OrderBook{};
            orders = buildBuyBatch(batch);
            i = 0;
            state.ResumeTiming();
        }

        auto trades = book.addOrder(std::move(orders[i++]));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_AddOrder_PreBuilt);

static void BM_AddOrder_SingleMatch(benchmark::State& state) {
    const std::size_t batch = 10'000;

    OrderBook book;
    std::vector<Order> buys = buildBuyBatch(batch);
    std::vector<Order> sells = buildSellBatch(batch);

    std::size_t i = 0;

    for (auto _ : state) {
        if (i == batch) {
            state.PauseTiming();
            book = OrderBook{};
            buys = buildBuyBatch(batch);
            sells = buildSellBatch(batch);
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

namespace {
    // Pre-populates a book with `depth` resting buys at distinct prices.
    // Used by depth-scaling benchmarks to isolate the cost of operating on a deep tree.
    void populateDepth(OrderBook& book, std::int64_t depth) {
        for (std::int64_t k = 0; k < depth; ++k) {
            book.addOrder(makeBuy(Price{ k + 1 }, Quantity{ 10 }));
        }
    }
}

static void BM_AddOrder_AtDepth(benchmark::State& state) {
    const std::int64_t depth = state.range(0);
    const std::size_t batch = 1'000;

    OrderBook book;
    populateDepth(book, depth);
    std::vector<Order> aggressors = buildBuyBatch(batch);
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == batch) {
            state.PauseTiming();
            book = OrderBook{};
            populateDepth(book, depth);
            aggressors = buildBuyBatch(batch);
            i = 0;
            state.ResumeTiming();
        }

        auto trades = book.addOrder(std::move(aggressors[i++]));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_AddOrder_AtDepth)->Arg(0)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_CancelOrder(benchmark::State& state) {
    const std::size_t batch = 10'000;

    OrderBook book;
    std::vector<OrderId> ids;
    ids.reserve(batch);

    auto refill = [&]() {
        book = OrderBook{};
        ids.clear();
        for (std::size_t k = 0; k < batch; ++k) {
            Order o = makeBuy(Price{ static_cast<std::int64_t>(k + 1) }, Quantity{ 10 });
            ids.push_back(o.getOrderId());
            book.addOrder(std::move(o));
        }
        };

    refill();
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == batch) {
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

static void BM_GetLevelInfos(benchmark::State& state) {
    const std::int64_t depth = state.range(0);

    OrderBook book;
    populateDepth(book, depth);

    for (auto _ : state) {
        auto infos = book.getLevelInfos();
        benchmark::DoNotOptimize(infos);
    }
}
BENCHMARK(BM_GetLevelInfos)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);