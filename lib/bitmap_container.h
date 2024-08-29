#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "prelude.h"

namespace froaring {
template <typename WordType, size_t DataBits>
class BitmapContainer {
public:
    static constexpr size_t BitsPerWord = 8 * sizeof(WordType);
    static constexpr size_t TotalBits = (1 << DataBits);
    static constexpr size_t WordCount =
        (TotalBits + BitsPerWord - 1) / BitsPerWord;  // ceiling
    using IndexOrNumType = froaring::can_fit_t<DataBits>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;

    BitmapContainer(const BitmapContainer&) = delete;
    BitmapContainer& operator=(const BitmapContainer&) = delete;

    void clear() { std::memset(bits.data(), 0, WordCount * sizeof(WordType)); }

    static BitmapContainer* create() {
        BitmapContainer* container = new BitmapContainer();
        return container;
    }

    void set(IndexOrNumType index) {
        bits[index / BitsPerWord] |= (1 << (index % BitsPerWord));
    }

    bool test(IndexOrNumType index) const {
        return bits[index / BitsPerWord] & (1 << (index % BitsPerWord));
    }

    bool test_and_set(IndexOrNumType index) {
        bool was_set = test(index);
        if (was_set) return false;
        set(index);
        return true;
    }

    void reset(IndexOrNumType index) {
        bits[index / BitsPerWord] &= ~(1 << (index % BitsPerWord));
    }

    SizeType cardinality() const {
        SizeType count = 0;
        for (const auto& word : bits) {
            count += __builtin_popcount(word);
        }
        return count;
    }

private:
    BitmapContainer() { memset(bits, 0, sizeof(bits)); }
    WordType bits[WordCount];
};
}  // namespace froaring