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
    static constexpr size_t WordsCount = (TotalBits + BitsPerWord - 1) / BitsPerWord;  // ceiling
    using NumType = froaring::can_fit_t<DataBits>;
    using IndexType = froaring::can_fit_t<DataBits - BitsPerWord>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;
    static constexpr WordType IndexInsideWordMask = (1 << cexpr_log2(BitsPerWord)) - 1;

public:
    BitmapContainer() { memset(words, 0, sizeof(words)); }

    BitmapContainer(const BitmapContainer&) = delete;
    BitmapContainer& operator=(const BitmapContainer&) = delete;

    static void destroy(BitmapContainer* obj) { delete (obj); }

    void debug_print() const {
        for (size_t i = 0; i < WordsCount; ++i) {
            // std::cout << words[i] << " ";
            WordType w = words[i];
            WordType t = w & (~w + 1);
            auto r = i * sizeof(WordType) + std::countr_zero(w);
            std::cout << r << " ";
        }
        std::cout << std::endl;
    }

    void clear() { std::memset(words.data(), 0, WordsCount * sizeof(WordType)); }

    void set(NumType index) { words[index / BitsPerWord] |= ((WordType)1 << (index % BitsPerWord)); }

    bool test(NumType index) const { return words[index / BitsPerWord] & ((WordType)1 << (index % BitsPerWord)); }

    bool test_and_set(NumType index) {
        bool was_set = test(index);
        if (was_set) return false;
        set(index);
        return true;
    }

    void reset(NumType index) { words[index / BitsPerWord] &= ~((WordType)1 << (index % BitsPerWord)); }

    bool containesRange(IndexType start, IndexType end) const {
        if (start >= end) return true;
        constexpr WordType low_bits_mask = (1 << BitsPerWord) - 1;
        constexpr WordType full_1_mask = (1 << BitsPerWord) - 1;
        const IndexType start_word = start / BitsPerWord;
        const IndexType end_word = end / BitsPerWord;

        // All "1" from `start` to MSB
        const WordType first_mask = ~(((SizeType)1 << (start & IndexInsideWordMask)) - 1);
        // All "1" from LSB to `end`
        const WordType last_mask = ((SizeType)1 << (end & IndexInsideWordMask)) - 1;

        if (start_word == end_word) {
            return ((words[end_word] & first_mask & last_mask) == (first_mask & last_mask));
        }

        if (start_word >= WordsCount || (words[start_word] & first_mask) != first_mask) {
            return false;
        }
        if (end_word >= WordsCount || (words[end_word] & last_mask) != last_mask) {
            return false;
        }

        for (IndexType i = start_word + 1; i < end_word; ++i) {
            if (words[i] != full_1_mask) {
                return false;
            }
        }

        return true;
    }
    SizeType cardinality() const {
        SizeType count = 0;
        for (const auto& word : words) {
            count += std::popcount(word);
        }
        return count;
    }

public:
    WordType words[WordsCount];
};
}  // namespace froaring