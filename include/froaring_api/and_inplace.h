#pragma once

#include <bit>

#include "array_container.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_bb(BitmapContainer<WordType, DataBits>* a,
                                              const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    for (size_t i = 0; i < a->WordsCount; ++i) {
        a->words[i] &= b->words[i];
    }
    result_type = CTy::Bitmap;

    return a;
    // TODO: may transform into array container if the cardinality is low
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_aa(ArrayContainer<WordType, DataBits>* a,
                                              const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    if (a->size == 0 || b->size == 0) {
        a->clear();
        result_type = CTy::Array;
        return a;
    }

    size_t i = 0, j = 0;
    // TODO: handle small & large arrays' intersection (skewed)

    // Linear scan
    auto new_size = 0;
    // We can modify while scan since the position being scanned moves forward faster or equal to the position being
    // overwrtitten
    while (i < a->size && j < b->size) {
        while (a->vals[i] < b->vals[j]) {
        SKIP_FIRST_COMPARE:
            ++i;
        }
        while (a->vals[i] > b->vals[j]) {
            ++j;
        }
        if (a->vals[i] == b->vals[j]) {
            a->vals[new_size++] = a->vals[i];
            ++i;
            ++j;
            if (i == a->size || j == b->size) {
                a->size = new_size;
                result_type = CTy::Array;
                return a;
            }
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    a->size = new_size;
    result_type = CTy::Array;
    return a;
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_rr(RLEContainer<WordType, DataBits>* a,
                                              const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    return froaring_and_rr(a, b, result_type);
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_ar(ArrayContainer<WordType, DataBits>* a,
                                              const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    return froaring_and_ar(a, b, result_type);  // No need to actually do it in-place
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_ra(RLEContainer<WordType, DataBits>* a,
                                              const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    return froaring_and_ar(b, a, result_type);
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_br(BitmapContainer<WordType, DataBits>* a,
                                              const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    // TODO: inplace!
    return froaring_and_br(a, b, result_type);
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_rb(RLEContainer<WordType, DataBits>* a,
                                              const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    // TODO: inplace!
    return froaring_and_br(b, a, result_type);
}
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_ba(BitmapContainer<WordType, DataBits>* a,
                                              const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    return froaring_and_ba(a, b, result_type);  // should be Array type
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_inplace_ab(ArrayContainer<WordType, DataBits>* a,
                                              const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Array;

    size_t new_card = 0;
    const size_t origcard = a->size;

    for (size_t i = 0; i < origcard; i++) {
        typename ArrayContainer<WordType, DataBits>::IndexOrNumType key = a->vals[i];
        a->vals[new_card] = key;
        new_card += b->test(key);
    }
    a->size = new_card;
    return a;
}
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_andi(froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb,
                                    CTy& result_type) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_and_inplace_bb(static_cast<BitmapSized*>(a), static_cast<const BitmapSized*>(b),
                                           result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_and_inplace_aa(static_cast<ArraySized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_and_inplace_rr(static_cast<RLESized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_and_inplace_ba(static_cast<BitmapSized*>(a), static_cast<const ArraySized*>(b),
                                           result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_and_inplace_ab(static_cast<ArraySized*>(a), static_cast<const BitmapSized*>(b),
                                           result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_and_inplace_br(static_cast<BitmapSized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_and_inplace_rb(static_cast<RLESized*>(a), static_cast<const BitmapSized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_and_inplace_ar(static_cast<ArraySized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_and_inplace_ra(static_cast<RLESized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring