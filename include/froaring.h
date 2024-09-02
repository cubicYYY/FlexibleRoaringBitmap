#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <vector>

#include "api.h"
#include "array_container.h"
#include "binsearch_index.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"

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
    static constexpr IndexType UNKNOWN_INDEX = std::numeric_limits<IndexType>::max();
    static constexpr IndexType ANY_INDEX = 0;

public:
    /// We start from an array container.
    FlexibleRoaringBitmap()
        : handle({
              new ArraySized(),
              CTy::Array,
              UNKNOWN_INDEX,
          }) {}
    FlexibleRoaringBitmap(froaring_container_t* container, CTy type, IndexType index = UNKNOWN_INDEX)
        : handle({
              container,
              type,
              index,
          }) {}
    FlexibleRoaringBitmap(ContainerHandle&& handle) : handle(std::move(handle)) {}
    FlexibleRoaringBitmap(FlexibleRoaringBitmap&& other) = default;
    FlexibleRoaringBitmap& operator=(FlexibleRoaringBitmap&& other) = default;

    ~FlexibleRoaringBitmap() {
        if (!handle.ptr) {
            return;
        }
        switch (handle.type) {
            case CTy::Array:
                delete static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr);
                break;
            case CTy::Bitmap:
                delete static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr);
                break;
            case CTy::RLE:
                delete static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr);
                break;
            case CTy::Containers:
                delete castToContainers(handle.ptr);
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    void debug_print() {
        if (!is_inited()) {
            std::cout << "NULL!" << std::endl;
        }
        switch (handle.type) {
            case CTy::Array:
                static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->debug_print();
                break;
            case CTy::Bitmap:
                static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr)->debug_print();
                break;
            case CTy::RLE:
                static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr)->debug_print();
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

        if (!is_inited()) {
            set_inited();
            handle.index = index;
            assert(handle.type == CTy::Array && "Invalid initial container type");
            static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->set(data);
            return;
        }
        if (handle.index != index) {  // Single container, and is set:
            switchToContainers();
            castToContainers(handle.ptr)->set(num);
            return;
        }

        switch (handle.type) {
            case CTy::Array:
                static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->set(data);
                break;
            case CTy::Bitmap:
                static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr)->set(data);
                break;
            case CTy::RLE:
                static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr)->set(data);
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    bool test(WordType num) const {
        if (!is_inited()) {
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
                return index == handle.index &&
                       static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->test(data);
            case CTy::Bitmap:
                return index == handle.index &&
                       static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr)->test(data);
            case CTy::RLE:
                return index == handle.index && static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr)->test(data);
            case CTy::Containers:
                return castToContainers(handle.ptr)->test(num);
            default:
                FROARING_UNREACHABLE
        }

        return false;
    }

    bool test_and_set(WordType num) {
        if (!is_inited()) {
            set(num);
            return true;
        }

        if (handle.type == CTy::Containers) {
            return castToContainers(handle.ptr)->test_and_set(num);
        }

        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(num, index, data);

        if (handle.index != index) {  // Single container, and is set:
            switchToContainers();
            castToContainers(handle.ptr)->set(num);
            return true;
        }

        switch (handle.type) {
            case CTy::Array:
                return static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->test_and_set(data);
            case CTy::Bitmap:
                return static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr)->test_and_set(data);
            case CTy::RLE:
                return static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr)->test_and_set(data);
            case CTy::Containers:
                return castToContainers(handle.ptr)->test_and_set(num);
            default:
                FROARING_UNREACHABLE
        }
        return false;
    }

    void reset(WordType num) {
        if (!is_inited()) {
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
                static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->reset(data);
                break;
            case CTy::Bitmap:
                static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr)->reset(data);
                break;
            case CTy::RLE:
                static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr)->reset(data);
                break;
            case CTy::Containers:
                castToContainers(handle.ptr)->reset(num);
                break;
            default:
                FROARING_UNREACHABLE
        }
    }

    size_t count() const {
        if (!is_inited()) {
            return 0;
        }

        switch (handle.type) {
            case CTy::Array:
                return static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->cardinality();
            case CTy::Bitmap:
                return static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr)->cardinality();
            case CTy::RLE:
                return static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr)->cardinality();
            case CTy::Containers:
                return castToContainers(handle.ptr)->cardinality();
            default:
                FROARING_UNREACHABLE
        }
        return 0;
    }

    void clear() {
        if (!is_inited()) {
            return;
        }

        switch (handle.type) {
            case CTy::Array:
                static_cast<ArrayContainer<WordType, DataBits>*>(handle.ptr)->clear();
                break;
            case CTy::Bitmap:
                static_cast<BitmapContainer<WordType, DataBits>*>(handle.ptr)->clear();
                break;
            case CTy::RLE:
                static_cast<RLEContainer<WordType, DataBits>*>(handle.ptr)->clear();
                break;
            case CTy::Containers:
                castToContainers(handle.ptr)->clear();
                break;
            default:
                FROARING_UNREACHABLE
        }
    }
    bool operator==(const FlexibleRoaringBitmap& other) const {
        if (!is_inited()) {
            return (other.count() == 0);
        }
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            return ContainersSized::equals(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
        }
        if (handle.type == CTy::Containers) {  // the other is not containers
            const ContainerHandle& lhs = castToContainers(handle.ptr)->containers[0];
            const ContainerHandle& rhs = other.handle;
            if (lhs.index != rhs.index) {
                return false;
            }
            return froaring_equal<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type);
        }
        if (other.handle.type == CTy::Containers) {  // other is not containers
            const ContainerHandle& lhs = castToContainers(other.handle.ptr)->containers[0];
            const ContainerHandle& rhs = other.handle;
            if (lhs.index != rhs.index) {
                return false;
            }
            return froaring_equal<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type);
        }
        return froaring_equal<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type);
    }

    FlexibleRoaringBitmap operator&(const FlexibleRoaringBitmap& other) const noexcept {
        if (!is_inited() || !other.is_inited()) {
            return FlexibleRoaringBitmap();
        }
        // Both containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            auto new_containers =
                ContainersSized::and_(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            if (new_containers->size == 0) {
                delete new_containers;
                return FlexibleRoaringBitmap();
            }
            if (new_containers->size == 1) {
                auto handle = std::move(new_containers->containers[0]);
                new_containers->size = 0;
                delete new_containers;
                return FlexibleRoaringBitmap(std::move(handle));
            }
            return FlexibleRoaringBitmap(new_containers, CTy::Containers, ANY_INDEX);
        }

        // One of them are containers:
        if (handle.type == CTy::Containers) {  // the other is not containers
            auto containers = castToContainers(handle.ptr);
            const ContainerHandle& rhs = other.handle;
            const ContainerHandle& lhs = containers->containers[containers->lower_bound(rhs.index)];
            if (lhs.index != rhs.index) {
                return FlexibleRoaringBitmap();
            }

            CTy local_res_type;
            auto ptr = froaring_and<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type, local_res_type);
            return FlexibleRoaringBitmap(ptr, local_res_type, lhs.index);
        }
        if (other.handle.type == CTy::Containers) {  // this is not containers
            auto containers = castToContainers(handle.ptr);
            const ContainerHandle& rhs = handle;
            const ContainerHandle& lhs = containers->containers[containers->lower_bound(rhs.index)];
            if (lhs.index != rhs.index) {
                return FlexibleRoaringBitmap();
            }
            CTy local_res_type;
            auto ptr = froaring_and<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type, local_res_type);
            return FlexibleRoaringBitmap(ptr, local_res_type, lhs.index);
        }
        // Both are single container:
        if (handle.index != other.handle.index) {
            return FlexibleRoaringBitmap();
        }
        CTy local_res_type;
        auto ptr = froaring_and<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                    local_res_type);
        return FlexibleRoaringBitmap(ptr, local_res_type, handle.index);
    }

    /// @brief Called when the current container exceeds the block size:
    /// transform into containers.
    void switchToContainers() {
        assert(handle.type != CTy::Containers && "Already indexed!");

        ContainersSized* containers = new ContainersSized(CONTAINERS_INIT_CAPACITY, 1);
        // TODO: do not modify the containers directly
        containers->containers[0] = std::move(handle);
        handle = ContainerHandle(containers, CTy::Containers, ANY_INDEX);
    }

    bool is_inited() const { return handle.index != UNKNOWN_INDEX; }
    void set_inited() {}

private:
    ContainersSized* castToContainers(froaring_container_t* p) { return static_cast<ContainersSized*>(p); }
    froaring_container_t* castToFroaring(ContainersSized* p) { return static_cast<froaring_container_t*>(p); }
    const ContainersSized* castToContainers(const froaring_container_t* p) {
        return static_cast<const ContainersSized*>(p);
    }
    const froaring_container_t* castToFroaring(const ContainersSized* p) {
        return static_cast<const froaring_container_t*>(p);
    }

    const ContainersSized* castToContainers(const froaring_container_t* p) const {
        return static_cast<const ContainersSized*>(p);
    }

public:
    ContainerHandle handle;
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