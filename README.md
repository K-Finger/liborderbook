# liborderbook - C++20 Limit Order Book Matching Engine

A C++20 matching engine with a *sub-100ns* add-order path on a single thread. Supports GTC, market, IOC and FOK orders.

## Usage

Needs CMake 3.15+ and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build --config Release
```

To link from another project:

```cmake
add_subdirectory(Limit-Order-Book)
target_link_libraries(your_app PRIVATE orderbook)
```

Then `#include "OrderBook.h"`.

```cpp
OrderBook book;

book.addOrder(OrderBuilder{}.id(OrderId{1}).buy().goodTillCancel()
                  .price(Price{100}).quantity(Quantity{10}).build());
book.addOrder(OrderBuilder{}.id(OrderId{2}).sell().goodTillCancel()
                  .price(Price{101}).quantity(Quantity{20}).build());

// market buy crosses at 101, fills 5 against order 2
auto trades = book.addOrder(OrderBuilder{}.id(OrderId{3}).buy().market()
                                .quantity(Quantity{5}).build());

book.cancelOrder(OrderId{1});
book.printBook();
// === ASKS ===
// 101 | 15 (1)
// -------------
// === BIDS ===
```

`OrderBuilder` validates required fields before returning an `Order`:

```cpp
OrderBuilder{}
    .id(OrderId{42})
    .buy()              // or .sell()
    .goodTillCancel()   // or .market() / .fillOrKill() / .immediateOrCancel()
    .price(Price{100})  // omit for market orders
    .quantity(Quantity{10})
    .build();
```

```cpp
std::vector<Trade>  addOrder(Order order);   // match + rest remainder, returns fills
bool                cancelOrder(OrderId id); // false if the id isn't on the book
std::size_t         size() const;            // live order count
OrderBookLevelInfos getLevelInfos() const;   // aggregated depth, both sides
const std::vector<Trade>& getTrades() const; // every trade so far
void                printBook();             // dump depth to stdout
```

```sh
./build/Release/orderbook_tests
```

## Benchmarks

Single thread, 20 cores at 2.99 GHz, MSVC Release with IPO and AVX2:

```
Add, no match          67 ns
Add, single match     143 ns
Add into 10k levels    58 ns
Cancel                 75 ns
```

Per-stage history in [BENCHMARKS.md](BENCHMARKS.md).

## Refs

- [How to Build a Fast Limit Order Book](https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/)
- [Limit Order Book in C++](https://alexabosi.wordpress.com/2014/08/28/limit-order-book-implementation-for-low-latency-trading-in-c/) — Abosi
- [brprojects/Limit-Order-Book](https://github.com/brprojects/Limit-Order-Book), [Tzadiko/Orderbook](https://github.com/Tzadiko/Orderbook/blob/master/Order.h)
- [Strong types with CRTP](https://youtu.be/fWcnp7Bulc8) — Fluent C++
- [TomaszRewak/cpp-allocator](https://github.com/TomaszRewak/cpp-allocator) — slab allocator

MIT
