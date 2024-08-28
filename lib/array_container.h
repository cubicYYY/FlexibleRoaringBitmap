#pragma once

#include <array>
#include <cstdint>
#include <cstring>  // for std::memmove
#include <iostream>

#include "prelude.h"

// FIXME: check all the memmove-s!
namespace froaring {
template <typename WordType, size_t DataBits>
class ArrayContainer {
    using DataType = froaring::can_fit_t<DataBits>;

public:
    ArrayContainer() : capacity(ARRAY_CONTAINER_INIT_SIZE), size(0) {
        vals = new DataType[capacity];
    }

    ~ArrayContainer() { delete[] vals; }

    ArrayContainer(const ArrayContainer&) = delete;
    ArrayContainer& operator=(const ArrayContainer&) = delete;

    void clear() { size = 0; }

    void set(DataType num) {
        auto pos = (size ? lower_bound(num) : 0);
        if (pos < size && vals[pos] == num) return;

        if (size == capacity) expand();

        std::memmove(&vals[pos + 1], &vals[pos],
                     (size - pos) * sizeof(DataType));

        vals[pos] = num;
        ++size;
    }

    void reset(DataType num) {
        if (!size) return;
        auto pos = lower_bound(num);
        if (pos == size || vals[pos] != num) return;

        std::memmove(&vals[pos], &vals[pos + 1],
                     (size - pos - 1) * sizeof(DataType));
        --size;
    }

    bool test(DataType num) const {
        if (!size) return false;
        auto pos = lower_bound(num);
        return pos < size && vals[pos] == num;
    }

    DataType cardinality() const { return size; }

private:
    void expand() {
        capacity *= 2;
        auto new_vals = new DataType[capacity];
        std::memmove(new_vals, vals, size * sizeof(DataType));
        delete[] vals;
        vals = new_vals;
    }

    DataType lower_bound(DataType num) const {
        assert(size && "Cannot find lower bound in an empty container");
        DataType left = 0;
        DataType right = size;
        while (left < right) {
            DataType mid = left + (right - left) / 2;
            if (vals[mid] < num) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        return left;
    }

    DataType* vals;
    size_t capacity;  // TODO: use less memory by using a smaller type
    size_t size;
};
}  // namespace froaring