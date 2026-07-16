# liborderbook

A high-performance C++20 limit order book matching engine with sub-100ns add-order latency.

[![Build](https://github.com/K-Finger/liborderbook/actions/workflows/ci.yml/badge.svg)](https://github.com/K-Finger/liborderbook/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

## Features

- **Sub-100ns latency** on the critical path
- **Order types**: GTC, Market, IOC, FOK
- **Price-time priority** matching (FIFO)
- **Strong types** for compile-time safety
- **Slab allocator** for cache locality
- **Zero dependencies** beyond STL

## Quick Start

### Build

```sh
cmake -S . -B build
cmake --build build --config Release
```

### Integrate

```cmake
add_subdirectory(liborderbook)
target_link_libraries(your_app PRIVATE orderbook::orderbook)
```

### Use

```cpp
#include "OrderBook.h"

using namespace orderbook;

OrderBook book;

// Place limit orders
book.addOrder(OrderBuilder{}.id(OrderId{1}).buy().goodTillCancel()
    .price(Price{100}).quantity(Quantity{10}).build());
book.addOrder(OrderBuilder{}.id(OrderId{2}).sell().goodTillCancel()
    .price(Price{101}).quantity(Quantity{20}).build());

// Market order crosses the spread
auto trades = book.addOrder(OrderBuilder{}.id(OrderId{3}).buy().market()
    .quantity(Quantity{5}).build());

book.cancelOrder(OrderId{1});
book.printBook();
```

Output:

```
=== ASKS ===
101 | 15 (1)
-------------
=== BIDS ===
```

## API

### OrderBuilder

```cpp
OrderBuilder{}
    .id(OrderId{42})
    .buy()              // or .sell()
    .goodTillCancel()   // or .market() / .fillOrKill() / .immediateOrCancel()
    .price(Price{100})  // omit for market orders
    .quantity(Quantity{10})
    .build();
```

### OrderBook

```cpp
std::vector<Trade>  addOrder(Order order);   // Match + rest remainder
bool                cancelOrder(OrderId id); // Remove resting order
std::size_t         size() const;            // Live order count
OrderBookLevelInfos getLevelInfos() const;   // Aggregated depth
const std::vector<Trade>& getTrades() const; // Trade history
void                printBook();             // Debug output
```

## Benchmarks

Single thread, MSVC Release with IPO and AVX2:

| Operation | Latency |
|-----------|---------|
| Add, no match | 67 ns |
| Add, single match | 143 ns |
| Add into 10k levels | 58 ns |
| Cancel | 75 ns |

```sh
./build/Release/orderbook_bench_all
```

See [BENCHMARKS.md](docs/BENCHMARKS.md) for history.

## Tests

```sh
cmake --build build --target orderbook_tests
./build/Release/orderbook_tests
```

## Documentation

| Document | Description |
|----------|-------------|
| [Introduction](docs/introduction.md) | Architecture and design |
| [Installation](docs/installation.md) | Build and integration |
| [Guide](docs/guide.md) | Core concepts and operations |
| [Examples](docs/examples.md) | Usage patterns |
| [Support](docs/support.md) | FAQ, contributing, license |

## References

- [How to Build a Fast Limit Order Book](https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/)
- [Limit Order Book in C++](https://alexabosi.wordpress.com/2014/08/28/limit-order-book-implementation-for-low-latency-trading-in-c/) — Abosi
- [brprojects/Limit-Order-Book](https://github.com/brprojects/Limit-Order-Book)
- [Strong types with CRTP](https://youtu.be/fWcnp7Bulc8) — Fluent C++
- [TomaszRewak/cpp-allocator](https://github.com/TomaszRewak/cpp-allocator) — slab allocator

## License

MIT
