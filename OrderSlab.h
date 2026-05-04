#pragma once
#include <cstddef>
#include <vector>
#include <utility>
#include <new>
#include "Order.h"

// A slab allocator for Orders. Replaces vector of unique pointers.
// https://medium.com/@kondam.reddy/a-deep-dive-into-building-a-memory-allocator-dd5333a98195
// https://medium.com/tuanhdotnet/understanding-the-high-water-mark-4b2611ddc050
// free list operates like a LIFO stack. Push on release. pop on allocate. last freed = first reused
// highwater only rises when free list is empty

// Caller must drain before destruction. slab does not destroy live orders

class OrderSlab
{
public:
    explicit OrderSlab(std::size_t capacity = 1'000'000)
        : storage_(capacity)
    {
    }

    template <typename... Args> // pack expansion for variadiac functions
    Order* allocate(Args&&... args) // && is a universal reference. Preserves lvalues & rvalues
    {
        Slot* slot;
        if (freeList_) {
            slot = freeList_; // pop head
            freeList_ = slot->nextFree; // update freeList_ head
        }
        else if (highWater_ < storage_.size()) {
            slot = &storage_[highWater_++]; // take highWater slot
        }
        else {
            throw std::bad_alloc{}; // out of slots
        }

        // bread and butter
        // builds a new order at address of (&slot->order) 
        // forwards parameters to order constructor
        return new (&slot->order) Order(std::forward<Args>(args)...);
    }
    void release(Order* order)
    {
        order->~Order();

        Slot* slot = reinterpret_cast<Slot*>(order); // reinterpret bytes as a slot 
        slot->nextFree = freeList_;
        freeList_ = slot;
    }

    // delete copy & move
    OrderSlab(const OrderSlab&) = delete;
    OrderSlab& operator=(const OrderSlab&) = delete;
    OrderSlab(OrderSlab&&) = default;
    OrderSlab& operator=(OrderSlab&&) = default;
private:
    // slot = region of memory for Order + pointer to next free slot
    union Slot {
        Order order;
        Slot* nextFree;
        Slot() {}
        ~Slot() {}
    };

    std::vector<Slot> storage_; // vector over array so we can chose reserve space at runtime time
    Slot* freeList_ = nullptr; // head of chain of freed slots
    std::size_t highWater_ = 0; // boundary between used region and never touched region 
};