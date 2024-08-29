#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "prelude.h"

// TODO: merge runs after insertion
// TODO: addRange may delete runs in middle!(
// FIXME: Check all the memmove-s!
namespace froaring {
template <typename WordType, size_t DataBits>
class RLEContainer {
    using IndexOrNumType = froaring::can_fit_t<DataBits>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;
    static constexpr SizeType initial_capacity = RLE_CONTAINER_INIT_SIZE;
    using RunPair =
        std::pair<IndexOrNumType, IndexOrNumType>;  // e.g., {5,10} stands for
                                                    // [5,6,7,8,9,10]
    SizeType capacity;
    IndexOrNumType size;  // Always less than 2**(DataBits-1)
    RunPair runs[];

public:
    RLEContainer(const RLEContainer&) = delete;
    RLEContainer& operator=(const RLEContainer&) = delete;

    static RLEContainer* create(SizeType capacity = RLE_CONTAINER_INIT_SIZE) {
        size_t totalSize = sizeof(RLEContainer) + capacity * sizeof(RunPair);
        void* memory = operator new(totalSize);
        RLEContainer* container = new (memory) RLEContainer();
        return container;
    }

    void clear() { size = 0; }

    /// @brief Set a bit of the container. NOTE: `c` may be changed!
    /// @param c Points to the container, may be changed if the container get
    /// expanded.
    /// @param num The bit to set.
    static void set(RLEContainer*& c, IndexOrNumType num) {
        if (!c->size) {
            c->runs[0] = {num, num};
            c->size = 1;
            return;
        }
        auto pos = c->upper_bound(num);
        if (pos < c->size && c->runs[pos].first <= num &&
            num <= c->runs[pos].second) {
            return;  // already set, do nothing.
        }
        set_raw(c, pos, num);
    }

    static void reset(RLEContainer*& c, IndexOrNumType num) {
        if (!c->size) return;
        auto pos = c->upper_bound(num);
        auto old_end = c->runs[pos].second;
        if (c->runs[pos].first > num || c->runs[pos].second < num) return;

        if (c->runs[pos].first == num &&
            c->runs[pos].second ==
                num) {  // run is a single element, just remove it
            memmove(&c->runs[pos], &c->runs[pos + 1],
                    (c->size - pos - 1) * sizeof(RunPair));
            --c->size;
        } else if (c->runs[pos].first == num) {
            c->runs[pos].first++;
        } else if (c->runs[pos].second == num) {
            c->runs[pos].second--;
        } else {  // split the run [a,b] into: [a, num-1] and [num+1, b]
            if (c->size == c->capacity) RLEContainer::expand(c);

            std::memmove(&c->runs[pos + 2], &c->runs[pos + 1],
                         (c->size - pos - 1) * sizeof(RunPair));
            c->size++;
            c->runs[pos].second = num - 1;
            c->runs[pos + 1] = {num + 1, old_end};
        }
    }

    bool test(IndexOrNumType num) const {
        if (!size) return false;
        auto pos = upper_bound(num);
        return (pos < size && runs[pos].first <= num &&
                num <= runs[pos].second);
    }

    bool test_and_set(IndexOrNumType num) {
        bool was_set;
        IndexOrNumType pos;
        if (!size) {
            was_set = false;
        } else {
            pos = upper_bound(num);
            was_set = (pos < size && runs[pos].first <= num &&
                       num <= runs[pos].second);
        }
        if (was_set) return false;
        set_raw(pos, num);
        return true;
    }

    SizeType cardinality() const {
        SizeType count = 0;
        for (IndexOrNumType i = 0; i < size; ++i) {
            count += runs[i].second - runs[i].first;
        }
        // We need to add 1 to the count because the range is inclusive
        return count + size;
    }

    IndexOrNumType pairs() const { return size; }

    void debug_print() const {
        for (size_t i = 0; i < size; ++i) {
            std::cout << "[" << int(runs[i].first) << "," << int(runs[i].second)
                      << "] ";
        }
        std::cout << std::endl;
    }

private:
    RLEContainer(SizeType capacity = RLE_CONTAINER_INIT_SIZE, SizeType size = 0)
        : capacity(capacity), size(size) {}

    SizeType upper_bound(IndexOrNumType num) const {
        assert(size && "Cannot find upper bound in an empty container");
        SizeType low = 0;
        SizeType high = size;
        while (low < high) {
            SizeType mid = low + (high - low) / 2;
            if (runs[mid].first <= num) {
                low = mid + 1;
            } else {
                high = mid;
            }
        }
        return (low > 0) ? low - 1 : 0;
    }

    static void expand(RLEContainer*& c) {
        auto new_cap = c->capacity * 2;

        size_t totalSize = sizeof(RLEContainer) + new_cap * sizeof(RunPair);
        void* new_memory = operator new(totalSize);
        RLEContainer* new_container =
            new (new_memory) RLEContainer(new_cap, c->size);

        std::memmove(&new_container->runs, &c->runs, c->size * sizeof(RunPair));
        c->~RLEContainer();
        operator delete(c);

        c = new_container;
    }

    static void set_raw(RLEContainer*& c, IndexOrNumType pos,
                        IndexOrNumType num) {
        // If the value is next to the previous run's end (and need merging)
        bool merge_prev = (num > 0 && num - 1 == c->runs[pos].second);
        // If the value is next to the next run's start (and need merging)
        bool merge_next = (pos < c->size - 1 && c->runs[pos + 1].first > 0 &&
                           c->runs[pos + 1].first - 1 == num);
        if (merge_prev && merge_next) {  // [a,num-1] + num + [num+1, b]

            c->runs[pos].second = c->runs[pos + 1].second;
            std::memmove(&c->runs[pos + 1], &c->runs[pos + 2],
                         (c->size - pos - 1) * sizeof(RunPair));
            c->size--;
            return;
        }
        if (merge_prev) {  // [a,num-1] + num
            c->runs[pos].second++;
            return;
        }
        if (merge_next) {  // num + [num+1, b]
            c->runs[pos + 1].first--;
            return;
        }

        if (c->size == c->capacity) {
            expand(c);
        }
        if (c->size - pos > 2)
            std::memmove(&c->runs[pos + 2], &c->runs[pos + 1],
                         (c->size - pos - 2) * sizeof(RunPair));
        c->size++;
        c->runs[pos + 1] = {num, num};
        return;
    }
};
}  // namespace froaring