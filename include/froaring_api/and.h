#pragma once

#include <bit>

#include "array_container.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_bb(const BitmapContainer<WordType, DataBits>* a,
                                      const BitmapContainer<WordType, DataBits>* b, CTy& result_type) {
    auto* result = new BitmapContainer<WordType, DataBits>();
    for (size_t i = 0; i < a->WordsCount; ++i) {
        result->words[i] = a->words[i] & b->words[i];
    }
    result_type = CTy::Bitmap;
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_aa(const ArrayContainer<WordType, DataBits>* a,
                                      const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    auto* result = new ArrayContainer<WordType, DataBits>(std::min(a->size, b->size));
    if (a->size == 0 || b->size == 0) {
        result_type = CTy::Array;
        return result;
    }
    size_t i = 0, j = 0;
    size_t new_card = 0;
    // Linear scan
    result_type = CTy::Array;
    while (true) {
        while (a->vals[i] < b->vals[j]) {
        SKIP_FIRST_COMPARE:
            ++i;
            if (i == a->size) {
                result->size = new_card;
                return result;
            }
        }
        while (a->vals[i] > b->vals[j]) {
            ++j;
            if (j == b->size) {
                result->size = new_card;
                return result;
            }
        }
        if (a->vals[i] == b->vals[j]) {
            result->vals[new_card++] = a->vals[i];
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
froaring_container_t* froaring_and_rr(const RLEContainer<WordType, DataBits>* a,
                                      const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::RLE;

    auto* result = new RLEContainer<WordType, DataBits>(a->runsCount() + b->runsCount());
    if (a->run_count == 0 || b->run_count == 0) {
        return result;
    }
    size_t i = 0, j = 0;
    size_t new_card = 0;
    while (true) {
        auto& run_a = a->runs[i];
        auto& run_b = b->runs[j];
        while (run_a.end < run_b.start) {
        SKIP_FIRST_COMPARE:
            ++i;
            if (i == a->run_count) {
                result->run_count = new_card;
                return result;
            }
        }
        while (run_b.end < run_a.start) {
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
froaring_container_t* froaring_and_ar(const ArrayContainer<WordType, DataBits>* a,
                                      const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Array;

    auto* result = new ArrayContainer<WordType, DataBits>(a->cardinality());
    if (a->size == 0 || b->run_count == 0) {
        return result;
    }

    size_t rlepos = 0;
    size_t arraypos = 0;
    size_t newcard = 0;
    auto rle = b->runs[rlepos];

    while (arraypos < a->size) {
        const auto arrayval = a->vals[arraypos];
        while (rle.end < arrayval) {
            ++rlepos;
            if (rlepos == b->run_count) {  // done
                result->size = newcard;
                return result;
            }
            rle = b->runs[rlepos];
        }
        if (rle.start > arrayval) {
            // FIXME: use Gallop Search (advanceUntil) if the array is big enough
            while (a->vals[arraypos] < rle.start && arraypos < a->size) {
                ++arraypos;
            }
        } else {
            result->vals[newcard++] = arrayval;
            ++arraypos;
        }
    }

    result->size = newcard;
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_br(const BitmapContainer<WordType, DataBits>* a,
                                      const RLEContainer<WordType, DataBits>* b, CTy& result_type) {
    auto rle_card = b->cardinality();
    if (rle_card == 0) {
        auto* result = new BitmapContainer<WordType, DataBits>();
        result_type = CTy::Bitmap;
        return result;
    }
    if (rle_card <= ArrayContainer<WordType, DataBits>::ArrayToBitmapCountThreshold) {
        auto* result = new ArrayContainer<WordType, DataBits>();
        size_t newcard = 0;

        // This branchless implementation reduces branch mispredictions
        for (size_t i = 0; i < rle_card; ++i) {
            auto run = b->runs[i];
            for (size_t val = run.start; val <= run.end; ++val) {
                result->vals[newcard] = val;
                newcard += a->test(val);
            }
        }
        result->size = newcard;
        result_type = CTy::Array;
        return result;
    }

    // If the cardinality is high, we first guess that the result will be a bitmap
    auto* result = new BitmapContainer<WordType, DataBits>(*a);
    typename BitmapContainer<WordType, DataBits>::NumType start;
    typename RLEContainer<WordType, DataBits>::RunPair first_run = b->runs[0];
    if (first_run.start > 0) {
        result->reset_range(0, first_run.start - 1);
    }
    start = first_run.end + 1;
    for (size_t i = 1; i < rle_card; ++i) {
        auto run = b->runs[i];
        auto end = run.start - 1;
        result->reset_range(start, end);
        start = end + 1;
    }
    result_type = CTy::Bitmap;

    // if the guess is wrong...
    // TODO: Should we convert it back to an array?

    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and_ba(const BitmapContainer<WordType, DataBits>* a,
                                      const ArrayContainer<WordType, DataBits>* b, CTy& result_type) {
    result_type = CTy::Array;

    auto* result = new ArrayContainer<WordType, DataBits>();
    auto array_size = b->cardinality();
    size_t newcard = 0;
    if (array_size == 0) {
        return result;
    }
    // This branchless implementation reduces branch mispredictions
    for (size_t i = 0; i < array_size; ++i) {
        auto val = b->vals[i];
        result->vals[newcard] = val;
        newcard += a->test(val);
    }
    result->size = newcard;
    return result;
}

template <typename WordType, size_t DataBits>
froaring_container_t* froaring_and(const froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb,
                                   CTy& result_type) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_and_bb(static_cast<const BitmapSized*>(a), static_cast<const BitmapSized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_and_aa(static_cast<const ArraySized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_and_rr(static_cast<const RLESized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_and_ba(static_cast<const BitmapSized*>(a), static_cast<const ArraySized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_and_ba(static_cast<const BitmapSized*>(b), static_cast<const ArraySized*>(a), result_type);
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_and_br(static_cast<const BitmapSized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_and_br(static_cast<const BitmapSized*>(b), static_cast<const RLESized*>(a), result_type);
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_and_ar(static_cast<const ArraySized*>(a), static_cast<const RLESized*>(b), result_type);
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_and_ar(static_cast<const ArraySized*>(b), static_cast<const RLESized*>(a), result_type);
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring