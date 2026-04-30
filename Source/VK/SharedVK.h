// © 2021 NVIDIA Corporation

#pragma once

#include <vulkan/vulkan.h>
#undef CreateSemaphore

#include "DispatchTable.h"
#include "SharedExternal.h"
#include "QueueSelectionVK.h"

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

struct VideoResourceProfileListVK {
    VkVideoDecodeH264ProfileInfoKHR decodeH264 = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR};
    VkVideoDecodeUsageInfoKHR decodeUsage = {VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR};
    VkVideoEncodeH264ProfileInfoKHR encodeH264 = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR};
    VkVideoEncodeUsageInfoKHR encodeUsage = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR};
    VkVideoProfileInfoKHR profiles[2] = {};
    VkVideoProfileListInfoKHR list = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR};

    void Fill(bool decode, bool encode, Format format) {
        list.profileCount = 0;
        list.pProfiles = profiles;

        const VkVideoComponentBitDepthFlagsKHR bitDepth = format == Format::P010_UNORM || format == Format::P016_UNORM ? VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR : VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;

        if (decode) {
            decodeH264.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
            decodeH264.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_KHR;
            decodeUsage.videoUsageHints = VK_VIDEO_DECODE_USAGE_DEFAULT_KHR;
            decodeUsage.pNext = &decodeH264;

            VkVideoProfileInfoKHR& profile = profiles[list.profileCount++];
            profile = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
            profile.pNext = &decodeUsage;
            profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
            profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
            profile.lumaBitDepth = bitDepth;
            profile.chromaBitDepth = bitDepth;
        }

        if (encode) {
            encodeH264.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
            encodeUsage.videoUsageHints = VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR;
            encodeUsage.pNext = &encodeH264;

            VkVideoProfileInfoKHR& profile = profiles[list.profileCount++];
            profile = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
            profile.pNext = &encodeUsage;
            profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR;
            profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
            profile.lumaBitDepth = bitDepth;
            profile.chromaBitDepth = bitDepth;
        }
    }
};

} // namespace nri

#include "DeviceVK.h"
