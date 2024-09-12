#pragma once

#include <bit>
#include <stop_token>

#include "array_container.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"
#include "mix_ops.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_bb(const BitmapContainer<WordType, DataBits>* a,
                                     const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    auto* result = new BitmapContainer<WordType, DataBits>();
    for (size_t i = 0; i < a->WordsCount; ++i) {
        result->words[i] = a->words[i] | b->words[i];
    }
    result_type = CTy::Bitmap;
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_aa(const ArrayContainer<WordType, DataBits>* a,
                                     const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    // If one of them is empty: just return the copy of the other one
    if (a->size == 0) {
        result_type = CTy::Array;
        return new ArrayContainer<WordType, DataBits>(*b);
    }
    if (b->size == 0) {
        result_type = CTy::Array;
        return new ArrayContainer<WordType, DataBits>(*a);
    }
    std::cout << "ORing two arrays" << std::endl;
    size_t max_new_card = a->size + b->size;
    if (a->size > b->size) {  // consider the small array first to make it faster
        std::swap(a, b);
    }

    // If both are small, we'll definitely get a new ArrayContainer
    if (max_new_card <=
        ArrayContainer<WordType, DataBits>::ArrayToBitmapCountThreshold) { 
        auto* result =
            new ArrayContainer<WordType, DataBits>(max_new_card);  // the union of sets never have no more elements

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
    }

    // Otherwise, the result may be dense, so we use a BitmapContainer
    result_type = CTy::Bitmap;
    auto* result = froaring_array_to_bitmap(a);
    froaring_bitmap_set_array(result, b);
    auto new_bitmap_card = b->cardinality();
    if (new_bitmap_card < ArrayContainer<WordType, DataBits>::ArrayToBitmapCountThreshold) { // 
        result_type = CTy::Array;
        return froaring_bitmap_to_array(result);
    }
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_rr(const RLEContainer<WordType, DataBits>* a,
                                     const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::RLE;
    if (a->run_count == 0) {
        return new RLEContainer<WordType, DataBits>(*b);
    }
    if (b->run_count == 0) {
        return new RLEContainer<WordType, DataBits>(*a);
    }

    auto* result = new RLEContainer<WordType, DataBits>(a->runsCount() + b->runsCount());
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

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_ar(const ArrayContainer<WordType, DataBits>* a,
                                     const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::RLE;

    auto* result = new RLEContainer<WordType, DataBits>(*b);
    auto array_size = a->size;
    for (size_t i=0;i<array_size;i++) {
        result->set(a->vals[i]);
    }
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_br(const BitmapContainer<WordType, DataBits>* a,
                                     const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Bitmap;
    auto* result = new BitmapContainer<WordType, DataBits>(*a);
    auto rle_count = b->runsCount();
    for (size_t i=0;i<rle_count;i++) {
        result->set_range(b->runs[i].start, b->runs[i].end);
    }
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or_ba(const BitmapContainer<WordType, DataBits>* a,
                                     const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Bitmap;
    auto* result = new BitmapContainer<WordType, DataBits>(*a);
    froaring_bitmap_set_array(result, b);
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_or(const froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb,
                                  CTy& result_type) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_or_bb(static_cast<const BitmapSized*>(a), static_cast<const BitmapSized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_or_aa(static_cast<const ArraySized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_or_rr(static_cast<const RLESized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_or_ba(static_cast<const BitmapSized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_or_ba(static_cast<const BitmapSized*>(b), static_cast<const ArraySized*>(a), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_or_br(static_cast<const BitmapSized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_or_br(static_cast<const BitmapSized*>(b), static_cast<const RLESized*>(a), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_or_ar(static_cast<const ArraySized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_or_ar(static_cast<const ArraySized*>(b), static_cast<const RLESized*>(a), result_type);
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring