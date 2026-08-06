#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "Order.h"
#include "Types.h"

namespace orderbook {

class OrderIndex
{
public:
    explicit OrderIndex(std::size_t expectedOrders = 1024)
    {
        std::size_t capacity = MinCapacity;
        while (capacity * MaxLoadNumerator < (expectedOrders + 1) * MaxLoadDenominator)
        {
            capacity <<= 1U;
        }
        slots_.assign(capacity, Slot{});
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] std::size_t capacity() const noexcept { return slots_.size(); }

    [[nodiscard]] Order* find(OrderId id) const noexcept
    {
        const std::size_t mask = slots_.size() - 1;
        std::size_t index = hash(id) & mask;

        while (slots_[index].order != nullptr)
        {
            if (slots_[index].id == id)
            {
                return slots_[index].order;
            }
            index = (index + 1) & mask;
        }

        return nullptr;
    }

    [[nodiscard]] bool contains(OrderId id) const noexcept { return find(id) != nullptr; }

    void insert(OrderId id, Order* order)
    {
        if ((size_ + 1) * MaxLoadDenominator > slots_.size() * MaxLoadNumerator)
        {
            grow();
        }

        const std::size_t mask = slots_.size() - 1;
        std::size_t index = hash(id) & mask;

        while (slots_[index].order != nullptr)
        {
            if (slots_[index].id == id)
            {
                slots_[index].order = order;
                return;
            }
            index = (index + 1) & mask;
        }

        slots_[index] = Slot{ .id = id, .order = order };
        ++size_;
    }

    bool erase(OrderId id) noexcept
    {
        const std::size_t mask = slots_.size() - 1;
        std::size_t hole = hash(id) & mask;

        while (slots_[hole].order != nullptr && slots_[hole].id != id)
        {
            hole = (hole + 1) & mask;
        }

        if (slots_[hole].order == nullptr)
        {
            return false;
        }

        slots_[hole].order = nullptr;
        --size_;

        std::size_t probe = hole;
        while (true)
        {
            probe = (probe + 1) & mask;
            if (slots_[probe].order == nullptr)
            {
                break;
            }

            const std::size_t ideal = hash(slots_[probe].id) & mask;
            const bool settled = (hole <= probe) ? (ideal > hole && ideal <= probe)
                                                 : (ideal > hole || ideal <= probe);
            if (settled)
            {
                continue;
            }

            slots_[hole] = slots_[probe];
            slots_[probe].order = nullptr;
            hole = probe;
        }

        return true;
    }

    void clear() noexcept
    {
        slots_.assign(slots_.size(), Slot{});
        size_ = 0;
    }

private:
    struct Slot
    {
        OrderId id;
        Order* order{ nullptr };
    };

    static constexpr std::size_t MinCapacity = 64;
    static constexpr std::size_t MaxLoadNumerator = 7;
    static constexpr std::size_t MaxLoadDenominator = 10;

    [[nodiscard]] static std::size_t hash(OrderId id) noexcept
    {
        std::uint64_t x = id.get();
        x ^= x >> 30U;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27U;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31U;
        return static_cast<std::size_t>(x);
    }

    void grow()
    {
        std::vector<Slot> previous = std::move(slots_);
        slots_.assign(previous.size() * 2, Slot{});
        size_ = 0;

        for (const Slot& slot : previous)
        {
            if (slot.order != nullptr)
            {
                insert(slot.id, slot.order);
            }
        }
    }

    std::vector<Slot> slots_;
    std::size_t size_{ 0 };
};

}
