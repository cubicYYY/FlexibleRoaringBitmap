#pragma once

#include <cstring>

#include "handle.h"
#include "prelude.h"
#include "transform.h"
#include "utils.h"

namespace froaring {

using Cty = froaring::ContainerType;

/// @brief An index layer uses binary search in a sorted array for container
/// lookup.
/// @tparam WordType Underlying word type for bitmap container.
/// @tparam IndexBits High bits for indexing the corresponding container(block).
/// @tparam DataBits How many bits should a container hold (at least).
template <typename WordType, size_t IndexBits, size_t DataBits>
class BinsearchIndex : public froaring_container_t {
    static_assert(IndexBits + DataBits <= sizeof(WordType) * 8, "IndexBits + DataBits must not exceed WordType size.");

public:
    using IndexType = froaring::can_fit_t<IndexBits>;
    using SizeType = froaring::can_fit_t<IndexBits + 1>;
    using ValueType = froaring::can_fit_t<(IndexBits + DataBits)>;
    using RLESized = RLEContainer<WordType, DataBits>;
    using BitmapSized = BitmapContainer<WordType, DataBits>;
    using ArraySized = ArrayContainer<WordType, DataBits>;

    // handy local aliases
    using CTy = froaring::ContainerType;
    using ContainerHandle = froaring::ContainerHandle<IndexType>;

public:
    BinsearchIndex(SizeType capacity = CONTAINERS_INIT_CAPACITY, SizeType size = 0)
        : size(size),
          capacity(capacity),
          containers(static_cast<ContainerHandle*>(malloc(capacity * sizeof(ContainerHandle)))) {
        assert(containers && "Failed to allocate memory for containers");
    }

    void debug_print() const {
        for (SizeType i = 0; i < size; ++i) {
            std::cout << "Index: " << containers[i].index << " Type: " << static_cast<int>(containers[i].type)
                      << " Card.:";
            switch (containers[i].type) {
                case CTy::RLE: {
                    std::cout << int(static_cast<RLEContainer<WordType, DataBits>*>(containers[i].ptr)->cardinality())
                              << std::endl;
                    static_cast<RLEContainer<WordType, DataBits>*>(containers[i].ptr)->debug_print();
                    break;
                }
                case CTy::Array: {
                    std::cout << int(static_cast<ArrayContainer<WordType, DataBits>*>(containers[i].ptr)->cardinality())
                              << std::endl;
                    static_cast<ArrayContainer<WordType, DataBits>*>(containers[i].ptr)->debug_print();
                    break;
                }
                case CTy::Bitmap: {
                    std::cout
                        << int(static_cast<BitmapContainer<WordType, DataBits>*>(containers[i].ptr)->cardinality())
                        << std::endl;
                    static_cast<BitmapContainer<WordType, DataBits>*>(containers[i].ptr)->debug_print();
                    break;
                }
                default:
                    FROARING_UNREACHABLE
            }
        }
    }

