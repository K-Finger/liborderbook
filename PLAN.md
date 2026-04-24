## Production-Grade Limit Order Book — Action Plan

### Goal

Build a production-quality C++ limit order book that supports multiple order types, price-time priority matching, fast cancellation, partial fills, audit-grade trade tracking, and observability hooks. Code targets correctness, low-latency hot paths, and testability — not toy-project minimums.

### Core requirements

* Two-sided book

  * bids = buy orders, sorted highest price first
  * asks = sell orders, sorted lowest price first
  * separate containers because each side has opposite priority

* Order identity and shape

  * unique order id (monotonic, 64-bit)
  * side (Buy / Sell)
  * type (GoodTillCancel, Market, FillOrKill, ImmediateOrCancel, GoodForDay, optional Stop variants)
  * price (integer cents, signed to allow sentinel for market orders)
  * initial quantity (immutable after creation, needed for audit + fill ratio)
  * remaining quantity (mutates on fills)
  * timestamp (exchange-assigned, nanosecond precision, monotonic)
  * client id (who submitted)

* Price-time priority

  * better price matches first
  * for equal prices, older orders match first
  * timestamp is authoritative; FIFO queue inside a price level handles intra-level ordering

* Order entry

  * accept GTC, Market, FOK, IOC, GFD
  * validate on entry (positive quantity, valid type, no duplicate id, price within sanity bounds)
  * route Buy -> bid book, Sell -> ask book
  * trigger match cycle after every accepted order

* Matching engine

  * while best bid >= best ask, generate trades
  * fill at the resting order's price (price improvement to incoming aggressor)
  * support partial fills, decrement `remainingQuantity_` via `Order::Fill`
  * remove fully-filled orders from book and lookup map atomically
  * Market orders cross until filled or book exhausted; convert remainder to GTC only if order type rules allow
  * FOK rejects entirely if cannot fully fill on entry
  * IOC fills available, kills remainder (no rest)

* Cancellation

  * O(1) cancel by order id given lookup map -> iterator into price-level list
  * erase from list, erase from lookup map, drop empty price levels
  * cancel of unknown id returns failure status, does not throw

* Trade record

  * buy order id, sell order id, trade price, trade quantity, timestamp
  * append-only audit log
  * exposed via `getTrades()` for replay / reconciliation

* Observability

  * top of book (best bid, best ask, sizes)
  * full depth (all price levels with aggregated quantity)
  * formatter via `std::formatter` specialization (C++20), not baked into Order class
  * separate `OrderFormat.h` so Order.h stays free of `<iostream>` / `<format>` for hot-path callers

### Three-level hierarchy

Canonical fast-book layout: `Book -> PriceLevel -> Order`.

* `Book` owns two sides (bids, asks), cached `bestBid` / `bestAsk` pointers, the order-id lookup, and the stop-order structure.
* `PriceLevel` holds a price, a FIFO doubly-linked list of resting Orders, and aggregate size/count for depth queries.
* `Order` is a node in its PriceLevel's list — intrusive `prev`/`next` pointers live on Order itself (v2). Carries id, side, type, price, initial/remaining qty, timestamp, client id, and a back-pointer to its PriceLevel for O(1) cancel.

Cached best-price pointers are the single biggest BBO optimization: `begin()` on a tree is O(log n) worst case and cache-cold; a cached pointer is one load. Maintained on:

* insert above current best (new level becomes the best)
* drain to zero (scan outward to next non-empty level, past cached empties)

### Data structures

**v1 — std::map baseline (correct, benchmarked, swap later):**

* bids: `std::map<Price, PriceLevel, std::greater<Price>>` — descending, `begin()` = best bid
* asks: `std::map<Price, PriceLevel>` — ascending, `begin()` = best ask
* lookup: `std::unordered_map<OrderId, OrderEntry>` where `OrderEntry` = `{OrderPointer, list::iterator}` for O(1) cancel
* price-level list: `std::list<OrderPointer>` (O(1) middle erase given iterator)
* trades: `std::vector<Trade>`, reserved upfront

**v2 — fastest path (target after v1 benchmarks land):**

