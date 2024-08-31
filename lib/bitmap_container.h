#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "prelude.h"

namespace froaring {
template <typename WordType, size_t DataBits>
class BitmapContainer : public froaring_container_t {
public:
    static constexpr size_t BitsPerWord = 8 * sizeof(WordType);
    static constexpr size_t TotalBits = (1 << DataBits);
    static constexpr size_t WordCount = (TotalBits + BitsPerWord - 1) / BitsPerWord;  // ceiling
    using NumType = froaring::can_fit_t<DataBits>;
    using IndexType = froaring::can_fit_t<DataBits - BitsPerWord>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;

public:
    BitmapContainer() { memset(words, 0, sizeof(words)); }

    BitmapContainer(const BitmapContainer&) = delete;
    BitmapContainer& operator=(const BitmapContainer&) = delete;

    static void destroy(BitmapContainer* obj) { delete (obj); }

    void debug_print() const {
        for (size_t i = 0; i < WordCount; ++i) {
            // std::cout << words[i] << " ";
            WordType w = words[i];
            WordType t = w & (~w + 1);
            WordType r = i * sizeof(WordType) + std::countr_zero(w);
            std::cout << r << " ";
        }
        std::cout << std::endl;
    }

    void clear() { std::memset(words.data(), 0, WordCount * sizeof(WordType)); }

    void set(NumType index) { words[index / BitsPerWord] |= ((WordType)1 << (index % BitsPerWord)); }

    bool test(NumType index) const { return words[index / BitsPerWord] & ((WordType)1 << (index % BitsPerWord)); }

    bool test_and_set(NumType index) {
        bool was_set = test(index);
        if (was_set) return false;
        set(index);
        return true;
    }

    void reset(NumType index) { words[index / BitsPerWord] &= ~((WordType)1 << (index % BitsPerWord)); }

    SizeType cardinality() const {
        SizeType count = 0;
        for (const auto& word : words) {
            count += std::popcount(word);
        }
        return count;
    }

public:
    WordType words[WordCount];
};
}  // namespace froaring