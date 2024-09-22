#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <vector>

#include "api.h"
#include "binsearch_index.h"
#include "froaring_api/array_container.h"
#include "froaring_api/bitmap_container.h"
#include "froaring_api/prelude.h"

namespace froaring {

template <typename WordType, size_t IndexBits, size_t DataBits>
class FlexibleRoaringIterator;
/// @brief A flexible Roaring bitmap consists with a binary-search-indexed
/// layer, and underlying containers. Optimized for the case that only a single
/// container is needed: no index layer will be constructed until it gets
/// expanded.
/// @tparam WordType Suggested type for internal data storage. Currently only
/// used by Bitmap containers as the word size.
/// @tparam IndexBits high bits used for indexing.
/// @tparam DataBits low bits to be stored in containers.
template <typename WordType = uint64_t, size_t IndexBits = 16, size_t DataBits = 8>
class FlexibleRoaring {
    /// The container type for the index layer. Editable.
    // TODO: Support more index layers with maybe different types (maybe by Curiously Recurring Template Pattern)
    using ContainersSized = BinsearchIndex<WordType, IndexBits, DataBits>;
    using IndexType = froaring::can_fit_t<IndexBits>;
    using RLESized = RLEContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;
    using CTy = froaring::ContainerType;  // handy local alias
    using ContainerHandle = froaring::ContainerHandle<IndexType>;
    static constexpr IndexType UNKNOWN_INDEX = 0;
    static constexpr IndexType ANY_INDEX = 0;
    friend FlexibleRoaringIterator<WordType, IndexBits, DataBits>;

public:
    /// We start from an array container.
    explicit FlexibleRoaring()
        : handle({
              nullptr,
              CTy::Array,
              UNKNOWN_INDEX,
          }) {}
    explicit FlexibleRoaring(froaring_container_t* container, CTy type, IndexType index = UNKNOWN_INDEX)
        : handle({
              container,
              type,
              index,
          }) {}

    explicit FlexibleRoaring(ContainerHandle&& handle) : handle(std::move(handle)) {}
    explicit FlexibleRoaring(const FlexibleRoaring& other) {
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
    FlexibleRoaring(FlexibleRoaring&& other) = default;
    FlexibleRoaring& operator=(FlexibleRoaring&& other) = default;

    ~FlexibleRoaring() {
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
            return;
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
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(num, index, data);
        if (!is_inited()) {
            auto new_array = new ArrayContainer<WordType, DataBits>();
            handle.ptr = new_array;
            handle.index = index;
            new_array->set(data);
            return;
        }
        if (handle.type == CTy::Containers) {
            castToContainers(handle.ptr)->set(num);
            return;
        }

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
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(num, index, data);
        if (!is_inited()) {
            auto new_array = new ArrayContainer<WordType, DataBits>();
            handle.ptr = new_array;
            handle.index = index;
            new_array->set(data);
            return true;
        }

        if (handle.type == CTy::Containers) {
            return castToContainers(handle.ptr)->test_and_set(num);
        }

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
                // FIXME: transform into a single container if its cardinality is less than 1!!
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

    /// This invalidates the handle!
    inline void clear() {
        if (!is_inited()) {
            return;
        }

        switch (handle.type) {
            case CTy::Array:
            case CTy::Bitmap:
            case CTy::RLE:
                release_container<WordType, DataBits>(handle.ptr, handle.type);
                break;
            case CTy::Containers: {
                castToContainers(handle.ptr)->clear();
                delete castToContainers(handle.ptr);
                break;
            }
            default:
                FROARING_UNREACHABLE
        }

        handle = ContainerHandle(nullptr, CTy::Array,
                                 UNKNOWN_INDEX);  // TODO: make it into "default type" instead of an Array
    }

    /// @brief Replace current single container with a new one.
    inline void updateSingleHandle(froaring_container_t* new_ptr, CTy new_type) {
        assert(this->handle.type != CTy::Containers && "Cannot update Containers type bitmap");
        if (new_ptr != this->handle.ptr) {
            release_container<WordType, DataBits>(this->handle.ptr, this->handle.type);
        }
        if (container_empty<WordType, DataBits>(new_ptr, new_type)) {
            release_container<WordType, DataBits>(new_ptr, new_type);
            handle = ContainerHandle(nullptr, CTy::Array, UNKNOWN_INDEX);
        } else {
            this->handle.ptr = new_ptr;
            this->handle.type = new_type;
            // this->handle.index remains unchanged
        }
    }

    bool operator==(const FlexibleRoaring& other) const {
        if (!is_inited()) {
            return (other.count() == 0);
        }
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            return ContainersSized::equals(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
        }
        if (handle.type == CTy::Containers) {  // the other is a single container
            const ContainerHandle& lhs = castToContainers(handle.ptr)->containers[0];
            const ContainerHandle& rhs = other.handle;
            if (lhs.index != rhs.index) {
                return false;
            }
            return froaring_equal<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type);
        }
        if (other.handle.type == CTy::Containers) {  // other is a single container
            const ContainerHandle& lhs = castToContainers(other.handle.ptr)->containers[0];
            const ContainerHandle& rhs = other.handle;
            if (lhs.index != rhs.index) {
                return false;
            }
            return froaring_equal<WordType, DataBits>(lhs.ptr, rhs.ptr, lhs.type, rhs.type);
        }
        return froaring_equal<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type);
    }

