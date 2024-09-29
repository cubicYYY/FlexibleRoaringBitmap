#pragma once

#include <bit>
#include <stop_token>

#include "array_container.h"
#include "bitmap_container.h"
#include "mix_ops.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
using CTy = froaring::ContainerType;

template <typename WordType, size_t DataBits>
bool froaring_intersects_bb(const BitmapContainer<WordType, DataBits>* a,
                            const BitmapContainer<WordType, DataBits>* b) {
    for (size_t i = 0; i < a->WordsCount; ++i) {
        if (a->words[i] & b->words[i]) {
            return true;
        }
    }
    return false;
}

template <typename WordType, size_t DataBits>
bool froaring_intersects_aa(const ArrayContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    if (a->size == 0 || b->size == 0) {
        return false;
    }

    if (a->size > b->size) {  // consider the small array first to make it faster
        std::swap(a, b);
    }

    size_t i = 0, j = 0;

    while (true) {
        while (a->vals[i] < b->vals[j]) {
        SKIP_FIRST_COMPARE:
            ++i;
            if (i == a->size) {
                return false;
            }
        }
        while (a->vals[i] > b->vals[j]) {
            ++j;
            if (j == b->size) {
                return false;
            }
        }
        if (a->vals[i] == b->vals[j]) {
            return true;
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE
}

template <typename WordType, size_t DataBits>
bool froaring_intersects_rr(const RLEContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    if (a->run_count == 0 || b->run_count == 0) {
        return false;
    }

    size_t i = 0, j = 0;
    while (true) {
        auto& run_a = a->runs[i];
        auto& run_b = b->runs[j];
        while (run_a.end < run_b.start) {
        SKIP_FIRST_COMPARE:
            ++i;
            if (i == a->run_count) {
                return false;
            }
        }
        while (run_b.end < run_a.start) {
            ++j;
            if (j == b->run_count) {
                return false;
            }
        }
        if (run_a.end >= run_b.start && run_b.end >= run_a.start) {
            return true;
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE
}

template <typename WordType, size_t DataBits>
bool froaring_intersects_ar(const ArrayContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    if (a->size == 0 || b->run_count == 0) {
        return false;
    }

    size_t i = 0, j = 0;
    while (true) {
        auto& val_a = a->vals[i];
        auto& run_b = b->runs[j];
        while (val_a < run_b.start) {
        SKIP_FIRST_COMPARE:
            ++i;
            if (i == a->size) {
                return false;
            }
        }
        while (run_b.end < val_a) {
            ++j;
            if (j == b->run_count) {
                return false;
            }
        }
        if (val_a >= run_b.start && run_b.end >= val_a) {
            return true;
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    FROARING_UNREACHABLE
}

template <typename WordType, size_t DataBits>
bool froaring_intersects_br(const BitmapContainer<WordType, DataBits>* a, const RLEContainer<WordType, DataBits>* b) {
    auto* result = new BitmapContainer<WordType, DataBits>(*a);
    auto rle_count = b->run_count;
    for (size_t i = 0; i < rle_count; i++) {
        if (result->any_range(b->runs[i].start, b->runs[i].end)) {
            return true;
        }
    }
    return false;
}

template <typename WordType, size_t DataBits>
bool froaring_intersects_ba(const BitmapContainer<WordType, DataBits>* a, const ArrayContainer<WordType, DataBits>* b) {
    auto array_size = b->size;
    if (array_size == 0) {
        return false;
    }
    for (size_t i = 0; i < array_size; i++) {
        if (a->test(b->vals[i])) {
            return true;
        }
    }
    return false;
}

template <typename WordType, size_t DataBits>
bool froaring_intersects(const froaring_container_t* a, const froaring_container_t* b, CTy ta, CTy tb) {
    using RLESized = RLEContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    switch (CTYPE_PAIR(ta, tb)) {
        case CTYPE_PAIR(CTy::Bitmap, CTy::Bitmap): {
            return froaring_intersects_bb(static_cast<const BitmapSized*>(a), static_cast<const BitmapSized*>(b));
        }
        case CTYPE_PAIR(CTy::Array, CTy::Array): {
            return froaring_intersects_aa(static_cast<const ArraySized*>(a), static_cast<const ArraySized*>(b));
        }
        case CTYPE_PAIR(CTy::RLE, CTy::RLE): {
            return froaring_intersects_rr(static_cast<const RLESized*>(a), static_cast<const RLESized*>(b));
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::Array): {
            return froaring_intersects_ba(static_cast<const BitmapSized*>(a), static_cast<const ArraySized*>(b));
        }
        case CTYPE_PAIR(CTy::Array, CTy::Bitmap): {
            return froaring_intersects_ba(static_cast<const BitmapSized*>(b), static_cast<const ArraySized*>(a));
        }
        case CTYPE_PAIR(CTy::Bitmap, CTy::RLE): {
            return froaring_intersects_br(static_cast<const BitmapSized*>(a), static_cast<const RLESized*>(b));
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Bitmap): {
            return froaring_intersects_br(static_cast<const BitmapSized*>(b), static_cast<const RLESized*>(a));
        }
        case CTYPE_PAIR(CTy::Array, CTy::RLE): {
            return froaring_intersects_ar(static_cast<const ArraySized*>(a), static_cast<const RLESized*>(b));
        }
        case CTYPE_PAIR(CTy::RLE, CTy::Array): {
            return froaring_intersects_ar(static_cast<const ArraySized*>(b), static_cast<const RLESized*>(a));
        }
        default:
            FROARING_UNREACHABLE
    }
}
}  // namespace froaring