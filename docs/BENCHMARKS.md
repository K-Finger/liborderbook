# Benchmarks

Latency of core operations measured with [Google Benchmark](https://github.com/google/benchmark).

## Environment

- 20 cores @ 2.99 GHz
- L1d 48 KiB, L2 1280 KiB, L3 24 MiB
- MSVC Release /O2 + IPO + /arch:AVX2
- Single threaded

Each operation is timed in a steady-state book. A batch of pre-built orders is replayed; timing pauses to rebuild the book between batches so allocation and teardown are excluded.

## Current Results (v2.1)

```
AddOrder_PreBuilt       67.0 ns
AddOrder_SingleMatch     143 ns
AddOrder_AtDepth/0      54.4 ns
AddOrder_AtDepth/10000  58.3 ns
CancelOrder             75.3 ns
```

## What Each Benchmark Measures

- **AddOrder_PreBuilt** - Resting insert with no match (book grows)
- **AddOrder_SingleMatch** - Aggressor fully matches one resting order
- **AddOrder_AtDepth/N** - Insert against a book holding N price levels
- **CancelOrder** - Locate by id and unlink from its level

## Optimization History

### v1 - Baseline

`std::map` per side, `std::list<Order*>` per level, `std::shared_ptr` ownership.

Each add allocates a list node. Each level walk costs two cache hops. Ownership via shared_ptr adds a control block and atomic refcount per order.

```
AddOrder_PreBuilt        170 ns
AddOrder_SingleMatch     357 ns
AddOrder_AtDepth/0       115 ns
AddOrder_AtDepth/10000   121 ns
CancelOrder              115 ns
```

### v2 - Intrusive Linked List

Order carries its own prev/next. PriceLevel holds head/tail. No per-add list-node allocation. shared_ptr swapped for unique_ptr.

~65 ns drop on resting path.

```
AddOrder_PreBuilt       91.7 ns
AddOrder_SingleMatch     222 ns
AddOrder_AtDepth/0      74.9 ns
AddOrder_AtDepth/10000  84.1 ns
CancelOrder             94.0 ns
```

### v2.1 - Slab Allocator

Orders live in a preallocated slab with LIFO free list. Allocation is a free-list pop or high-water bump. Just-freed slots are hot in cache on reuse.

~27% improvement on resting path.

```
AddOrder_PreBuilt       67.0 ns
AddOrder_SingleMatch     143 ns
AddOrder_AtDepth/0      54.4 ns
AddOrder_AtDepth/10000  58.3 ns
CancelOrder             75.3 ns
```

## Running Benchmarks

```sh
./build/Release/orderbook_bench_all
```

## Next Steps

### v3 - Array-Indexed Price Ladder (planned)

The `std::map` price index is the remaining bottleneck. Every add, cancel and level-drain
pays a red-black tree descent: `try_emplace` on insert, `at` on cancel, `erase` when a level
empties. Each descent is a chain of dependent pointer loads into scattered heap nodes, so the
cost is cache misses rather than comparisons, and it grows with book depth.

The replacement is a dense ladder indexed directly by price.

**Layout.** One `PriceLevel` array per side, sized to a fixed tick band chosen at construction,
where `index = price - minPrice`. Locating a level becomes an add and a load. There is no tree,
no per-level node allocation, and no rebalancing.

**Occupancy bitset.** A dense array is mostly empty, so "which level is best now?" cannot be a
linear scan. Occupancy is mirrored into a bitset, 64 levels per word, scanned with
`std::countr_zero` / `std::countl_zero`. When the best level drains, finding the next populated
one is typically a single word test rather than a walk.

**Tracked extent.** The ladder remembers its best and worst occupied index. Traversals
(`getLevelInfos`, the FOK liquidity check) are bounded to the populated region instead of the
whole band, so a 10-level book never scans a 110k-level array.

**Side as a template parameter.** `PriceLadder<Side::Buy>` treats the highest occupied price as
best, `PriceLadder<Side::Sell>` the lowest. Resolving this at compile time removes a branch from
every best-level access and lets the matching code treat both sides symmetrically.

**Trade-off.** The band is bounded, so memory is `O(band)` rather than `O(levels)` and prices
outside the band are rejected — the same price-collar constraint real venues enforce. The
default band is deliberately wide enough to cover ordinary use and still fits in a few MB.

Expected: `CancelOrder` and `AddOrder_AtDepth` improve most, since those are the paths that
touch the price index on every operation. `AddOrder_PreBuilt` should improve less — it already
hits a cached best-level pointer.
