#pragma once

#include "array_container.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"
namespace froaring {
using CTy = froaring::ContainerType;
template <typename WordType, size_t DataBits>
inline bool container_empty(const froaring_container_t* c, CTy type) {
    switch (type) {
        case CTy::Array:
            return static_cast<const ArrayContainer<WordType, DataBits>*>(c)->cardinality() == 0;
        case CTy::Bitmap:
            return static_cast<const BitmapContainer<WordType, DataBits>*>(c)->cardinality() == 0;
        case CTy::RLE:
            return static_cast<const RLEContainer<WordType, DataBits>*>(c)->cardinality() == 0;
        default:
            FROARING_UNREACHABLE
    }
}

template <typename WordType, size_t DataBits>
inline void release_container(froaring_container_t* c, CTy type) {
    std::cout << "RELEASINGddd: " << (void*)c << std::endl;
    switch (type) {
        case CTy::Array:
            delete static_cast<ArrayContainer<WordType, DataBits>*>(c);
            break;
        case CTy::Bitmap:
            delete static_cast<BitmapContainer<WordType, DataBits>*>(c);
            break;
        case CTy::RLE:
            delete static_cast<RLEContainer<WordType, DataBits>*>(c);
            break;
        default:
            FROARING_UNREACHABLE
    }
}

template <typename WordType, size_t DataBits>
inline void release_container(ArrayContainer<WordType, DataBits>* c) {
    std::cout << "RELEASINGa: " << (void*)c << std::endl;
    delete c;
}

template <typename WordType, size_t DataBits>
inline void release_container(RLEContainer<WordType, DataBits>* c) {
    std::cout << "RELEASINGr: " << (void*)c << std::endl;
    delete c;
}

template <typename WordType, size_t DataBits>
inline void release_container(BitmapContainer<WordType, DataBits>* c) {
    std::cout << "RELEASINGb: " << (void*)c << std::endl;
    delete c;
}

}  // namespace froaring