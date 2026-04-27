// © 2021 NVIDIA Corporation

#pragma once

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
    return (familyProps.videoCodecOperations & VIDEO_DECODE_CODEC_OPERATION_MASK) != 0;
}

inline bool HasVideoEncodeCodec(const QueueFamilyPropsVK& familyProps) {
    return (familyProps.videoCodecOperations & VIDEO_ENCODE_CODEC_OPERATION_MASK) != 0;
}

inline uint32_t GetQueueFamilyScoreVK(const QueueFamilyPropsVK& familyProps, QueueType queueType) {
    bool graphics = (familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    bool compute = (familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
    bool copy = (familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
    bool sparse = (familyProps.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0;
    bool videoDecode = (familyProps.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) != 0 && HasVideoDecodeCodec(familyProps);
    bool videoEncode = (familyProps.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) != 0 && HasVideoEncodeCodec(familyProps);
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
        return videoDecode ? VIDEO_DECODE_QUEUE_SCORE : 0;
    case QueueType::VIDEO_ENCODE:
        return videoEncode ? VIDEO_ENCODE_QUEUE_SCORE : 0;
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
