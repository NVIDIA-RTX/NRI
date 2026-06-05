// © 2026 NVIDIA Corporation

static VkVideoCodecOperationFlagBitsKHR GetVideoCodecOperationVK(const VideoSessionDesc& videoSessionDesc) {
    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264:
                return VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
            case VideoCodec::H265:
                return VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
            case VideoCodec::AV1:
                return VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR;
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                return (VkVideoCodecOperationFlagBitsKHR)0;
        }
    } else if (videoSessionDesc.type == VideoSessionType::ENCODE) {
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264:
                return VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR;
            case VideoCodec::H265:
                return VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR;
            case VideoCodec::AV1:
                return VK_VIDEO_CODEC_OPERATION_ENCODE_AV1_BIT_KHR;
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                return (VkVideoCodecOperationFlagBitsKHR)0;
        }
    }

    return (VkVideoCodecOperationFlagBitsKHR)0;
}

static void* FillVideoProfileCodecInfoVK(const VideoSessionDesc& videoSessionDesc, void* storage) {
    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264: {
                VkVideoDecodeH264ProfileInfoKHR& info = *(VkVideoDecodeH264ProfileInfoKHR*)storage;
                info = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR};
                info.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
                info.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_KHR;
                return &info;
            }
            case VideoCodec::H265: {
                VkVideoDecodeH265ProfileInfoKHR& info = *(VkVideoDecodeH265ProfileInfoKHR*)storage;
                info = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR};
                info.stdProfileIdc = videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10 : STD_VIDEO_H265_PROFILE_IDC_MAIN;
                return &info;
            }
            case VideoCodec::AV1: {
                VkVideoDecodeAV1ProfileInfoKHR& info = *(VkVideoDecodeAV1ProfileInfoKHR*)storage;
                info = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PROFILE_INFO_KHR};
                info.stdProfile = STD_VIDEO_AV1_PROFILE_MAIN;
                info.filmGrainSupport = VK_TRUE;
                return &info;
            }
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                return nullptr;
        }
    } else if (videoSessionDesc.type == VideoSessionType::ENCODE) {
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264: {
                VkVideoEncodeH264ProfileInfoKHR& info = *(VkVideoEncodeH264ProfileInfoKHR*)storage;
                info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR};
                info.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
                return &info;
            }
            case VideoCodec::H265: {
                VkVideoEncodeH265ProfileInfoKHR& info = *(VkVideoEncodeH265ProfileInfoKHR*)storage;
                info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR};
                info.stdProfileIdc = videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10 : STD_VIDEO_H265_PROFILE_IDC_MAIN;
                return &info;
            }
            case VideoCodec::AV1: {
                VkVideoEncodeAV1ProfileInfoKHR& info = *(VkVideoEncodeAV1ProfileInfoKHR*)storage;
                info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_PROFILE_INFO_KHR};
                info.stdProfile = STD_VIDEO_AV1_PROFILE_MAIN;
                return &info;
            }
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                return nullptr;
        }
    }

    return nullptr;
}

static bool FindVideoSessionMemoryTypeVK(const DeviceVK& device, uint32_t memoryTypeBits, uint32_t& memoryTypeIndex) {
    uint32_t compatibleIndex = uint32_t(-1);
    for (uint32_t i = 0; i < 32; i++) {
        if ((memoryTypeBits & (1u << i)) == 0)
            continue;

        MemoryTypeInfo memoryTypeInfo = {};
        if (!device.GetMemoryTypeByIndex(i, memoryTypeInfo))
            continue;

        if (memoryTypeInfo.location == MemoryLocation::DEVICE) {
            memoryTypeIndex = i;
            return true;
        }

        if (compatibleIndex == uint32_t(-1))
            compatibleIndex = i;
    }

    if (compatibleIndex == uint32_t(-1))
        return false;

    memoryTypeIndex = compatibleIndex;

    return true;
}

static inline VkVideoComponentBitDepthFlagsKHR GetVideoBitDepthVK(Format format) {
    return format == Format::P010_UNORM || format == Format::P016_UNORM ? VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR : VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
}

VideoSessionVK::~VideoSessionVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_EncodeFeedbackQueryPool)
        vk.DestroyQueryPool(m_Device, m_EncodeFeedbackQueryPool, m_Device.GetVkAllocationCallbacks());

    if (m_Handle)
        vk.DestroyVideoSessionKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());

    for (VkDeviceMemory memory : m_Memory)
        vk.FreeMemory(m_Device, memory, m_Device.GetVkAllocationCallbacks());
}

