#pragma once

#include <bit>

#include "array_container.h"
#include "bitmap_container.h"
#include "mix_ops.h"
#include "prelude.h"
#include "rle_container.h"
#include "utils.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
bool froaring_contains_bb(const BitmapContainer<WordType, DataBits>* a, const BitmapContainer<WordType, DataBits>* b) {
    for (size_t i = 0; i < a->WordsCount; ++i) {
        if ((a->words[i] & b->words[i]) != b->words[i]) {
            return false;
        }
    }
    return true;
}

template <typename WordType, size_t DataBits>
bool froaring_contains_aa(const ArrayContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    if (b->size == 0) {
        return true;
    }

    if (a->size < b->size) {
        return false;
    }

    size_t i = 0, j = 0;

    while (i < a->size && j < b->size) {
        if (a->vals[i] == b->vals[j]) {
            i++;
            j++;
        } else if (b->vals[j] > a->vals[i]) {
            i++;
        } else {
            return false;
        }
    }
    if (j == b->size) {
        return true;
    } else {
        return false;
    }
}

template <typename WordType, size_t DataBits>
bool froaring_contains_rr(const RLEContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    // FIXME: TODO: ...
    FROARING_NOT_IMPLEMENTED
}

template <typename WordType, size_t DataBits>
bool froaring_contains_ar(const ArrayContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    auto run_card = b->cardinality();
    if (run_card == 0) {
        return true;
    }
    if (run_card > a->size) {
        return false;
    }
    size_t start_pos = 0, stop_pos = 0;
    for (int i = 0; i < b->run_count; ++i) {
        typename ArrayContainer<WordType, DataBits>::SizeType start = b->runs[i].start;
        typename ArrayContainer<WordType, DataBits>::SizeType stop = b->runs[i].end;
        start_pos = a->advanceUntil(start, stop_pos);
        stop_pos = a->advanceUntil(stop, stop_pos);
        if (stop_pos == a->size) {
            return false;
        } else if (stop_pos - start_pos != stop - start || a->vals[start_pos] != start || a->vals[stop_pos] != stop) {
            return false;
        }
    }
    return true;
}

template <typename WordType, size_t DataBits>
bool froaring_contains_ra(const RLEContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    if (b->size == 0) {
        return true;
    }
    if (b->size > a->cardinality()) {
        return false;
    }
    int i_array = 0, i_run = 0;
    while (i_array < b->size && i_run < a->run_count) {
        typename RLEContainer<WordType, DataBits>::SizeType start = a->runs[i_run].start;
        typename RLEContainer<WordType, DataBits>::SizeType stop = a->runs[i_run].end;
        if (b->vals[i_array] < start) {
            return false;
        } else if (b->vals[i_array] > stop) {
            i_run++;
        } else {  // the value of the array is in the run
            i_array++;
        }
    }
    if (i_array == b->size) {
        return true;
    } else {
        return false;
    }
}

template <typename WordType, size_t DataBits>
bool froaring_contains_br(const BitmapContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    if (a->cardinality() < b->cardinality()) {
        return false;
    }
    for (int i = 0; i < b->run_count; ++i) {
        typename RLEContainer<WordType, DataBits>::SizeType run_start = b->runs[i].start;
        typename RLEContainer<WordType, DataBits>::SizeType run_end = b->runs[i].end;
        if (!a->test_range(run_start, run_end)) {
            return false;
        }
    }
    return true;
}

template <typename WordType, size_t DataBits>
bool froaring_contains_rb(const RLEContainer<WordType, DataBits>* a, const BitmapContainer<WordType, DataBits>* b) {
    if (b->cardinality() > a->cardinality()) {
        return false;
    }
    size_t i_bitset = 0, i_run = 0;
    while (i_bitset < BitmapContainer<WordType, DataBits>::WordsCount && i_run < a->run_count) {
        auto w = b->words[i_bitset];
        while (w != 0 && i_run < a->run_count) {
            auto start = a->runs[i_run].start;
            auto stop = a->runs[i_run].end;
            auto t = w & (~w + 1);
            size_t r = i_bitset * BitmapContainer<WordType, DataBits>::BitsPerWord + std::countr_zero(w);
            if (r < start) {
                return false;
            } else if (r > stop) {
                i_run++;
                continue;
            } else {
                w ^= t;
            }
        }
        if (w == 0) {
            i_bitset++;
        } else {
            return false;
        }
    }
    if (i_bitset < BitmapContainer<WordType, DataBits>::WordsCount) {
        // terminated iterating on the run containers, check that rest of bitset
        // is empty
        for (; i_bitset < BitmapContainer<WordType, DataBits>::WordsCount; i_bitset++) {
            if (b->words[i_bitset] != 0) {
                return false;
            }
        }
    }
    return true;
}

template <typename WordType, size_t DataBits>
bool froaring_contains_ba(const BitmapContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    if (a->cardinality() < b->size) {
        return false;
    }

    for (size_t i = 0; i < b->size; ++i) {
        if (!a->test(b->vals[i])) {
            return false;
        }
    }
    return true;
}

template <typename WordType, size_t DataBits>
bool froaring_contains_ab(const ArrayContainer<WordType, DataBits>* a, const BitmapContainer<WordType, DataBits>* b) {
    // FIXME: We should assume that a bitset is always bigger, so this function should always return false.
    auto bitset_card = b->cardinality();
    if (a->size < bitset_card) {
        return false;
    }
    size_t found = 0;
    for (size_t i = 0; i < a->size; i++) {
        found += b->test(a->vals[i]);
    }
    if (found == bitset_card) {
        return true;
    } else {
        return false;
    }
}

template <typename WordType, size_t DataBits>
bool froaring_contains(const froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_contains_bb(static_cast<const BitmapSized*>(a), static_cast<const BitmapSized*>(b));
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_contains_aa(static_cast<const ArraySized*>(a), static_cast<const ArraySized*>(b));
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_contains_rr(static_cast<const RLESized*>(a), static_cast<const RLESized*>(b));
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_contains_ba(static_cast<const BitmapSized*>(a), static_cast<const ArraySized*>(b));
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_contains_ab(static_cast<const ArraySized*>(a), static_cast<const BitmapSized*>(b));
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_contains_br(static_cast<const BitmapSized*>(a), static_cast<const RLESized*>(b));
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_contains_rb(static_cast<const RLESized*>(a), static_cast<const BitmapSized*>(b));
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_contains_ar(static_cast<const ArraySized*>(a), static_cast<const RLESized*>(b));
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_contains_ra(static_cast<const RLESized*>(a), static_cast<const ArraySized*>(b));
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring