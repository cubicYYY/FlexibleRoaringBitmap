#pragma once

#include <cassert>

#define FROARING_UNREACHABLE assert(false && "Should never reach here");

namespace froaring {

enum class ContainerType { Array, Bitmap, RLE, Containers };
const int ARRAY_CONTAINER_INIT_SIZE = 4;
const int RLE_CONTAINER_INIT_SIZE = 4;

template <size_t Bits>
struct can_fit {
    // Define an error type that triggers a static assertion when used
    struct InvalidBigType {
        static_assert(Bits <= 64, "Bits cannot exceed 64 bits");
    };

    /// Choose the type that can hold the specified number of bits
    /// For example,
    /// 24 bits => uint32_t,
    /// 64 bits => uint64_t,
    /// 65 bits => Compilation error.
    using type = std::conditional_t<
        Bits <= 8, uint8_t,
        std::conditional_t<
            Bits <= 16, uint16_t,
            std::conditional_t<
                Bits <= 32, uint32_t,
                std::conditional_t<Bits <= 64, uint64_t, InvalidBigType>>>>;
};

// Define a convenient alias for IndexType
template <size_t Bits>
using can_fit_t = typename can_fit<Bits>::type;

// Separeate the index bits ("high" bits) and data bits (low bits) from a value
// TODO: Need unit test
template <size_t IndexBits, size_t DataBits>
void num2index_n_data(can_fit_t<IndexBits + DataBits> value,
                      can_fit_t<IndexBits>& index, can_fit_t<DataBits>& data) {
    static_assert(IndexBits + DataBits <= sizeof(value) * 8,
                  "IndexBits + DataBits exceeds the type size of the value.");
    assert(value < (~can_fit_t<IndexBits + DataBits>(0)) &&
           "Value exceeds the allowed index bits.");  // FIXME: the check is not
                                                      // sound if can_fit_t is
                                                      // larger than actual bits
                                                      // needed
    data = value & ((can_fit_t<IndexBits + DataBits>(1) << DataBits) - 1);
    index = value >> DataBits;
}

}  // namespace froaring