#pragma once

#include <bit>

#include "utils.h"

namespace froaring {
using CTy = froaring::ContainerType;
template <typename WordType, size_t DataBits>
bool froaring_equal_bb(BitmapContainer<WordType, DataBits>* a,
                       BitmapContainer<WordType, DataBits>* b) {
    for (auto i = 0; i < BitmapContainer<WordType, DataBits>::WordCount; ++i) {
        if (a->words[i] != b->words[i]) return false;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_aa(ArrayContainer<WordType, DataBits>* a,
                       ArrayContainer<WordType, DataBits>* b) {
    if (a->size != b->size) return false;
    for (auto i = 0; i < a->size; ++i) {
        if (a->vals[i] != b->vals[i]) return false;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_rr(RLEContainer<WordType, DataBits>* a,
                       RLEContainer<WordType, DataBits>* b) {
    if (a->size != b->size) return false;
    for (auto i = 0; i < a->size; ++i) {
        if (a->runs[i].first != b->runs[i].first ||
            a->runs[i].second != b->runs[i].second)
            return false;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_ar(ArrayContainer<WordType, DataBits>* a,
                       RLEContainer<WordType, DataBits>* b) {
    // TODO:
    FROARING_NOT_IMPLEMENTED
    return false;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_br(BitmapContainer<WordType, DataBits>* a,
                       RLEContainer<WordType, DataBits>* b) {
    // TODO:
    FROARING_NOT_IMPLEMENTED
    return false;
}

template <typename WordType, size_t DataBits>
bool froaring_equal_ba(BitmapContainer<WordType, DataBits>* a,
                       ArrayContainer<WordType, DataBits>* b) {
    if (a->cardinality() != b->cardinality()) {
        return false;
    }

    typename BitmapContainer<WordType, DataBits>::SizeType pos = 0;
    for (auto i = 0; i < BitmapContainer<WordType, DataBits>::WordCount; ++i) {
        WordType w = a->words[i];
        while (w != 0) {
            if (pos >= b->cardinality()) {
                return false;
            }
            WordType t = w & (~w + 1);
            WordType r = i * sizeof(WordType) + std::countr_zero(w);
            if (b->vals[pos] != r) {
                return false;
            }
            ++pos;
            w ^= t;
        }
    }
    return (pos == b->cardinality());
}

template <typename WordType, size_t DataBits>
bool froaring_equal(froaring_container_t* a, froaring_container_t* b, CTy ta,
                    CTy tb) {
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_equal_bb(CAST_TO_BITMAP(a), CAST_TO_BITMAP(b));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_equal_aa(CAST_TO_ARRAY(a), CAST_TO_ARRAY(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_equal_rr(CAST_TO_RLE(a), CAST_TO_RLE(b));
            break;
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_equal_ba(CAST_TO_BITMAP(a), CAST_TO_ARRAY(b));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_equal_ba(CAST_TO_BITMAP(b), CAST_TO_ARRAY(a));
            break;
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_equal_br(CAST_TO_BITMAP(a), CAST_TO_RLE(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_equal_br(CAST_TO_BITMAP(b), CAST_TO_RLE(a));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_equal_ar(CAST_TO_ARRAY(a), CAST_TO_RLE(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_equal_ar(CAST_TO_ARRAY(b), CAST_TO_RLE(a));
            break;
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring