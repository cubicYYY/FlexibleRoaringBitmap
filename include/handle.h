#pragma once

#include "prelude.h"
#include "utils.h"

namespace froaring {
/// @brief to hold container data: pointer, type, and index
/// No copying is allowed: a handle is a unique pointer in essence.
template <typename IndexType>
struct ContainerHandle {
public:
    ContainerHandle() = default;
    ContainerHandle(ContainerHandle&&) = default;
    ContainerHandle& operator=(ContainerHandle&&) = default;

    ContainerHandle(const ContainerHandle&) = delete;
    ContainerHandle& operator=(const ContainerHandle&) = delete;

    ContainerHandle(froaring_container_t* ptr, ContainerType type, IndexType index)
        : ptr(ptr), type(type), index(index){};
    ~ContainerHandle() = default;

public:
    /// Pointer to the container (could be RLE, Array, or Bitmap)
    /// Type is erased, handle carefully!
    froaring_container_t* ptr = nullptr;
    froaring::ContainerType type = ContainerType::Array;  // Type of container (RLE, Array, Bitmap, ...)
    IndexType index = 0;                                  // Index for this container
};

template <typename IndexType>
using ContainerHandle = struct ContainerHandle;
}  // namespace froaring