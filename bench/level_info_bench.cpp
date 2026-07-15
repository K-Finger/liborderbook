#include <benchmark/benchmark.h>
#include "fixtures/book_fixtures.h"

using namespace fixtures;

// Snapshot entire book. Measures tree traversal + vector allocation.
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

// Book with both bid and ask sides populated.
static void BM_GetLevelInfos_BothSides(benchmark::State& state) {
    const std::int64_t depth = state.range(0);

    OrderBook book;
    populateBothSides(book, depth);

    for (auto _ : state) {
        auto infos = book.getLevelInfos();
        benchmark::DoNotOptimize(infos);
    }
}
BENCHMARK(BM_GetLevelInfos_BothSides)->Arg(10)->Arg(100)->Arg(1000);

#ifndef BENCHMARK_COMBINED
BENCHMARK_MAIN();
#endif
