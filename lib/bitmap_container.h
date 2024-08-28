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
    std::array<WordType, WordCount> bits;
    using DataType = froaring::can_fit_t<DataBits>;

    BitmapContainer() { bits.fill(0); }

    BitmapContainer(const BitmapContainer&) = delete;
    BitmapContainer& operator=(const BitmapContainer&) = delete;

    void clear() { std::memset(bits.data(), 0, WordCount * sizeof(WordType)); }

    void set(DataType index) {
        bits[index / BitsPerWord] |= (1 << (index % BitsPerWord));
    }

    bool test(DataType index) const {
        return bits[index / BitsPerWord] & (1 << (index % BitsPerWord));
    }

    bool test_and_set(DataType index) {
        bool was_set = test(index);
        if (was_set) return false;
        set(index);
        return true;
    }

    void reset(DataType index) {
        bits[index / BitsPerWord] &= ~(1 << (index % BitsPerWord));
    }

    size_t cardinality() const {
        size_t count = 0;
        for (const auto& word : bits) {
            count += __builtin_popcount(word);
        }
        return count;
    }
};
}  // namespace froaring