    bool operator!=(const FlexibleRoaring& other) const { return !(*this == other); }

    FlexibleRoaring operator&(const FlexibleRoaring& other) const noexcept {
        if (!is_inited() || !other.is_inited()) {
            return FlexibleRoaring<WordType, IndexBits, DataBits>();
        }
        // Both containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            auto new_containers =
                ContainersSized::and_(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            if (new_containers->size == 0) {
                delete new_containers;
                return FlexibleRoaring<WordType, IndexBits, DataBits>();
            }
            if (new_containers->size == 1) {
                auto handle = std::move(new_containers->containers[0]);
                new_containers->size = 0;
                delete new_containers;
                return FlexibleRoaring<WordType, IndexBits, DataBits>(std::move(handle));
            }
            return FlexibleRoaring<WordType, IndexBits, DataBits>(new_containers, CTy::Containers, ANY_INDEX);
        }

        // One of them are containers:
        if (handle.type == CTy::Containers) {  // the other is a single container
            auto this_containers = castToContainers(handle.ptr);
            const ContainerHandle& other_single = other.handle;
            auto pos = this_containers->lower_bound(other_single.index);
            if (pos == this_containers->size) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>();
            }
            const ContainerHandle& lhs = this_containers->containers[pos];
            if (lhs.index != other_single.index) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>();
            }
            CTy local_res_type;
            auto ptr = froaring_and<WordType, DataBits>(lhs.ptr, other_single.ptr, lhs.type, other_single.type,
                                                        local_res_type);
            return FlexibleRoaring<WordType, IndexBits, DataBits>(ptr, local_res_type, lhs.index);
        }
        if (other.handle.type == CTy::Containers) {  // this is a single container
            auto other_containers = castToContainers(other.handle.ptr);
            const ContainerHandle& this_single = handle;
            auto pos = other_containers->lower_bound(this_single.index);
            if (pos == other_containers->size) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>();
            }
            if (other_containers->containers[pos].index != this_single.index) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>();
            }
            CTy local_res_type;
            auto ptr = froaring_and<WordType, DataBits>(other_containers->containers[pos].ptr, this_single.ptr,
                                                        other_containers->containers[pos].type, this_single.type,
                                                        local_res_type);
            return FlexibleRoaring<WordType, IndexBits, DataBits>(ptr, local_res_type,
                                                                  other_containers->containers[pos].index);
        }

        // Both are single container:
        if (handle.index != other.handle.index) {
            return FlexibleRoaring<WordType, IndexBits, DataBits>();
        }
        CTy local_res_type;
        auto ptr = froaring_and<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                    local_res_type);
        return FlexibleRoaring<WordType, IndexBits, DataBits>(ptr, local_res_type, handle.index);
    }

    FlexibleRoaring& operator&=(const FlexibleRoaring& other) noexcept {
        if (!is_inited() || !other.is_inited()) {
            clear();
            return *this;
        }
        // Both containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            ContainersSized::andi(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            return *this;
        }

        // One of them are containers: the result must be a single container
        if (handle.type == CTy::Containers) {  // the other is a single container
            auto this_containers = castToContainers(handle.ptr);
            const ContainerHandle& other_single = other.handle;
            auto pos = this_containers->lower_bound(other_single.index);
            if (pos == this_containers->size || this_containers->containers[pos].index != other_single.index) {
                clear();
                return *this;
            }
            // Now we found the corresponding container
            CTy local_res_type;
            auto ptr = froaring_andi<WordType, DataBits>(this_containers->containers[pos].ptr, other_single.ptr,
                                                         this_containers->containers[pos].type, other_single.type,
                                                         local_res_type);
            if (ptr != this_containers->containers[pos].ptr) {
                // All containers should be released
                castToContainers(handle.ptr)->clear();
                delete castToContainers(handle.ptr);
            } else {
                // All containers except the corresponding one should be released
                for (size_t i = 0; i < pos; ++i) {
                    release_container<WordType, DataBits>(this_containers->containers[i].ptr,
                                                          this_containers->containers[i].type);
                }
                for (size_t i = pos + 1; i < this_containers->size; ++i) {
                    release_container<WordType, DataBits>(this_containers->containers[i].ptr,
                                                          this_containers->containers[i].type);
                }
                delete this->handle.ptr;
            }
            this->handle = ContainerHandle(ptr, local_res_type, other_single.index);
            return *this;
        }
        if (other.handle.type == CTy::Containers) {  // this is a single container
            auto other_containers = castToContainers(handle.ptr);
            const ContainerHandle& this_single = handle;
            auto pos = other_containers->lower_bound(this_single.index);
            if (pos == other_containers->size || other_containers->containers[pos].index != this_single.index) {
                clear();
                return *this;
            }
            CTy local_res_type;
            auto ptr = froaring_andi<WordType, DataBits>(this_single.ptr, other_containers->containers[pos].ptr,
                                                         this_single.type, other_containers->containers[pos].type,
                                                         local_res_type);
            updateSingleHandle(ptr, local_res_type);
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
        updateSingleHandle(ptr, local_res_type);
        return *this;
    }

    FlexibleRoaring operator|(const FlexibleRoaring& other) const noexcept {
        if (!is_inited()) {
            return FlexibleRoaring<WordType, IndexBits, DataBits>(other);
        }
        if (!other.is_inited()) {
            return FlexibleRoaring<WordType, IndexBits, DataBits>(*this);
        }
        // Both are single container:
        if (handle.type != CTy::Containers && other.handle.type != CTy::Containers &&
            handle.index == other.handle.index) {
            CTy local_res_type;
            auto ptr = froaring_or<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                       local_res_type);
            return FlexibleRoaring<WordType, IndexBits, DataBits>(ptr, local_res_type, handle.index);
        }

        // Both are containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            auto new_containers =
                ContainersSized::or_(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            return FlexibleRoaring<WordType, IndexBits, DataBits>(new_containers, CTy::Containers, ANY_INDEX);
        }

        // One of them are containers:
        const ContainersSized* containers;
        const ContainerHandle* single;
        if (handle.type == CTy::Containers) {  // the other is a single container
            containers = castToContainers(handle.ptr);
            single = &other.handle;
        } else if (other.handle.type == CTy::Containers) {  // the other is containers
            containers = castToContainers(other.handle.ptr);
            single = &handle;
        }

        auto pos = containers->lower_bound(single->index);
        auto result_ctns = new ContainersSized(0, containers->size + 1);
        typename ContainersSized::IndexType new_size = 0;
        // before pos
        for (size_t i = 0; i < pos; ++i) {
            result_ctns->containers[new_size++] =
                duplicate_container<WordType, IndexType, DataBits>(containers->containers[i]);
        }

        // at pos: Update or insert
        if (pos < containers->size && containers->containers[pos].index == single->index) {
            CTy local_res_type;
            auto ptr = froaring_or<WordType, DataBits>(containers->containers[pos].ptr, single->ptr,
                                                       containers->containers[pos].type, single->type, local_res_type);
            result_ctns->containers[new_size++] = ContainerHandle(ptr, local_res_type, single->index);
            pos++;  // pos is handled
        } else {    // insert
            result_ctns->containers[new_size++] = duplicate_container<WordType, IndexType, DataBits>(*single);
            // pos is not handled
        }
        // after pos
        for (size_t i = pos; i < containers->size; ++i) {
            result_ctns->containers[new_size++] =
                duplicate_container<WordType, IndexType, DataBits>(containers->containers[i]);
        }
        result_ctns->size = new_size;
        return FlexibleRoaring<WordType, IndexBits, DataBits>(result_ctns, CTy::Containers, ANY_INDEX);
    }

    FlexibleRoaring& operator|=(const FlexibleRoaring& other) noexcept {
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
                ContainersSized* containers = new ContainersSized(2, 2);
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
        if (handle.type == CTy::Containers) {  // the other is a single container
            auto this_containers = castToContainers(handle.ptr);
            auto&& other_single = other.handle;
            auto pos = this_containers->lower_bound(other_single.index);
            // before pos: no change
            // at pos: Update or insert
            if (pos < this_containers->size && this_containers->containers[pos].index == other_single.index) {
                // The corresponding container is found:
                CTy local_res_type;
                auto ptr = froaring_ori<WordType, DataBits>(this_containers->containers[pos].ptr, other_single.ptr,
                                                            this_containers->containers[pos].type, other_single.type,
                                                            local_res_type);
                if (ptr != this_containers->containers[pos].ptr) {
                    release_container<WordType, DataBits>(this_containers->containers[pos].ptr,
                                                          this_containers->containers[pos].type);
                }
                this_containers->containers[pos] = ContainerHandle(ptr, local_res_type, other_single.index);
            } else {
                // We need to insert a new container if the corresponding container not found:
                if (this_containers->size == this_containers->capacity) {
                    this_containers->expand();
                }
                std::memmove(&this_containers->containers[pos + 1], &this_containers->containers[pos],
                             (this_containers->size - pos) * sizeof(ContainerHandle));
                this_containers->containers[pos] = duplicate_container<WordType, IndexType, DataBits>(other_single);
                this_containers->size++;
            }
        } else {  // the other are containers: duplicate and insert. We will create new Containers
            auto other_containers = castToContainers(other.handle.ptr);
            auto this_single = std::move(handle);
            size_t pos = other_containers->lower_bound(this_single.index);
            typename ContainersSized::IndexType new_size = 0;
            ContainersSized* new_containers = new ContainersSized(other_containers->size + 1);
            handle = ContainerHandle(other_containers, CTy::Containers, ANY_INDEX);
            // before pos
            for (size_t i = 0; i < pos; i++) {
                new_containers->containers[new_size++] =
                    duplicate_container<WordType, IndexType, DataBits>(other_containers->containers[i]);
            }
            // at pos: Update or insert
            if (pos < other_containers->size &&
                other_containers->containers[pos].index == this_single.index) {  // update
                CTy local_res_type;
                auto ptr = froaring_ori<WordType, DataBits>(this_single.ptr, other_containers->containers[pos].ptr,
                                                            this_single.type, other_containers->containers[pos].type,
                                                            local_res_type);
                if (ptr != this_single.ptr) {
                    release_container<WordType, DataBits>(this_single.ptr, this_single.type);
                }
                new_containers->containers[new_size++] = ContainerHandle(ptr, local_res_type, this_single.index);
                pos++;  // container at pos handled
            } else {    // just insert it
                new_containers->containers[new_size++] = std::move(handle);
            }
            // after pos:
            for (size_t i = pos; i < other_containers->size; ++i) {
                new_containers->containers[new_size++] =
                    duplicate_container<WordType, IndexType, DataBits>(other_containers->containers[i]);
            }
            new_containers->size = new_size;
            this->handle = ContainerHandle(new_containers, CTy::Containers, ANY_INDEX);
        }

        return *this;
    }

    FlexibleRoaring operator-(const FlexibleRoaring& other) const noexcept {
        if (!is_inited()) {
            return FlexibleRoaring<WordType, IndexBits, DataBits>();
        }
        if (!other.is_inited()) {
            return FlexibleRoaring<WordType, IndexBits, DataBits>(*this);
        }
        // Both are single container:
        if (handle.type != CTy::Containers && other.handle.type != CTy::Containers &&
            handle.index == other.handle.index) {
            CTy local_res_type;
            auto ptr = froaring_diff<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                         local_res_type);
            return FlexibleRoaring<WordType, IndexBits, DataBits>(ptr, local_res_type, handle.index);
        }

        // Both are containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            auto new_containers =
                ContainersSized::diff(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            return FlexibleRoaring<WordType, IndexBits, DataBits>(new_containers, CTy::Containers, ANY_INDEX);
        }

        // One of them are containers:
        if (handle.type == CTy::Containers) {  // the other is a single container
            auto this_containers = castToContainers(handle.ptr);
            const ContainerHandle& other_single = other.handle;
            auto pos = this_containers->lower_bound(other_single.index);
            // No corresponding container found: just return as-is
            if (pos == this_containers->size) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>(*this);
            }
            if (this_containers->containers[pos].index != other_single.index) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>(*this);
            }
            // We found the corresponding contianer:
            CTy local_res_type;
            auto ptr = froaring_diff<WordType, DataBits>(this_containers->containers[pos].ptr, other_single.ptr,
                                                         this_containers->containers[pos].type, other_single.type,
                                                         local_res_type);
            auto new_containers = new ContainersSized(0, this_containers->size);
            auto result = FlexibleRoaring<WordType, IndexBits, DataBits>(
                ContainerHandle(new_containers, CTy::Containers, ANY_INDEX));

            size_t new_containers_size = 0;
            // before pos
            for (size_t i = 0; i < pos; i++) {
                new_containers->containers[new_containers_size++] =
                    duplicate_container<WordType, IndexType, DataBits>(this_containers->containers[i]);
            }
            // at pos: insert ptr
            if (container_empty<WordType, DataBits>(ptr, local_res_type)) {  // skip containers[pos]
                release_container<WordType, DataBits>(ptr, local_res_type);
            } else {
                new_containers->containers[new_containers_size++] =
                    ContainerHandle(ptr, local_res_type, other_single.index);
            }
            // after pos
            for (size_t i = pos + 1; i < this_containers->size; i++) {
                new_containers->containers[new_containers_size++] =
                    duplicate_container<WordType, IndexType, DataBits>(this_containers->containers[i]);
            }
            new_containers->size = new_containers_size;
            return FlexibleRoaring<WordType, IndexBits, DataBits>(new_containers, CTy::Containers, ANY_INDEX);
        }
        if (other.handle.type == CTy::Containers) {  // this is a single container
            auto other_containers = castToContainers(other.handle.ptr);
            const ContainerHandle& this_single = handle;
            auto pos = other_containers->lower_bound(this_single.index);
            // No corresponding container found: just return as-is
            if (pos == other_containers->size) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>(*this);
            }
            const ContainerHandle& lhs = other_containers->containers[pos];
            if (lhs.index != this_single.index) {
                return FlexibleRoaring<WordType, IndexBits, DataBits>(*this);
            }
            // We found the corresponding contianer:
            CTy local_res_type;
            auto ptr =
                froaring_diff<WordType, DataBits>(lhs.ptr, this_single.ptr, lhs.type, this_single.type, local_res_type);
            return FlexibleRoaring<WordType, IndexBits, DataBits>(ptr, local_res_type, this_single.index);
        }
        FROARING_UNREACHABLE
    }

    FlexibleRoaring& operator-=(const FlexibleRoaring& other) noexcept {
        if (!is_inited() || !other.is_inited()) {  // Nothing happens
            return *this;
        }
        // Both containers
        if (handle.type == CTy::Containers && other.handle.type == CTy::Containers) {
            ContainersSized::diffi(castToContainers(handle.ptr), castToContainers(other.handle.ptr));
            return *this;
        }

        // One of them are containers:
        if (handle.type == CTy::Containers) {  // the other is a single container
            auto this_containers = castToContainers(handle.ptr);
            const ContainerHandle& other_single = other.handle;
            auto pos = this_containers->lower_bound(other_single.index);
            if (pos == this_containers->size) {
                return *this;
            }
            if (this_containers->containers[pos].index != other_single.index) {
                return *this;
            }
            // before pos: do nothing
            // at pos: update or remove
            CTy local_res_type;
            auto ptr = froaring_diffi<WordType, DataBits>(this_containers->containers[pos].ptr, other_single.ptr,
                                                          this_containers->containers[pos].type, other_single.type,
                                                          local_res_type);
            if (ptr != this_containers->containers[pos].ptr) {
                release_container<WordType, DataBits>(this_containers->containers[pos].ptr,
                                                      this_containers->containers[pos].type);
            }
            if (container_empty<WordType, DataBits>(ptr, local_res_type)) {  // remove containers[pos]
                release_container<WordType, DataBits>(ptr, local_res_type);
                std::memmove(&this_containers->containers[pos], &this_containers->containers[pos + 1],
                             (this_containers->size - pos - 1) * sizeof(ContainerHandle));
            } else {  // update
                this->containers[pos] = ContainerHandle(ptr, local_res_type, other_single.index);
            }
            return *this;
        }
        if (other.handle.type == CTy::Containers) {  // this is a single container
            auto other_containers = castToContainers(other.handle.ptr);
            const ContainerHandle& this_single = handle;
            auto pos = other_containers->lower_bound(this_single.index);
            if (pos == other_containers->size) {
                return *this;
            }
            const ContainerHandle& corresponding = other_containers->containers[pos];
            if (corresponding.index != this_single.index) {
                return *this;
            }
            CTy local_res_type;
            auto ptr = froaring_diffi<WordType, DataBits>(this_single.ptr, corresponding.ptr, this_single.type,
                                                          corresponding.type, local_res_type);
            updateSingleHandle(ptr, local_res_type);
            return *this;
        }
        // Both are single container:
        if (handle.index != other.handle.index) {
            return *this;
        }
        CTy local_res_type;
        auto ptr = froaring_diffi<WordType, DataBits>(handle.ptr, other.handle.ptr, handle.type, other.handle.type,
                                                      local_res_type);
        // new container has been created, and the old one should be released by the caller
        // (i.e., this function)
        updateSingleHandle(ptr, local_res_type);
        return *this;
    }

    // FlexibleRoaring operator^(const FlexibleRoaring& other) const noexcept {
    //     // TODO...
    // }
    // FlexibleRoaring& operator^=(const FlexibleRoaring& other) noexcept {
    //     // TODO...
    // }

    /// @brief Called when the current container exceeds the block size:
    /// transform into containers.
    void switchToContainers() {
        assert(handle.type != CTy::Containers && "Already indexed!");

        ContainersSized* containers = new ContainersSized(1);
        containers->containers[0] = std::move(handle);
        handle = ContainerHandle(containers, CTy::Containers, ANY_INDEX);
    }

    bool is_inited() const { return handle.ptr != nullptr; }
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
class FlexibleRoaringIterator {
public:
    using NumberType = can_fit_t<IndexBits + DataBits>;
    using DataType = typename ArrayContainer<WordType, DataBits>::IndexOrNumType;
    explicit FlexibleRoaringIterator(const FlexibleRoaring<WordType, IndexBits, DataBits>& tracking,
                                     size_t pos_or_index, size_t arraypos,
                                     ArrayContainer<WordType, DataBits>* array = nullptr)
        : tracking(tracking), pos_or_index(pos_or_index), arraypos(arraypos), array(array) {}
    ~FlexibleRoaringIterator() { delete array; }

