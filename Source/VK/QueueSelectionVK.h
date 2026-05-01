// © 2021 NVIDIA Corporation

#pragma once

#include <algorithm>
#include <array>
#include <vulkan/vulkan.h>

#include "../Shared/SharedExternal.h"

namespace nri {

constexpr uint32_t INVALID_QUEUE_FAMILY_INDEX_VK = uint32_t(-1);
constexpr VkVideoCodecOperationFlagsKHR VIDEO_DECODE_CODEC_OPERATION_MASK = 0x0000FFFF;
constexpr VkVideoCodecOperationFlagsKHR VIDEO_ENCODE_CODEC_OPERATION_MASK = 0xFFFF0000;

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

inline bool HasVideoCodecOperation(const QueueFamilyPropsVK& familyProps, VkVideoCodecOperationFlagBitsKHR operation) {
    if (operation == 0)
        return false;

    const VkQueueFlags requiredQueueFlag = (operation & VIDEO_DECODE_CODEC_OPERATION_MASK) != 0 ? VK_QUEUE_VIDEO_DECODE_BIT_KHR : VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
    return (familyProps.queueFlags & requiredQueueFlag) != 0 && ((familyProps.videoCodecOperations & operation) != 0 || familyProps.videoCodecOperations == 0);
}

// Video queues are selected for the exact codec operation first. Among matching families, prefer
// families with fewer unrelated capabilities so video work lands on dedicated queues when available.
inline uint32_t GetVideoQueueFamilyScoreVK(const QueueFamilyPropsVK& familyProps, VkVideoCodecOperationFlagBitsKHR operation) {
    if (!HasVideoCodecOperation(familyProps, operation))
        return 0;

    const bool graphics = (familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    const bool compute = (familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
    const bool copy = (familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
    const bool decode = (familyProps.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) != 0;
    const bool encode = (familyProps.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) != 0;
    const bool sameDirectionDecode = (operation & VIDEO_DECODE_CODEC_OPERATION_MASK) != 0;
    const bool alsoOppositeVideo = sameDirectionDecode ? encode : decode;

    uint32_t score = 1000;
    score += graphics ? 0 : 100;
    score += compute ? 0 : 50;
    score += copy ? 0 : 25;
    score += alsoOppositeVideo ? 0 : 10;
    score += std::min(familyProps.queueCount, 8u);

    return score;
}

inline uint32_t SelectVideoQueueFamilyVK(const QueueFamilyPropsVK* familyProps, uint32_t familyNum, VkVideoCodecOperationFlagBitsKHR operation) {
    uint32_t bestFamilyIndex = INVALID_QUEUE_FAMILY_INDEX_VK;
    uint32_t bestScore = 0;

    for (uint32_t familyIndex = 0; familyIndex < familyNum; familyIndex++) {
        const uint32_t score = GetVideoQueueFamilyScoreVK(familyProps[familyIndex], operation);
        if (score > bestScore) {
            bestFamilyIndex = familyIndex;
            bestScore = score;
        }
    }

    return bestFamilyIndex;
}

inline VkVideoCodecOperationFlagBitsKHR GetRepresentativeVideoCodecOperationVK(const QueueFamilyPropsVK& familyProps, QueueType queueType) {
    const std::array<VkVideoCodecOperationFlagBitsKHR, 3> decodeOperations = {
        VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR,
        VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR,
        VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR,
    };
    const std::array<VkVideoCodecOperationFlagBitsKHR, 3> encodeOperations = {
        VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
        VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR,
        VK_VIDEO_CODEC_OPERATION_ENCODE_AV1_BIT_KHR,
    };

    const auto& operations = queueType == QueueType::VIDEO_DECODE ? decodeOperations : encodeOperations;
    for (VkVideoCodecOperationFlagBitsKHR operation : operations) {
        if (HasVideoCodecOperation(familyProps, operation))
            return operation;
    }

    return (VkVideoCodecOperationFlagBitsKHR)0;
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
    case QueueType::VIDEO_DECODE: {
        const VkVideoCodecOperationFlagBitsKHR operation = GetRepresentativeVideoCodecOperationVK(familyProps, QueueType::VIDEO_DECODE);
        return videoDecode ? GetVideoQueueFamilyScoreVK(familyProps, operation) : 0;
    }
    case QueueType::VIDEO_ENCODE: {
        const VkVideoCodecOperationFlagBitsKHR operation = GetRepresentativeVideoCodecOperationVK(familyProps, QueueType::VIDEO_ENCODE);
        return videoEncode ? GetVideoQueueFamilyScoreVK(familyProps, operation) : 0;
    }
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
