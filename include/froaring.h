#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <vector>

#include "api.h"
#include "array_container.h"
#include "binsearch_index.h"
#include "bitmap_container.h"
#include "froaring_api/equal.h"
#include "mix_ops.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {

template <typename WordType = uint64_t, size_t IndexBits = 16, size_t DataBits = 8>
class FlexibleRoaringBitmapIterator;
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
    friend FlexibleRoaringBitmapIterator<>;

public:
    /// We start from an array container.
    explicit FlexibleRoaringBitmap()
        : handle({
              new ArraySized(),
              CTy::Array,
              UNKNOWN_INDEX,
          }) {}
    explicit FlexibleRoaringBitmap(froaring_container_t* container, CTy type, IndexType index = UNKNOWN_INDEX)
        : handle({
              container,
              type,
              index,
          }) {}

    explicit FlexibleRoaringBitmap(ContainerHandle&& handle) : handle(std::move(handle)) {}
    explicit FlexibleRoaringBitmap(const FlexibleRoaringBitmap& other) {
        handle.type = other.handle.type;
        handle.index = other.handle.index;
        if (!other.is_inited()) {
            handle.ptr = nullptr;
            return;
        }
        switch (handle.type) {
            case ContainerType::Array:
                handle.ptr = new ArrayContainer<WordType, DataBits>(
                    *static_cast<const ArrayContainer<WordType, DataBits>*>(other.handle.ptr));
                break;
            case ContainerType::Bitmap:
                handle.ptr = new BitmapContainer<WordType, DataBits>(
                    *static_cast<const BitmapContainer<WordType, DataBits>*>(other.handle.ptr));
                break;
            case ContainerType::RLE:
                handle.ptr = new RLEContainer<WordType, DataBits>(
                    *static_cast<const RLEContainer<WordType, DataBits>*>(other.handle.ptr));
                break;
            case ContainerType::Containers:
                handle.ptr = new ContainersSized(*static_cast<const ContainersSized*>(other.handle.ptr));
                break;
            default:
                FROARING_UNREACHABLE
        }
    }
    FlexibleRoaringBitmap(FlexibleRoaringBitmap&& other) = default;
    FlexibleRoaringBitmap& operator=(FlexibleRoaringBitmap&& other) = default;

    ~FlexibleRoaringBitmap() {
        if (!handle.ptr) {
            return;
        }
        if (handle.type == CTy::Containers) {
            delete castToContainers(handle.ptr);
        } else {
            release_container<WordType, DataBits>(handle.ptr, handle.type);
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
                // FIXME: transform into a single container if its cardinality is 1
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
        // FIXME: convert to initial state (i.e., not inited) if empty
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

    bool operator!=(const FlexibleRoaringBitmap& other) const { return !(*this == other); }

    FlexibleRoaringBitmap operator&(const FlexibleRoaringBitmap& other) const noexcept {
        if (!is_inited() || !other.is_inited()) {
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>();
        }
        // Both containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            auto new_containers =
                ContainersSized::and_(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            if (new_containers->size == 0) {
                delete new_containers;
                return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>();
            }
            if (new_containers->size == 1) {
                auto handle = std::move(new_containers->containers[0]);
                new_containers->size = 0;
                delete new_containers;
                return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(std::move(handle));
            }
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(new_containers, CTy::Containers, ANY_INDEX);
        }

        // One of them are containers:
        if (handle.type == CTy::Containers) {  // the other is not containers
            auto containers = castToContainers(handle.ptr);
            const ContainerHandle& rhs = other.handle;
            auto pos = containers->lower_bound(rhs.index);
            if (pos == containers->size) {
                return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>();
            }
            const ContainerHandle& lhs = containers->containers[pos];
            if (lhs.index != rhs.index) {
                return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>();
            }
            CTy local_res_type;
            auto ptr = froaring_and<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type, local_res_type);
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(ptr, local_res_type, lhs.index);
        }
        if (other.handle.type == CTy::Containers) {  // this is not containers
            auto containers = castToContainers(other.handle.ptr);
            const ContainerHandle& rhs = handle;
            auto pos = containers->lower_bound(rhs.index);
            if (pos == containers->size) {
                return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>();
            }
            const ContainerHandle& lhs = containers->containers[pos];
            if (lhs.index != rhs.index) {
                return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>();
            }
            CTy local_res_type;
            auto ptr = froaring_and<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type, local_res_type);
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(ptr, local_res_type, lhs.index);
        }

        // Both are single container:
        if (handle.index != other.handle.index) {
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>();
        }
        CTy local_res_type;
        auto ptr = froaring_and<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                    local_res_type);
        return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(ptr, local_res_type, handle.index);
    }

    FlexibleRoaringBitmap& operator&=(const FlexibleRoaringBitmap& other) noexcept {
        if (!is_inited() || !other.is_inited()) {
            clear();
            return *this;
        }
        // Both containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            ContainersSized::andi(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            return *this;
        }

        // One of them are containers:
        if (handle.type == CTy::Containers) {  // the other is not containers
            auto containers = castToContainers(handle.ptr);
            const ContainerHandle& rhs = other.handle;
            const ContainerHandle& lhs = containers->containers[containers->lower_bound(rhs.index)];
            if (lhs.index != rhs.index) {
                clear();
                return *this;
            }

            CTy local_res_type;
            auto ptr = froaring_andi<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type, local_res_type);
            clear();
            this->handle = ContainerHandle(ptr, local_res_type, lhs.index);
            return *this;
        }
        if (other.handle.type == CTy::Containers) {  // this is not containers
            auto containers = castToContainers(handle.ptr);
            const ContainerHandle& rhs = handle;
            const ContainerHandle& lhs = containers->containers[containers->lower_bound(rhs.index)];
            if (lhs.index != rhs.index) {
                clear();
                return *this;
            }
            CTy local_res_type;
            auto ptr = froaring_andi<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type, local_res_type);
            clear();
            this->handle = ContainerHandle(ptr, local_res_type, lhs.index);
            return *this;
        }
        // Both are single container:
        if (handle.index != other.handle.index) {
            clear();
            return *this;
        }
        CTy local_res_type;
        auto ptr = froaring_andi<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                     local_res_type);
        // new container has been created, and the old one should be released by the caller
        // (i.e., this function)
        if (ptr != handle.ptr) {
            release_container<WordType, DataBits>(handle.ptr, handle.type);
        } else if (container_empty<WordType, DataBits>(ptr, local_res_type)) {
            release_container<WordType, DataBits>(ptr, local_res_type);
            handle.ptr = nullptr;
            handle.type = CTy::Array;
            handle.index = UNKNOWN_INDEX;
            return *this;
        }
        handle.ptr = ptr;
        handle.type = local_res_type;
        return *this;
    }

    FlexibleRoaringBitmap operator|(const FlexibleRoaringBitmap& other) const noexcept {
        if (!is_inited()) {
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(other);
        }
        if (!other.is_inited()) {
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(*this);
        }
        // Both are single container:
        if (handle.type != CTy::Containers && other.handle.type != CTy::Containers &&
            handle.index == other.handle.index) {
            CTy local_res_type;
            auto ptr = froaring_or<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                       local_res_type);
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(ptr, local_res_type, handle.index);
        }

        // Both are containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            auto new_containers =
                ContainersSized::or_(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(new_containers, CTy::Containers, ANY_INDEX);
        }

        // One of them are containers:
        const ContainersSized* containers;
        const ContainerHandle* single;
        if (handle.type == CTy::Containers) {  // the other is not containers
            containers = castToContainers(handle.ptr);
            single = &other.handle;
        } else if (other.handle.type == CTy::Containers) {  // the other is containers
            containers = castToContainers(other.handle.ptr);
            single = &handle;
        }

        auto pos = containers->lower_bound(single->index);
        auto result_ctns = new ContainersSized(0, containers->size + 1);
        typename ContainersSized::IndexType new_size = 0;
        for (size_t i = 0; i < pos; ++i) {
            result_ctns->containers[new_size++] =
                duplicate_container<WordType, IndexType, DataBits>(containers->containers[i]);
        }

        // Update or insert
        if (pos < containers->size && containers->containers[pos].index == single->index) {
            CTy local_res_type;
            auto ptr = froaring_or<WordType, DataBits>(containers->containers[pos].ptr, single->ptr,
                                                       containers->containers[pos].type, single->type, local_res_type);
            result_ctns->containers[new_size++] = ContainerHandle(ptr, local_res_type, single->index);
            pos++;
        } else {
            result_ctns->containers[new_size++] = duplicate_container<WordType, IndexType, DataBits>(*single);
        }

        for (size_t i = pos; i < containers->size; ++i) {
            result_ctns->containers[new_size++] =
                duplicate_container<WordType, IndexType, DataBits>(containers->containers[i]);
        }
        result_ctns->size = new_size;
        return FlexibleRoaringBitmap<WordType, IndexBits, DataBits>(result_ctns, CTy::Containers, ANY_INDEX);
    }
    FlexibleRoaringBitmap& operator|=(const FlexibleRoaringBitmap& other) noexcept {
        if (!is_inited()) {
            this->handle = duplicate_container<WordType, IndexType, DataBits>(other.handle);
            return *this;
        }
        if (!other.is_inited()) {
            return *this;
        }
        // Both are single container:
        if (handle.type != CTy::Containers && other.handle.type != CTy::Containers) {
            if (handle.index == other.handle.index) {
                CTy local_res_type;
                auto ptr = froaring_ori<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type,
                                                            other.handle.type, local_res_type);
                // new container has been created, and the old one should be released by the caller
                // (i.e., this function)
                if (ptr != handle.ptr) {
                    release_container<WordType, DataBits>(handle.ptr, handle.type);
                }
                handle.ptr = ptr;
                handle.type = local_res_type;
                return *this;
            } else {  // So we need to make it into Containers
                ContainersSized* containers = new ContainersSized(2);
                if (handle.index < other.handle.index) {
                    containers->containers[0] = std::move(handle);
                    containers->containers[1] = duplicate_container<WordType, IndexType, DataBits>(other.handle);
                } else {
                    containers->containers[0] = duplicate_container<WordType, IndexType, DataBits>(other.handle);
                    containers->containers[1] = std::move(handle);
                }
                handle = ContainerHandle(containers, CTy::Containers, ANY_INDEX);
                return *this;
            }
        }

        // Both are containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            ContainersSized::ori(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            return *this;
        }

        // One of them are containers:
        if (handle.type == CTy::Containers) {  // the other is not containers
            auto containers = castToContainers(handle.ptr);
            auto&& single = other.handle;
            auto pos = containers->lower_bound(single.index);

            // Update or insert
            if (pos < containers->size && containers->containers[pos].index == single.index) {
                // The corresponding container is found:
                CTy local_res_type;
                auto ptr =
                    froaring_ori<WordType, DataBits>(containers->containers[pos].ptr, single.ptr,
                                                     containers->containers[pos].type, single.type, local_res_type);
                if (ptr != containers->containers[pos].ptr) {
                    release_container<WordType, DataBits>(containers->containers[pos].ptr,
                                                          containers->containers[pos].type);
                    containers->containers[pos] = ContainerHandle(ptr, local_res_type, single.index);
                }
            } else {
                // We need to insert a new container if the corresponding container not found:
                if (containers->size == containers->capacity) {
                    containers->expand_to(containers->size + 1);
                }
                std::memmove(&containers->containers[pos + 1], &containers->containers[pos],
                             (containers->size - pos) * sizeof(ContainerHandle));
                containers->containers[pos] = duplicate_container<WordType, IndexType, DataBits>(single);
                containers->size++;
            }
        } else {  // the other is containers, duplicate and insert
            auto containers = castToContainers(other.handle.ptr);
            auto single = std::move(handle);
            size_t pos = containers->lower_bound(single.index);
            typename ContainersSized::IndexType new_size = 0;
            ContainersSized* new_containers = new ContainersSized(containers->size + 1);
            handle = ContainerHandle(containers, CTy::Containers, ANY_INDEX);
            for (size_t i = 0; i < pos; i++) {
                new_containers->containers[new_size++] =
                    duplicate_container<WordType, IndexType, DataBits>(containers->containers[i]);
            }
            // Update or insert
            if (pos < containers->size && containers->containers[pos].index == single.index) {
                CTy local_res_type;
                auto ptr =
                    froaring_ori<WordType, DataBits>(containers->containers[pos].ptr, single.ptr,
                                                     containers->containers[pos].type, single.type, local_res_type);
                new_containers->containers[new_size++] = ContainerHandle(ptr, local_res_type, single.index);
                pos++;
            } else {  // just insert it
                new_containers->containers[new_size++] = duplicate_container<WordType, IndexType, DataBits>(single);
            }

            for (size_t i = pos; i < containers->size; ++i) {
                new_containers->containers[new_size++] =
                    duplicate_container<WordType, IndexType, DataBits>(containers->containers[i]);
            }
            new_containers->size = new_size;
        }

        return *this;
    }
    // FlexibleRoaringBitmap operator^(const FlexibleRoaringBitmap& other) const noexcept {
    //     // TODO...
    // }
    // FlexibleRoaringBitmap& operator^=(const FlexibleRoaringBitmap& other) noexcept {
    //     // TODO...
    // }
    FlexibleRoaringBitmap operator-(const FlexibleRoaringBitmap& other) const noexcept {
        // TODO...
    }
    FlexibleRoaringBitmap& operator-=(const FlexibleRoaringBitmap& other) noexcept {
        // TODO...
    }

    /// @brief Called when the current container exceeds the block size:
    /// transform into containers.
    void switchToContainers() {
        assert(handle.type != CTy::Containers && "Already indexed!");

        ContainersSized* containers = new ContainersSized(1);
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

template <typename WordType, size_t IndexBits, size_t DataBits>
class FlexibleRoaringBitmapIterator {
    // TODO:...
};
}  // namespace froaring