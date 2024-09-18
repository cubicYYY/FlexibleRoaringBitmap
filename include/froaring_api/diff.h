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
froaring_container_t* froaring_diff_bb(const BitmapContainer<WordType, DataBits>* a,
                                       const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    auto* result = new BitmapContainer<WordType, DataBits>();
    for (size_t i = 0; i < a->WordsCount; ++i) {
        result->words[i] = a->words[i] & (~b->words[i]);
    }
    result_type = CTy::Bitmap;
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_aa(const ArrayContainer<WordType, DataBits>* a,
                                       const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    if (a->size == 0) {
        result_type = CTy::Array;
        return new ArrayContainer<WordType, DataBits>();
    }

    if (b->size == 0) {
        result_type = CTy::Array;
        return new ArrayContainer<WordType, DataBits>(*a);
    }

    auto* result = new ArrayContainer<WordType, DataBits>(a->size);
    size_t i = 0, j = 0;
    size_t new_card = 0;
    // Linear scan
    result_type = CTy::Array;
    while (true) {
        while (a->vals[i] < b->vals[j]) {
        SKIP_FIRST_COMPARE:
            result->vals[new_card++] = a->vals[i];
            ++i;
            if (i == a->size) {
                result->size = new_card;
                return result;
            }
        }
        while (a->vals[i] > b->vals[j]) {
            ++j;
            if (j == b->size) {
                while (i < a->size) {
                    result->vals[new_card++] = a->vals[i];
                    i++;
                }
                result->size = new_card;
                return result;
            }
        }
        if (a->vals[i] == b->vals[j]) {
            ++i;
            ++j;
            if (i == a->size || j == b->size) {
                result->size = new_card;
                return result;
            }
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_rr(const RLEContainer<WordType, DataBits>* a,
                                       const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO:...
    FROARING_NOT_IMPLEMENTED
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_ar(const ArrayContainer<WordType, DataBits>* a,
                                       const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Array;

    if (a->size == 0) {
        return new ArrayContainer<WordType, DataBits>();
    }

    if (b->run_count == 0) {
        return new ArrayContainer<WordType, DataBits>(*a);
    }
    auto* result = new ArrayContainer<WordType, DataBits>(a->cardinality());

    size_t rlepos = 0;
    size_t arraypos = 0;
    size_t newcard = 0;
    auto rle = b->runs[rlepos];

    while (true) {
        while (rle.end < a->vals[arraypos]) {
        SKIP_FIRST_COMPARE:
            ++rlepos;
            if (rlepos == b->run_count) {  // done
                while (arraypos < a->size) {
                    result->vals[newcard++] = a->vals[arraypos];
                    ++arraypos;
                }
                result->size = newcard;
                return result;
            }
            rle = b->runs[rlepos];
        }
        while (rle.start > a->vals[arraypos]) {
            result->vals[newcard++] = a->vals[arraypos];
            ++arraypos;
            if (arraypos == a->size) {
                result->size = newcard;
                return result;
            }
        }
        while (rle.start <= a->vals[arraypos] && a->vals[arraypos] <= rle.end) {
            // TODO: use Gallop search
            ++arraypos;
            if (arraypos == a->size) {
                result->size = newcard;
                return result;
            }
            goto SKIP_FIRST_COMPARE;
        }
    }

    FROARING_UNREACHABLE;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_br(const BitmapContainer<WordType, DataBits>* a,
                                       const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    if (b->run_count == 0) {
        auto* result = new BitmapContainer<WordType, DataBits>(*a);
        result_type = CTy::Bitmap;
        return result;
    }

    auto* result = new BitmapContainer<WordType, DataBits>(*a);
    for (size_t i = 0; i < b->run_count; i++) {
        result->reset_range(b->runs[i].start, b->runs[i].end);
    }
    result_type = CTy::Bitmap;
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff_ba(const BitmapContainer<WordType, DataBits>* a,
                                       const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Bitmap;
    auto* result = new BitmapContainer<WordType, DataBits>(*a);
    for (size_t i = 0; i < b->size; ++i) {
        auto val = b->vals[i];
        result->reset(val);
    }
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_diff(const froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb,
                                    CTy& result_type) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_diff_bb(static_cast<const BitmapSized*>(a), static_cast<const BitmapSized*>(b),
                                    result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_diff_aa(static_cast<const ArraySized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_diff_rr(static_cast<const RLESized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_diff_ba(static_cast<const BitmapSized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_diff_ba(static_cast<const BitmapSized*>(b), static_cast<const ArraySized*>(a), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_diff_br(static_cast<const BitmapSized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_diff_br(static_cast<const BitmapSized*>(b), static_cast<const RLESized*>(a), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_diff_ar(static_cast<const ArraySized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_diff_ar(static_cast<const ArraySized*>(b), static_cast<const RLESized*>(a), result_type);
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring