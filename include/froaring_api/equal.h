#pragma once
// TODO: pointer to referece
// TODO: to const pointers
#include <bit>

#include "utils.h"

namespace froaring {
using CTy = froaring::ContainerType;
template <typename WordType, size_t DataBits>
bool froaring_equal_bb(const BitmapContainer<WordType, DataBits>* a, const BitmapContainer<WordType, DataBits>* b) {
    for (auto i = 0; i < a->WordsCount; ++i) {
        if (a->words[i] != b->words[i]) return false;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_aa(const ArrayContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    if (a->size != b->size) return false;
    for (auto i = 0; i < a->size; ++i) {
        if (a->vals[i] != b->vals[i]) return false;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_rr(const RLEContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    if (a->run_count != b->run_count) return false;
    for (auto i = 0; i < a->run_count; ++i) {
        if (a->runs[i].first != b->runs[i].first || a->runs[i].second != b->runs[i].second) return false;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_ar(const ArrayContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    if (a->size > b->run_count) return false;
    if (a->cardinality() != b->cardinality()) return false;
    size_t pos = 0;
    for (size_t i = 0; i < b->run_count; ++i) {
        if (pos >= a->size) return false;
        if (a->vals[pos] != b->runs[i].first) return false;
        if (pos + (b->runs[i].second - b->runs[i].first) >= a->size) return false;
        if (a->vals[pos + (b->runs[i].second - b->runs[i].first)] != b->runs[i].second) return false;
        pos += (b->runs[i].second - b->runs[i].first) + 1;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_br(const BitmapContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    // TODO:
    return false;
}

template <typename WordType, size_t DataBits>
bool froaring_equal_ba(const BitmapContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    if (a->cardinality() != b->cardinality()) {
        return false;
    }

    typename BitmapContainer<WordType, DataBits>::SizeType pos = 0;
    for (auto i = 0; i < a->WordsCount; ++i) {
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

template <typename WordType, size_t IndexBits, size_t DataBits>
bool froaring_equal_bsbs(const BinsearchIndex<WordType, IndexBits, DataBits>* a,
                         const BinsearchIndex<WordType, IndexBits, DataBits>* b) {
    if (a->size != b->size) {
        return false;
    }

    for (size_t i = 0; i < a->size; ++i) {  // quick check
        if (a->containers[i].index != b->containers[i].index) {
            return false;
        }
    }
    for (size_t i = 0; i < a->size; ++i) {
        auto res =
            froaring_equal<WordType, DataBits>(a->containers[i].ptr, b->containers[i].ptr, a->containers[i].type, b->containers[i].type);
        if (!res) {
            return false;
        }
    }
    return true;
}

template <typename WordType, size_t DataBits>
bool froaring_equal(const froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb) {
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_equal_bb(CAST_TO_BITMAP_CONST(a), CAST_TO_BITMAP_CONST(b));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_equal_aa(CAST_TO_ARRAY_CONST(a), CAST_TO_ARRAY_CONST(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_equal_rr(CAST_TO_RLE_CONST(a), CAST_TO_RLE_CONST(b));
            break;
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_equal_ba(CAST_TO_BITMAP_CONST(a), CAST_TO_ARRAY_CONST(b));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_equal_ba(CAST_TO_BITMAP_CONST(b), CAST_TO_ARRAY_CONST(a));
            break;
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_equal_br(CAST_TO_BITMAP_CONST(a), CAST_TO_RLE_CONST(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_equal_br(CAST_TO_BITMAP_CONST(b), CAST_TO_RLE_CONST(a));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_equal_ar(CAST_TO_ARRAY_CONST(a), CAST_TO_RLE_CONST(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_equal_ar(CAST_TO_ARRAY_CONST(b), CAST_TO_RLE_CONST(a));
            break;
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring