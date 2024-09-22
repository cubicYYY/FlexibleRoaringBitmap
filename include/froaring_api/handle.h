#pragma once

#include "prelude.h"
#include "utils.h"

namespace froaring {
/// @brief to hold container data: pointer, type, and index
/// No copying is allowed: a handle is a unique pointer in essence.
template <typename IndexType>
struct ContainerHandleS {
public:
    ContainerHandleS() = default;
    ContainerHandleS(ContainerHandleS&&) = default;
    ContainerHandleS& operator=(ContainerHandleS&&) = default;

    ContainerHandleS(const ContainerHandleS&) = delete;
    ContainerHandleS& operator=(const ContainerHandleS&) = delete;

    ContainerHandleS(froaring_container_t* ptr, ContainerType type, IndexType index)
        : ptr(ptr), type(type), index(index){};
    ~ContainerHandleS() = default;

public:
    /// Pointer to the container (could be RLE, Array, Bitmap or other types)
    /// "Containers" are considered as a special type of container, which refers to a custom type of "index layer".
    /// You can use different "indexes layer" to manage real containers.
    /// Type is erased, handle carefully!
    froaring_container_t* ptr = nullptr;
    froaring::ContainerType type = ContainerType::Array;  // Type of container (RLE, Array, Bitmap, ...)
    IndexType index = 0;                                  // Index for this container
};

template <typename IndexType>
using ContainerHandle = struct ContainerHandleS<IndexType>;
}  // namespace froaring