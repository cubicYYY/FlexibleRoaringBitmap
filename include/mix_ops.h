#pragma once

#include "array_container.h"
#include "bitmap_container.h"
#include "handle.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
template <typename WordType, size_t DataBits>
inline ArrayContainer<WordType, DataBits>* froaring_bitmap_to_array(const BitmapContainer<WordType, DataBits>* c) {
    // TODO: accelerate with SSE, AVX2 or AVX512
    auto cardinality = c->cardinality();
    auto ans = new ArrayContainer<WordType, DataBits>(cardinality, cardinality);
    int outpos = 0;
    typename ArrayContainer<WordType, DataBits>::IndexOrNumType base = 0;
    for (size_t i = 0; i < c->WordsCount; ++i) {
        WordType w = c->words[i];
        while (w != 0) {
            WordType t = w & (~w + 1);
            auto r = std::countr_zero(w);
            ans->vals[outpos++] = (typename ArrayContainer<WordType, DataBits>::IndexOrNumType)(r + base);
            w ^= t;
        }
        base += DataBits;
    }
    return ans;
}
template <typename WordType, size_t DataBits>
inline BitmapContainer<WordType, DataBits>* froaring_array_to_bitmap(const ArrayContainer<WordType, DataBits>* c) {
    auto ans = new BitmapContainer<WordType, DataBits>();
    auto size = c->cardinality();
    for (size_t i = 0; i < size; ++i) {
        ans->set(c->vals[i]);
    }
    return ans;
}
template <typename WordType, size_t DataBits>
inline void froaring_bitmap_set_array(BitmapContainer<WordType, DataBits>* b,
                                      const ArrayContainer<WordType, DataBits>* a) {
    auto size = a->cardinality();
    for (size_t i = 0; i < size; ++i) {
        b->set(a->vals[i]);
    }
}
template <typename WordType, size_t DataBits>
inline froaring_container_t* froaring_duplicate_container(const froaring_container_t* c, ContainerType ctype) {
    switch (ctype) {
        case ContainerType::Array:
            return new ArrayContainer<WordType, DataBits>(*static_cast<const ArrayContainer<WordType, DataBits>*>(c));
        case ContainerType::Bitmap:
            return new BitmapContainer<WordType, DataBits>(*static_cast<const BitmapContainer<WordType, DataBits>*>(c));
        case ContainerType::RLE:
            return new RLEContainer<WordType, DataBits>(*static_cast<const RLEContainer<WordType, DataBits>*>(c));
        default:
            FROARING_UNREACHABLE
    }
}

template <typename WordType, typename IndexType, size_t DataBits>
inline ContainerHandle<IndexType> froaring_duplicate_container(const ContainerHandle<IndexType>& c) {
    froaring_container_t* ptr;
    switch (c.type) {
        case ContainerType::Array:
            ptr = new ArrayContainer<WordType, DataBits>(*static_cast<const ArrayContainer<WordType, DataBits>*>(c.ptr));
            break;
        case ContainerType::Bitmap:
            ptr = new BitmapContainer<WordType, DataBits>(*static_cast<const BitmapContainer<WordType, DataBits>*>(c.ptr));
            break;
        case ContainerType::RLE:
            ptr = new RLEContainer<WordType, DataBits>(*static_cast<const RLEContainer<WordType, DataBits>*>(c.ptr));
            break;
        default:
            FROARING_UNREACHABLE
    }
    return ContainerHandle<IndexType>(ptr, c.type, c.index);
}
};  // namespace froaring