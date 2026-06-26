#include <benchmark/benchmark.h>
#include <cstdint>
#include <random>
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

// V2 - Use Intrusive Linked List for orders instead of shared pointers
// --------------------------------------------------------------------
// Benchmark                          Time             CPU   Iterations
// --------------------------------------------------------------------
// BM_AddOrder_PreBuilt            91.7 ns         97.5 ns      5767356
// BM_AddOrder_SingleMatch          222 ns          219 ns      2635294
// BM_AddOrder_AtDepth/0           74.9 ns         71.1 ns     11200000
// BM_AddOrder_AtDepth/100         84.6 ns         74.0 ns      8028160
// BM_AddOrder_AtDepth/1000        97.4 ns         85.8 ns      7466667
// BM_AddOrder_AtDepth/10000       84.1 ns         84.4 ns     10000000
// BM_CancelOrder                  94.0 ns          105 ns      7466667
// BM_GetLevelInfos/10             89.3 ns         87.9 ns      7466667
// BM_GetLevelInfos/100             528 ns          531 ns      1000000
// BM_GetLevelInfos/1000           5784 ns         5781 ns       100000
// BM_GetLevelInfos/10000         89149 ns        85449 ns         8960

// Intrusive list eliminates per-add std::list node allocation and a cache hop on level walks; 
// shared_ptr→unique_ptr removes the control block. Combined drop ~65ns
// GetLevelInfos regressed. likely heap fragmentation from orderStorage_ vector growth during populate

// v2.1.0 - Order Slab Allocator instead of unbounded orderStorage_ vector.
// --------------------------------------------------------------------
// Benchmark                          Time             CPU   Iterations
// --------------------------------------------------------------------
// BM_AddOrder_PreBuilt            67.0 ns         73.9 ns     11200000
// BM_AddOrder_SingleMatch          143 ns          122 ns      4480000
// BM_AddOrder_AtDepth/0           54.4 ns         54.7 ns     10000000
// BM_AddOrder_AtDepth/100         63.9 ns         62.8 ns     11200000
// BM_AddOrder_AtDepth/1000        56.6 ns         54.7 ns     10000000
// BM_AddOrder_AtDepth/10000       58.3 ns         53.1 ns     10000000
// BM_AddOrder_Scatter/100         90.0 ns          100 ns      5600000
// BM_AddOrder_Scatter/1000         105 ns          101 ns      8028160
// BM_AddOrder_Scatter/10000        203 ns          188 ns      4977778
// BM_CancelOrder                  75.3 ns         59.4 ns     10000000
// BM_GetLevelInfos/10             65.1 ns         64.1 ns     10000000
// BM_GetLevelInfos/100             441 ns          439 ns      1600000
// BM_GetLevelInfos/1000           4938 ns         4708 ns       149333
// BM_GetLevelInfos/10000         50398 ns        50000 ns        10000

// Note. I realized that the benchmarks up until this point have just been building at the same price
// PriceLevel could be always available in L1 Cache. skewing results
// Overall. the slab paid off massively. reduced prebuilt benchmark by ~27% and fixed the getLevelInfos regression

// v2.2.0 - Cached price pointers. More conditionals for maintaining cache.
// no real performance improvement besides single match.
// --------------------------------------------------------------------
// Benchmark                          Time             CPU   Iterations
// --------------------------------------------------------------------
// BM_AddOrder_PreBuilt            89.8 ns         85.4 ns      6400000
// BM_AddOrder_SingleMatch          184 ns          164 ns      4480000
// BM_AddOrder_AtDepth/0           63.9 ns         57.5 ns     11150223
// BM_AddOrder_AtDepth/100         74.1 ns         62.8 ns     11200000
// BM_AddOrder_AtDepth/1000        66.0 ns         73.9 ns     11200000
// BM_AddOrder_AtDepth/10000       72.2 ns         85.1 ns     11200000
// BM_AddOrder_Scatter/100          110 ns          100 ns      7466667
// BM_AddOrder_Scatter/1000         114 ns         99.3 ns      6291661
// BM_AddOrder_Scatter/10000        197 ns          243 ns      4181334
// BM_CancelOrder                 100.0 ns          110 ns      8960000
// BM_GetLevelInfos/10             76.9 ns         73.2 ns      8960000
// BM_GetLevelInfos/100             539 ns          502 ns      1493333
// BM_GetLevelInfos/1000           5350 ns         5000 ns       100000
// BM_GetLevelInfos/10000         56354 ns        51562 ns        10000

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

    std::vector<Order> buildScatterBuyBatch(std::size_t n, std::int64_t priceRange) {
        std::mt19937 rng{ 42 };
        std::uniform_int_distribution<std::int64_t> dist{ 1, priceRange };

        std::vector<Order> orders;
        orders.reserve(n);
        for (std::size_t k = 0; k < n; ++k) {
            orders.push_back(makeBuy(Price{ dist(rng) }, Quantity{ 10 }));
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

// Pre-populates book with `depth` levels, then aggressor buys hit random prices
// uniformly across [1, depth]. Unlike AtDepth (all aggressors at one price), this
// forces map descent on every add and prevents the destination PriceLevel from
// staying pinned in L1.
static void BM_AddOrder_Scatter(benchmark::State& state) {
    const std::int64_t depth = state.range(0);
    const std::size_t batch = 1'000;

    OrderBook book;
    populateDepth(book, depth);
    std::vector<Order> aggressors = buildScatterBuyBatch(batch, depth);
    std::size_t i = 0;

    for (auto _ : state) {
        if (i == batch) {
            state.PauseTiming();
            book = OrderBook{};
            populateDepth(book, depth);
            aggressors = buildScatterBuyBatch(batch, depth);
            i = 0;
            state.ResumeTiming();
        }

        auto trades = book.addOrder(std::move(aggressors[i++]));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_AddOrder_Scatter)->Arg(100)->Arg(1000)->Arg(10000);

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