// © 2021 NVIDIA Corporation

#pragma once

#include <vulkan/vulkan.h>
#undef CreateSemaphore

#include "DispatchTable.h"
#include "SharedExternal.h"

typedef uint16_t MemoryTypeIndex;

struct MemoryTypeInfo {
    MemoryTypeIndex index;
    nri::MemoryLocation location;
    bool mustBeDedicated;
};

inline nri::MemoryType Pack(const MemoryTypeInfo& memoryTypeInfo) {
    return *(nri::MemoryType*)&memoryTypeInfo;
}

inline MemoryTypeInfo Unpack(const nri::MemoryType& memoryType) {
    return *(MemoryTypeInfo*)&memoryType;
}

static_assert(sizeof(MemoryTypeInfo) == sizeof(nri::MemoryType), "Must be equal");

inline bool IsHostVisibleMemory(nri::MemoryLocation location) {
    return location > nri::MemoryLocation::DEVICE;
}

inline bool IsHostMemory(nri::MemoryLocation location) {
    return location > nri::MemoryLocation::DEVICE_UPLOAD;
}

constexpr uint32_t INVALID_FAMILY_INDEX = uint32_t(-1);

#if 1
#    define IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
#    define IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
#else
#    define IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL VK_IMAGE_LAYOUT_GENERAL
#    define IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL VK_IMAGE_LAYOUT_GENERAL
#endif

// Requires {}
#define APPEND_EXT(desc) \
    *tail = &desc; \
    tail = &desc.pNext

struct VmaAllocator_T;
struct VmaAllocation_T;

#include "DeviceVK.h"
