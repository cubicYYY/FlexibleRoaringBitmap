#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "prelude.h"

namespace froaring {
template <typename WordType, size_t DataBits>
class BitmapContainer : public froaring_container_t {
    static_assert(std::popcount(DataBits) == 1,
                  "DataBits must be a power of 2. This restriction is for efficient bit operations and may be removed "
                  "in the future.");

public:
    static constexpr size_t BitsPerWord = 8 * sizeof(WordType);
    static constexpr size_t TotalBits = (1 << DataBits);
    static constexpr size_t WordsCount = (TotalBits + BitsPerWord - 1) / BitsPerWord;  // ceiling
    using NumType = froaring::can_fit_t<DataBits>;
    using IndexType = froaring::can_fit_t<WordsCount>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;
    static constexpr WordType IndexInsideWordMask = (1ULL << cexpr_log2(BitsPerWord)) - 1;

public:
    BitmapContainer() { memset(words, 0, sizeof(words)); }

    BitmapContainer(const BitmapContainer& other) { std::memcpy(words, other.words, WordsCount * sizeof(WordType)); }
    BitmapContainer& operator=(const BitmapContainer&) = delete;

    static void destroy(BitmapContainer* obj) { delete (obj); }

    void debug_print() const {
        for (size_t i = 0; i < WordsCount; ++i) {
            // std::cout << words[i] << " ";
            WordType w = words[i];
            while (w) {
                WordType t = w & (~w + 1);
                auto r = i * BitsPerWord + std::countr_zero(w);
                std::cout << r << " ";
                w ^= t;
            }
        }
        std::cout << std::endl;
    }

    void clear() { std::memset(words, 0, WordsCount * sizeof(WordType)); }

    void set(NumType index) { words[index / BitsPerWord] |= ((WordType)1 << (index % BitsPerWord)); }

    bool test(NumType index) const { return words[index / BitsPerWord] & ((WordType)1 << (index % BitsPerWord)); }

    bool test_and_set(NumType index) {
        bool was_set = test(index);
        if (was_set) return false;
        set(index);
        return true;
    }

    void reset(NumType index) { words[index / BitsPerWord] &= ~((WordType)1 << (index % BitsPerWord)); }

    void reset_range(NumType start, NumType end) {
        if (start >= end) {
            return;
        }
        const IndexType start_word = start / BitsPerWord;
        const IndexType end_word = end / BitsPerWord;
        if (end_word >= WordsCount || start_word >= WordsCount) {
            return;
        }
        // All "0" from `start` to MSB
        const WordType first_mask = (1ULL << (start & IndexInsideWordMask)) - 1;
        // All "0" from LSB to `end`
        const WordType last_mask = ~((1ULL << ((end & IndexInsideWordMask))) - 1);

        if (start_word == end_word) {
            words[start_word] &= (first_mask | last_mask);
            return;
        }

        words[start_word] &= first_mask;
        words[end_word] &= last_mask;

        for (IndexType i = start_word + 1; i < end_word; ++i) {
            words[i] = 0;
        }
    }

    /// @brief Check if the range is fully contained in the container.
    /// @param start inclusive.
    /// @param end inclusive.
    /// @return If [start, end] is fully contained in the container.
    bool containesRange(IndexType start, IndexType end) const {
        if (start >= end) {
            return true;
        }
        const IndexType start_word = start / BitsPerWord;
        const IndexType end_word = end / BitsPerWord;
        if (end_word >= WordsCount || start_word >= WordsCount) {
            return false;
        }
        // All "1" from `start` to MSB
        const WordType first_mask = ~((1ULL << (start & IndexInsideWordMask)) - 1);
        // All "1" from LSB to `end`
        const WordType last_mask =
            ((1ULL << ((end & IndexInsideWordMask))) - 1) ^ (1ULL << ((end & IndexInsideWordMask)));

        if (start_word == end_word) {
            return ((words[end_word] & first_mask & last_mask) == (first_mask & last_mask));
            std::cout << "FAIL=3";
        }
        if ((words[start_word] & first_mask) != first_mask) {
            return false;
            std::cout << "FAIL=1";
        }
        if ((words[end_word] & last_mask) != last_mask) {
            return false;
            std::cout << "FAIL=2";
        }

        for (IndexType i = start_word + 1; i < end_word; ++i) {
            if (~words[i] != 0) {
                std::cout << "FAIL=1";
                return false;
            }
        }

        return true;
    }

    void intersect_range(IndexType start, IndexType end) {
        if (start >= end) {
            clear();
            return;
        }
        const IndexType start_word = start / BitsPerWord;
        const IndexType end_word = end / BitsPerWord;
        if (end_word >= WordsCount || start_word >= WordsCount) {
            clear();
            return;
        }
        // All "1" from `start` to MSB
        const WordType first_mask = ~((1ULL << (start & IndexInsideWordMask)) - 1);
        // All "1" from LSB to `end`
        const WordType last_mask =
            ((1ULL << ((end & IndexInsideWordMask))) - 1) ^ (1ULL << ((end & IndexInsideWordMask)));

        if (start_word == end_word) {
            words[start_word] &= first_mask & last_mask;
            return;
        }

        words[start_word] &= first_mask;
        words[end_word] &= last_mask;
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