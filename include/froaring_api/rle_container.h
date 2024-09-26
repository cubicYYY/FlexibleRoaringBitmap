#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "prelude.h"

// TODO: addRange may delete runs in middle!(not implemented, just a reminder for whom will)
namespace froaring {
template <typename WordType, size_t DataBits>
class RLEContainer : public froaring_container_t {
public:
    using IndexOrNumType = froaring::can_fit_t<DataBits>;
    using SizeType = froaring::can_fit_t<DataBits + 1>;
    static constexpr SizeType initial_capacity = RLE_CONTAINER_INIT_CAPACITY;
    using RunPair = struct {
        IndexOrNumType start;
        IndexOrNumType end;
    };  // e.g., {5,10} stands for [5, 6, 7, 8, 9, 10]

    /// Bit capacity for containers indexed
    static constexpr size_t ContainerCapacity = (1 << DataBits);
    /// RLE threshold (RLE will not be optimum for more runs)
    static constexpr size_t RleToBitmapRunThreshold = ContainerCapacity / (DataBits * 2);
    static constexpr size_t UseLinearScanThreshold = 8;

public:
    explicit RLEContainer(SizeType capacity = RLE_CONTAINER_INIT_CAPACITY, SizeType run_count = 0)
        : capacity(std::max(capacity, run_count)),
          run_count(run_count),
          runs(static_cast<RunPair*>(malloc(capacity * sizeof(RunPair)))) {
        assert(runs && "Failed to allocate memory for RLEContainer");
    }

    explicit RLEContainer(const RLEContainer& other)
        : capacity(other.run_count),
          run_count(other.run_count),
          runs(static_cast<RunPair*>(malloc(this->capacity * sizeof(RunPair)))) {
        std::memcpy(this->runs, other.runs, sizeof(RunPair) * run_count);
    }

    ~RLEContainer() { free(runs); }

    RLEContainer& operator=(const RLEContainer&) = delete;

    void debug_print() const {
        for (size_t i = 0; i < run_count; ++i) {
            std::cout << "[" << int(runs[i].start) << "," << int(runs[i].end) << "] ";
        }
        std::cout << std::endl;
    }

    void clear() { run_count = 0; }

    /// @brief Set a bit of the container.
    /// @param num The bit to set.
    void set(IndexOrNumType num) {
        if (!run_count) {
            runs[0] = {num, num};
            run_count = 1;
            return;
        }
        auto pos = lower_bound(num);
        if (pos < run_count && runs[pos].start <= num && num <= runs[pos].end) {
            return;  // already set, do nothing.
        }
        set_raw(pos, num);
    }

    void reset(IndexOrNumType num) {
        if (!run_count) return;
        auto pos = lower_bound(num);
        if (pos == run_count || runs[pos].start > num || runs[pos].end < num) return;
        if (runs[pos].start == num && runs[pos].end == num) {  // run is a single element, just remove it
            if (pos < run_count) memmove(&runs[pos], &runs[pos + 1], (run_count - pos - 1) * sizeof(RunPair));
            --run_count;
        } else if (runs[pos].start == num) {
            runs[pos].start++;
        } else if (runs[pos].end == num) {
            runs[pos].end--;
        } else {  // split the run [a,b] into: [a, num-1] and [num+1, b]
            if (run_count == capacity) expand();
            std::memmove(&runs[pos + 1], &runs[pos], (run_count - pos) * sizeof(RunPair));
            run_count++;
            runs[pos].end = num - 1;
            runs[pos + 1].start = num + 1;
        }
    }

    bool test(IndexOrNumType num) const {
        if (!run_count) return false;
        auto pos = lower_bound(num);
        return (pos < run_count && runs[pos].start <= num && num <= runs[pos].end);
    }

    bool test_and_set(IndexOrNumType num) {
        bool was_set;
        IndexOrNumType pos;
        if (!run_count) {
            was_set = false;
        } else {
            pos = lower_bound(num);
            was_set = (pos < run_count && runs[pos].start <= num && num <= runs[pos].end);
        }
        if (was_set) return false;
        set_raw(pos, num);
        return true;
    }

    SizeType cardinality() const {
        SizeType count = 0;
        for (IndexOrNumType i = 0; i < run_count; ++i) {
            count += runs[i].end - runs[i].start;
        }
        // We need to add 1 to the count because the range is inclusive
        return count + run_count;
    }

    bool is_full() const { return run_count == 1 && runs[0].start == 0 && runs[0].end == ContainerCapacity - 1; }

private:
    SizeType lower_bound(IndexOrNumType num) const {
        if (run_count < UseLinearScanThreshold) {
            for (SizeType i = 0; i < run_count; ++i) {
                if (runs[i].end >= num) {
                    return i;
                }
            }
            return run_count;
        }
        SizeType left = 0;
        SizeType right = run_count;
        while (left < right) {
            SizeType mid = left + (right - left) / 2;
            if (runs[mid].end < num) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        return left;
    }

    void expand() { expand_to(this->capacity * 2); }

    void expand_to(SizeType new_cap) {
        void* new_memory = realloc(runs, new_cap * sizeof(RunPair));
        assert(new_memory && "Failed to reallocate memory for ArrayContainer");
        runs = static_cast<RunPair*>(new_memory);
        this->capacity = new_cap;
    }

    void set_raw(IndexOrNumType pos, IndexOrNumType num) {
        // If the value is next to the previous run's end (and need merging)
        bool merge_prev = (pos > 0 && num > 0 && num - 1 == runs[pos - 1].end);
        // If the value is next to the next run's start (and need merging)
        bool merge_next = (pos < run_count && runs[pos].start > 0 && runs[pos].start - 1 == num);
        if (merge_prev && merge_next) {  // [a,num-1] + num + [num+1, b]

            runs[pos - 1].end = runs[pos].end;
            if (pos < run_count) std::memmove(&runs[pos], &runs[pos + 1], (run_count - pos - 1) * sizeof(RunPair));
            run_count--;
            return;
        }
        if (merge_prev) {  // [a,num-1] + num -> [a, num]
            runs[pos - 1].end++;
            return;
        }
        if (merge_next) {  // num + [num+1, b] -> [num, b]
            runs[pos].start--;
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
    IndexOrNumType run_count;  // Always less than 2**(DataBits-1), so we do not need SizeType
    RunPair* runs;
};
}  // namespace froaring