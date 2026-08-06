#pragma once
#include <cstddef>
#include <new>
#include <utility>
#include <vector>
#include "Order.h"

namespace orderbook {

class OrderSlab
{
public:
    explicit OrderSlab(std::size_t capacity = 1'000'000) : storage_(capacity) {}

    template<typename... Args>
    Order* allocate(Args&&... args)
    {
        Slot* slot = nullptr;
        if (freeList_ != nullptr)
        {
            slot = freeList_;
            freeList_ = slot->nextFree;
        }
        else if (highWater_ < storage_.size())
        {
            slot = &storage_[highWater_++];
        }
        else
        {
            throw std::bad_alloc{};
        }

        return new (&slot->order) Order(std::forward<Args>(args)...);
    }

    void release(Order* order)
    {
        order->~Order();

        Slot* slot = reinterpret_cast<Slot*>(order);
        slot->nextFree = freeList_;
        freeList_ = slot;
    }

    OrderSlab(const OrderSlab&) = delete;
    OrderSlab& operator=(const OrderSlab&) = delete;
    OrderSlab(OrderSlab&&) = default;
    OrderSlab& operator=(OrderSlab&&) = default;
    ~OrderSlab() = default;

private:
    union Slot
    {
        Order order;
        Slot* nextFree;

        Slot() : nextFree{ nullptr } {}
        Slot(const Slot&) = delete;
        Slot& operator=(const Slot&) = delete;
        Slot(Slot&&) = delete;
        Slot& operator=(Slot&&) = delete;
        ~Slot() {}
    };

    std::vector<Slot> storage_;
    Slot* freeList_ = nullptr;
    std::size_t highWater_ = 0;
};

}
