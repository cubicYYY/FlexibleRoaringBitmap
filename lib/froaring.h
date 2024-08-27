#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

#include "array_container.h"
#include "bitmap_container.h"
#include "containers.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
/// @brief A flexible Roaring bitmap that stores containers of different types.
/// If only a single container is needed, the bitmap will be the container
/// itself.
template <typename WordType = uint64_t, size_t IndexBits = 16,
          size_t DataBits = 8>
class FlexibleRoaringBitmap {
    using IndexType = froaring::can_fit_t<IndexBits>;

    IndexType start_index = 0;  // The index of the container. This is only used
                                // if the bitmap is storing a single container.
    ContainerType type;
    union {
        // If only a single container is needed, we can store it directly.
        RLEContainer<WordType, DataBits> rleContainer;
        ArrayContainer<WordType, DataBits> arrayContainer;
        BitmapContainer<WordType, DataBits> bitmapContainer;
        // Otherwise, the bit map is of containers.
        Containers<WordType, IndexBits, DataBits> containers;
    };
    using CTy = froaring::ContainerType;  // handy local alias

public:
    /// We start from an array container.
    FlexibleRoaringBitmap() : type(CTy::Array) {
        new (&arrayContainer) ArrayContainer<WordType, DataBits>();
    }

    ~FlexibleRoaringBitmap() { destroyCurrentContainer(); }

    void set(WordType index) {
        switch (type) {
            case CTy::RLE:
                rleContainer.set(index);
                if (arrayContainer.cardinality() > DataBits) {
                    switchToContainers();
                }
                break;
            case CTy::Array:
                arrayContainer.set(index);
                if (arrayContainer.cardinality() > DataBits) {
                    switchToContainers();
                }
                break;
            case CTy::Bitmap:
                bitmapContainer.set(index);
                if (arrayContainer.cardinality() > DataBits) {
                    switchToContainers();
                }
                break;
            case CTy::Containers:
                containers.set(index);
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    bool test(WordType index) const {
        switch (type) {
            case CTy::RLE:
                return rleContainer.test(index);
            case CTy::Array:
                return arrayContainer.test(index);
            case CTy::Bitmap:
                return bitmapContainer.test(index);
            case CTy::Containers:
                return containers.test(index);
            default:
                FROARING_UNREACHABLE
        }

        return false;
    }

    size_t cardinality() const {
        switch (type) {
            case CTy::RLE:
                return rleContainer.cardinality();
            case CTy::Array:
                return arrayContainer.cardinality();
            case CTy::Bitmap:
                return bitmapContainer.cardinality();
            case CTy::Containers:
                return containers.cardinality();
            default:
                FROARING_UNREACHABLE
        }
        return 0;
    }

private:
    void destroyCurrentContainer() {
        switch (type) {
            case CTy::RLE:
                rleContainer.~RLEContainer();
                break;
            case CTy::Array:
                arrayContainer.~ArrayContainer();
                break;
            case CTy::Bitmap:
                bitmapContainer.~BitmapContainer();
                break;
            case CTy::Containers:
                containers.~Containers();
                break;
            default:
                FROARING_UNREACHABLE
        }
        return;
    }

    /// @brief Called when the current container exceeds the block size:
    /// transform into containers.
    void switchToContainers() {
        assert(
            type == CTy::Array &&
            "Only array can switch to containers (otherwise not implemented)");
        Containers<WordType, IndexBits, DataBits> newContainers;
        // TODO: newContainers.add(arrayContainer);
        destroyCurrentContainer();
        new (&containers)
            Containers<WordType, IndexBits, DataBits>(std::move(newContainers));
        type = CTy::Containers;
    }
};
}  // namespace froaring