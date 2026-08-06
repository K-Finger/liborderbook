#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "PriceLevel.h"
#include "Side.h"
#include "Types.h"

namespace orderbook {

template<Side S>
class PriceLadder
{
public:
    static constexpr std::size_t NoIndex = static_cast<std::size_t>(-1);

    PriceLadder(Price minPrice, Price maxPrice) : minPrice_{ minPrice }, maxPrice_{ maxPrice }
    {
        if (maxPrice < minPrice)
        {
            throw std::invalid_argument("PriceLadder: maxPrice is below minPrice");
        }

        const std::uint64_t span = static_cast<std::uint64_t>(maxPrice.get()) -
                                   static_cast<std::uint64_t>(minPrice.get());
        if (span >= MaxSpan)
        {
            throw std::length_error("PriceLadder: price band is too wide");
        }

        levels_.resize(static_cast<std::size_t>(span) + 1);
        occupancy_.assign((levels_.size() + BitsPerWord - 1) / BitsPerWord, 0);
    }

    bool contains(Price price) const noexcept
    {
        return price >= minPrice_ && price <= maxPrice_;
    }

    PriceLevel* find(Price price) noexcept
    {
        if (!contains(price))
        {
            return nullptr;
        }

        const std::size_t index = indexOf(price);
        return isOccupied(index) ? &levels_[index] : nullptr;
    }

    PriceLevel& getOrCreate(Price price)
    {
        assert(contains(price) && "PriceLadder::getOrCreate called with out-of-band price");

        const std::size_t index = indexOf(price);
        PriceLevel& level = levels_[index];

        if (!isOccupied(index))
        {
            level.price = price;
            markOccupied(index);
        }

        return level;
    }

    void erase(Price price) noexcept
    {
        if (!contains(price))
        {
            return;
        }

        const std::size_t index = indexOf(price);
        if (!isOccupied(index))
        {
            return;
        }

        occupancy_[index / BitsPerWord] &= ~bitMask(index);
        levels_[index] = PriceLevel{};
        --occupiedCount_;

        if (occupiedCount_ == 0)
        {
            bestIndex_ = NoIndex;
            worstIndex_ = NoIndex;
        }
        else if (index == bestIndex_)
        {
            bestIndex_ = scanTowardWorse(index);
        }
        else if (index == worstIndex_)
        {
            worstIndex_ = scanTowardBetter(index);
        }
    }

    PriceLevel* best() noexcept
    {
        return bestIndex_ == NoIndex ? nullptr : &levels_[bestIndex_];
    }

    const PriceLevel* best() const noexcept
    {
        return bestIndex_ == NoIndex ? nullptr : &levels_[bestIndex_];
    }

    bool empty() const noexcept { return occupiedCount_ == 0; }
    std::size_t levelCount() const noexcept { return occupiedCount_; }
    std::size_t bandSize() const noexcept { return levels_.size(); }
    Price minPrice() const noexcept { return minPrice_; }
    Price maxPrice() const noexcept { return maxPrice_; }

    template<typename Fn>
    void forEachFromBest(Fn&& fn) const
    {
        std::size_t index = bestIndex_;

        while (index != NoIndex)
        {
            if (!fn(levels_[index]))
            {
                return;
            }

            if (index == worstIndex_)
            {
                return;
            }

            index = scanTowardWorse(index);
        }
    }

private:
    static constexpr std::size_t BitsPerWord = 64;
    static constexpr std::uint64_t MaxSpan = 1'000'000'000ULL;

    static constexpr bool isBetterIndex(std::size_t lhs, std::size_t rhs) noexcept
    {
        if constexpr (S == Side::Buy)
        {
            return lhs > rhs;
        }
        else
        {
            return lhs < rhs;
        }
    }

    static constexpr std::uint64_t bitMask(std::size_t index) noexcept
    {
        return std::uint64_t{ 1 } << (index % BitsPerWord);
    }

    std::size_t indexOf(Price price) const noexcept
    {
        return static_cast<std::size_t>(static_cast<std::uint64_t>(price.get()) -
                                        static_cast<std::uint64_t>(minPrice_.get()));
    }

    bool isOccupied(std::size_t index) const noexcept
    {
        return (occupancy_[index / BitsPerWord] & bitMask(index)) != 0;
    }

    void markOccupied(std::size_t index) noexcept
    {
        occupancy_[index / BitsPerWord] |= bitMask(index);

        if (occupiedCount_ == 0)
        {
            bestIndex_ = index;
            worstIndex_ = index;
        }
        else if (isBetterIndex(index, bestIndex_))
        {
            bestIndex_ = index;
        }
        else if (isBetterIndex(worstIndex_, index))
        {
            worstIndex_ = index;
        }

        ++occupiedCount_;
    }

    std::size_t nextOccupiedAtOrAbove(std::size_t from) const noexcept
    {
        if (from >= levels_.size())
        {
            return NoIndex;
        }

        std::size_t word = from / BitsPerWord;
        std::uint64_t bits = occupancy_[word] & (~std::uint64_t{ 0 } << (from % BitsPerWord));

        while (bits == 0)
        {
            if (++word == occupancy_.size())
            {
                return NoIndex;
            }
            bits = occupancy_[word];
        }

        return word * BitsPerWord + static_cast<std::size_t>(std::countr_zero(bits));
    }

    std::size_t prevOccupiedAtOrBelow(std::size_t from) const noexcept
    {
        if (levels_.empty())
        {
            return NoIndex;
        }

        if (from >= levels_.size())
        {
            from = levels_.size() - 1;
        }

        std::size_t word = from / BitsPerWord;
        const std::size_t bit = from % BitsPerWord;
        const std::uint64_t mask = (bit == BitsPerWord - 1)
                                       ? ~std::uint64_t{ 0 }
                                       : ((std::uint64_t{ 1 } << (bit + 1)) - 1);
        std::uint64_t bits = occupancy_[word] & mask;

        while (bits == 0)
        {
            if (word == 0)
            {
                return NoIndex;
            }
            bits = occupancy_[--word];
        }

        return word * BitsPerWord + (BitsPerWord - 1 - static_cast<std::size_t>(std::countl_zero(bits)));
    }

    std::size_t scanTowardWorse(std::size_t from) const noexcept
    {
        if constexpr (S == Side::Buy)
        {
            return from == 0 ? NoIndex : prevOccupiedAtOrBelow(from - 1);
        }
        else
        {
            return nextOccupiedAtOrAbove(from + 1);
        }
    }

    std::size_t scanTowardBetter(std::size_t from) const noexcept
    {
        if constexpr (S == Side::Buy)
        {
            return nextOccupiedAtOrAbove(from + 1);
        }
        else
        {
            return from == 0 ? NoIndex : prevOccupiedAtOrBelow(from - 1);
        }
    }

    Price minPrice_;
    Price maxPrice_;

    std::vector<PriceLevel> levels_;
    std::vector<std::uint64_t> occupancy_;

    std::size_t occupiedCount_{ 0 };
    std::size_t bestIndex_{ NoIndex };
    std::size_t worstIndex_{ NoIndex };
};

}
