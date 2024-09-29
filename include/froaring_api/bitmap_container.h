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
    using IndexType = froaring::can_fit_t<cexpr_log2(WordsCount)>;  // FIXME: weird... we need the result floored, but
                                                                    // currently it is ceiled.
    using SizeType = froaring::can_fit_t<DataBits + 1>;
    static constexpr WordType IndexInsideWordMask = (1ULL << cexpr_log2(BitsPerWord)) - 1;

    static_assert(WordsCount * BitsPerWord == TotalBits, "Size of WordType must divides DataBits");

public:
    explicit BitmapContainer() { memset(words, 0, sizeof(words)); }

    explicit BitmapContainer(const BitmapContainer& other) {
        std::memcpy(words, other.words, WordsCount * sizeof(WordType));
    }
    BitmapContainer& operator=(const BitmapContainer&) = delete;

    void debug_print() const {
        for (size_t i = 0; i < WordsCount; ++i) {
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

    void set_range(NumType start, NumType end) {
        if (start >= end) {
            return;
        }
        const IndexType start_word = start / BitsPerWord;
        const IndexType end_word = end / BitsPerWord;

        if (end_word >= WordsCount || start_word >= WordsCount) {
            return;
        }
        // All "1" from `start` to MSB
        const WordType first_mask = ~((1ULL << (start & IndexInsideWordMask)) - 1);
        // All "1" from LSB to `end`
        const WordType last_mask =
            ((1ULL << ((end & IndexInsideWordMask))) - 1) ^ (1ULL << ((end & IndexInsideWordMask)));

        if (start_word == end_word) {
            words[end_word] |= (first_mask & last_mask);
            return;
        }

        words[start_word] |= first_mask;
        words[end_word] |= last_mask;

        std::memset(&words[start_word + 1], 0xFF, (end_word - start_word - 1) * sizeof(NumType));
    }

    bool any_range(NumType start, NumType end) const {
        if (start >= end) {
            return false;
        }
        const IndexType start_word = start / BitsPerWord;
        const IndexType end_word = end / BitsPerWord;

        if (start_word >= WordsCount) {
            return false;
        }
        // All "1" from `start` to MSB
        const WordType first_mask = ~((1ULL << (start & IndexInsideWordMask)) - 1);
        // All "1" from LSB to `end`
        const WordType last_mask =
            ((1ULL << ((end & IndexInsideWordMask))) - 1) ^ (1ULL << ((end & IndexInsideWordMask)));

        if (start_word == end_word) {
            if (words[start_word] & (first_mask & last_mask)) return true;
        }

        if (words[start_word] & first_mask) {
            return true;
        }
        if (end_word < WordsCount && words[end_word] & last_mask) {
            return true;
        }
        for (size_t i = start_word + 1; i < std::min((size_t)(end_word), WordsCount); ++i) {
            if (words[i]) {
                return true;
            }
        }
        return false;
    }

    bool test(NumType index) const { return words[index / BitsPerWord] & ((WordType)1 << (index % BitsPerWord)); }

    bool test_and_set(NumType index) {
        bool was_set = test(index);
        if (was_set) return false;
        set(index);
        return true;
    }

    void reset(NumType index) { words[index / BitsPerWord] &= ~((WordType)1 << (index % BitsPerWord)); }

    /// @brief Reset [start, end], inclusive
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
        const WordType first_mask = ((1ULL << (start & IndexInsideWordMask)) - 1);
        ;
        // All "0" from LSB to `end`
        const WordType last_mask =
            (~((1ULL << ((end & IndexInsideWordMask))) - 1)) ^ (1ULL << ((end & IndexInsideWordMask)));

        if (start_word == end_word) {
            words[start_word] &= (first_mask | last_mask);
            return;
        }

        words[start_word] &= first_mask;
        words[end_word] &= last_mask;

        std::memset(&words[start_word + 1], 0, (end_word - start_word - 1) * sizeof(NumType));
    }

    /// @brief Check if the range is fully contained in the container.
    /// @param start inclusive.
    /// @param end inclusive.
    /// @return If [start, end] is fully contained in the container.
    bool test_range(NumType start, NumType end) const {
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
        }
        if ((words[start_word] & first_mask) != first_mask) {
            return false;
        }
        if ((words[end_word] & last_mask) != last_mask) {
            return false;
        }

        for (size_t i = start_word + 1; i < end_word; ++i) {
            if (~words[i] != 0) {
                return false;
            }
        }

        return true;
    }

    void intersect_range(NumType start, NumType end) {
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