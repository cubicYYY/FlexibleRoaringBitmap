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
    using IndexOrNumType = froaring::can_fit_t<DataBits>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;

public:
    void debug_print() {
        for (SizeType i = 0; i < size; ++i) {
            std::cout << int(vals[i]) << " ";
        }
        std::cout << std::endl;
    }
    static ArrayContainer* create(
        SizeType capacity = ARRAY_CONTAINER_INIT_SIZE) {
        size_t totalSize =
            sizeof(ArrayContainer) + capacity * sizeof(IndexOrNumType);
        void* memory = operator new(totalSize);
        ArrayContainer* container = new (memory) ArrayContainer();
        return container;
    }

    ArrayContainer(const ArrayContainer&) = delete;
    ArrayContainer& operator=(const ArrayContainer&) = delete;

    void clear() { size = 0; }

    static void set(ArrayContainer*& c, IndexOrNumType num) {
        auto pos = (c->size ? c->lower_bound(num) : 0);
        if (pos < c->size && c->vals[pos] == num) return;

        if (c->size == c->capacity) expand(c);

        std::memmove(&c->vals[pos + 1], &c->vals[pos],
                     (c->size - pos) *
                         sizeof(IndexOrNumType));  // TODO: Boost by combining
                                                   // memmove with expand()

        c->vals[pos] = num;
        ++c->size;
    }

    void reset(IndexOrNumType num) {
        if (!size) return;
        auto pos = lower_bound(num);
        if (pos == size || vals[pos] != num) return;

        std::memmove(&vals[pos], &vals[pos + 1],
                     (size - pos - 1) * sizeof(IndexOrNumType));
        --size;
    }

    bool test(IndexOrNumType num) const {
        if (!size) return false;
        auto pos = lower_bound(num);
        std::cout << "pos=" << int(pos) << " v[p]=" << int(vals[pos])
                  << std::endl;
        return pos < size && vals[pos] == num;
    }

    static bool test_and_set(ArrayContainer*& c, IndexOrNumType num) {
        bool was_set;
        IndexOrNumType pos;
        if (!c->size) {
            was_set = false;
        } else {
            pos = c->lower_bound(num);
            was_set = (pos < c->size && c->vals[pos] == num);
        }

        if (was_set) return false;

        if (c->size == c->capacity) expand(c);

        std::memmove(&c->vals[pos + 1], &c->vals[pos],
                     (c->size - pos) * sizeof(IndexOrNumType));

        c->vals[pos] = num;
        ++c->size;

        return true;
    }

    SizeType cardinality() const { return size; }

private:
    static void expand(ArrayContainer*& c) {
        auto new_cap = c->capacity * 2;

        size_t totalSize =
            sizeof(ArrayContainer) + new_cap * sizeof(IndexOrNumType);
        void* new_memory = operator new(totalSize);
        ArrayContainer* new_container =
            new (new_memory) ArrayContainer(new_cap, c->size);

        std::memmove(&new_container->vals, &c->vals,
                     c->size * sizeof(IndexOrNumType));
        c->~ArrayContainer();
        operator delete(c);

        c = new_container;
    }

    IndexOrNumType lower_bound(IndexOrNumType num) const {
        assert(size && "Cannot find lower bound in an empty container");
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

    ArrayContainer(SizeType capacity = ARRAY_CONTAINER_INIT_SIZE,
                   SizeType size = 0)
        : capacity(capacity), size(size) {}

    SizeType capacity;
    SizeType size;
    IndexOrNumType vals[];
};
}  // namespace froaring