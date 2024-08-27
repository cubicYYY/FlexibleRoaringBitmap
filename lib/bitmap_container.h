#pragma once

#include <array>
#include <cstdint>
#include <iostream>

namespace froaring {
template <typename WordType, size_t DataBits>
class BitmapContainer {
public:
    static constexpr size_t BitsPerWord = 8 * sizeof(WordType);
    static constexpr size_t WordCount =
        (DataBits + BitsPerWord - 1) / BitsPerWord;  // ceiling
    std::array<WordType, WordCount> bits;

    BitmapContainer() { bits.fill(0); }

    void set(uint16_t index) {
        bits[index / BitsPerWord] |= (1 << (index % BitsPerWord));
    }

    bool test(uint16_t index) const {
        return bits[index / BitsPerWord] & (1 << (index % BitsPerWord));
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