    inline static FlexibleRoaringIterator begin(const FlexibleRoaring<WordType, IndexBits, DataBits>& tracking) {
        static auto end_ = FlexibleRoaringIterator(tracking, (size_t)(~0), 0);
        if (!tracking.is_inited()) {
            return end_;
        }

        switch (tracking.handle.type) {
            case CTy::Array:
                return FlexibleRoaringIterator(tracking, 0, 0);
            case CTy::RLE: {
                auto bitmap_ptr = static_cast<RLEContainer<WordType, DataBits>*>(tracking.handle.ptr);
                auto converted_array = froaring_rle_to_array<WordType, DataBits>(bitmap_ptr);
                ;
                return FlexibleRoaringIterator(tracking, tracking.handle.index, 0, converted_array);
            }
            case CTy::Bitmap: {
                auto bitmap_ptr = static_cast<BitmapContainer<WordType, DataBits>*>(tracking.handle.ptr);
                auto converted_array = froaring_bitmap_to_array<WordType, DataBits>(bitmap_ptr);
                ;
                return FlexibleRoaringIterator(tracking, tracking.handle.index, 0, converted_array);
            }
            case CTy::Containers: {
                const auto containers = tracking.castToContainers(tracking.handle.ptr);
                if (containers->size == 0) {
                    return end_;
                }
                switch (containers->containers[0].type) {
                    case CTy::Array:
                        return FlexibleRoaringIterator(tracking, 0, 0);
                    case CTy::RLE: {
                        auto converted_array = froaring_rle_to_array<WordType, DataBits>(
                            static_cast<RLEContainer<WordType, DataBits>*>(containers->containers[0].ptr));

                        return FlexibleRoaringIterator(tracking, 0, 0, converted_array);
                    }

                    case CTy::Bitmap: {
                        auto converted_array = froaring_bitmap_to_array<WordType, DataBits>(
                            static_cast<BitmapContainer<WordType, DataBits>*>(containers->containers[0].ptr));

                        return FlexibleRoaringIterator(tracking, 0, 0, converted_array);
                    }
                    default:
                        FROARING_UNREACHABLE
                }
                return FlexibleRoaringIterator(tracking, 0, 0);
            }

            default:
                FROARING_UNREACHABLE
        }
        return FlexibleRoaringIterator(tracking, 0, 0);
    }
    inline static FlexibleRoaringIterator end(const FlexibleRoaring<WordType, IndexBits, DataBits>& tracking) {
        static auto end_ = FlexibleRoaringIterator(tracking, (size_t)(~0), 0);
        return end_;
    }

