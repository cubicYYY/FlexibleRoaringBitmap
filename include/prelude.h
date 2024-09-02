#pragma once

#include <bit>
#include <cassert>
#include <new>

#define INIT_FLAG 0x1
#define FROARING_UNREACHABLE assert(false && "Should never reach here");
#define FROARING_NOT_IMPLEMENTED assert(false && "Not implemented yet");

namespace froaring {

#define CTYPE_PAIR(t1, t2) (static_cast<int>(t1) * 4 + static_cast<int>(t2))
enum class ContainerType { Array, Bitmap, RLE, Containers };
const int ARRAY_CONTAINER_INIT_CAPACITY = 4;
const int RLE_CONTAINER_INIT_CAPACITY = 4;
const int CONTAINERS_INIT_CAPACITY = 16;
/// so we will use linear scan instead of bin-search for small containers
const size_t MINIMAL_SIZE_TO_BINSEARCH = 8;
struct froaring_container_t {};
struct froaring_indices_t : public froaring_container_t {};
using froaring_container_t = struct froaring_container_t;
using froaring_indices_t = struct froaring_indices_t;

template <typename Derived>
class Destroyable {
public:
    void destroy() {
        static_cast<Derived*>(this)->~Derived();
        free(static_cast<void*>(this));
    }

protected:
    Destroyable() = default;
    Destroyable(const Destroyable&) = delete;
    Destroyable& operator=(const Destroyable&) = delete;
    ~Destroyable() = default;
};

// Helper functions
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
            std::conditional_t<Bits <= 32, uint32_t, std::conditional_t<Bits <= 64, uint64_t, InvalidBigType>>>>;
};

// Define a convenient alias for IndexType
template <size_t Bits>
using can_fit_t = typename can_fit<Bits>::type;

// Separeate the index bits ("high" bits) and data bits (low bits) from a value
// TODO: Need unit test
template <size_t IndexBits, size_t DataBits>
void num2index_n_data(can_fit_t<IndexBits + DataBits> value, can_fit_t<IndexBits>& index, can_fit_t<DataBits>& data) {
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

/// @brief Get log2(x) floored at compile time.
/// @example cexpr_log2(8) = 3, cexpr_log2(9) = 3, cexpr_log2(16) = 4
template <typename T>
constexpr T cexpr_log2(T x) {
    return x == 1 ? 0 : 1 + cexpr_log2(x >> 1);
}

}  // namespace froaring