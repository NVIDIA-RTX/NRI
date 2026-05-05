// © 2021 NVIDIA Corporation

#pragma once

#include <vulkan/vulkan.h>
#undef CreateSemaphore

#include "DispatchTable.h"
#include "QueueSelectionVK.h"
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

struct VideoResourceProfileListVK {
    VkVideoDecodeH264ProfileInfoKHR decodeH264 = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR};
    VkVideoDecodeH265ProfileInfoKHR decodeH265 = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR};
    VkVideoDecodeAV1ProfileInfoKHR decodeAV1 = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PROFILE_INFO_KHR};
    std::array<VkVideoDecodeUsageInfoKHR, 3> decodeUsage = {};
    VkVideoEncodeH264ProfileInfoKHR encodeH264 = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR};
    VkVideoEncodeH265ProfileInfoKHR encodeH265 = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR};
    VkVideoEncodeAV1ProfileInfoKHR encodeAV1 = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_PROFILE_INFO_KHR};
    std::array<VkVideoEncodeUsageInfoKHR, 3> encodeUsage = {};
    std::array<VkVideoProfileInfoKHR, 6> profiles = {};
    VkVideoProfileListInfoKHR list = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR};

    bool AppendProfile(VkVideoCodecOperationFlagsKHR codecOperations, VkVideoCodecOperationFlagBitsKHR operation, void* codecInfo, VkVideoComponentBitDepthFlagsKHR bitDepth) {
        if ((codecOperations & operation) == 0)
            return false;

        VkVideoProfileInfoKHR& profile = profiles[list.profileCount++];
        profile = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
        profile.pNext = codecInfo;
        profile.videoCodecOperation = operation;
        profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
        profile.lumaBitDepth = bitDepth;
        profile.chromaBitDepth = bitDepth;

        return true;
    }

    void Fill(bool decode, bool encode, Format format, VideoCodec codec, VkVideoCodecOperationFlagsKHR codecOperations) {
        list.profileCount = 0;
        list.pProfiles = profiles.data();

        const VkVideoComponentBitDepthFlagsKHR bitDepth = format == Format::P010_UNORM || format == Format::P016_UNORM ? VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR : VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;

        if (decode) {
            decodeH264.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
            decodeH264.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_KHR;
            decodeH265.stdProfileIdc = bitDepth == VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10 : STD_VIDEO_H265_PROFILE_IDC_MAIN;
            decodeAV1.stdProfile = STD_VIDEO_AV1_PROFILE_MAIN;
            decodeAV1.filmGrainSupport = VK_TRUE;

            decodeUsage[0] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR};
            decodeUsage[0].videoUsageHints = VK_VIDEO_DECODE_USAGE_DEFAULT_KHR;
            decodeUsage[0].pNext = &decodeH264;

            decodeUsage[1] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR};
            decodeUsage[1].videoUsageHints = VK_VIDEO_DECODE_USAGE_DEFAULT_KHR;
            decodeUsage[1].pNext = &decodeH265;

            decodeUsage[2] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR};
            decodeUsage[2].videoUsageHints = VK_VIDEO_DECODE_USAGE_DEFAULT_KHR;
            decodeUsage[2].pNext = &decodeAV1;

            if (codec == VideoCodec::H264)
                AppendProfile(codecOperations, VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR, &decodeUsage[0], bitDepth);
            else if (codec == VideoCodec::H265)
                AppendProfile(codecOperations, VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR, &decodeUsage[1], bitDepth);
            else if (codec == VideoCodec::AV1) {
                AppendProfile(codecOperations, VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR, &decodeUsage[2], bitDepth);
            }
        }

        if (encode) {
            encodeH264.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
            encodeH265.stdProfileIdc = bitDepth == VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10 : STD_VIDEO_H265_PROFILE_IDC_MAIN;
            encodeAV1.stdProfile = STD_VIDEO_AV1_PROFILE_MAIN;

            encodeUsage[0] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR};
            encodeUsage[0].videoUsageHints = VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR;
            encodeUsage[0].pNext = &encodeH264;

            encodeUsage[1] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR};
            encodeUsage[1].videoUsageHints = VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR;
            encodeUsage[1].pNext = &encodeH265;

            encodeUsage[2] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR};
            encodeUsage[2].videoUsageHints = VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR;
            encodeUsage[2].pNext = &encodeAV1;

            if (codec == VideoCodec::H264)
                AppendProfile(codecOperations, VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR, &encodeUsage[0], bitDepth);
            else if (codec == VideoCodec::H265)
                AppendProfile(codecOperations, VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR, &encodeUsage[1], bitDepth);
            else if (codec == VideoCodec::AV1) {
                AppendProfile(codecOperations, VK_VIDEO_CODEC_OPERATION_ENCODE_AV1_BIT_KHR, &encodeUsage[2], bitDepth);
            }
        }
    }
};

} // namespace nri

#include "DeviceVK.h"
