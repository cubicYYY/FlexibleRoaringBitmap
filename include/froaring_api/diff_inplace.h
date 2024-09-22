#pragma once

#include <bit>

#include "array_container.h"
#include "bitmap_container.h"
#include "diff.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_bb(BitmapContainer<WordType, DataBits>* a,
                                               const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    for (size_t i = 0; i < a->WordsCount; ++i) {
        a->words[i] &= (~b->words[i]);
    }
    result_type = CTy::Bitmap;

    return a;
    // TODO: may transform into array container if the cardinality is low
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_aa(ArrayContainer<WordType, DataBits>* a,
                                               const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    if (a->size == 0 || b->size == 0) {
        result_type = CTy::Array;
        return a;
    }

    size_t i = 0, j = 0;
    // Linear scan
    auto new_card = 0;
    // We can modify while scanning since the position being scanned moves forward faster or equal to the position being
    // overwrtitten
    result_type = CTy::Array;
    while (true) {
        while (a->vals[i] < b->vals[j]) {
        SKIP_FIRST_COMPARE:
            a->vals[new_card++] = a->vals[i];
            ++i;
            if (i == a->size) {
                a->size = new_card;
                return a;
            }
        }
        while (a->vals[i] > b->vals[j]) {
            ++j;
            if (j == b->size) {
                a->size = new_card;
                return a;
            }
        }
        if (a->vals[i] == b->vals[j]) {
            ++i;
            ++j;
            if (i == a->size || j == b->size) {
                a->size = new_card;
                return a;
            }
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_rr(RLEContainer<WordType, DataBits>* a,
                                               const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    return froaring_diff_rr(a, b, result_type);
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_ar(ArrayContainer<WordType, DataBits>* a,
                                               const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Array;

    if (a->size == 0 || b->size == 0) {
        return a;
    }

    if (b->run_count == 0) {
        return a;
    }

    size_t rlepos = 0;
    size_t arraypos = 0;
    size_t newcard = 0;
    auto rle = b->runs[rlepos];

    // We can overwrite the values since arraypos >= newcard
    while (true) {
        while (rle.end < a->vals[arraypos]) {
        SKIP_FIRST_COMPARE:
            ++rlepos;
            if (rlepos == b->run_count) {  // done
                while (arraypos < a->size) {
                    a->vals[newcard++] = a->vals[arraypos];
                    ++arraypos;
                }
                a->size = newcard;
                return a;
            }
            rle = b->runs[rlepos];
        }
        while (rle.start > a->vals[arraypos]) {
            a->vals[newcard++] = a->vals[arraypos];
            ++arraypos;
            if (arraypos == a->size) {
                a->size = newcard;
                return a;
            }
        }
        while (rle.start <= a->vals[arraypos] && a->vals[arraypos] <= rle.end) {
            // TODO: use Gallop search
            ++arraypos;
            if (arraypos == a->size) {
                a->size = newcard;
                return a;
            }
            goto SKIP_FIRST_COMPARE;
        }
    }

    FROARING_UNREACHABLE;
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_ra(RLEContainer<WordType, DataBits>* a,
                                               const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: inplace!
    return froaring_diff_ra(a, b, result_type);
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_br(BitmapContainer<WordType, DataBits>* a,
                                               const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Bitmap;
    for (size_t i = 0; i < b->run_count; i++) {
        a->reset_range(b->runs[i].start, b->runs[i].end);
    }
    return a;
    // TODO: convert into ArrayContainer if the cardinality is low?
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_rb(RLEContainer<WordType, DataBits>* a,
                                               const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    return froaring_diff_rb(a, b, result_type);
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_ba(BitmapContainer<WordType, DataBits>* a,
                                               const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: accelerate with assembly
    result_type = CTy::Bitmap;
    for (size_t i = 0; i < b->size; ++i) {
        auto val = b->vals[i];
        a->reset(val);
    }
    return a;
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_inplace_ab(ArrayContainer<WordType, DataBits>* a,
                                               const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    return froaring_diff_ab(a, b, result_type);
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diffi(froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb,
                                     CTy& result_type) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_diff_inplace_bb(static_cast<BitmapSized*>(a), static_cast<const BitmapSized*>(b),
                                            result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_diff_inplace_aa(static_cast<ArraySized*>(a), static_cast<const ArraySized*>(b),
                                            result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_diff_inplace_rr(static_cast<RLESized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_diff_inplace_ba(static_cast<BitmapSized*>(a), static_cast<const ArraySized*>(b),
                                            result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_diff_inplace_ab(static_cast<ArraySized*>(a), static_cast<const BitmapSized*>(b),
                                            result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_diff_inplace_br(static_cast<BitmapSized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_diff_inplace_rb(static_cast<RLESized*>(a), static_cast<const BitmapSized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_diff_inplace_ar(static_cast<ArraySized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_diff_inplace_ra(static_cast<RLESized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring