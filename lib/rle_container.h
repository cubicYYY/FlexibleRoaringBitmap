#pragma once

#include <array>
#include <cstdint>
#include <iostream>

namespace froaring {
template <typename WordType, size_t DataBits>
class RLEContainer {
public:
    std::vector<std::pair<uint16_t, uint16_t>> runs;  // (start, length) pairs

    void set(uint16_t index) {
        for (auto it = runs.begin(); it != runs.end(); ++it) {
            uint16_t start = it->first;
            uint16_t length = it->second;

            if (index >= start && index < start + length) {
                // Index already within an existing run, no need to insert.
                return;
            } else if (index == start + length) {
                // Index directly follows an existing run, extend the run.
                it->second++;
                // Check if this extension connects to the next run and merge.
                auto next_it = std::next(it);
                if (next_it != runs.end() && start + length == next_it->first) {
                    it->second += next_it->second;
                    runs.erase(next_it);
                }
                return;
            } else if (index < start) {
                // New run needed before the current run.
                runs.insert(it, std::make_pair(index, 1));
                return;
            }
        }
    }

    bool test(uint16_t index) const {
        for (const auto& run : runs) {
            if (index >= run.first && index < run.first + run.second) {
                return true;
            }
        }
        return false;
    }

    size_t cardinality() const {
        size_t count = 0;
        for (const auto& run : runs) {
            count += run.second;
        }
        return count;
    }
};
}  // namespace froaring