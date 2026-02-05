// © 2021 NVIDIA Corporation

#pragma once

#include <vulkan/vulkan.h>
#undef CreateSemaphore

#include "DispatchTable.h"
#include "SharedExternal.h"

typedef uint16_t MemoryTypeIndex;

#define PNEXTCHAIN_DECLARE(next) \
    const void** _tail = (const void**)&next

#define PNEXTCHAIN_SET(next) \
    _tail = (const void**)&next

// Requires {}
#define PNEXTCHAIN_APPEND_STRUCT(desc) \
    *_tail = &desc; \
    _tail = (const void**)&desc.pNext

#define PNEXTCHAIN_APPEND_FEATURES(condition, ext, nameLower, nameUpper) \
    VkPhysicalDevice##nameLower##Features##ext nameLower##Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##nameUpper##_FEATURES_##ext}; \
    if (IsExtensionSupported(VK_##ext##_##nameUpper##_EXTENSION_NAME, desiredDeviceExts) && (condition)) { \
        PNEXTCHAIN_APPEND_STRUCT(nameLower##Features); \
    }

#define PNEXTCHAIN_APPEND_PROPS(condition, ext, nameLower, nameUpper) \
    VkPhysicalDevice##nameLower##Properties##ext nameLower##Props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##nameUpper##_PROPERTIES_##ext}; \
    if (IsExtensionSupported(VK_##ext##_##nameUpper##_EXTENSION_NAME, desiredDeviceExts) && (condition)) { \
        PNEXTCHAIN_APPEND_STRUCT(nameLower##Props); \
    }

#define APPEND_EXT(condition, ext) \
    if (IsExtensionSupported(ext, supportedExts) && (condition)) \
        desiredDeviceExts.push_back(ext)

namespace nri {

constexpr uint32_t INVALID_FAMILY_INDEX = uint32_t(-1);

struct MemoryTypeInfo {
    MemoryTypeIndex index;
    MemoryLocation location;
    bool mustBeDedicated;
};

inline MemoryType Pack(const MemoryTypeInfo& memoryTypeInfo) {
    return *(MemoryType*)&memoryTypeInfo;
}

inline MemoryTypeInfo Unpack(const MemoryType& memoryType) {
    return *(MemoryTypeInfo*)&memoryType;
}

static_assert(sizeof(MemoryTypeInfo) == sizeof(MemoryType), "Must be equal");

inline bool IsHostVisibleMemory(MemoryLocation location) {
    return location > MemoryLocation::DEVICE;
}

inline bool IsHostMemory(MemoryLocation location) {
    return location > MemoryLocation::DEVICE_UPLOAD;
}

} // namespace nri

#include "DeviceVK.h"