    /// Return the entry position if found, otherwise the iterator points to the
    /// lower bound (to be inserted at).
    ///
    SizeType lower_bound(IndexType index) const {
        SizeType left = 0;
        SizeType right = size;

        while (left < right) {
            SizeType mid = left + (right - left) / 2;
            if (containers[mid].index < index) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        return left;
    }

    // Add a new container with a given index
    void addContainer(froaring_container_t* ptr, IndexType index, CTy type = CTy::Array) {
        assert(index < (1 << IndexBits) && "Container index exceeds the allowed bits.");

        ContainerHandle entry;
        entry.ptr = ptr;
        entry.type = type;
        entry.index = index;
        containers.push_back(entry);
    }

    // Check if `value` is present in the container
    bool test(ValueType value) const {
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(value, index, data);

        SizeType entry_pos = lower_bound(index);

        if (entry_pos == size || containers[entry_pos].index != index) return false;

        // Now we found the corresponding container
        switch (containers[entry_pos].type) {
            case CTy::RLE:
                return static_cast<RLEContainer<WordType, DataBits>*>(containers[entry_pos].ptr)->test(data);
            case CTy::Array:
                return static_cast<ArrayContainer<WordType, DataBits>*>(containers[entry_pos].ptr)->test(data);
            case CTy::Bitmap:
                return static_cast<BitmapContainer<WordType, DataBits>*>(containers[entry_pos].ptr)->test(data);
            default:
                FROARING_UNREACHABLE
        }
    }

    // Set a value in the corresponding container
    void set(ValueType value) {
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(value, index, data);

        SizeType pos = lower_bound(index);

        // Not found, insert a new container:
        if (pos == size || containers[pos].index != index) {
            if (size == capacity) {
                expand();
            }
            std::memmove(&containers[pos + 1], &containers[pos], (size - pos) * sizeof(ContainerHandle));
            auto array_ptr = new ArraySized(ARRAY_CONTAINER_INIT_CAPACITY, 1);
            array_ptr->vals[0] = data;
            containers[pos] = ContainerHandle(array_ptr, CTy::Array, index);
            size++;
            return;
        }

        // Now we found the corresponding container
        switch (containers[pos].type) {
            case CTy::RLE: {
                static_cast<RLEContainer<WordType, DataBits>*>(containers[pos].ptr)->set(data);
                break;
            }
            case CTy::Array: {
                auto array_ptr = static_cast<ArrayContainer<WordType, DataBits>*>(containers[pos].ptr);
                array_ptr->set(data);
                // Transform into a bitmap container if it gets bigger
                if (array_ptr->size >= ArraySized::ArrayToBitmapCountThreshold) {
                    auto new_bitmap = froaring_array_to_bitmap<WordType, DataBits>(array_ptr);
                    release_container(array_ptr);
                    containers[pos].ptr = new_bitmap;
                    containers[pos].type = CTy::Bitmap;
                }
                break;
            }
            case CTy::Bitmap: {
                static_cast<BitmapContainer<WordType, DataBits>*>(containers[pos].ptr)->set(data);
                break;
            }
            default:
                FROARING_UNREACHABLE
        }
    }

    bool test_and_set(ValueType value) {
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(value, index, data);

        SizeType pos = lower_bound(index);

        // Not found, insert a new container:
        if (pos == size || containers[pos].index != index) {
            if (size == capacity) {
                expand();
            }
            std::memmove(&containers[pos + 1], &containers[pos], (size - pos) * sizeof(ContainerHandle));
            auto array_ptr = new ArraySized(ARRAY_CONTAINER_INIT_CAPACITY, 1);
            array_ptr->vals[0] = data;
            containers[pos] = ContainerHandle(array_ptr, CTy::Array, index);
            size++;
            return true;
        }

        // Now we found the corresponding container
        switch (containers[pos].type) {
            case CTy::RLE: {
                return static_cast<RLEContainer<WordType, DataBits>*>(containers[pos].ptr)->test_and_set(data);
            }
            case CTy::Array: {
                auto array_ptr = static_cast<ArrayContainer<WordType, DataBits>*>(containers[pos].ptr);
                bool was_set = array_ptr->test_and_set(data);
                // Transform into a bitmap container if it gets bigger
                if (array_ptr->size >= ArraySized::ArrayToBitmapCountThreshold) {
                    auto new_bitmap = froaring_array_to_bitmap<WordType, DataBits>(array_ptr);
                    release_container(array_ptr);
                    containers[pos].ptr = new_bitmap;
                    containers[pos].type = CTy::Bitmap;
                }
                return was_set;
            }
            case CTy::Bitmap: {
                return static_cast<BitmapContainer<WordType, DataBits>*>(containers[pos].ptr)->test_and_set(data);
            }
            default:
                FROARING_UNREACHABLE
        }
    }

    // Calculate the total cardinality of all containers
    SizeType cardinality() const {
        SizeType total = 0;
        for (SizeType i = 0; i < size; ++i) {
            auto& entry = containers[i];
            switch (entry.type) {
                case CTy::RLE:
                    total += static_cast<RLEContainer<WordType, DataBits>*>(entry.ptr)->cardinality();
                    break;
                case CTy::Array:
                    total += static_cast<ArrayContainer<WordType, DataBits>*>(entry.ptr)->cardinality();
                    break;
                case CTy::Bitmap:
                    total += static_cast<BitmapContainer<WordType, DataBits>*>(entry.ptr)->cardinality();
                    break;
                default:
                    FROARING_UNREACHABLE
            }
        }
        return total;
    }

    void reset(ValueType value) {
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(value, index, data);

        SizeType pos = lower_bound(index);
        if (pos == size || containers[pos].index != index) {  // not found: return directly
            return;
        }

        ContainerHandle& entry = containers[pos];
        assert(entry.index == index && "??? Wrong container found or created");

        // Now we found the corresponding container
        switch (entry.type) {
            case CTy::RLE: {
                auto rle_ptr = static_cast<RLEContainer<WordType, DataBits>*>(entry.ptr);
                rle_ptr->reset(data);
                if (rle_ptr->run_count == 0) {
                    release_container(rle_ptr);
                    std::memmove(&containers[pos], &containers[pos + 1], (size - pos - 1) * sizeof(ContainerHandle));

                    size--;
                }
                break;
            }
            case CTy::Array: {
                auto array_ptr = static_cast<ArrayContainer<WordType, DataBits>*>(entry.ptr);
                array_ptr->reset(data);
                if (array_ptr->cardinality() == 0) {
                    release_container(array_ptr);
                    std::memmove(&containers[pos], &containers[pos + 1], (size - pos - 1) * sizeof(ContainerHandle));
                    size--;
                }
                break;
            }
            case CTy::Bitmap: {
                auto bitmap_ptr = static_cast<BitmapContainer<WordType, DataBits>*>(entry.ptr);
                bitmap_ptr->reset(data);
                if (bitmap_ptr->cardinality() == 0) {
                    release_container(bitmap_ptr);
                    std::memmove(&containers[pos], &containers[pos + 1], (size - pos - 1) * sizeof(ContainerHandle));
                    size--;
                }
                break;
            }
            default:
                FROARING_UNREACHABLE
        }
    }
    void clear() {
        for (SizeType i = 0; i < size; ++i) {
            switch (containers[i].type) {
                case CTy::Array:
                    static_cast<ArrayContainer<WordType, DataBits>*>(containers[i].ptr)->clear();
                    break;
                case CTy::Bitmap:
                    static_cast<BitmapContainer<WordType, DataBits>*>(containers[i].ptr)->clear();
                    break;
                case CTy::RLE:
                    static_cast<RLEContainer<WordType, DataBits>*>(containers[i].ptr)->clear();
                    break;
                default:
                    FROARING_UNREACHABLE
            }
        }
        size = 0;
    }
    // Release all containers
    ~BinsearchIndex() {
        std::cout << "~Index" << (void*)this << std::endl;

        for (SizeType i = 0; i < size; ++i) {
            std::cout << "Deleting c[" << i << "]..." << std::endl;
            std::cout << "Index=" << int(containers[i].index) << std::endl;
            release_container<WordType, DataBits>(containers[i].ptr, containers[i].type);
            std::cout << "Deleted c[" << i << "]." << std::endl;
        }
        std::cout << "freeing cs..." << (void*)containers << std::endl;
        free(containers);
    }

    static BinsearchIndex<WordType, IndexBits, DataBits>* and_(const BinsearchIndex<WordType, IndexBits, DataBits>* a,
                                                               const BinsearchIndex<WordType, IndexBits, DataBits>* b) {
        auto result = new BinsearchIndex<WordType, IndexBits, DataBits>(a->capacity, 0);
        SizeType i = 0, j = 0;
        SizeType new_container_counts = 0;
        while (true) {
            while (a->containers[i].index < b->containers[j].index) {
            SKIP_FIRST_COMPARE:
                i++;
                if (i >= a->size) {
                    result->size = new_container_counts;
                    return result;
                }
            }
            while (a->containers[i].index > b->containers[j].index) {
                j++;
                if (j >= b->size) {
                    result->size = new_container_counts;
                    return result;
                }
            }
            if (a->containers[i].index == b->containers[j].index) {
                CTy local_res_type;
                auto res =
                    froaring_and<WordType, DataBits>(a->containers[i].ptr, b->containers[j].ptr, a->containers[i].type,
                                                     b->containers[j].type, local_res_type);
                result->containers[new_container_counts++] =
                    ContainerHandle(res, local_res_type, a->containers[i].index);
                i++;
                j++;
                if (i == a->size || j == b->size) {
                    result->size = new_container_counts;
                    return result;
                }
            } else {
                goto SKIP_FIRST_COMPARE;
            }
        }
        FROARING_UNREACHABLE
    }

    static void andi(BinsearchIndex<WordType, IndexBits, DataBits>* a,
                     const BinsearchIndex<WordType, IndexBits, DataBits>* b) {
        SizeType i = 0, j = 0;
        SizeType new_container_counts = 0;
        while (i < a->size && j < b->size) {
            auto keya = a->containers[i].index;
            auto keyb = b->containers[j].index;
            if (keya == keyb) {
                CTy local_res_type;
                auto new_container =
                    froaring_andi<WordType, DataBits>(a->containers[i].ptr, b->containers[j].ptr, a->containers[i].type,
                                                      b->containers[j].type, local_res_type);
                if (new_container != a->containers[i].ptr) {  // New container is created: release the old one
                    release_container<WordType, DataBits>(a->containers[i].ptr, a->containers[i].type);
                } else if (container_empty<WordType, DataBits>(new_container, local_res_type)) {
                    release_container<WordType, DataBits>(new_container, local_res_type);
                } else {
                    a->containers[new_container_counts].ptr = new_container;
                    a->containers[new_container_counts].type = local_res_type;
                    new_container_counts++;
                }
                ++i;
                ++j;
            } else if (keya < keyb) {
                // TODO: use Gallop search
                i = a->advanceAndReleaseUntil(keyb, i);
            } else {
                j = b->advanceAndReleaseUntil(keya, j);
            }
        }
        // Release the rest of the containers
        for (auto pos = new_container_counts; pos < a->size; ++pos) {
            release_container<WordType, DataBits>(a->containers[pos].ptr, a->containers[pos].type);
            a->containers[pos].ptr = nullptr;
            pos++;
        }
        a->size = new_container_counts;
    }

    SizeType advanceAndReleaseUntil(IndexType key, SizeType pos) const {
        while (pos < size && containers[pos].index < key) {
            release_container<WordType, DataBits>(containers[pos].ptr, containers[pos].type);
            containers[pos].ptr = nullptr;
            pos++;
        }
        return pos;
    }

    static bool equals(const BinsearchIndex<WordType, IndexBits, DataBits>* a,
                       const BinsearchIndex<WordType, IndexBits, DataBits>* b) {
        if (a->size != b->size) {
            return false;
        }

        for (SizeType i = 0; i < a->size; ++i) {  // quick check
            if (a->containers[i].index != b->containers[i].index) {
                return false;
            }
        }
        for (SizeType i = 0; i < a->size; ++i) {
            auto res = froaring_equal<WordType, DataBits>(a->containers[i].ptr, b->containers[i].ptr,
                                                          a->containers[i].type, b->containers[i].type);
            if (!res) {
                return false;
            }
        }
        return true;
    }

private:
    void expand() {
        capacity *= 2;
        containers = static_cast<ContainerHandle*>(realloc(containers, capacity * sizeof(ContainerHandle)));
        assert(containers && "Failed to reallocate memory for containers");
    }

public:
    IndexType size = 0;
    IndexType capacity = 0;
    ContainerHandle* containers = nullptr;
};
}  // namespace froaring