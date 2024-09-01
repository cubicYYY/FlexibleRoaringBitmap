#pragma once

#include "handle.h"
#include "utils.h"
#include "transform.h"

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

    /// Bit capacity for containers indexed
    static constexpr size_t ContainerCapacity = (1 << DataBits);
    /// Array threshold (Array will not be optimum for storing more
    /// elements)
    static constexpr size_t ArrayToBitmapCountThreshold = ContainerCapacity / DataBits;
    /// RLE threshold (RLE will not be optimum for more runs)
    static constexpr size_t RleToBitmapRunThreshold = ContainerCapacity / (DataBits * 2);
public:

    BinsearchIndex(SizeType capacity = CONTAINERS_INIT_CAPACITY, SizeType size = 0)
        : capacity(capacity),
          size(size),
          containers(static_cast<ContainerHandle*>(malloc(capacity * sizeof(ContainerHandle)))) {
        assert(containers && "Failed to allocate memory for containers");
    }

    void debug_print() const {
        for (SizeType i = 0; i < size; ++i) {
            std::cout << "Index: " << containers[i].index << " Type: " << static_cast<int>(containers[i].type) << std::endl;
            switch (containers[i].type) {
                case CTy::RLE:
                    CAST_TO_RLE(containers[i].ptr)->debug_print();
                    break;
                case CTy::Array:
                    CAST_TO_ARRAY(containers[i].ptr)->debug_print();
                    break;
                case CTy::Bitmap:
                    CAST_TO_BITMAP(containers[i].ptr)->debug_print();
                    break;
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
                return CAST_TO_RLE(containers[entry_pos].ptr)->test(data);
            case CTy::Array:
                return CAST_TO_ARRAY(containers[entry_pos].ptr)->test(data);
            case CTy::Bitmap:
                return CAST_TO_BITMAP(containers[entry_pos].ptr)->test(data);
            default:
                FROARING_UNREACHABLE
        }
        return false;
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
                CAST_TO_RLE(containers[pos].ptr)->set(data);
                break;
            }
            case CTy::Array: {
                auto array_ptr = CAST_TO_ARRAY(containers[pos].ptr);
               array_ptr->set(data);
                // Transform into a bitmap container if it gets bigger
                if (array_ptr->size >= ArrayToBitmapCountThreshold) {
                    auto new_bitmap = froaring_array_to_bitmap<WordType, DataBits>(array_ptr);
                    delete containers[pos].ptr;
                    containers[pos].ptr = new_bitmap;
                    containers[pos].type = CTy::Bitmap;
                }
                break;
            }
            case CTy::Bitmap: {
                CAST_TO_BITMAP(containers[pos].ptr)->set(data);
                break;
            }
            default:
                FROARING_UNREACHABLE
        }
    }

    // Calculate the total cardinality of all containers
    size_t cardinality() const {
        size_t total = 0;
        for (SizeType i = 0; i < size; ++i) {
            auto& entry = containers[i];
            switch (entry.type) {
                case CTy::RLE:
                    total += CAST_TO_RLE(entry.ptr)->cardinality();
                    break;
                case CTy::Array:
                    total += CAST_TO_ARRAY(entry.ptr)->cardinality();
                    break;
                case CTy::Bitmap:
                    total += CAST_TO_BITMAP(entry.ptr)->cardinality();
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
                CAST_TO_RLE(entry.ptr)->reset(data);
                if (CAST_TO_RLE(entry.ptr)->run_count == 0) {
                    delete entry.ptr;
                    if (size > 1) {
                        std::memmove(&containers[pos], &containers[pos + 1],
                                     (size - pos - 1) * sizeof(ContainerHandle));
                    }
                    size--;
                }
                break;
            }
            case CTy::Array: {
                CAST_TO_ARRAY(entry.ptr)->reset(data);
                if (CAST_TO_ARRAY(entry.ptr)->size == 0) {
                    delete entry.ptr;
                    if (size > 1) {
                        std::memmove(&containers[pos], &containers[pos + 1],
                                     (size - pos - 1) * sizeof(ContainerHandle));
                    }
                    size--;
                }
                break;
            }
            case CTy::Bitmap: {
                CAST_TO_BITMAP(entry.ptr)->reset(data);
                if (CAST_TO_BITMAP(entry.ptr)->cardinality() == 0) {
                    delete entry.ptr;
                    if (size > 1) {
                        std::memmove(&containers[pos], &containers[pos + 1],
                                     (size - pos - 1) * sizeof(ContainerHandle));
                    }
                    size--;
                }
                break;
            }
            default:
                FROARING_UNREACHABLE
        }
    }


    // Release all containers
    ~BinsearchIndex() {
        for (SizeType i = 0; i < size; ++i) {
            switch (containers[i].type) {
                case CTy::Array:
                    CAST_TO_ARRAY(containers[i].ptr)->~ArrayContainer();
                    break;
                case CTy::Bitmap:
                    CAST_TO_BITMAP(containers[i].ptr)->~BitmapContainer();
                    break;
                case CTy::RLE:
                    CAST_TO_RLE(containers[i].ptr)->~RLEContainer();
                    break;
                default:
                    FROARING_UNREACHABLE
            }
            delete containers[i].ptr;
        }
        free(containers);
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