# Limit Order Book

A C++20 limit order matching engine that handles multiple order types in sub-100ns

## v1 baseline (20-core 2.99 GHz, MSVC Release + IPO + AVX2)

`std::map` per side, `std::list<Order*>` per price level, `std::shared_ptr<Order>` ownership.

```
BM_AddOrder_PreBuilt           170 ns   (add to growing book, no match)
BM_AddOrder_SingleMatch        357 ns   (resting + aggressor pair, both removed)
BM_AddOrder_AtDepth/0          115 ns
BM_AddOrder_AtDepth/100        118 ns
BM_AddOrder_AtDepth/1000       114 ns
BM_AddOrder_AtDepth/10000      121 ns
BM_CancelOrder                 115 ns
BM_GetLevelInfos/10           66.1 ns
BM_GetLevelInfos/100           452 ns
BM_GetLevelInfos/1000         4440 ns
BM_GetLevelInfos/10000       45313 ns
```

## v2 — intrusive linked list + unique_ptr ownership

Order carries its own `prev`/`next`; PriceLevel holds head/tail. No more `std::list::_Node` allocation per add, no second cache hop on level walks. `shared_ptr` swapped for `unique_ptr` in `orderStorage_` (slab allocator deferred).

```
BM_AddOrder_PreBuilt          91.7 ns
BM_AddOrder_SingleMatch        222 ns
BM_AddOrder_AtDepth/0         74.9 ns
BM_AddOrder_AtDepth/100       84.6 ns
BM_AddOrder_AtDepth/1000      97.4 ns
BM_AddOrder_AtDepth/10000     84.1 ns
BM_CancelOrder                94.0 ns
BM_GetLevelInfos/10           89.3 ns
BM_GetLevelInfos/100           528 ns
BM_GetLevelInfos/1000         5784 ns
BM_GetLevelInfos/10000       89149 ns
```

Match-path wins land where the plan predicted. AtDepth especially: `level->head` is one load instead of two (was `*list.begin()` -> list_node -> `Order*`), and the level walk itself is intrusive-cache-friendly.

`getLevelInfos` regressed. `orderStorage_` grows unbounded as a `vector<unique_ptr<Order>>` — every cancel and full fill leaks the slot.

## References

- [How to Build a Fast Limit Order Book](https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/) — howtohft.wordpress.com
- [Limit Order Book in C++](https://alexabosi.wordpress.com/2014/08/28/limit-order-book-implementation-for-low-latency-trading-in-c/) — Alex Abosi
- [github.com/brprojects/Limit-Order-Book](https://github.com/brprojects/Limit-Order-Book)
- [github.com/Tzadiko/Orderbook](https://github.com/Tzadiko/Orderbook/blob/master/Order.h)
- [Strong types via CRTP](https://youtu.be/fWcnp7Bulc8) — fluent C++
- [github.com/TomaszRewak/cpp-allocator](https://github.com/TomaszRewak/cpp-allocator) - slab allocator implementation
