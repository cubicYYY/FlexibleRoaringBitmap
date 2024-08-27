#pragma once

#include <array>
#include <cstdint>
#include <iostream>

#include "prelude.h"

namespace froaring {
template <typename WordType, size_t DataBits>
class ArrayContainer {
    using DataType = froaring::can_fit_t<DataBits>;

public:
    /// @brief Indices of "1" bits. Always sorted.
    std::vector<DataType> vals;

    void set(DataType num) {
        auto it = std::lower_bound(vals.begin(), vals.end(), num);
        if (it == vals.end() || *it != num) {
            vals.insert(it, num);
        }
    }

    void reset(DataType num) {
        auto it = std::lower_bound(vals.begin(), vals.end(), num);
        if (it != vals.end() && *it == num) {
            vals.erase(it);
        }
    }

    bool test(DataType num) const {
        auto it = std::lower_bound(vals.begin(), vals.end(), num);
        if (it == vals.end() || *it != num) {
            return false;
        }
        return true;
    }

    size_t cardinality() const { return vals.size(); }
};
}  // namespace froaring