Result VideoSessionVK::Create(const VideoSessionDesc& videoSessionDesc) {
    if (videoSessionDesc.width == 0 || videoSessionDesc.height == 0 || videoSessionDesc.format == Format::UNKNOWN)
        return Result::INVALID_ARGUMENT;

    VkVideoCodecOperationFlagBitsKHR operation = GetVideoCodecOperationVK(videoSessionDesc);
    if (!operation) {
        NRI_REPORT_ERROR(&m_Device, "Unsupported Vulkan video codec operation");
        return Result::UNSUPPORTED;
    }

    Queue* queue = nullptr;
    const QueueType queueType = videoSessionDesc.type == VideoSessionType::DECODE ? QueueType::VIDEO_DECODE : QueueType::VIDEO_ENCODE;
    Result result = m_Device.GetQueue(queueType, 0, queue);
    if (result != Result::SUCCESS) {
        NRI_REPORT_ERROR(&m_Device, "Failed to get Vulkan video queue for codec operation 0x%X", operation);
        return result;
    }

    alignas(8) char codecProfileStorage[64] = {};
    VkVideoProfileInfoKHR profile = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
    void* codecProfileInfo = FillVideoProfileCodecInfoVK(videoSessionDesc, codecProfileStorage);
    VkVideoDecodeUsageInfoKHR decodeUsage = {VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR};
    VkVideoEncodeUsageInfoKHR encodeUsage = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR};
    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        decodeUsage.videoUsageHints = VK_VIDEO_DECODE_USAGE_DEFAULT_KHR;
        decodeUsage.pNext = codecProfileInfo;
        profile.pNext = &decodeUsage;
    } else {
        encodeUsage.videoUsageHints = VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR;
        encodeUsage.pNext = codecProfileInfo;
        profile.pNext = &encodeUsage;
    }
    profile.videoCodecOperation = operation;
    profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
    profile.lumaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    profile.chromaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    if (!codecProfileInfo) {
        NRI_REPORT_ERROR(&m_Device, "Unsupported Vulkan video profile");
        return Result::UNSUPPORTED;
    }

    VkVideoCapabilitiesKHR capabilities = {VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR};
    VkVideoDecodeCapabilitiesKHR decodeCapabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR};
    VkVideoDecodeH264CapabilitiesKHR decodeH264Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR};
    VkVideoDecodeH265CapabilitiesKHR decodeH265Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_KHR};
    VkVideoDecodeAV1CapabilitiesKHR decodeAV1Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_CAPABILITIES_KHR};
    VkVideoEncodeCapabilitiesKHR encodeCapabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR};
    VkVideoEncodeH264CapabilitiesKHR encodeH264Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_KHR};
    VkVideoEncodeH265CapabilitiesKHR encodeH265Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_KHR};
    VkVideoEncodeAV1CapabilitiesKHR encodeAV1Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_CAPABILITIES_KHR};

    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        capabilities.pNext = &decodeCapabilities;
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264:
                decodeCapabilities.pNext = &decodeH264Capabilities;
                break;
            case VideoCodec::H265:
                decodeCapabilities.pNext = &decodeH265Capabilities;
                break;
            case VideoCodec::AV1:
                decodeCapabilities.pNext = &decodeAV1Capabilities;
                break;
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                break;
        }
    } else {
        capabilities.pNext = &encodeCapabilities;
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264:
                encodeCapabilities.pNext = &encodeH264Capabilities;
                break;
            case VideoCodec::H265:
                encodeCapabilities.pNext = &encodeH265Capabilities;
                break;
            case VideoCodec::AV1:
                encodeCapabilities.pNext = &encodeAV1Capabilities;
                break;
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                break;
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.GetPhysicalDeviceVideoCapabilitiesKHR(m_Device, &profile, &capabilities);
    if (vkResult != VK_SUCCESS) {
        NRI_REPORT_ERROR(&m_Device, "vkGetPhysicalDeviceVideoCapabilitiesKHR failed for operation 0x%X, format %u, result %d", operation, (uint32_t)videoSessionDesc.format, vkResult);
    }
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceVideoCapabilitiesKHR");
    if (videoSessionDesc.type == VideoSessionType::ENCODE)
        m_RateControlModes = GetSupportedVideoEncodeRateControlModesVK(encodeCapabilities.rateControlModes);

    if (videoSessionDesc.width < capabilities.minCodedExtent.width || videoSessionDesc.height < capabilities.minCodedExtent.height || videoSessionDesc.width > capabilities.maxCodedExtent.width
        || videoSessionDesc.height > capabilities.maxCodedExtent.height) {
        NRI_REPORT_ERROR(&m_Device, "Vulkan video coded extent %ux%u is outside supported range %ux%u..%ux%u", videoSessionDesc.width, videoSessionDesc.height, capabilities.minCodedExtent.width,
            capabilities.minCodedExtent.height, capabilities.maxCodedExtent.width, capabilities.maxCodedExtent.height);
        return Result::UNSUPPORTED;
    }
    if (!IsAligned(videoSessionDesc.width, capabilities.pictureAccessGranularity.width) || !IsAligned(videoSessionDesc.height, capabilities.pictureAccessGranularity.height)) {
        NRI_REPORT_ERROR(&m_Device, "Vulkan video coded extent %ux%u does not satisfy picture access granularity %ux%u", videoSessionDesc.width, videoSessionDesc.height,
            capabilities.pictureAccessGranularity.width, capabilities.pictureAccessGranularity.height);
        return Result::UNSUPPORTED;
    }

    m_PictureAccessGranularity = capabilities.pictureAccessGranularity;
    m_BitstreamOffsetAlignment = (uint32_t)std::max<VkDeviceSize>(capabilities.minBitstreamBufferOffsetAlignment, 1);
    m_BitstreamSizeAlignment = (uint32_t)std::max<VkDeviceSize>(capabilities.minBitstreamBufferSizeAlignment, 1);
    m_CanGenerateH264PrefixNalu = videoSessionDesc.type == VideoSessionType::ENCODE && videoSessionDesc.codec == VideoCodec::H264
        && (encodeH264Capabilities.flags & VK_VIDEO_ENCODE_H264_CAPABILITY_GENERATE_PREFIX_NALU_BIT_KHR) != 0;

    VkVideoSessionCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR};
    VkVideoEncodeH264SessionCreateInfoKHR encodeH264SessionCreateInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_CREATE_INFO_KHR};
    VkVideoEncodeH265SessionCreateInfoKHR encodeH265SessionCreateInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_CREATE_INFO_KHR};
    VkVideoEncodeAV1SessionCreateInfoKHR encodeAV1SessionCreateInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_SESSION_CREATE_INFO_KHR};
    const uint32_t maxActiveReferencePictures = std::min(videoSessionDesc.maxReferenceNum, capabilities.maxActiveReferencePictures);
    const uint32_t maxDpbSlots = videoSessionDesc.maxReferenceNum ? std::min(videoSessionDesc.maxReferenceNum + 1u, capabilities.maxDpbSlots) : 0;

    if (videoSessionDesc.type == VideoSessionType::ENCODE) {
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264:
                encodeH264SessionCreateInfo.useMaxLevelIdc = true;
                encodeH264SessionCreateInfo.maxLevelIdc = STD_VIDEO_H264_LEVEL_IDC_4_2;
                createInfo.pNext = &encodeH264SessionCreateInfo;
                break;
            case VideoCodec::H265:
                encodeH265SessionCreateInfo.useMaxLevelIdc = true;
                encodeH265SessionCreateInfo.maxLevelIdc = GetVideoH265LevelIdcVK(videoSessionDesc.width, videoSessionDesc.height);
                createInfo.pNext = &encodeH265SessionCreateInfo;
                break;
            case VideoCodec::AV1:
                encodeAV1SessionCreateInfo.useMaxLevel = true;
                encodeAV1SessionCreateInfo.maxLevel = GetVideoAV1LevelVK(videoSessionDesc.width, videoSessionDesc.height);
                createInfo.pNext = &encodeAV1SessionCreateInfo;
                break;
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                break;
        }
    }
    createInfo.queueFamilyIndex = ((QueueVK*)queue)->GetFamilyIndex();
    createInfo.pVideoProfile = &profile;
    createInfo.pictureFormat = GetVkFormat(videoSessionDesc.format);
    createInfo.maxCodedExtent = videoSessionDesc.type == VideoSessionType::DECODE ? capabilities.maxCodedExtent : VkExtent2D{videoSessionDesc.width, videoSessionDesc.height};
    createInfo.referencePictureFormat = maxDpbSlots ? createInfo.pictureFormat : VK_FORMAT_UNDEFINED;
    createInfo.maxDpbSlots = maxDpbSlots;
    createInfo.maxActiveReferencePictures = maxActiveReferencePictures;
    createInfo.pStdHeaderVersion = &capabilities.stdHeaderVersion;
