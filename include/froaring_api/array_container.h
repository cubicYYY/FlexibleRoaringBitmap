#pragma once

#include <array>
#include <cstdint>
#include <cstring>  // for std::memmove
#include <iostream>

#include "prelude.h"
namespace froaring {
template <typename WordType, size_t DataBits>
class ArrayContainer : public froaring_container_t {
public:
    using IndexOrNumType = froaring::can_fit_t<DataBits>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;
    /// Bit capacity for containers indexed
    static constexpr size_t ContainerCapacity = (1 << DataBits);
    /// Array threshold (Array will not be optimum for storing more
    /// elements)
    static constexpr size_t ArrayToBitmapCountThreshold = ContainerCapacity / DataBits;
    static constexpr size_t UseLinearScanThreshold = 8;

public:
    void debug_print() {
        for (SizeType i = 0; i < size; ++i) {
            std::cout << int(vals[i]) << " ";
        }
        std::cout << std::endl;
    }

    explicit ArrayContainer(SizeType capacity = ARRAY_CONTAINER_INIT_CAPACITY, SizeType size = 0)
        : capacity(std::max(capacity, size)),
          size(size),
          vals(static_cast<IndexOrNumType*>(malloc(capacity * sizeof(IndexOrNumType)))) {
        assert(vals && "Failed to allocate memory for ArrayContainer");
    }
    explicit ArrayContainer(const ArrayContainer& other)
        : capacity(other.size),
          size(other.size),
          vals(static_cast<IndexOrNumType*>(malloc(this->capacity * sizeof(IndexOrNumType)))) {
        std::memcpy(vals, other.vals, other.size * sizeof(IndexOrNumType));
    }

    ~ArrayContainer() { free(vals); }

    ArrayContainer& operator=(const ArrayContainer&) = delete;

    void clear() { size = 0; }

    void set(IndexOrNumType num) {
        IndexOrNumType pos = (size ? lower_bound(num) : 0);
        if (pos < size && vals[pos] == num) return;

        if (size == capacity) expand();

        std::memmove(&vals[pos + 1], &vals[pos],
                     (size - pos) * sizeof(IndexOrNumType));  // TODO: Boost by combining
                                                              // memmove with expand()

        vals[pos] = num;
        ++size;
    }

    void reset(IndexOrNumType num) {
        if (!size) return;
        auto pos = lower_bound(num);
        if (pos == size || vals[pos] != num) return;

        std::memmove(&vals[pos], &vals[pos + 1], (size - pos - 1) * sizeof(IndexOrNumType));
        --size;
    }

    bool test(IndexOrNumType num) const {
        if (!size) return false;
        auto pos = lower_bound(num);
        return pos < size && vals[pos] == num;
    }

    bool test_and_set(IndexOrNumType num) {
        bool was_set;
        IndexOrNumType pos;
        if (!size) {
            was_set = false;
        } else {
            pos = lower_bound(num);
            was_set = (pos < size && vals[pos] == num);
        }

        if (was_set) return false;

        if (size == capacity) expand();

        std::memmove(&vals[pos + 1], &vals[pos], (size - pos) * sizeof(IndexOrNumType));

        vals[pos] = num;
        ++size;

        return true;
    }

    SizeType cardinality() const { return size; }

    void expand() { expand_to(this->capacity * 2); }

    void expand_to(SizeType new_cap) {
        void* new_memory = realloc(vals, new_cap * sizeof(IndexOrNumType));
        assert(new_memory && "Failed to reallocate memory for ArrayContainer");
        vals = static_cast<IndexOrNumType*>(new_memory);
        this->capacity = new_cap;
    }

    IndexOrNumType lower_bound(IndexOrNumType num) const {
        if (size < UseLinearScanThreshold) {
            for (SizeType i = 0; i < size; ++i) {
                if (vals[i] >= num) {
                    return i;
                }
            }
            return size;
        }
        SizeType left = 0;
        SizeType right = size;
        while (left < right) {
            SizeType mid = left + (right - left) / 2;
            if (vals[mid] < num) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        return left;
    }

    SizeType advanceUntil(IndexOrNumType key, SizeType pos) const {
        // TODO: use Gallop search
        while (pos < size && vals[pos] < key) {
            pos++;
        }
        return pos;
    }

public:
    SizeType capacity;
    SizeType size;
    IndexOrNumType* vals;
};
}  // namespace froaring