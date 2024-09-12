#pragma once
// TODO: pointer to referece
// TODO: to const pointers
#include <bit>

#include "prelude.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
bool froaring_equal_bb(const BitmapContainer<WordType, DataBits>* a, const BitmapContainer<WordType, DataBits>* b) {
    for (size_t i = 0; i < a->WordsCount; ++i) {
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
        if (a->runs[i].start != b->runs[i].start || a->runs[i].end != b->runs[i].end) return false;
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
        auto&& val = a->vals[pos];
        auto&& run = b->runs[i];
        if (val != run.start) return false;
        if (pos + (run.end - run.start) >= a->size) return false;
        if (a->vals[pos + (run.end - run.start)] != run.end) return false;
        pos += (run.end - run.start) + 1;
    }
    return true;
}
template <typename WordType, size_t DataBits>
bool froaring_equal_br(const BitmapContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    if (a->cardinality() != b->cardinality()) return false;
    for (size_t i = 0; i < b->run_count; ++i) {
        if (!a->containesRange(b->runs[i].start, b->runs[i].end)) {
            return false;
        }
    }
    return true;
}

template <typename WordType, size_t DataBits>
bool froaring_equal_ba(const BitmapContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    if (a->cardinality() != b->cardinality()) {
        return false;
    }

    typename BitmapContainer<WordType, DataBits>::SizeType pos = 0;
    for (size_t i = 0; i < a->WordsCount; ++i) {
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
bool froaring_equal(const froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb) {
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_equal_bb(static_cast<const BitmapContainer<WordType, DataBits>*>(a),
                                     static_cast<const BitmapContainer<WordType, DataBits>*>(b));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_equal_aa(static_cast<const ArrayContainer<WordType, DataBits>*>(a),
                                     static_cast<const ArrayContainer<WordType, DataBits>*>(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_equal_rr(static_cast<const RLEContainer<WordType, DataBits>*>(a),
                                     static_cast<const RLEContainer<WordType, DataBits>*>(b));
            break;
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_equal_ba(static_cast<const BitmapContainer<WordType, DataBits>*>(a),
                                     static_cast<const ArrayContainer<WordType, DataBits>*>(b));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_equal_ba(static_cast<const BitmapContainer<WordType, DataBits>*>(b),
                                     static_cast<const ArrayContainer<WordType, DataBits>*>(a));
            break;
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_equal_br(static_cast<const BitmapContainer<WordType, DataBits>*>(a),
                                     static_cast<const RLEContainer<WordType, DataBits>*>(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_equal_br(static_cast<const BitmapContainer<WordType, DataBits>*>(b),
                                     static_cast<const RLEContainer<WordType, DataBits>*>(a));
            break;
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_equal_ar(static_cast<const ArrayContainer<WordType, DataBits>*>(a),
                                     static_cast<const RLEContainer<WordType, DataBits>*>(b));
            break;
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_equal_ar(static_cast<const ArrayContainer<WordType, DataBits>*>(b),
                                     static_cast<const RLEContainer<WordType, DataBits>*>(a));
            break;
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring