// © 2021 NVIDIA Corporation

#pragma once

#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>

#include "../Shared/SharedExternal.h"

namespace nri {

constexpr uint32_t INVALID_QUEUE_FAMILY_INDEX_VK = uint32_t(-1);
constexpr VkVideoCodecOperationFlagsKHR VIDEO_DECODE_CODEC_OPERATION_MASK = 0x0000FFFF;
constexpr VkVideoCodecOperationFlagsKHR VIDEO_ENCODE_CODEC_OPERATION_MASK = 0xFFFF0000;
constexpr std::array<VkVideoCodecOperationFlagBitsKHR, 3> g_VideoDecodeCodecOperations = {
    VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR,
    VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR,
    VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR,
};
constexpr std::array<VkVideoCodecOperationFlagBitsKHR, 3> g_VideoEncodeCodecOperations = {
    VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
    VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR,
    VK_VIDEO_CODEC_OPERATION_ENCODE_AV1_BIT_KHR,
};

struct QueueFamilyPropsVK {
    VkQueueFlags queueFlags;
    uint32_t queueCount;
    VkVideoCodecOperationFlagsKHR videoCodecOperations;
};

inline bool HasVideoDecodeCodec(const QueueFamilyPropsVK& familyProps) {
    return (familyProps.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) != 0 && ((familyProps.videoCodecOperations & VIDEO_DECODE_CODEC_OPERATION_MASK) != 0 || familyProps.videoCodecOperations == 0);
}

inline bool HasVideoEncodeCodec(const QueueFamilyPropsVK& familyProps) {
    return (familyProps.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) != 0 && ((familyProps.videoCodecOperations & VIDEO_ENCODE_CODEC_OPERATION_MASK) != 0 || familyProps.videoCodecOperations == 0);
}

inline const std::array<VkVideoCodecOperationFlagBitsKHR, 3>& GetVideoCodecOperationsVK(QueueType queueType) {
    return queueType == QueueType::VIDEO_DECODE ? g_VideoDecodeCodecOperations : g_VideoEncodeCodecOperations;
}

inline uint32_t GetSupportedVideoCodecNumVK(const QueueFamilyPropsVK& familyProps, QueueType queueType) {
    const VkVideoCodecOperationFlagsKHR mask = queueType == QueueType::VIDEO_DECODE ? VIDEO_DECODE_CODEC_OPERATION_MASK : VIDEO_ENCODE_CODEC_OPERATION_MASK;
    if (familyProps.videoCodecOperations == 0)
        return 3;

    uint32_t num = 0;
    for (VkVideoCodecOperationFlagBitsKHR operation : GetVideoCodecOperationsVK(queueType))
        num += (familyProps.videoCodecOperations & operation) ? 1 : 0;

    return num && (familyProps.videoCodecOperations & mask) ? num : 0;
}

// Prefer families supporting more codecs. Among ties, prefer more queues and then fewer unrelated capabilities
// so video work still lands on dedicated queues when available.
inline uint32_t GetVideoQueueFamilyScoreVK(const QueueFamilyPropsVK& familyProps, QueueType queueType) {
    const bool videoDecode = queueType == QueueType::VIDEO_DECODE && HasVideoDecodeCodec(familyProps);
    const bool videoEncode = queueType == QueueType::VIDEO_ENCODE && HasVideoEncodeCodec(familyProps);
    if (!videoDecode && !videoEncode)
        return 0;

    const bool graphics = (familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    const bool compute = (familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
    const bool copy = (familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
    const bool decode = (familyProps.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) != 0;
    const bool encode = (familyProps.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) != 0;
    const bool alsoOppositeVideo = queueType == QueueType::VIDEO_DECODE ? encode : decode;

    uint32_t score = 1000;
    score += GetSupportedVideoCodecNumVK(familyProps, queueType) * 1000;
    score += std::min(familyProps.queueCount, 8u) * 10;
    score += graphics ? 0 : 100;
    score += compute ? 0 : 50;
    score += copy ? 0 : 25;
    score += alsoOppositeVideo ? 0 : 10;

    return score;
}

inline uint32_t GetQueueFamilyScoreVK(const QueueFamilyPropsVK& familyProps, QueueType queueType) {
    bool graphics = (familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    bool compute = (familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
    bool copy = (familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
    bool sparse = (familyProps.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0;
    bool videoDecode = HasVideoDecodeCodec(familyProps);
    bool videoEncode = HasVideoEncodeCodec(familyProps);
    bool protect = (familyProps.queueFlags & VK_QUEUE_PROTECTED_BIT) != 0;
    bool opticalFlow = (familyProps.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) != 0;

    switch (queueType) {
        case QueueType::GRAPHICS:
            return graphics ? GRAPHICS_QUEUE_SCORE : 0;
        case QueueType::COMPUTE:
            return compute ? COMPUTE_QUEUE_SCORE : 0;
        case QueueType::COPY:
            return copy ? COPY_QUEUE_SCORE : 0;
        case QueueType::VIDEO_DECODE:
            return videoDecode ? GetVideoQueueFamilyScoreVK(familyProps, QueueType::VIDEO_DECODE) : 0;
        case QueueType::VIDEO_ENCODE:
            return videoEncode ? GetVideoQueueFamilyScoreVK(familyProps, QueueType::VIDEO_ENCODE) : 0;
        default:
            return 0;
    }
}

inline void SelectQueueFamiliesVK(const QueueFamilyPropsVK* familyProps, uint32_t familyNum, std::array<uint32_t, (size_t)QueueType::MAX_NUM>& queueFamilyIndices, std::array<uint32_t, (size_t)QueueType::MAX_NUM>* queueNums = nullptr) {
    std::array<uint32_t, (size_t)QueueType::MAX_NUM> scores = {};
    queueFamilyIndices.fill(INVALID_QUEUE_FAMILY_INDEX_VK);

    if (queueNums)
        queueNums->fill(0);

    for (uint32_t familyIndex = 0; familyIndex < familyNum; familyIndex++) {
        for (uint32_t queueTypeIndex = 0; queueTypeIndex < (uint32_t)QueueType::MAX_NUM; queueTypeIndex++) {
            QueueType queueType = (QueueType)queueTypeIndex;
            uint32_t score = GetQueueFamilyScoreVK(familyProps[familyIndex], queueType);

            if (score > scores[queueTypeIndex]) {
                queueFamilyIndices[queueTypeIndex] = familyIndex;
                scores[queueTypeIndex] = score;

                if (queueNums)
                    (*queueNums)[queueTypeIndex] = familyProps[familyIndex].queueCount;
            }
        }
    }
}

} // namespace nri