    /// Note: we assume that you will never compare iterators tracking different FlexibleRoaring bitmaps...
    bool operator==(const FlexibleRoaringIterator& o) const {
        return pos_or_index == o.pos_or_index && arraypos == o.arraypos;
    }

    // ++i
    FlexibleRoaringIterator& operator++() {
        switch (tracking.handle.type) {
            case CTy::Array: {
                auto arr_ptr = static_cast<ArrayContainer<WordType, DataBits>*>(tracking.handle.ptr);
                if (arraypos + 1 != arr_ptr->size) {
                    arraypos++;
                    return *this;
                }
                pos_or_index = (size_t)(~0);
                arraypos = 0;
                return *this;
            }
            case CTy::RLE:
            case CTy::Bitmap: {
                if (arraypos + 1 != array->size) {
                    arraypos++;
                    return *this;
                }
                pos_or_index = (size_t)(~0);
                arraypos = 0;
                delete array;
                array = nullptr;
                return *this;
            }
            case CTy::Containers: {
                const auto containers = tracking.castToContainers(tracking.handle.ptr);
                switch (containers->containers[pos_or_index].type) {
                    case CTy::Array: {
                        auto arr_ptr = static_cast<ArrayContainer<WordType, DataBits>*>(tracking.handle.ptr);
                        if (arraypos + 1 != arr_ptr->size) {
                            arraypos++;
                            return *this;
                        }
                        arraypos = 0;
                        if (pos_or_index + 1 != containers->size) {
                            pos_or_index++;
                            arraypos = 0;
                            set_array_with_container(containers->containers[pos_or_index]);
                            return *this;
                        }
                        pos_or_index = (size_t)(~0);
                        arraypos = 0;
                        return *this;
                    }

                    case CTy::RLE:
                    case CTy::Bitmap: {
                        if (arraypos + 1 != array->size) {
                            arraypos++;
                            return *this;
                        }
                        arraypos = 0;
                        if (pos_or_index + 1 != containers->size) {
                            pos_or_index++;
                            delete array;
                            set_array_with_container(containers->containers[pos_or_index]);
                            return *this;
                        }
                        pos_or_index = (size_t)(~0);
                        delete array;
                        array = nullptr;
                        return *this;
                    }
                    default:
                        FROARING_UNREACHABLE
                }
            }
        }
        FROARING_UNREACHABLE
    }
    NumberType operator*() const {
        switch (tracking.handle.type) {
            case CTy::Array:
                return (NumberType)(pos_or_index << DataBits) |
                       (NumberType)(static_cast<ArrayContainer<WordType, DataBits>*>(tracking.handle.ptr)
                                        ->vals[arraypos]);
            case CTy::RLE:
            case CTy::Bitmap: {
                return (NumberType)(pos_or_index << DataBits) | (NumberType)(array->vals[arraypos]);
            }
            case CTy::Containers: {
                const auto containers = tracking.castToContainers(tracking.handle.ptr);
                switch (containers->containers[pos_or_index].type) {
                    case CTy::Array:
                        return (NumberType)(containers->containers[pos_or_index].index << DataBits) |
                               (NumberType)(static_cast<ArrayContainer<WordType, DataBits>*>(
                                                containers->containers[pos_or_index].ptr)
                                                ->vals[arraypos]);
                    case CTy::RLE:
                    case CTy::Bitmap: {
                        return (NumberType)(containers->containers[pos_or_index].index << DataBits) |
                               (NumberType)(static_cast<ArrayContainer<WordType, DataBits>*>(array)->vals[arraypos]);
                    }
                    default:
                        FROARING_UNREACHABLE
                }
            }

            default:
                FROARING_UNREACHABLE
        }
    }

private:
    using IndexType = typename FlexibleRoaring<WordType, IndexBits, DataBits>::IndexType;
    void set_array_with_container(const ContainerHandle<IndexType>& ch) {
        assert(array == nullptr);
        switch (ch.type) {
            case CTy::Array: {
                array = nullptr;
                break;
            }
            case CTy::RLE: {
                array =
                    froaring_rle_to_array<WordType, DataBits>(static_cast<RLEContainer<WordType, DataBits>*>(ch.ptr));
                break;
            }
            case CTy::Bitmap: {
                array = froaring_bitmap_to_array<WordType, DataBits>(
                    static_cast<BitmapContainer<WordType, DataBits>*>(ch.ptr));
                break;
            }
            default:
                FROARING_UNREACHABLE
        }
    }

    const FlexibleRoaring<WordType, IndexBits, DataBits>& tracking;
    size_t pos_or_index = 0;
    size_t arraypos = 0;
    ArrayContainer<WordType, DataBits>* array = nullptr;
};
}  // namespace froaring