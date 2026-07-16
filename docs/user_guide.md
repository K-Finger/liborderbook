# User Guide

Complete guide for using liborderbook in your trading systems.

## Table of Contents

- [Introduction](#introduction)
- [Installation](#installation)
- [Core Concepts](#core-concepts)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Running Tests](#running-tests)
- [Running Benchmarks](#running-benchmarks)

## Introduction

liborderbook is a high-performance limit order book matching engine for financial trading systems. It provides:

- Sub-100ns latency on the critical add-order path
- Multiple order types: GTC, Market, IOC, FOK
- Price-time priority matching (FIFO)
- Strong types for compile-time safety
- Slab allocator for cache locality

## Installation

### Requirements

- CMake 3.15+
- C++20 compiler (MSVC 19.29+, GCC 10+, Clang 10+)

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

```cpp
#include "OrderBook.h"
using namespace orderbook;
```

## Core Concepts

### Order Types

- **Good Till Cancel** (`.goodTillCancel()`) - Rests on book until filled or canceled
- **Market** (`.market()`) - Executes immediately at best available price, no resting
- **Immediate or Cancel** (`.immediateOrCancel()`) - Fill what's available, cancel remainder
- **Fill or Kill** (`.fillOrKill()`) - Fill entire quantity or reject completely

### Matching Rules

1. **Price priority**: Best price matches first
2. **Time priority**: FIFO at same price level
3. **Trade price**: Resting order's price

## API Reference

### OrderBuilder

```cpp
Order order = OrderBuilder{}
    .id(OrderId{42})          // Required
    .buy()                    // Required: .buy() or .sell()
    .goodTillCancel()         // Required: order type
    .price(Price{100})        // Required for limit orders
    .quantity(Quantity{10})   // Required
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

### Trade

```cpp
struct Trade {
    TradeId   id;
    OrderId   buyOrderId;
    OrderId   sellOrderId;
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;
};
```

### LevelInfo

```cpp
struct LevelInfo {
    Price       price;
    Quantity    quantity;
    std::size_t orderCount;
};

struct OrderBookLevelInfos {
    std::vector<LevelInfo> bids;
    std::vector<LevelInfo> asks;
};
```

## Examples

### Basic Usage

```cpp
OrderBook book;

book.addOrder(OrderBuilder{}.id(OrderId{1}).buy().goodTillCancel()
    .price(Price{100}).quantity(Quantity{10}).build());

book.addOrder(OrderBuilder{}.id(OrderId{2}).sell().goodTillCancel()
    .price(Price{101}).quantity(Quantity{20}).build());

auto trades = book.addOrder(OrderBuilder{}.id(OrderId{3}).buy().market()
    .quantity(Quantity{5}).build());

book.cancelOrder(OrderId{1});
book.printBook();
```

### IOC Order

```cpp
// Fill what's available, cancel rest
auto trades = book.addOrder(OrderBuilder{}.id(OrderId{1}).buy()
    .immediateOrCancel().price(Price{100}).quantity(Quantity{50}).build());
```

### FOK Order

```cpp
// Fill entire quantity or reject
auto trades = book.addOrder(OrderBuilder{}.id(OrderId{1}).buy()
    .fillOrKill().price(Price{100}).quantity(Quantity{50}).build());
```

### Query Depth

```cpp
auto infos = book.getLevelInfos();

for (const auto& level : infos.bids) {
    std::cout << level.price.v << " x " << level.quantity.v << "\n";
}
```

## Running Tests

```sh
./build/Release/orderbook_tests
```

## Running Benchmarks

```sh
./build/Release/orderbook_bench_all
```
