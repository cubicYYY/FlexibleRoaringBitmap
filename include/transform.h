#pragma once

#include "array_container.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
template <typename WordType, size_t DataBits>
ArrayContainer<WordType, DataBits>* froaring_bitmap_to_array(const BitmapContainer<WordType, DataBits>* c) {
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
            ans->vals[outpos++] = (ArrayContainer<WordType, DataBits>::IndexOrNumType)(r + base);
            w ^= t;
        }
        base += DataBits;
    }
    return ans;
}
template <typename WordType, size_t DataBits>
BitmapContainer<WordType, DataBits>* froaring_array_to_bitmap(const ArrayContainer<WordType, DataBits>* c) {
    auto ans = new BitmapContainer<WordType, DataBits>();
    auto size = c->cardinality();
    for (int i = 0; i < size; ++i) {
        ans->set(c->vals[i]);
    }
    return ans;
}
};  // namespace froaring