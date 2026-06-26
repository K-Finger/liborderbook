# Benchmarks

Latency of the core operations, measured with [Google Benchmark](https://github.com/google/benchmark) across each optimization stage.

## Environment

- 20 cores @ 2.99 GHz
- L1d 48 KiB · L2 1280 KiB · L3 24 MiB
- MSVC, Release `/O2` + IPO + `/arch:AVX2`
- Single threaded

Each operation is timed in a steady-state book. A batch of pre-built orders is replayed; timing pauses to rebuild the book between batches so allocation and teardown are excluded from the measured path.

## What each benchmark measures

| Benchmark              | Measures                                                          |
| ---------------------- | ---------------------------------------------------------------- |
| `AddOrder_PreBuilt`    | Resting insert with no match (book grows)                        |
| `AddOrder_SingleMatch` | Aggressor fully matches one resting order; both removed          |
| `AddOrder_AtDepth/N`   | Insert against a book already holding `N` price levels           |
| `AddOrder_Scatter/N`   | Insert at random prices across `N` levels (forces tree descent)  |
| `CancelOrder`          | Locate by id and unlink from its level                           |
| `GetLevelInfos/N`      | Snapshot all `N` levels of aggregated depth                      |

## Stage progression — AddOrder hot path (ns)

| Stage                          | PreBuilt | SingleMatch | AtDepth/0 | AtDepth/10000 | Cancel |
| ------------------------------ | -------: | ----------: | --------: | ------------: | -----: |
| v1 — `std::list` + `shared_ptr`|      170 |         357 |       115 |           121 |    115 |
| v2 — intrusive list            |     91.7 |         222 |      74.9 |          84.1 |   94.0 |
| **v2.1 — slab allocator**      | **67.0** |     **143** |  **54.4** |      **58.3** | **75.3** |

## v1 — baseline

`std::map` per side, `std::list<Order*>` per price level, `std::shared_ptr<Order>` ownership.

Each add allocates a `std::list` node; each level walk costs two cache hops (`*begin()` → node → `Order*`). Ownership via `shared_ptr` adds a control block and atomic refcount per order.

```
AddOrder_PreBuilt        170 ns
AddOrder_SingleMatch     357 ns
AddOrder_AtDepth/0       115 ns
AddOrder_AtDepth/10000   121 ns
CancelOrder              115 ns
```

## v2 — intrusive linked list

`Order` carries its own `prev`/`next`; `PriceLevel` holds `head`/`tail`. No per-add list-node allocation, and `level->head` is a single load instead of two. `shared_ptr` swapped for `unique_ptr`, removing the control block.

Combined drop of ~65 ns on the resting path.

```
AddOrder_PreBuilt       91.7 ns
AddOrder_SingleMatch     222 ns
AddOrder_AtDepth/0      74.9 ns
AddOrder_AtDepth/10000  84.1 ns
CancelOrder             94.0 ns
```

`GetLevelInfos` regressed here: the `unique_ptr` storage vector grew unbounded as cancels and fills leaked slots, fragmenting the heap.

## v2.1 — slab allocator

`Order`s live in a preallocated slab with a LIFO free list. Allocation is a free-list pop or a high-water bump; release runs the destructor and pushes the slot back. Orders stay contiguous and a just-freed slot is hot in cache on reuse.

```
AddOrder_PreBuilt       67.0 ns
AddOrder_SingleMatch     143 ns
AddOrder_AtDepth/0      54.4 ns
AddOrder_AtDepth/10000  58.3 ns
CancelOrder             75.3 ns
GetLevelInfos/10        65.1 ns
GetLevelInfos/10000   50398 ns
```

The slab cut the resting path another ~27% and fixed the `GetLevelInfos` regression. It also exposed a measurement caveat: earlier benchmarks inserted at a single price, keeping one `PriceLevel` pinned in L1. `AddOrder_Scatter` was added to force tree descent across many levels:

```
AddOrder_Scatter/100     90.0 ns
AddOrder_Scatter/1000     105 ns
AddOrder_Scatter/10000    203 ns
```

## Next

The `std::map` price index is the remaining bottleneck — a red-black tree with pointer chasing and poor cache locality, most visible in `Scatter/10000` and `GetLevelInfos`. The planned step is an array-indexed price ladder for O(1) level access over a bounded tick range.
