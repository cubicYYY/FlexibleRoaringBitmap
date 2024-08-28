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
    using DataType = froaring::can_fit_t<DataBits>;
    static constexpr DataType initial_capacity = RLE_CONTAINER_INIT_SIZE;
    using RunPair = std::pair<DataType, DataType>;  // e.g., {5,10} stands for
                                                    // [5,6,7,8,9,10]
    RunPair* runs;
    size_t capacity;
    DataType run_count;  // Always less than 2**(DataBits-1)

public:
    RLEContainer()
        : runs(new RunPair[initial_capacity]),
          capacity(initial_capacity),
          run_count(0) {}

    ~RLEContainer() { delete[] runs; }

    RLEContainer(const RLEContainer&) = delete;
    RLEContainer& operator=(const RLEContainer&) = delete;

    void clear() { run_count = 0; }

    void set(DataType num) {
        if (!run_count) {
            assert(capacity > 0 && "???");
            runs[0] = {num, num};
            run_count = 1;
            return;
        }
        auto pos = upper_bound(num);
        if (pos < run_count && runs[pos].first <= num &&
            num <= runs[pos].second) {
            return;  // already set, do nothing.
        }
        // If the value is next to the previous run's end (and need merging)
        bool merge_prev = (num > 0 && num - 1 == runs[pos].second);
        // If the value is next to the next run's start (and need merging)
        bool merge_next = (pos < run_count - 1 && runs[pos + 1].first > 0 &&
                           runs[pos + 1].first - 1 == num);
        if (merge_prev && merge_next) {  // [a,num-1] + num + [num+1, b]

            runs[pos].second = runs[pos + 1].second;
            std::memmove(&runs[pos + 1], &runs[pos + 2],
                         (run_count - pos - 1) * sizeof(RunPair));
            run_count--;
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

        if (run_count == capacity) {
            expand();
        }
        if (run_count - pos > 2)
            std::memmove(&runs[pos + 2], &runs[pos + 1],
                         (run_count - pos - 2) * sizeof(RunPair));
        run_count++;
        runs[pos + 1] = {num, num};
        return;
    }

    void reset(DataType num) {
        if (!run_count) return;
        auto pos = upper_bound(num);
        auto& run = runs[pos];
        if (run.first > num || run.second < num) return;

        if (run.first == num &&
            run.second == num) {  // run is a single element, just remove it
            memmove(&runs[pos], &runs[pos + 1],
                    (run_count - pos - 1) * sizeof(RunPair));
            --run_count;
        } else if (run.first == num) {
            run.first++;
        } else if (run.second == num) {
            run.second--;
        } else {  // split the run [a,b] into: [a, num-1] and [num+1, b]
            if (run_count == capacity) {
                expand();
            }
            auto old_end = run.second;  // b
            std::memmove(&runs[pos + 2], &runs[pos + 1],
                         (run_count - pos - 1) * sizeof(RunPair));
            run_count++;
            runs[pos].second = num - 1;
            runs[pos + 1] = {num + 1, old_end};
        }
    }

    bool test(DataType num) const {
        if (!run_count) return false;
        auto pos = upper_bound(num);
        return (pos < run_count && runs[pos].first <= num &&
                num <= runs[pos].second);
    }

    bool test_and_set(DataType num) {
        bool was_set = test(num);
        if (was_set) return false;
        set(num);
        return true;
    }

    size_t cardinality() const {
        size_t count = 0;
        for (DataType i = 0; i < run_count; ++i) {
            count += runs[i].second - runs[i].first;
        }
        // We need to add 1 to the count because the range is inclusive
        return count + run_count;
    }

    DataType pairs() const { return run_count; }

    void debug_print() const {
        for (size_t i = 0; i < run_count; ++i) {
            std::cout << "[" << int(runs[i].first) << "," << int(runs[i].second)
                      << "] ";
        }
        std::cout << std::endl;
    }

private:
    DataType upper_bound(DataType num) const {
        assert(run_count && "Cannot find upper bound in an empty container");
        DataType low = 0;
        DataType high = run_count;
        while (low < high) {
            DataType mid = low + (high - low) / 2;
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
        auto new_runs = new RunPair[capacity];
        std::memmove(new_runs, runs, run_count * sizeof(RunPair));
        delete[] runs;
        runs = new_runs;
    }
};
}  // namespace froaring