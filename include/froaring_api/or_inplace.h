#pragma once

// TODO: convert to full rle?

#include <bit>

#include "array_container.h"
#include "bitmap_container.h"
#include "froaring_api/utils.h"
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
froaring_container_t* froaring_or_inplace_bb_changed_chk(BitmapContainer<WordType, DataBits>* a,
                                                         const BitmapContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    changed = false;
    for (size_t i = 0; i < a->WordsCount; ++i) {
        changed |= ((a->words[i] & b->words[i]) != b->words[i]);
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
    // If the result may be large...
    if (estimate_card >= ArrayContainer<WordType, DataBits>::ArrayToBitmapCountThreshold) {
        result_type = CTy::Bitmap;
        auto bitset_ptr = array_to_bitmap(a);
        bitmap_set_array(bitset_ptr, b);
        return bitset_ptr;
    }

    // If the result is not so large:
    // Linear scan
    result_type = CTy::Array;
    size_t max_new_card = a->size + b->size;
    auto* result =
        new ArrayContainer<WordType, DataBits>(max_new_card);  // the union of sets never have no more elements
    size_t i = 0, j = 0, new_card = 0;
    while (true) {
        while (a->vals[i] < b->vals[j]) {
        SKIP_FIRST_COMPARE:
            result->vals[new_card++] = a->vals[i];
            ++i;
            if (i == a->size) {
                while (j < b->size) {
                    result->vals[new_card++] = b->vals[j];
                    j++;
                }
                result->size = new_card;
                return result;
            }
        }
        while (a->vals[i] > b->vals[j]) {
            result->vals[new_card++] = b->vals[j];
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
            result->vals[new_card++] = a->vals[i];
            ++i;
            ++j;
            if (i == a->size) {
                while (j < b->size) {
                    result->vals[new_card++] = b->vals[j];
                    j++;
                }
                result->size = new_card;
                return result;
            }
            if (j == b->size) {
                while (i < a->size) {
                    result->vals[new_card++] = a->vals[i];
                    i++;
                }
                result->size = new_card;
                return result;
            }
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE

    // TODO: maybe convert to array if the result cardinality is low?
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_aa_changed_chk(ArrayContainer<WordType, DataBits>* a,
                                                         const ArrayContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    changed = false;
    if (b->size == 0) {
        result_type = CTy::Array;
        changed = false;
        return a;
    }
    if (a->size == 0) {
        a->expand_to(b->size);
        a->size = b->size;
        std::memcpy(a->vals, b->vals, b->size * sizeof(typename ArrayContainer<WordType, DataBits>::IndexOrNumType));
        result_type = CTy::Array;
        changed = (b->size != 0);
        return a;
    }
    size_t estimate_card = a->size + b->size;
    // If the result may be large...
    if (estimate_card >= ArrayContainer<WordType, DataBits>::ArrayToBitmapCountThreshold) {
        result_type = CTy::Bitmap;
        auto bitset_ptr = array_to_bitmap(a);
        bitmap_set_array(bitset_ptr, b);
        changed = (bitset_ptr->cardinality() != a->size);
        return bitset_ptr;
    }

    // If the result is not so large:
    // Linear scan
    result_type = CTy::Array;
    size_t max_new_card = a->size + b->size;
    auto* result =
        new ArrayContainer<WordType, DataBits>(max_new_card);  // the union of sets never have no more elements
    size_t i = 0, j = 0, new_card = 0;
    while (true) {
        while (a->vals[i] < b->vals[j]) {
        SKIP_FIRST_COMPARE:
            result->vals[new_card++] = a->vals[i];
            ++i;
            if (i == a->size) {
                while (j < b->size) {
                    result->vals[new_card++] = b->vals[j];
                    j++;
                }
                result->size = new_card;
                return result;
            }
        }
        while (a->vals[i] > b->vals[j]) {
            changed = true;  // MODIFIED!

            result->vals[new_card++] = b->vals[j];
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
            result->vals[new_card++] = a->vals[i];
            ++i;
            ++j;
            if (i == a->size) {
                while (j < b->size) {
                    changed = true;  // MODIFIED!

                    result->vals[new_card++] = b->vals[j];
                    j++;
                }
                result->size = new_card;
                return result;
            }
            if (j == b->size) {
                while (i < a->size) {
                    result->vals[new_card++] = a->vals[i];
                    i++;
                }
                result->size = new_card;
                return result;
            }
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE

    // TODO: maybe convert to array if the result cardinality is low?
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_rr(RLEContainer<WordType, DataBits>* a,
                                             const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::RLE;
    if (a->run_count == 0) {
        return new RLEContainer<WordType, DataBits>(*b);
    }
    if (b->run_count == 0) {
        return new RLEContainer<WordType, DataBits>(*a);
    }

    auto* result = new RLEContainer<WordType, DataBits>(a->run_count + b->run_count);
    size_t i = 0, j = 0;
    size_t new_card = 0;
    // FIXME: Uncombined runs! e.g., [1,2] + [3,4] => [1,4], however this method will NOT perform this.
    // This MUST be fixed since we must make sure no equivalent representations exist.
    while (true) {
        auto& run_a = a->runs[i];
        auto& run_b = b->runs[j];
        while (run_a.end < run_b.start) {
        SKIP_FIRST_COMPARE:
            result->runs[new_card++] = {run_a.start, run_a.end};
            ++i;
            if (i == a->run_count) {
                result->run_count = new_card;
                return result;
            }
        }
        while (run_b.end < run_a.start) {
            result->runs[new_card++] = {run_b.start, run_b.end};
            ++j;
            if (j == b->run_count) {
                result->run_count = new_card;
                return result;
            }
        }
        if (run_a.end >= run_b.start && run_b.end >= run_a.start) {
            result->runs[new_card++] = {std::max(run_a.start, run_b.start), std::min(run_a.end, run_b.end)};
            if (run_a.end < run_b.end) {
                ++i;
            } else {
                ++j;
            }
            if (i == a->run_count || j == b->run_count) {
                result->run_count = new_card;
                return result;
            }
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_rr_changed_chk(RLEContainer<WordType, DataBits>* a,
                                                         const RLEContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    changed = false;
    result_type = CTy::RLE;
    if (a->run_count == 0) {
        return new RLEContainer<WordType, DataBits>(*b);
    }
    if (b->run_count == 0) {
        return new RLEContainer<WordType, DataBits>(*a);
    }

    auto* result = new RLEContainer<WordType, DataBits>(a->run_count + b->run_count);
    size_t i = 0, j = 0;
    size_t new_card = 0;
    // FIXME: Uncombined runs! e.g., [1,2] + [3,4] => [1,4], however this method will NOT perform this.
    // This MUST be fixed since we must make sure no equivalent representations exist.
    while (true) {
        auto& run_a = a->runs[i];
        auto& run_b = b->runs[j];
        while (run_a.end < run_b.start) {
        SKIP_FIRST_COMPARE:
            result->runs[new_card++] = {run_a.start, run_a.end};
            ++i;
            if (i == a->run_count) {
                result->run_count = new_card;
                return result;
            }
        }
        while (run_b.end < run_a.start) {
            changed = true;  // MODIFIED!
            result->runs[new_card++] = {run_b.start, run_b.end};
            ++j;
            if (j == b->run_count) {
                result->run_count = new_card;
                return result;
            }
        }
        if (run_a.end >= run_b.start && run_b.end >= run_a.start) {
            changed = (run_a.end != run_b.end || run_a.start != run_b.start);  // MODIFIED!
            result->runs[new_card++] = {std::max(run_a.start, run_b.start), std::min(run_a.end, run_b.end)};
            if (run_a.end < run_b.end) {
                ++i;
            } else {
                ++j;
            }
            if (i == a->run_count || j == b->run_count) {
                result->run_count = new_card;
                return result;
            }
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ar_changed_chk(ArrayContainer<WordType, DataBits>* a,
                                                         const RLEContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    result_type = CTy::RLE;
    // FIXME: RLE container may overflow!

    changed = false;
    auto* result = new RLEContainer<WordType, DataBits>(*b);
    auto array_size = a->size;
    for (size_t i = 0; i < array_size; i++) {
        changed |= result->test_and_set(a->vals[i]);  // FIXME: Do not use set() but manually set on run!
    }
    return result;
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ar(ArrayContainer<WordType, DataBits>* a,
                                             const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::RLE;
    // FIXME: RLE container may overflow!

    auto* result = new RLEContainer<WordType, DataBits>(*b);
    auto array_size = a->size;
    for (size_t i = 0; i < array_size; i++) {
        result->set(a->vals[i]);  // FIXME: Do not use set() but manually set on run!
    }
    return result;
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ra_changed_chk(RLEContainer<WordType, DataBits>* a,
                                                         const ArrayContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    result_type = CTy::RLE;
    // FIXME: RLE container may overflow!

    changed = false;
    auto array_size = b->size;
    for (size_t i = 0; i < array_size; i++) {
        changed |= a->test_and_set(b->vals[i]);  // FIXME: Do not use set() but manually set on run!
    }
    return a;
}

/// NOT in-place internally
template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ra(RLEContainer<WordType, DataBits>* a,
                                             const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::RLE;
    // FIXME: RLE container may overflow!

    auto array_size = b->size;
    for (size_t i = 0; i < array_size; i++) {
        a->set(b->vals[i]);  // FIXME: Do not use set() but manually set on run!
    }
    return a;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_br_changed_chk(BitmapContainer<WordType, DataBits>* a,
                                                         const RLEContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    // TODO: Auto transform into full RLE
    auto origin_size = a->cardinality();
    result_type = CTy::Bitmap;
    auto rle_count = b->run_count;
    for (size_t i = 0; i < rle_count; i++) {
        a->set_range(b->runs[i].start, b->runs[i].end);
    }
    changed = (a->cardinality() != origin_size);
    return a;
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
froaring_container_t* froaring_or_inplace_rb_changed_chk(RLEContainer<WordType, DataBits>* a,
                                                         const BitmapContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    // TODO: Auto transform into full RLE
    result_type = CTy::Bitmap;
    auto* result = new BitmapContainer<WordType, DataBits>(*b);

    auto origin_size = b->cardinality();
    auto rle_count = a->run_count;
    for (size_t i = 0; i < rle_count; i++) {
        result->set_range(a->runs[i].start, a->runs[i].end);
    }
    changed = (result->cardinality() != origin_size);
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_rb(RLEContainer<WordType, DataBits>* a,
                                             const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    // TODO: Auto transform into full RLE
    result_type = CTy::Bitmap;
    auto* result = new BitmapContainer<WordType, DataBits>(*b);

    auto rle_count = a->run_count;
    for (size_t i = 0; i < rle_count; i++) {
        result->set_range(a->runs[i].start, a->runs[i].end);
    }
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ba_changed_chk(BitmapContainer<WordType, DataBits>* a,
                                                         const ArrayContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    result_type = CTy::Bitmap;
    auto size = b->cardinality();
    changed = false;
    for (size_t i = 0; i < size; ++i) {
        changed |= a->test_and_set(b->vals[i]);
    }
    return a;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ba(BitmapContainer<WordType, DataBits>* a,
                                             const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Bitmap;
    auto size = b->cardinality();
    for (size_t i = 0; i < size; ++i) {
        a->set(b->vals[i]);
    }
    return a;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ab_changed_chk(ArrayContainer<WordType, DataBits>* a,
                                                         const BitmapContainer<WordType, DataBits>* b, CTy& result_type,
                                                         bool& changed) {
    result_type = CTy::Bitmap;
    auto* result = new BitmapContainer<WordType, DataBits>(*b);
    auto size = a->cardinality();
    changed = false;
    for (size_t i = 0; i < size; ++i) {
        changed |= result->test_and_set(a->vals[i]);
    }
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_inplace_ab(ArrayContainer<WordType, DataBits>* a,
                                             const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Bitmap;
    auto* result = new BitmapContainer<WordType, DataBits>(*b);
    auto size = a->cardinality();
    for (size_t i = 0; i < size; ++i) {
        result->set(a->vals[i]);
    }
    return result;
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

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_ori_changed_chk(froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb,
                                               CTy& result_type, bool& changed) {
    changed = false;  // init
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_or_inplace_bb_changed_chk(static_cast<BitmapSized*>(a), static_cast<const BitmapSized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_or_inplace_aa_changed_chk(static_cast<ArraySized*>(a), static_cast<const ArraySized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_or_inplace_rr_changed_chk(static_cast<RLESized*>(a), static_cast<const RLESized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_or_inplace_ba_changed_chk(static_cast<BitmapSized*>(a), static_cast<const ArraySized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_or_inplace_ab_changed_chk(static_cast<ArraySized*>(a), static_cast<const BitmapSized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_or_inplace_br_changed_chk(static_cast<BitmapSized*>(a), static_cast<const RLESized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_or_inplace_rb_changed_chk(static_cast<RLESized*>(a), static_cast<const BitmapSized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_or_inplace_ar_changed_chk(static_cast<ArraySized*>(a), static_cast<const RLESized*>(b),
                                                      result_type, changed);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_or_inplace_ra_changed_chk(static_cast<RLESized*>(a), static_cast<const ArraySized*>(b),
                                                      result_type, changed);
        }
        default:
            FROARING_UNREACHABLE
    }
}

}  // namespace froaring