* price levels keyed by *price-indexed array* when tick space is bounded: `std::vector<PriceLevel*>` sized to `(maxPrice - minPrice) / tickSize`. O(1) access at any price, O(1) insert at existing level, zero tree descent. Empty levels stay allocated — reuse on next order at that price.
* fall back to *open-addressed hash* (e.g. robin hood, flat map) if tick space too wide or sparse. Never `std::unordered_map` in the hot path — node-based, cache-hostile.
* price-ordered view for matching: separate singly-linked list of occupied levels per side, sorted by price. Maintained on level-creation / last-order-drain. Walked from cached best during match.
* order-id lookup: open-addressed hash keyed on `OrderId.value`. Load factor capped to keep probe chains short.
* `PriceLevel::orders` = intrusive doubly-linked list with head/tail. Order nodes are allocated from a slab (see memory model). No `std::list`, no `shared_ptr` indirection in steady state.

### Stop orders

Separate structure from the limit book — stops don't rest on the bid/ask ladder.

* `stopBids` / `stopAsks` indexed by trigger price
* on every trade, check whether last-trade price crossed any stop thresholds
* triggered stops convert to market (or stop-limit -> limit) and re-enter the normal add path
* deferred past v1: enum reserves the slot, impl lands with Market order type stable

### Memory model

**v1** — `std::shared_ptr<Order>`. Same order referenced from the price-level list and the id lookup without duplication or dangling. `Order` copy deleted (id is identity). Moves allowed. Price-level lists are `std::list` — O(1) middle erase given iterator, pay the cache cost for correctness first.

**v2** — intrusive + slab allocated:

* Order carries its own `prev`/`next` pointers; list has no separate node wrapper. One cache line per Order vs two hops (`std::list::_Node` + Order) under shared_ptr.
* Slab allocator (fixed-size pool of Order slots, free list on cancel) — zero heap traffic in hot path, orders pack contiguously for prefetch-friendly walks when matching deep.
* Ownership becomes raw Order* held by PriceLevel list + id hash. Slab owns lifetime; cancel + full-fill release back to the free list.
* Measured payoff drives the swap — v1 benchmarks define the baseline, v2 is not speculative optimization.

### File layout

* `Types.h`          — strong-typedef aliases (`OrderId`, `ClientId`, `TradeId`, `Price`, `Quantity`, `Timestamp`) via tagged `StrongType<T, Tag>` template
* `Constants.h`      — `InvalidPrice`, tick size, sanity bounds
* `Side.h`           — `enum class Side : uint8_t { Buy, Sell }`
* `OrderType.h`      — `enum class OrderType : uint8_t { GoodTillCancel, Market, FillOrKill, ImmediateOrCancel, GoodForDay }`
* `Order.h`          — `Order` class declaration
* `Order.cpp`        — `Order` method definitions (Fill, type transitions)
* `Trade.h`          — `Trade` struct + `TradeInfo` aggregate
* `OrderBook.h`      — `OrderBook` class declaration
* `OrderBook.cpp`    — add, match, cancel, modify, depth queries
* `OrderFormat.h`    — `std::formatter` specializations for Order, Trade, OrderBook
* `main.cpp`         — driver / smoke tests
* `tests/`           — unit tests (Catch2 or GoogleTest)
* `bench/`           — microbenchmarks (Google Benchmark)
* `CMakeLists.txt`   — build, test, bench targets

### Order class contract

* private fields, public getters returning by value
* primary constructor takes all fields, init-list assignment, no body work
* delegating constructor for Market orders (omits price, plugs `Constants::InvalidPrice`)
* `Fill(Quantity)` — validates `q <= remainingQuantity_`, throws `std::logic_error` on over-fill
* `IsFilled()` — `remainingQuantity_ == 0`
* `FilledQuantity()` — `initialQuantity_ - remainingQuantity_`
* `ToGoodTillCancel(Price)` — converts Market to GTC; throws if current type is not Market
* no setters for id, side, timestamp — immutable post-construction
* not thread-safe; OrderBook owns synchronization

### OrderBook class contract

* `AddOrder(OrderPointer)` returns `Trades` (vector of fills generated)
* `CancelOrder(OrderId)` returns bool
* `ModifyOrder(OrderModify)` — cancel + re-add with new price/quantity (preserves no priority, by design)
* `Size()` — total resting orders
* `GetOrderInfos()` — depth snapshot (price + total qty per level, both sides)
* private: `CanMatch(Side, Price)`, `MatchOrders()`, `CanFullyFill(Side, Price, Quantity)` for FOK pre-check
* internal mutex for thread safety; document lock order

### Matching rules in detail

* Aggressor = incoming order. Resting = already-on-book order.
* Trade price = resting order price (price improvement to aggressor).
* Trade quantity = `min(aggressor.remaining, resting.remaining)`.
* On full fill, erase resting order from list + lookup. Price level is *not* destroyed — leave the PriceLevel slot alive for reuse, update cached best pointer to scan to next non-empty level. (v1 `std::map` may erase the node; v2 price-indexed array keeps it.)
* Continue while aggressor has remaining qty AND `CanMatch(side, price)` true — match loop advances via cached best pointer, never a fresh `begin()`.
* After match cycle, if aggressor still has qty:
  * GTC -> rest on book
  * Market -> reject remainder (or convert to GTC if explicitly allowed)
  * IOC -> drop remainder, no rest
  * FOK -> should never reach here (pre-check rejected at entry)

### Validation and errors

* Order entry validation: throw or return `OrderRejectReason` enum
* `Fill` over-quantity: throw `std::logic_error` (programmer error)
* Cancel of unknown id: return false (not exceptional)
* Hot path avoids exceptions; reserve them for genuine programmer errors

### Concurrency

* Single mutex inside OrderBook v1 (coarse but correct)
* Document lock order if multiple books later
* Lock-free / sharded design = future work; benchmark first

### Testing

* Unit tests cover every method, every order type, every rejection path
* Property tests for invariants: book never has crossed prices at rest, total resting qty matches sum of price levels, fills never exceed initial qty
* Replay tests: feed recorded order stream, verify trade output deterministic
* Test plan files in `tests/` mirror source layout

### Benchmarks and latency targets

Microbenchmarks drive every v1 -> v2 swap. No optimization lands without a measured win.

* AddOrder latency (no-match, single-match, deep-match)
* CancelOrder latency
* Match step per trade
* Throughput orders/sec under mixed load
* Memory footprint per resting order
* BBO query latency (should be one load after v2)

Rough v2 targets (informed by published fast-book numbers, not a promise):

* AddOrder at existing level: ~100-200 ns
* CancelOrder: ~100 ns
* Match per trade: ~400-500 ns
* BBO query: < 10 ns

### Build order

1. `Types.h`, `Constants.h`, `Side.h`, `OrderType.h` — primitives (strong typedefs + enums)
2. `Order.h` / `Order.cpp` — order class with Fill, type transitions
3. `Trade.h` — trade record
4. `OrderBook.h` skeleton — public API, v1 container members, cached best pointers, no logic
5. `OrderBook.cpp` — `AddOrder`, `CanMatch`, `MatchOrders` on std::map baseline
6. Cancellation path with id lookup map
7. Modify + advanced order types (FOK, IOC, GFD, Market)
8. `OrderFormat.h` — formatter specializations
9. `main.cpp` driver
10. Unit tests in parallel with each step
11. Benchmarks — establish v1 baseline
12. v2 swap: price-indexed array + intrusive list + slab allocator, only where benchmarks justify
13. Stop-order tree — after Market order type stable

### Out of scope v1

* networking / FIX gateway
* persistence / journaling
* multi-instrument / multi-book routing
* lock-free engine
* stop / iceberg / hidden orders (enum reserves slots, impl deferred)
* intrusive list + slab allocator + price-indexed array (v2 — lands behind benchmarks)

### What this project demonstrates

* market microstructure understanding (price-time priority, order types, partial fills)
* production C++ discipline (RAII, smart pointers, deleted copies, formatter separation, header hygiene)
* matching engine correctness under all order-type rules
* observability and audit-grade trade logging
* tested + benchmarked codebase, not a one-shot demo
