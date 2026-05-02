# Limit Order Book

A C++20 limit order matching engine that handles multiple order types in ~165ns

## v1 baseline (sample run, 20-core 2.99 GHz, MSVC Release + IPO + AVX2)

```

BM_AddOrder_PreBuilt 165 ns (add to growing book, no match)
BM_AddOrder_SingleMatch 322 ns (resting + aggressor pair, both removed)
BM_AddOrder_AtDepth/0 ~165 ns (baseline)
BM_AddOrder_AtDepth/100 ~ ... ns
BM_AddOrder_AtDepth/1000 ~ ... ns
BM_AddOrder_AtDepth/10000 ~ ... ns (rerun to fill in scaling row)
BM_CancelOrder ~ ... ns
BM_GetLevelInfos/10 ~ ... ns
BM_GetLevelInfos/10000 ~ ... ns


## References

- [How to Build a Fast Limit Order Book](https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/) — howtohft.wordpress.com
- [Limit Order Book in C++](https://alexabosi.wordpress.com/2014/08/28/limit-order-book-implementation-for-low-latency-trading-in-c/) — Alex Abosi
- [github.com/brprojects/Limit-Order-Book](https://github.com/brprojects/Limit-Order-Book)
- [github.com/Tzadiko/Orderbook](https://github.com/Tzadiko/Orderbook/blob/master/Order.h)
- [Strong types via CRTP](https://youtu.be/fWcnp7Bulc8) — fluent C++
```
