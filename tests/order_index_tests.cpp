#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "OrderIndex.h"

using namespace orderbook;

namespace {

Order* fakeOrder(std::uintptr_t marker)
{
    return reinterpret_cast<Order*>(marker * sizeof(void*));
}

}

TEST(OrderIndex, EmptyIndexFindsNothing)
{
    const OrderIndex index;

    EXPECT_TRUE(index.empty());
    EXPECT_EQ(index.size(), 0u);
    EXPECT_EQ(index.find(OrderId{ 1 }), nullptr);
    EXPECT_FALSE(index.contains(OrderId{ 1 }));
}

TEST(OrderIndex, InsertThenFind)
{
    OrderIndex index;
    index.insert(OrderId{ 7 }, fakeOrder(7));

    EXPECT_EQ(index.size(), 1u);
    EXPECT_EQ(index.find(OrderId{ 7 }), fakeOrder(7));
    EXPECT_TRUE(index.contains(OrderId{ 7 }));
    EXPECT_EQ(index.find(OrderId{ 8 }), nullptr);
}

TEST(OrderIndex, InsertingSameIdOverwritesWithoutGrowingSize)
{
    OrderIndex index;
    index.insert(OrderId{ 7 }, fakeOrder(1));
    index.insert(OrderId{ 7 }, fakeOrder(2));

    EXPECT_EQ(index.size(), 1u);
    EXPECT_EQ(index.find(OrderId{ 7 }), fakeOrder(2));
}

TEST(OrderIndex, EraseRemovesOnlyTheTarget)
{
    OrderIndex index;
    for (std::uint64_t id = 1; id <= 10; ++id)
    {
        index.insert(OrderId{ id }, fakeOrder(id));
    }

    EXPECT_TRUE(index.erase(OrderId{ 5 }));
    EXPECT_FALSE(index.erase(OrderId{ 5 }));
    EXPECT_EQ(index.size(), 9u);
    EXPECT_EQ(index.find(OrderId{ 5 }), nullptr);

    for (std::uint64_t id = 1; id <= 10; ++id)
    {
        if (id != 5)
        {
            EXPECT_EQ(index.find(OrderId{ id }), fakeOrder(id)) << "id " << id;
        }
    }
}

TEST(OrderIndex, GrowsAndKeepsEveryEntryReachable)
{
    OrderIndex index;
    constexpr std::uint64_t Count = 5000;

    for (std::uint64_t id = 1; id <= Count; ++id)
    {
        index.insert(OrderId{ id }, fakeOrder(id));
    }

    EXPECT_EQ(index.size(), Count);
    EXPECT_GT(index.capacity(), Count);

    for (std::uint64_t id = 1; id <= Count; ++id)
    {
        ASSERT_EQ(index.find(OrderId{ id }), fakeOrder(id)) << "id " << id;
    }
}

TEST(OrderIndex, SurvivesCollidingKeys)
{
    OrderIndex index;
    std::vector<std::uint64_t> ids;

    for (std::uint64_t k = 0; k < 40; ++k)
    {
        const std::uint64_t id = 1 + (k * 4096);
        ids.push_back(id);
        index.insert(OrderId{ id }, fakeOrder(id));
    }

    for (std::uint64_t id : ids)
    {
        ASSERT_EQ(index.find(OrderId{ id }), fakeOrder(id)) << "id " << id;
    }

    for (std::size_t k = 0; k < ids.size(); k += 2)
    {
        ASSERT_TRUE(index.erase(OrderId{ ids[k] }));
    }

    for (std::size_t k = 0; k < ids.size(); ++k)
    {
        if (k % 2 == 0)
        {
            ASSERT_EQ(index.find(OrderId{ ids[k] }), nullptr) << "id " << ids[k];
        }
        else
        {
            ASSERT_EQ(index.find(OrderId{ ids[k] }), fakeOrder(ids[k])) << "id " << ids[k];
        }
    }
}

TEST(OrderIndex, MatchesUnorderedMapUnderRandomChurn)
{
    OrderIndex index;
    std::unordered_map<std::uint64_t, Order*> reference;

    std::mt19937_64 rng{ 20260806 };
    std::uniform_int_distribution<std::uint64_t> keys{ 1, 4000 };

    for (int step = 0; step < 200000; ++step)
    {
        const std::uint64_t id = keys(rng);

        if ((rng() & 1U) != 0)
        {
            Order* value = fakeOrder(id);
            index.insert(OrderId{ id }, value);
            reference[id] = value;
        }
        else
        {
            const bool erased = index.erase(OrderId{ id });
            const bool referenceErased = reference.erase(id) != 0;
            ASSERT_EQ(erased, referenceErased) << "step " << step << " id " << id;
        }

        ASSERT_EQ(index.size(), reference.size()) << "step " << step;
    }

    for (const auto& [id, value] : reference)
    {
        ASSERT_EQ(index.find(OrderId{ id }), value) << "id " << id;
    }
}

TEST(OrderIndex, ClearEmptiesWithoutLosingCapacity)
{
    OrderIndex index;
    for (std::uint64_t id = 1; id <= 100; ++id)
    {
        index.insert(OrderId{ id }, fakeOrder(id));
    }

    const std::size_t capacityBefore = index.capacity();
    index.clear();

    EXPECT_TRUE(index.empty());
    EXPECT_EQ(index.capacity(), capacityBefore);
    EXPECT_EQ(index.find(OrderId{ 1 }), nullptr);
}
