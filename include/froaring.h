#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

#include "array_container.h"
#include "binsearch_index.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"
#include "utils.h"
#include "api.h"

namespace froaring {
/// @brief A flexible Roaring bitmap consists with a binary-search-indexed
/// layer, and underlying containers. Optimized for the case that only a single
/// container is needed: no index layer will be constructed until it gets
/// expanded.
/// @tparam WordType Suggested type for internal data storage. Currently only
/// used by Bitmap containers as the word size.
/// @tparam IndexBits high bits used for indexing.
/// @tparam DataBits low bits to be stored in containers.
template <typename WordType = uint64_t, size_t IndexBits = 16, size_t DataBits = 8>
class FlexibleRoaringBitmap {
    /// The container type for the index layer. Editable.
    // TODO: Support more index layers with maybe different types (maybe by Curiously Recurring Template Pattern)
    using ContainersSized = BinsearchIndex<WordType, IndexBits, DataBits>;

    using IndexType = froaring::can_fit_t<IndexBits>;
    using RLESized = RLEContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using CTy = froaring::ContainerType;  // handy local alias
    using ContainerHandle = froaring::ContainerHandle<IndexType>;

public:
    /// We start from an array container.
    FlexibleRoaringBitmap()
        : handle({
              new ArraySized(),
              CTy::Array,
              0,
          }) {}

    ~FlexibleRoaringBitmap() {
        switch (handle.type) {
            case CTy::Array:
                delete CAST_TO_ARRAY(handle.ptr);
                break;
            case CTy::Bitmap:
                delete CAST_TO_BITMAP(handle.ptr);
                break;
            case CTy::RLE:
                delete CAST_TO_RLE(handle.ptr);
                break;
            case CTy::Containers:
                delete castToContainers(handle.ptr);
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    void debug_print() {
        switch (handle.type) {
            case CTy::Array:
                CAST_TO_ARRAY(handle.ptr)->debug_print();
                break;
            case CTy::Bitmap:
                CAST_TO_BITMAP(handle.ptr)->debug_print();
                break;
            case CTy::RLE:
                CAST_TO_RLE(handle.ptr)->debug_print();
                break;
            case CTy::Containers:
                castToContainers(handle.ptr)->debug_print();
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    void set(WordType num) {
        if (handle.type == CTy::Containers) {
            castToContainers(handle.ptr)->set(num);
            return;
        }

        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(num, index, data);

        if (!was_set()) {
            flag |= WAS_SET;
            handle.index = index;
            assert(handle.type == CTy::Array && "Invalid initial container type");
            CAST_TO_ARRAY(handle.ptr)->set(data);
            return;
        }
        if (handle.index != index) {  // Single container, and is set:
            switchToContainers();
            castToContainers(handle.ptr)->set(num);
            return;
        }

        switch (handle.type) {
            case CTy::Array:
                CAST_TO_ARRAY(handle.ptr)->set(data);
                break;
            case CTy::Bitmap:
                CAST_TO_BITMAP(handle.ptr)->set(data);
                break;
            case CTy::RLE:
                CAST_TO_RLE(handle.ptr)->set(data);
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    bool test(WordType num) const {
        if (!was_set()) {
            return false;
        }

        if (handle.type == CTy::Containers) {
            return castToContainers(handle.ptr)->test(num);
        }

        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(num, index, data);

        if (handle.index != index) {  // Single container, and is set:
            return false;
        }
        switch (handle.type) {
            case CTy::Array:
                return index == handle.index && CAST_TO_ARRAY(handle.ptr)->test(data);
            case CTy::Bitmap:
                return index == handle.index && CAST_TO_BITMAP(handle.ptr)->test(data);
            case CTy::RLE:
                return index == handle.index && CAST_TO_RLE(handle.ptr)->test(data);
            case CTy::Containers:
                return castToContainers(handle.ptr)->test(num);
            default:
                FROARING_UNREACHABLE
        }

        return false;
    }

    void reset(WordType num) {
        if (!was_set()) {
            return;
        }
        if (handle.type == CTy::Containers) {
            castToContainers(handle.ptr)->reset(num);
            return;
        }
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(num, index, data);

        if (handle.index != index) {  // Single container, and is set:
            return;
        }

        switch (handle.type) {
            case CTy::Array:
                CAST_TO_ARRAY(handle.ptr)->reset(data);
                break;
            case CTy::Bitmap:
                CAST_TO_BITMAP(handle.ptr)->reset(data);
                break;
            case CTy::RLE:
                CAST_TO_RLE(handle.ptr)->reset(data);
                break;
            case CTy::Containers:
                castToContainers(handle.ptr)->reset(num);
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    size_t count() const {
        if (!was_set()) {
            return 0;
        }

        switch (handle.type) {
            case CTy::Array:
                return CAST_TO_ARRAY(handle.ptr)->cardinality();
            case CTy::Bitmap:
                return CAST_TO_BITMAP(handle.ptr)->cardinality();
            case CTy::RLE:
                return CAST_TO_RLE(handle.ptr)->cardinality();
            case CTy::Containers:
                return castToContainers(handle.ptr)->cardinality();
            default:
                FROARING_UNREACHABLE
        }
        return 0;
    }

    bool operator==(const FlexibleRoaringBitmap& other) const {
        if (!was_set()) {
            return (other.count() == 0);
        }
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            return froaring_equal_bsbs(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
        }
        if (handle.type == CTy::Containers) { // the other is not containers
            const ContainerHandle& lhs = castToContainers(handle.ptr)->containers[0];
            const ContainerHandle& rhs = other.handle;
            return froaring_equal<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type);
        }
        if (other.handle.type == CTy::Containers) { // other is not containers
            const ContainerHandle& lhs = castToContainers(other.handle.ptr)->containers[0];
            const ContainerHandle& rhs = other.handle;
            return froaring_equal<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type);
        }
        return froaring_equal<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type);
    }

    /// @brief Called when the current container exceeds the block size:
    /// transform into containers.
    void switchToContainers() {
        assert(handle.type != CTy::Containers && "Already indexed!");

        ContainersSized* containers = new ContainersSized(CONTAINERS_INIT_CAPACITY, 1);
        // TODO: do not modify the containers directly
        containers->containers[0] = std::move(handle);
        handle = ContainerHandle(containers, CTy::Containers, -1);
    }

    bool was_set() const { return flag & WAS_SET; }

private:
    ContainersSized* castToContainers(froaring_container_t* p) { return static_cast<ContainersSized*>(p); }
    froaring_container_t* castToFroaring(ContainersSized* p) { return static_cast<froaring_container_t*>(p); }
    const ContainersSized* castToContainers(const froaring_container_t* p) { return static_cast<const ContainersSized*>(p); }
    const froaring_container_t* castToFroaring(const ContainersSized* p) { return static_cast<const froaring_container_t*>(p); }

    const ContainersSized* castToContainers(const froaring_container_t* p) const {
        return static_cast<const ContainersSized*>(p);
    }

public:
    /// Bit#0: =0 if was NEVER set and the index in the handle is NOT
    /// determined; otherwise =1.
    /// Other bits: preserved
    uint8_t flag = 0;
    ContainerHandle handle{};
    // Possibilities:
    // If only a single container is needed, we can store it directly:
    // RLESized* rleContainer;
    // ArraySized* arrayContainer;
    // BitmapSized* bitmapContainer;
    //
    // Otherwise, the bit map is of containers:
    // ContainersSized* containers;
};
}  // namespace froaring