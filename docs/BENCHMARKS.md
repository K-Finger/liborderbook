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

The std::map price index is the remaining bottleneck. Pointer chasing and poor cache locality. Planned: array-indexed price ladder for O(1) level access over bounded tick range.