#if defined(VK_VIDEO_SESSION_CREATE_INLINE_SESSION_PARAMETERS_BIT_KHR)
    if (videoSessionDesc.type == VideoSessionType::DECODE && videoSessionDesc.codec == VideoCodec::AV1 && m_Device.m_IsSupported.videoMaintenance2) {
        createInfo.flags |= VK_VIDEO_SESSION_CREATE_INLINE_SESSION_PARAMETERS_BIT_KHR;
        m_UseInlineSessionParameters = true;
    }
#endif
    vkResult = vk.CreateVideoSessionKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    if (vkResult != VK_SUCCESS) {
        NRI_REPORT_ERROR(&m_Device, "vkCreateVideoSessionKHR failed for queue family %u, operation 0x%X, format %u, result %d", createInfo.queueFamilyIndex, operation, (uint32_t)videoSessionDesc.format, vkResult);
    }
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateVideoSessionKHR");

    const VkVideoEncodeFeedbackFlagsKHR requiredFeedbackFlags = VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR;
    if (videoSessionDesc.type == VideoSessionType::ENCODE && (encodeCapabilities.supportedEncodeFeedbackFlags & requiredFeedbackFlags) == requiredFeedbackFlags) {
        VkQueryPoolVideoEncodeFeedbackCreateInfoKHR feedbackInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_VIDEO_ENCODE_FEEDBACK_CREATE_INFO_KHR};
        feedbackInfo.pNext = &profile;
        feedbackInfo.encodeFeedbackFlags = requiredFeedbackFlags;

        VkQueryPoolCreateInfo queryPoolInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        queryPoolInfo.pNext = &feedbackInfo;
        queryPoolInfo.queryType = VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR;
        queryPoolInfo.queryCount = VideoSessionVK::ENCODE_FEEDBACK_QUERY_NUM;

        vkResult = vk.CreateQueryPool(m_Device, &queryPoolInfo, m_Device.GetVkAllocationCallbacks(), &m_EncodeFeedbackQueryPool);
        if (vkResult != VK_SUCCESS)
            NRI_REPORT_WARNING(&m_Device, "vkCreateQueryPool failed for video encode feedback, result %d. Resolved encode metadata will be unavailable.", vkResult);
    }

    uint32_t memoryRequirementNum = 0;
    vkResult = vk.GetVideoSessionMemoryRequirementsKHR(m_Device, m_Handle, &memoryRequirementNum, nullptr);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetVideoSessionMemoryRequirementsKHR");

    Scratch<VkVideoSessionMemoryRequirementsKHR> memoryRequirements = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoSessionMemoryRequirementsKHR, memoryRequirementNum);
    for (uint32_t i = 0; i < memoryRequirementNum; i++)
        memoryRequirements[i] = {VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR};

    vkResult = vk.GetVideoSessionMemoryRequirementsKHR(m_Device, m_Handle, &memoryRequirementNum, memoryRequirements);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetVideoSessionMemoryRequirementsKHR");

    m_Memory.resize(memoryRequirementNum);
    Scratch<VkBindVideoSessionMemoryInfoKHR> bindInfos = NRI_ALLOCATE_SCRATCH(m_Device, VkBindVideoSessionMemoryInfoKHR, memoryRequirementNum);
    for (uint32_t i = 0; i < memoryRequirementNum; i++) {
        uint32_t memoryTypeIndex = 0;
        if (!FindVideoSessionMemoryTypeVK(m_Device, memoryRequirements[i].memoryRequirements.memoryTypeBits, memoryTypeIndex))
            return Result::UNSUPPORTED;

        VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocateInfo.allocationSize = memoryRequirements[i].memoryRequirements.size;
        allocateInfo.memoryTypeIndex = memoryTypeIndex;

        vkResult = vk.AllocateMemory(m_Device, &allocateInfo, m_Device.GetVkAllocationCallbacks(), &m_Memory[i]);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateMemory");

        bindInfos[i] = {VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR};
        bindInfos[i].memoryBindIndex = memoryRequirements[i].memoryBindIndex;
        bindInfos[i].memory = m_Memory[i];
        bindInfos[i].memorySize = memoryRequirements[i].memoryRequirements.size;
    }

    vkResult = vk.BindVideoSessionMemoryKHR(m_Device, m_Handle, memoryRequirementNum, bindInfos);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkBindVideoSessionMemoryKHR");

    m_Desc = videoSessionDesc;
    m_Desc.maxReferenceNum = maxDpbSlots ? maxDpbSlots - 1 : 0;
    return Result::SUCCESS;
}

