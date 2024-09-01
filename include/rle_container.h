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
    RLEContainer(SizeType capacity = RLE_CONTAINER_INIT_CAPACITY, SizeType run_count = 0)
        : capacity(std::max(capacity, run_count)), run_count(run_count), runs(static_cast<RunPair*>(malloc(capacity * sizeof(RunPair)))) {
        assert(runs && "Failed to allocate memory for RLEContainer");
    }

    ~RLEContainer() { free(runs); }

    RLEContainer(const RLEContainer&) = delete;
    RLEContainer& operator=(const RLEContainer&) = delete;

    void debug_print() const {
        for (size_t i = 0; i < run_count; ++i) {
            std::cout << "[" << int(runs[i].first) << "," << int(runs[i].second) << "] ";
        }
        std::cout << std::endl;
    }

    void clear() { run_count = 0; }

    /// @brief Set a bit of the container.
    /// @param num The bit to set.
    void set(IndexType num) {
        if (!run_count) {
            runs[0] = {num, num};
            run_count = 1;
            return;
        }
        auto pos = lower_bound(num);
        if (pos < run_count && runs[pos].first <= num && num <= runs[pos].second) {
            return;  // already set, do nothing.
        }
        set_raw(pos, num);
    }

    void reset(IndexType num) {
        if (!run_count) return;
        auto pos = lower_bound(num);
        if (pos == run_count || runs[pos].first > num || runs[pos].second < num) return;
        auto old_end = runs[pos].second;
        if (runs[pos].first == num && runs[pos].second == num) {  // run is a single element, just remove it
            if (pos < run_count) memmove(&runs[pos], &runs[pos + 1], (run_count - pos - 1) * sizeof(RunPair));
            --run_count;
        } else if (runs[pos].first == num) {
            runs[pos].first++;
        } else if (runs[pos].second == num) {
            runs[pos].second--;
        } else {  // split the run [a,b] into: [a, num-1] and [num+1, b]
            if (run_count == capacity) expand();
            std::memmove(&runs[pos + 1], &runs[pos], (run_count - pos) * sizeof(RunPair));
            run_count++;
            runs[pos].second = num - 1;
            runs[pos + 1].first = num + 1;
        }
    }

    bool test(IndexType num) const {
        if (!run_count) return false;
        auto pos = lower_bound(num);
        return (pos < run_count && runs[pos].first <= num && num <= runs[pos].second);
    }

    bool test_and_set(IndexType num) {
        bool was_set;
        IndexType pos;
        if (!run_count) {
            was_set = false;
        } else {
            pos = lower_bound(num);
            was_set = (pos < run_count && runs[pos].first <= num && num <= runs[pos].second);
        }
        if (was_set) return false;
        set_raw(pos, num);
        return true;
    }

    SizeType cardinality() const {
        SizeType count = 0;
        for (IndexType i = 0; i < run_count; ++i) {
            count += runs[i].second - runs[i].first;
        }
        // We need to add 1 to the count because the range is inclusive
        return count + run_count;
    }

    IndexType runsCount() const { return run_count; }

private:
    SizeType lower_bound(IndexType num) const {
        assert(run_count && "Cannot find lower bound in an empty container");
        SizeType left = 0;
        SizeType right = run_count;
        while (left < right) {
            SizeType mid = left + (right - left) / 2;
            if (runs[mid].second < num) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        return left;
    }

    void expand() {
        capacity *= 2;

        void* new_memory = realloc(runs, capacity * sizeof(RunPair));
        assert(new_memory && "Failed to reallocate memory for ArrayContainer");
        runs = static_cast<RunPair*>(new_memory);
    }

    void set_raw(IndexType pos, IndexType num) {
        // If the value is next to the previous run's end (and need merging)
        bool merge_prev = (pos > 0 && num > 0 && num - 1 == runs[pos - 1].second);
        // If the value is next to the next run's start (and need merging)
        bool merge_next = (pos < run_count && runs[pos].first > 0 && runs[pos].first - 1 == num);
        if (merge_prev && merge_next) {  // [a,num-1] + num + [num+1, b]

            runs[pos - 1].second = runs[pos].second;
            if (pos < run_count) std::memmove(&runs[pos], &runs[pos + 1], (run_count - pos - 1) * sizeof(RunPair));
            run_count--;
            return;
        }
        if (merge_prev) {  // [a,num-1] + num -> [a, num]
            runs[pos - 1].second++;
            return;
        }
        if (merge_next) {  // num + [num+1, b] -> [num, b]
            runs[pos].first--;
            return;
        }

        if (run_count == capacity) {
            expand();
        }
        if (pos < run_count) std::memmove(&runs[pos + 1], &runs[pos], (run_count - pos) * sizeof(RunPair));
        run_count++;
        runs[pos] = {num, num};
        return;
    }

public:
    SizeType capacity;
    IndexType run_count;  // Always less than 2**(DataBits-1), so we do not need SizeType
    RunPair* runs;
};
}  // namespace froaring