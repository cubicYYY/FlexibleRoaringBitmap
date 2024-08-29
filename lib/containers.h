#pragma once

#include "array_container.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"

namespace froaring {
/// @brief
/// @tparam WordType Underlying word type for bitmap container.
/// @tparam IndexBits High bits for indexing the corresponding container(block).
/// @tparam DataBits How many bits should a container hold (at least).
template <typename WordType, size_t IndexBits, size_t DataBits>
class Containers {
    static_assert(IndexBits + DataBits <= sizeof(WordType) * 8,
                  "IndexBits + DataBits must not exceed WordType size.");

    using IndexType = froaring::can_fit_t<IndexBits>;
    using ValueType = froaring::can_fit_t<(IndexBits + DataBits)>;

    // Struct to hold container data: pointer, type, and index
    struct ContainerEntry {
        /// Pointer to the container (could be RLE, Array, or Bitmap)
        /// Type is erased, handle carefully!
        void* containerPtr;
        froaring::ContainerType
            containerType;         // Type of container (RLE, Array, Bitmap)
        IndexType containerIndex;  // Index for this container
        uint8_t flag;  // Reserved for future use (e.g., status flags)

        ContainerEntry()
            : containerPtr(nullptr),
              containerType(CTy::Array),
              containerIndex(0),
              flag(0) {}
    };

    // Array of ContainerEntry structs
    std::vector<ContainerEntry> containers;
    using CTy = froaring::ContainerType;  // handy local alias
public:
    // Constructor
    Containers() = default;

    /// Return the entry iterator if found, otherwise the iterator points to the
    /// lower bound (to be inserted at).
    ///
    typename std::vector<ContainerEntry>::iterator getContainerPosByIndex(
        IndexType index) {
        size_t left = 0;
        size_t right = containers.size();

        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (containers[mid].containerIndex < index) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        assert(containers.begin() + containers.size() == containers.end());
        return containers.begin() + left;
    }

    typename std::vector<ContainerEntry>::const_iterator getContainerPosByIndex(
        IndexType index) const {
        size_t left = 0;
        size_t right = containers.size();

        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (containers[mid].containerIndex < index) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        assert(containers.begin() + containers.size() == containers.end());
        return containers.begin() + left;
    }

    // Add a new container with a given index
    void addContainer(void* ptr, IndexType index, uint8_t type = CTy::Array) {
        assert(index < (1 << IndexBits) &&
               "Container index exceeds the allowed bits.");

        ContainerEntry entry;
        entry.containerPtr = ptr;
        entry.containerType = type;
        entry.containerIndex = index;
        containers.push_back(entry);
    }

    // Check if `value` is present in the container
    bool test(ValueType value) const {
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(value, index, data);

        auto entry_it = getContainerPosByIndex(index);
        if (entry_it == containers.end()) return false;
        auto entry = *entry_it;
        if (entry.containerIndex != index) return false;

        // Now we found the corresponding container
        switch (entry.containerType) {
            case CTy::RLE:
                return static_cast<RLEContainer<WordType, DataBits>*>(
                           entry.containerPtr)
                    ->test(data);
            case CTy::Array:
                return static_cast<ArrayContainer<WordType, DataBits>*>(
                           entry.containerPtr)
                    ->test(data);
            case CTy::Bitmap:
                return static_cast<BitmapContainer<WordType, DataBits>*>(
                           entry.containerPtr)
                    ->test(data);
            default:
                FROARING_UNREACHABLE
        }
    }

    // Set a value in the corresponding container
    void set(ValueType value) {
        can_fit_t<IndexBits> index;
        can_fit_t<DataBits> data;
        num2index_n_data<IndexBits, DataBits>(value, index, data);

        auto it = getContainerPosByIndex(index);
        if (it == containers.end() || it->containerIndex != index) {
            ContainerEntry newEntry;
            newEntry.containerIndex = index;
            newEntry.containerType = CTy::Array;
            newEntry.containerPtr =
                ArrayContainer<WordType, DataBits>::create();

            containers.insert(it, newEntry);
            it = getContainerPosByIndex(index);
            return;
        }

        ContainerEntry& entry = *it;
        assert(entry.containerIndex == index &&
               "??? Wrong container found or created");

        // Now we found the corresponding container
        switch (entry.containerType) {
            case CTy::RLE: {
                auto rleptr = static_cast<RLEContainer<WordType, DataBits>*>(
                    entry.containerPtr);
                RLEContainer<WordType, DataBits>::set(rleptr, data);
                entry.containerPtr = (void*)rleptr;
                break;
            }
            case CTy::Array: {
                auto aptr = static_cast<ArrayContainer<WordType, DataBits>*>(
                    entry.containerPtr);
                ArrayContainer<WordType, DataBits>::set(aptr, data);
                entry.containerPtr = (void*)aptr;
                break;
            }
            case CTy::Bitmap: {
                static_cast<BitmapContainer<WordType, DataBits>*>(
                    entry.containerPtr)
                    ->set(data);
                break;
            }
            default:
                FROARING_UNREACHABLE
        }
    }

    // Calculate the total cardinality of all containers
    size_t cardinality() const {
        size_t total = 0;
        for (const auto& entry : containers) {
            switch (entry.containerType) {
                case CTy::RLE:
                    total += static_cast<RLEContainer<WordType, DataBits>*>(
                                 entry.containerPtr)
                                 ->cardinality();
                    break;
                case CTy::Array:
                    total += static_cast<ArrayContainer<WordType, DataBits>*>(
                                 entry.containerPtr)
                                 ->cardinality();
                    break;
                case CTy::Bitmap:
                    total += static_cast<BitmapContainer<WordType, DataBits>*>(
                                 entry.containerPtr)
                                 ->cardinality();
                    break;
                default:
                    FROARING_UNREACHABLE
            }
        }
        return total;
    }

    // Remove a container by index
    void remove(IndexType index) {
        // !FIXME: container not freed
        // TODO: !!!
    }

    // Preserved
    void setFlag(IndexType index, uint8_t newFlag) {
        auto* entry = get(index);
        if (entry) {
            entry->flag = newFlag;
        }
    }

    // Destructor to handle cleanup
    ~Containers() {
        for (auto& entry : containers) {
            deleteContainer(entry);
        }
    }

private:
    // Helper function to delete containers based on their type
    void deleteContainer(ContainerEntry& entry) {
        switch (entry.containerType) {
            case CTy::RLE:
                delete static_cast<RLEContainer<WordType, DataBits>*>(
                    entry.containerPtr);
                break;
            case CTy::Array:
                delete static_cast<ArrayContainer<WordType, DataBits>*>(
                    entry.containerPtr);
                break;
            case CTy::Bitmap:
                delete static_cast<BitmapContainer<WordType, DataBits>*>(
                    entry.containerPtr);
                break;
            default:
                FROARING_UNREACHABLE
                break;
        }
    }
};
}  // namespace froaring