static Result NRI_CALL GetVideoCapabilities(const Device& device, const VideoSessionDesc& videoSessionDesc, VideoCapabilities& videoCapabilities) {
    if (videoSessionDesc.width == 0 || videoSessionDesc.height == 0 || videoSessionDesc.format == Format::UNKNOWN)
        return Result::INVALID_ARGUMENT;

    const VkVideoCodecOperationFlagBitsKHR operation = GetVideoCodecOperationVK(videoSessionDesc);
    if (!operation)
        return Result::UNSUPPORTED;

    alignas(8) char codecProfileStorage[64] = {};
    VkVideoProfileInfoKHR profile = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
    void* codecProfileInfo = FillVideoProfileCodecInfoVK(videoSessionDesc, codecProfileStorage);
    VkVideoDecodeUsageInfoKHR decodeUsage = {VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR};
    VkVideoEncodeUsageInfoKHR encodeUsage = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR};
    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        decodeUsage.videoUsageHints = VK_VIDEO_DECODE_USAGE_DEFAULT_KHR;
        decodeUsage.pNext = codecProfileInfo;
        profile.pNext = &decodeUsage;
    } else {
        encodeUsage.videoUsageHints = VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR;
        encodeUsage.pNext = codecProfileInfo;
        profile.pNext = &encodeUsage;
    }
    profile.videoCodecOperation = operation;
    profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
    profile.lumaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    profile.chromaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    if (!codecProfileInfo)
        return Result::UNSUPPORTED;

    VkVideoCapabilitiesKHR capabilities = {VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR};
    VkVideoDecodeCapabilitiesKHR decodeCapabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR};
    VkVideoDecodeH264CapabilitiesKHR decodeH264Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR};
    VkVideoDecodeH265CapabilitiesKHR decodeH265Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_KHR};
    VkVideoDecodeAV1CapabilitiesKHR decodeAV1Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_CAPABILITIES_KHR};
    VkVideoEncodeCapabilitiesKHR encodeCapabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR};
    VkVideoEncodeH264CapabilitiesKHR encodeH264Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_KHR};
    VkVideoEncodeH265CapabilitiesKHR encodeH265Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_KHR};
    VkVideoEncodeAV1CapabilitiesKHR encodeAV1Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_CAPABILITIES_KHR};
    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        capabilities.pNext = &decodeCapabilities;
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264:
                decodeCapabilities.pNext = &decodeH264Capabilities;
                break;
            case VideoCodec::H265:
                decodeCapabilities.pNext = &decodeH265Capabilities;
                break;
            case VideoCodec::AV1:
                decodeCapabilities.pNext = &decodeAV1Capabilities;
                break;
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                break;
        }
    } else {
        capabilities.pNext = &encodeCapabilities;
        switch (videoSessionDesc.codec) {
            case VideoCodec::H264:
                encodeCapabilities.pNext = &encodeH264Capabilities;
                break;
            case VideoCodec::H265:
                encodeCapabilities.pNext = &encodeH265Capabilities;
                break;
            case VideoCodec::AV1:
                encodeCapabilities.pNext = &encodeAV1Capabilities;
                break;
            case VideoCodec::NONE:
            case VideoCodec::MAX_NUM:
                break;
        }
    }

    DeviceVK& deviceVK = (DeviceVK&)device;
    const auto& vk = deviceVK.GetDispatchTable();

    VkResult vkResult = vk.GetPhysicalDeviceVideoCapabilitiesKHR(deviceVK, &profile, &capabilities);
    NRI_RETURN_ON_BAD_VKRESULT(&deviceVK, vkResult, "vkGetPhysicalDeviceVideoCapabilitiesKHR");

    FillVideoCapabilitiesVK(videoCapabilities, capabilities);

    const bool isExtentSupported = videoSessionDesc.width >= videoCapabilities.widthMin
        && videoSessionDesc.height >= videoCapabilities.heightMin
        && videoSessionDesc.width <= videoCapabilities.widthMax
        && videoSessionDesc.height <= videoCapabilities.heightMax;
    const bool isGranularitySupported = IsAligned(videoSessionDesc.width, videoCapabilities.pictureAccessGranularityWidth) && IsAligned(videoSessionDesc.height, videoCapabilities.pictureAccessGranularityHeight);

    return isExtentSupported && isGranularitySupported ? Result::SUCCESS : Result::UNSUPPORTED;
}
