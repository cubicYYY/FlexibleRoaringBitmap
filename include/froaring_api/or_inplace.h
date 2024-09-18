#pragma once

// FIXME: convert to full rle

#include <bit>

#include "array_container.h"
#include "bitmap_container.h"
#include "mix_ops.h"
#include "or.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_bb(BitmapContainer<WordType, DataBits>* a,
                                             const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    for (size_t i = 0; i < a->WordsCount; ++i) {
        a->words[i] |= b->words[i];
    }
    result_type = CTy::Bitmap;

    return a;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_aa(ArrayContainer<WordType, DataBits>* a,
                                             const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    if (b->size == 0) {
        result_type = CTy::Array;
        return a;
    }
    if (a->size == 0) {
        a->expand_to(b->size);
        a->size = b->size;
        std::memcpy(a->vals, b->vals, b->size * sizeof(typename ArrayContainer<WordType, DataBits>::IndexOrNumType));
        result_type = CTy::Array;
        return a;
    }
    size_t estimate_card = a->size + b->size;
    if (estimate_card < ArrayContainer<WordType, DataBits>::ArrayToBitmapCountThreshold) {
        // No inplace op is available
        return froaring_or_aa(a, b, result_type);
    }
    // If the result may be large...
    result_type = CTy::Bitmap;
    auto bitset_ptr = new BitmapContainer<WordType, DataBits>();
    bitmap_set_array(bitset_ptr, a);
    bitmap_set_array(bitset_ptr, b);
    return bitset_ptr;
    // TODO: maybe convert to array if the result cardinality is low?
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_rr(RLEContainer<WordType, DataBits>* a,
                                             const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    // TODO: inplace!
    return froaring_or_rr(a, b, result_type);
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ar(ArrayContainer<WordType, DataBits>* a,
                                             const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    return froaring_or_ar(a, b, result_type);  // No need to actually do it in-place
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ra(RLEContainer<WordType, DataBits>* a,
                                             const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    // TODO: inplace!
    return froaring_or_ar(b, a, result_type);
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_br(BitmapContainer<WordType, DataBits>* a,
                                             const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: Auto transform into full RLE
    result_type = CTy::Bitmap;
    auto rle_count = b->run_count;
    for (size_t i = 0; i < rle_count; i++) {
        a->set_range(b->runs[i].start, b->runs[i].end);
    }
    return a;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_rb(RLEContainer<WordType, DataBits>* a,
                                             const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: convert RLE to efficient type
    // TODO: inplace!
    return froaring_or_br(b, a, result_type);
}
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ba(BitmapContainer<WordType, DataBits>* a,
                                             const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    bitmap_set_array(a, b);
    result_type = CTy::Bitmap;
    return a;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ab(ArrayContainer<WordType, DataBits>* a,
                                             const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    return froaring_or_ba(b, a, result_type);
}
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_ori(froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb,
                                   CTy& result_type) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_or_inplace_bb(static_cast<BitmapSized*>(a), static_cast<const BitmapSized*>(b),
                                          result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_or_inplace_aa(static_cast<ArraySized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_or_inplace_rr(static_cast<RLESized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_or_inplace_ba(static_cast<BitmapSized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_or_inplace_ab(static_cast<ArraySized*>(a), static_cast<const BitmapSized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_or_inplace_br(static_cast<BitmapSized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_or_inplace_rb(static_cast<RLESized*>(a), static_cast<const BitmapSized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_or_inplace_ar(static_cast<ArraySized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_or_inplace_ra(static_cast<RLESized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring