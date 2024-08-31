#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "prelude.h"

// TODO: addRange may delete runs in middle!(
// FIXME: Check all the memmove-s!
namespace froaring {
template <typename WordType, size_t DataBits>
class RLEContainer : public froaring_container_t {
public:
    using IndexType = froaring::can_fit_t<DataBits>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;
    static constexpr SizeType initial_capacity = RLE_CONTAINER_INIT_CAPACITY;
    using RunPair = std::pair<IndexType, IndexType>;  // e.g., {5,10} stands for
                                                      // [5,6,7,8,9,10]

public:
    RLEContainer(SizeType capacity = RLE_CONTAINER_INIT_CAPACITY, SizeType size = 0)
        : capacity(capacity), size(size), runs(static_cast<RunPair*>(malloc(capacity * sizeof(RunPair)))) {
        assert(runs && "Failed to allocate memory for RLEContainer");
    }

    ~RLEContainer() { free(runs); }

    RLEContainer(const RLEContainer&) = delete;
    RLEContainer& operator=(const RLEContainer&) = delete;

    void debug_print() const {
        for (size_t i = 0; i < size; ++i) {
            std::cout << "[" << int(runs[i].first) << "," << int(runs[i].second) << "] ";
        }
        std::cout << std::endl;
    }

    void clear() { size = 0; }

    /// @brief Set a bit of the container.
    /// @param num The bit to set.
    void set(IndexType num) {
        if (!size) {
            runs[0] = {num, num};
            size = 1;
            return;
        }
        auto pos = upper_bound(num);
        if (pos < size && runs[pos].first <= num && num <= runs[pos].second) {
            return;  // already set, do nothing.
        }
        set_raw(pos, num);
    }

    void reset(IndexType num) {
        if (!size) return;
        auto pos = upper_bound(num);
        auto old_end = runs[pos].second;
        if (runs[pos].first > num || runs[pos].second < num) return;

        if (runs[pos].first == num && runs[pos].second == num) {  // run is a single element, just remove it
            if (pos < size) memmove(&runs[pos], &runs[pos + 1], (size - pos - 1) * sizeof(RunPair));
            --size;
        } else if (runs[pos].first == num) {
            runs[pos].first++;
        } else if (runs[pos].second == num) {
            runs[pos].second--;
        } else {  // split the run [a,b] into: [a, num-1] and [num+1, b]
            if (size == capacity) expand();
            if (pos < size) std::memmove(&runs[pos + 2], &runs[pos + 1], (size - pos - 1) * sizeof(RunPair));
            size++;
            runs[pos].second = num - 1;
            runs[pos + 1] = {num + 1, old_end};
        }
    }

    bool test(IndexType num) const {
        if (!size) return false;
        auto pos = upper_bound(num);
        return (pos < size && runs[pos].first <= num && num <= runs[pos].second);
    }

    bool test_and_set(IndexType num) {
        bool was_set;
        IndexType pos;
        if (!size) {
            was_set = false;
        } else {
            pos = upper_bound(num);
            was_set = (pos < size && runs[pos].first <= num && num <= runs[pos].second);
        }
        if (was_set) return false;
        set_raw(pos, num);
        return true;
    }

    SizeType cardinality() const {
        SizeType count = 0;
        for (IndexType i = 0; i < size; ++i) {
            count += runs[i].second - runs[i].first;
        }
        // We need to add 1 to the count because the range is inclusive
        return count + size;
    }

    IndexType runsCount() const { return size; }

private:
    SizeType upper_bound(IndexType num) const {
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

    void expand() {
        capacity *= 2;

        void* new_memory = realloc(runs, capacity * sizeof(RunPair));
        assert(new_memory && "Failed to reallocate memory for ArrayContainer");
        runs = static_cast<RunPair*>(new_memory);
    }

    void set_raw(IndexType pos, IndexType num) {
        // If the value is next to the previous run's end (and need merging)
        bool merge_prev = (num > 0 && num - 1 == runs[pos].second);
        // If the value is next to the next run's start (and need merging)
        bool merge_next = (pos < size - 1 && runs[pos + 1].first > 0 && runs[pos + 1].first - 1 == num);
        if (merge_prev && merge_next) {  // [a,num-1] + num + [num+1, b]

            runs[pos].second = runs[pos + 1].second;
            if (pos < size) std::memmove(&runs[pos + 1], &runs[pos + 2], (size - pos - 1) * sizeof(RunPair));
            size--;
            return;
        }
        if (merge_prev) {  // [a,num-1] + num
            runs[pos].second++;
            return;
        }
        if (merge_next) {  // num + [num+1, b]
            runs[pos + 1].first--;
            return;
        }

        if (size == capacity) {
            expand();
        }
        if (pos < size) std::memmove(&runs[pos + 2], &runs[pos + 1], (size - pos - 1) * sizeof(RunPair));
        size++;
        runs[pos + 1] = {num, num};
        return;
    }

public:
    SizeType capacity;
    IndexType size;  // Always less than 2**(DataBits-1), so we do not need SizeType
    RunPair* runs;
};
}  // namespace froaring