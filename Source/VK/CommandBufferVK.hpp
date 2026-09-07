// © 2021 NVIDIA Corporation

#include <math.h>

static inline VkPipelineBindPoint GetPipelineBindPoint(BindPoint bindPoint) {
    switch (bindPoint) {
        case BindPoint::COMPUTE:
            return VK_PIPELINE_BIND_POINT_COMPUTE;
        case BindPoint::RAY_TRACING:
            return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        default:
            return VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
}

static inline bool RangesOverlap(const InputAttachmentRange& a, const VkImageSubresourceRange& b) {
    uint32_t mipOffset = a.mipOffset;
    uint32_t mipNum = a.mipNum;
    uint32_t layerOffset = a.layerOffset;
    uint32_t layerNum = a.layerNum;

    return (a.aspects & b.aspectMask)
        && mipOffset < b.baseMipLevel + b.levelCount
        && b.baseMipLevel < mipOffset + mipNum
        && layerOffset < b.baseArrayLayer + b.layerCount
        && b.baseArrayLayer < layerOffset + layerNum;
}

static inline VkImageAspectFlags GetLowestBit(VkImageAspectFlags x) {
    return x & (~x + 1);
}

static inline bool ContainsRange(const InputAttachmentRange& a, VkImage image, VkImageAspectFlags aspects, uint32_t mipOffset, uint32_t mipNum, uint32_t layerOffset, uint32_t layerNum) {
    uint32_t aMipOffset = a.mipOffset;
    uint32_t aMipNum = a.mipNum;
    uint32_t aLayerOffset = a.layerOffset;
    uint32_t aLayerNum = a.layerNum;

    return a.image == image
        && (a.aspects & aspects) == aspects
        && aMipOffset <= mipOffset
        && mipOffset + mipNum <= aMipOffset + aMipNum
        && aLayerOffset <= layerOffset
        && layerOffset + layerNum <= aLayerOffset + aLayerNum;
}

static inline bool TryMergeInputAttachmentRanges(InputAttachmentRange& a, const InputAttachmentRange& b) {
    if (a.image != b.image)
        return false;

    if (a.mipOffset == b.mipOffset && a.mipNum == b.mipNum && a.layerOffset == b.layerOffset && a.layerNum == b.layerNum) {
        a.aspects |= b.aspects;
        return true;
    }

    if (a.aspects != b.aspects)
        return false;

    if (ContainsRange(a, b.image, b.aspects, b.mipOffset, b.mipNum, b.layerOffset, b.layerNum))
        return true;

    if (ContainsRange(b, a.image, a.aspects, a.mipOffset, a.mipNum, a.layerOffset, a.layerNum)) {
        a = b;
        return true;
    }

    uint32_t aMipOffset = a.mipOffset;
    uint32_t aMipEnd = aMipOffset + a.mipNum;
    uint32_t bMipOffset = b.mipOffset;
    uint32_t bMipEnd = bMipOffset + b.mipNum;
    uint32_t aLayerOffset = a.layerOffset;
    uint32_t aLayerEnd = aLayerOffset + a.layerNum;
    uint32_t bLayerOffset = b.layerOffset;
    uint32_t bLayerEnd = bLayerOffset + b.layerNum;

    if (aLayerOffset == bLayerOffset && aLayerEnd == bLayerEnd && aMipOffset <= bMipEnd && bMipOffset <= aMipEnd) {
        uint32_t mipOffset = std::min(aMipOffset, bMipOffset);
        a.mipOffset = (Dim_t)mipOffset;
        a.mipNum = (Dim_t)(std::max(aMipEnd, bMipEnd) - mipOffset);
        return true;
    }

    if (aMipOffset == bMipOffset && aMipEnd == bMipEnd && aLayerOffset <= bLayerEnd && bLayerOffset <= aLayerEnd) {
        uint32_t layerOffset = std::min(aLayerOffset, bLayerOffset);
        a.layerOffset = (Dim_t)layerOffset;
        a.layerNum = (Dim_t)(std::max(aLayerEnd, bLayerEnd) - layerOffset);
        return true;
    }

    return false;
}

static inline void NormalizeInputAttachmentRanges(Vector<InputAttachmentRange>& ranges) {
    if (ranges.size() < 2)
        return;

    // This is O(n^2) and can repeat until stable, but it's a win on average
    bool hasMerged = false;
    do {
        hasMerged = false;

        for (size_t i = 0; i < ranges.size(); i++) {
            for (size_t j = i + 1; j < ranges.size();) {
                if (TryMergeInputAttachmentRanges(ranges[i], ranges[j])) {
                    ranges[j] = ranges.back();
                    ranges.pop_back();
                    hasMerged = true;
                } else
                    j++;
            }
        }
    } while (hasMerged);
}

static inline VkImageSubresourceRange GetSubresourceRange(const TextureVK& textureVK, const VkImageSubresourceRange& range) {
    const TextureDesc& textureDesc = textureVK.GetDesc();

    VkImageSubresourceRange out = range;
    if (out.levelCount == VK_REMAINING_MIP_LEVELS)
        out.levelCount = textureDesc.mipNum - out.baseMipLevel;
    if (out.layerCount == VK_REMAINING_ARRAY_LAYERS)
        out.layerCount = textureDesc.layerNum - out.baseArrayLayer;

    return out;
}

static inline VkImageSubresourceRange GetSubresourceRange(const DescriptorVK& descriptorVK) {
    const TexViewDesc& texViewDesc = descriptorVK.GetTexViewDesc();
    const TextureDesc& textureDesc = texViewDesc.texture->GetDesc();

    VkImageSubresourceRange out = {};
    out.aspectMask = texViewDesc.aspectMask;
    out.baseMipLevel = texViewDesc.mipOffset;
    out.levelCount = texViewDesc.mipNum;
    out.baseArrayLayer = textureDesc.type == TextureType::TEXTURE_3D ? 0 : texViewDesc.layerOrSliceOffset;
    out.layerCount = textureDesc.type == TextureType::TEXTURE_3D ? 1 : texViewDesc.layerOrSliceNum;

    return out;
}

static inline void AppendInputAttachmentRange(Vector<InputAttachmentRange>& ranges, VkImage image, VkImageAspectFlags aspectMask, uint32_t mipOffset, uint32_t mipNum, uint32_t layerOffset, uint32_t layerNum) {
    if (!aspectMask || !mipNum || !layerNum)
        return;

    ranges.push_back({image, aspectMask, (Dim_t)mipOffset, (Dim_t)mipNum, (Dim_t)layerOffset, (Dim_t)layerNum});
}

static inline void AppendSubtractedInputAttachmentRanges(Vector<InputAttachmentRange>& ranges, VkImage image, const InputAttachmentRange& oldRange, const VkImageSubresourceRange& range) {
    VkImageAspectFlags removedAspects = oldRange.aspects & range.aspectMask;
    VkImageAspectFlags remainingAspects = oldRange.aspects & ~range.aspectMask;

    if (remainingAspects)
        AppendInputAttachmentRange(ranges, image, remainingAspects, oldRange.mipOffset, oldRange.mipNum, oldRange.layerOffset, oldRange.layerNum);

    uint32_t mipBegin = std::max((uint32_t)oldRange.mipOffset, range.baseMipLevel);
    uint32_t mipEnd = std::min((uint32_t)(oldRange.mipOffset + oldRange.mipNum), range.baseMipLevel + range.levelCount);
    uint32_t layerBegin = std::max((uint32_t)oldRange.layerOffset, range.baseArrayLayer);
    uint32_t layerEnd = std::min((uint32_t)(oldRange.layerOffset + oldRange.layerNum), range.baseArrayLayer + range.layerCount);

    AppendInputAttachmentRange(ranges, image, removedAspects, oldRange.mipOffset, mipBegin - oldRange.mipOffset, oldRange.layerOffset, oldRange.layerNum);
    AppendInputAttachmentRange(ranges, image, removedAspects, mipEnd, oldRange.mipOffset + oldRange.mipNum - mipEnd, oldRange.layerOffset, oldRange.layerNum);
    AppendInputAttachmentRange(ranges, image, removedAspects, mipBegin, mipEnd - mipBegin, oldRange.layerOffset, layerBegin - oldRange.layerOffset);
    AppendInputAttachmentRange(ranges, image, removedAspects, mipBegin, mipEnd - mipBegin, layerEnd, oldRange.layerOffset + oldRange.layerNum - layerEnd);
}

static inline bool HasInputAttachmentRange(const Vector<InputAttachmentRange>& ranges, VkImage image, const VkImageSubresourceRange& range) {
    bool isFullyContained = true;
    for (VkImageAspectFlags aspects = range.aspectMask; aspects; aspects &= aspects - 1) {
        VkImageAspectFlags aspectMask = GetLowestBit(aspects);
        bool isAspectContained = false;

        for (const InputAttachmentRange& inputAttachmentRange : ranges) {
            if (ContainsRange(inputAttachmentRange, image, aspectMask, range.baseMipLevel, range.levelCount, range.baseArrayLayer, range.layerCount)) {
                isAspectContained = true;
                break;
            }
        }

        if (!isAspectContained) {
            isFullyContained = false;
            break;
        }
    }

    if (isFullyContained)
        return true;

    for (VkImageAspectFlags aspects = range.aspectMask; aspects; aspects &= aspects - 1) {
        VkImageAspectFlags aspectMask = GetLowestBit(aspects);
        for (uint32_t mip = range.baseMipLevel; mip < range.baseMipLevel + range.levelCount; mip++) {
            for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount; layer++) {
                VkImageSubresourceRange subresource = {aspectMask, mip, 1, layer, 1};
                bool isCovered = false;

                for (const InputAttachmentRange& inputAttachmentRange : ranges) {
                    if (inputAttachmentRange.image == image && RangesOverlap(inputAttachmentRange, subresource)) {
                        isCovered = true;
                        break;
                    }
                }

                if (!isCovered)
                    return false;
            }
        }
    }

    return true;
}

static inline void AddInputAttachmentRange(Vector<InputAttachmentRange>& ranges, VkImage image, const VkImageSubresourceRange& range) {
    if (ranges.empty()) {
        AppendInputAttachmentRange(ranges, image, range.aspectMask, range.baseMipLevel, range.levelCount, range.baseArrayLayer, range.layerCount);
        return;
    }

    if (HasInputAttachmentRange(ranges, image, range))
        return;

    AppendInputAttachmentRange(ranges, image, range.aspectMask, range.baseMipLevel, range.levelCount, range.baseArrayLayer, range.layerCount);
    NormalizeInputAttachmentRanges(ranges);
}

static inline void RemoveInputAttachmentRange(Vector<InputAttachmentRange>& ranges, VkImage image, const VkImageSubresourceRange& range) {
    if (ranges.empty())
        return;

    bool hasChanged = false;
    for (size_t i = 0; i < ranges.size();) {
        InputAttachmentRange inputAttachmentRange = ranges[i];
        if (inputAttachmentRange.image != image || !RangesOverlap(inputAttachmentRange, range)) {
            i++;
            continue;
        }

        ranges[i] = ranges.back();
        ranges.pop_back();
        AppendSubtractedInputAttachmentRanges(ranges, image, inputAttachmentRange, range);
        hasChanged = true;
    }

    if (hasChanged)
        NormalizeInputAttachmentRanges(ranges);
}

static inline void FillRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, const AttachmentDesc& attachmentDesc, bool storeOpNoneSupported, Dim_t& renderWidth, Dim_t& renderHeight, Dim_t& layerNum) {
    const DescriptorVK& descriptorVK = *(DescriptorVK*)attachmentDesc.descriptor;
    const TexViewDesc& texViewDesc = descriptorVK.GetTexViewDesc();

    attachmentInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    attachmentInfo.imageView = descriptorVK.GetImageView();
    attachmentInfo.imageLayout = texViewDesc.expectedLayout;
    attachmentInfo.loadOp = GetLoadOp(attachmentDesc.loadOp);
    attachmentInfo.storeOp = GetStoreOp(attachmentDesc.storeOp, storeOpNoneSupported);
    attachmentInfo.clearValue = *(VkClearValue*)&attachmentDesc.clearValue;

    if (attachmentDesc.resolveDst) {
        const DescriptorVK& resolveDst = *(DescriptorVK*)attachmentDesc.resolveDst;

        attachmentInfo.resolveMode = GetResolveOp(attachmentDesc.resolveOp);
        attachmentInfo.resolveImageView = resolveDst.GetImageView();
        attachmentInfo.resolveImageLayout = resolveDst.GetTexViewDesc().expectedLayout;
    }

    // If "INPUT_ATTACHMENT" usage is set, "VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ" is expected
    const TextureDesc& textureDesc = texViewDesc.texture->GetDesc();
    if (textureDesc.usage & TextureUsageBits::INPUT_ATTACHMENT)
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ;

    Dim_t w = texViewDesc.texture->GetSize(0, texViewDesc.mipOffset);
    Dim_t h = texViewDesc.texture->GetSize(1, texViewDesc.mipOffset);

    renderWidth = std::min(renderWidth, w);
    renderHeight = std::min(renderHeight, h);
    layerNum = std::min(layerNum, texViewDesc.layerOrSliceNum);
}

static inline RenderPassAttachmentDesc GetRenderPassAttachmentDesc(const AttachmentDesc& attachmentDesc, bool storeOpNoneSupported, bool isInputAttachment = false) {
    const DescriptorVK& descriptorVK = *(DescriptorVK*)attachmentDesc.descriptor;
    const TexViewDesc& texViewDesc = descriptorVK.GetTexViewDesc();

    RenderPassAttachmentDesc out = {};
    out.format = GetVkFormat(descriptorVK.GetFormat());
    out.sampleNum = (VkSampleCountFlagBits)texViewDesc.texture->GetDesc().sampleNum;
    out.loadOp = GetLoadOp(attachmentDesc.loadOp);
    out.storeOp = GetStoreOp(attachmentDesc.storeOp, storeOpNoneSupported);
    out.stencilLoadOp = out.loadOp;
    out.stencilStoreOp = out.storeOp;
    out.layout = isInputAttachment ? VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ : texViewDesc.expectedLayout;

    return out;
}

static inline RenderPassAttachmentDesc GetRenderPassResolveAttachmentDesc(const DescriptorVK& descriptorVK) {
    const TexViewDesc& texViewDesc = descriptorVK.GetTexViewDesc();

    RenderPassAttachmentDesc out = {};
    out.format = GetVkFormat(descriptorVK.GetFormat());
    out.sampleNum = VK_SAMPLE_COUNT_1_BIT;
    out.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    out.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    out.stencilLoadOp = out.loadOp;
    out.stencilStoreOp = out.storeOp;
    out.layout = texViewDesc.expectedLayout;

    return out;
}

static inline void UpdateRenderingExtent(const DescriptorVK& descriptorVK, Dim_t& renderWidth, Dim_t& renderHeight, Dim_t& layerNum) {
    const TexViewDesc& texViewDesc = descriptorVK.GetTexViewDesc();

    Dim_t w = texViewDesc.texture->GetSize(0, texViewDesc.mipOffset);
    Dim_t h = texViewDesc.texture->GetSize(1, texViewDesc.mipOffset);

    renderWidth = std::min(renderWidth, w);
    renderHeight = std::min(renderHeight, h);
    layerNum = std::min(layerNum, texViewDesc.layerOrSliceNum);
}

static inline StdVideoH264PictureType GetVideoEncodeH264PictureType(VideoFrameType frameType) {
    switch (frameType) {
        case VideoFrameType::IDR:
            return STD_VIDEO_H264_PICTURE_TYPE_IDR;
        case VideoFrameType::I:
            return STD_VIDEO_H264_PICTURE_TYPE_I;
        case VideoFrameType::P:
            return STD_VIDEO_H264_PICTURE_TYPE_P;
        case VideoFrameType::B:
            return STD_VIDEO_H264_PICTURE_TYPE_B;
        default:
            return STD_VIDEO_H264_PICTURE_TYPE_INVALID;
    }
}

static inline StdVideoH265PictureType GetVideoEncodeH265PictureType(VideoFrameType frameType) {
    switch (frameType) {
        case VideoFrameType::IDR:
            return STD_VIDEO_H265_PICTURE_TYPE_IDR;
        case VideoFrameType::I:
            return STD_VIDEO_H265_PICTURE_TYPE_I;
        case VideoFrameType::P:
            return STD_VIDEO_H265_PICTURE_TYPE_P;
        case VideoFrameType::B:
            return STD_VIDEO_H265_PICTURE_TYPE_B;
        default:
            return STD_VIDEO_H265_PICTURE_TYPE_INVALID;
    }
}

static inline VkAccessFlags2 GetAccessFlags(AccessBits accessBits) {
    VkAccessFlags2 flags = VK_ACCESS_2_NONE; // = 0

    if (accessBits & AccessBits::INDEX_BUFFER)
        flags |= VK_ACCESS_2_INDEX_READ_BIT;

    if (accessBits & AccessBits::VERTEX_BUFFER)
        flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;

    if (accessBits & AccessBits::CONSTANT_BUFFER)
        flags |= VK_ACCESS_2_UNIFORM_READ_BIT;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

    if (accessBits & AccessBits::SCRATCH_BUFFER)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    if (accessBits & AccessBits::COLOR_ATTACHMENT_READ)
        flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

    if (accessBits & AccessBits::COLOR_ATTACHMENT_WRITE)
        flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_READ)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::SHADING_RATE_ATTACHMENT)
        flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;

    if (accessBits & AccessBits::INPUT_ATTACHMENT)
        flags |= VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_READ)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    if (accessBits & AccessBits::MICROMAP_READ)
        flags |= VK_ACCESS_2_MICROMAP_READ_BIT_EXT;

    if (accessBits & AccessBits::MICROMAP_WRITE)
        flags |= VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT;

    if (accessBits & AccessBits::SHADER_BINDING_TABLE)
        flags |= VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR;

    if (accessBits & AccessBits::SHADER_RESOURCE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT;

    if (accessBits & AccessBits::SHADER_RESOURCE_STORAGE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;

    if (accessBits & (AccessBits::COPY_SOURCE | AccessBits::RESOLVE_SOURCE))
        flags |= VK_ACCESS_2_TRANSFER_READ_BIT;

    if (accessBits & (AccessBits::COPY_DESTINATION | AccessBits::RESOLVE_DESTINATION | AccessBits::CLEAR_STORAGE))
        flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;

    if (accessBits & AccessBits::HOST_READ)
        flags |= VK_ACCESS_2_HOST_READ_BIT;

    if (accessBits & AccessBits::HOST_WRITE)
        flags |= VK_ACCESS_2_HOST_WRITE_BIT;

    if (accessBits & AccessBits::VIDEO_DECODE_READ)
        flags |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;

    if (accessBits & AccessBits::VIDEO_DECODE_WRITE)
        flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;

    if (accessBits & AccessBits::VIDEO_ENCODE_READ)
        flags |= VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR;

    if (accessBits & AccessBits::VIDEO_ENCODE_WRITE)
        flags |= VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR;

    return flags;
}

CommandBufferVK::~CommandBufferVK() {
    if (m_CommandPool) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.FreeCommandBuffers(m_Device, m_CommandPool, 1, &m_Handle);
    }
}

void CommandBufferVK::Create(VkCommandPool commandPool, VkCommandBuffer commandBuffer, QueueType type) {
    m_CommandPool = commandPool;
    m_Handle = commandBuffer;
    m_Type = type;
}

Result CommandBufferVK::Create(const CommandBufferVKDesc& commandBufferVKDesc) {
    m_CommandPool = VK_NULL_HANDLE;
    m_Handle = (VkCommandBuffer)commandBufferVKDesc.vkCommandBuffer;
    m_Type = commandBufferVKDesc.queueType;

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)m_Handle, name);
}

NRI_INLINE Result CommandBufferVK::Begin(const DescriptorPool*) {
    VkCommandBufferBeginInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.BeginCommandBuffer(m_Handle, &info);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkBeginCommandBuffer");

    m_PipelineLayout = nullptr;
    m_PipelineBindPoint = BindPoint::INHERIT;
    m_InputAttachmentRanges.clear();

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferVK::End() {
    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.EndCommandBuffer(m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkEndCommandBuffer");

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferVK::DecodeVideo(const VideoDecodeDesc& videoDecodeDesc) {
    VideoSessionVK& session = *(VideoSessionVK*)videoDecodeDesc.session;
    VideoSessionParametersVK& parameters = *(VideoSessionParametersVK*)videoDecodeDesc.parameters;

    BufferVK& bitstream = *(BufferVK*)videoDecodeDesc.bitstream.buffer;

    const uint32_t h264ReferenceNum = session.GetDesc().codec == VideoCodec::H264 ? videoDecodeDesc.referenceNum : 0;
    const uint32_t h265ReferenceNum = session.GetDesc().codec == VideoCodec::H265 ? videoDecodeDesc.referenceNum : 0;
    const uint32_t av1ReferenceNum = session.GetDesc().codec == VideoCodec::AV1 ? videoDecodeDesc.referenceNum : 0;
    Scratch<VkVideoReferenceSlotInfoKHR> referenceSlots = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoReferenceSlotInfoKHR, videoDecodeDesc.referenceNum + 1);
    Scratch<StdVideoDecodeH264ReferenceInfo> h264StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoDecodeH264ReferenceInfo, h264ReferenceNum);
    Scratch<VkVideoDecodeH264DpbSlotInfoKHR> h264References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoDecodeH264DpbSlotInfoKHR, h264ReferenceNum);
    Scratch<StdVideoDecodeH265ReferenceInfo> h265StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoDecodeH265ReferenceInfo, h265ReferenceNum);
    Scratch<VkVideoDecodeH265DpbSlotInfoKHR> h265References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoDecodeH265DpbSlotInfoKHR, h265ReferenceNum);
    Scratch<StdVideoDecodeAV1ReferenceInfo> av1StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoDecodeAV1ReferenceInfo, av1ReferenceNum);
    Scratch<VkVideoDecodeAV1DpbSlotInfoKHR> av1References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoDecodeAV1DpbSlotInfoKHR, av1ReferenceNum);
    for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
        VideoPictureVK& picture = *(VideoPictureVK*)videoDecodeDesc.references[i].picture;
        referenceSlots[i] = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
        referenceSlots[i].slotIndex = videoDecodeDesc.references[i].slot;
        referenceSlots[i].pPictureResource = &picture.GetResource();
        if (session.GetDesc().codec == VideoCodec::H264) {
            const VideoH264DecodePictureDesc* h264PictureDesc = videoDecodeDesc.h264PictureDesc;
            const VideoH264DecodeReferenceDesc* referenceDesc = h264PictureDesc ? video::FindReferenceDesc(h264PictureDesc->references, h264PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            NRI_CHECK(referenceDesc, "H.264 reference is missing after NRI validation");

            h264StdReferences[i] = {};
            h264StdReferences[i].flags.top_field_flag = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::TOP_FIELD);
            h264StdReferences[i].flags.bottom_field_flag = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::BOTTOM_FIELD);
            h264StdReferences[i].flags.used_for_long_term_reference = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::LONG_TERM);
            h264StdReferences[i].flags.is_non_existing = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::NON_EXISTING);
            h264StdReferences[i].FrameNum = (uint16_t)referenceDesc->frameNum;
            h264StdReferences[i].PicOrderCnt[0] = referenceDesc->topFieldOrderCount;
            h264StdReferences[i].PicOrderCnt[1] = referenceDesc->bottomFieldOrderCount;
            h264References[i] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR};
            h264References[i].pStdReferenceInfo = &h264StdReferences[i];
            referenceSlots[i].pNext = &h264References[i];
        } else if (session.GetDesc().codec == VideoCodec::H265) {
            const VideoH265DecodePictureDesc* h265PictureDesc = videoDecodeDesc.h265PictureDesc;
            const VideoH265ReferenceDesc* referenceDesc = h265PictureDesc ? video::FindReferenceDesc(h265PictureDesc->references, h265PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            h265StdReferences[i] = {};
            h265StdReferences[i].flags.used_for_long_term_reference = referenceDesc && referenceDesc->longTerm;
            h265StdReferences[i].PicOrderCntVal = referenceDesc ? referenceDesc->pictureOrderCount : (int32_t)videoDecodeDesc.references[i].slot;
            h265References[i] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR};
            h265References[i].pStdReferenceInfo = &h265StdReferences[i];
            referenceSlots[i].pNext = &h265References[i];
        } else if (session.GetDesc().codec == VideoCodec::AV1) {
            const VideoAV1DecodePictureDesc* av1PictureDesc = videoDecodeDesc.av1PictureDesc;
            const VideoAV1ReferenceDesc* referenceDesc = av1PictureDesc ? video::FindReferenceDesc(av1PictureDesc->references, av1PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            NRI_CHECK(referenceDesc, "AV1 reference is missing after NRI validation");

            FillVideoDecodeAV1ReferenceInfo(av1StdReferences[i], referenceDesc->frameType, referenceDesc->orderHint, referenceDesc->savedOrderHints);
            av1References[i] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_DPB_SLOT_INFO_KHR};
            av1References[i].pStdReferenceInfo = &av1StdReferences[i];
            referenceSlots[i].pNext = &av1References[i];
        }
    }

    VkVideoDecodeH264PictureInfoKHR h264Picture = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR};
    StdVideoDecodeH264PictureInfo h264StdPicture = {};
    VkVideoDecodeH264DpbSlotInfoKHR h264DpbSlot = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR};
    StdVideoDecodeH264ReferenceInfo h264StdReference = {};
    VkVideoDecodeH265PictureInfoKHR h265Picture = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_KHR};
    StdVideoDecodeH265PictureInfo h265StdPicture = {};
    VkVideoDecodeH265DpbSlotInfoKHR h265DpbSlot = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR};
    StdVideoDecodeH265ReferenceInfo h265StdReference = {};
    VkVideoDecodeAV1PictureInfoKHR av1Picture = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PICTURE_INFO_KHR};
    StdVideoDecodeAV1PictureInfo av1StdPicture = {};
    VkVideoDecodeAV1DpbSlotInfoKHR av1DpbSlot = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_DPB_SLOT_INFO_KHR};
    StdVideoDecodeAV1ReferenceInfo av1StdReference = {};
    StdVideoAV1TileInfo av1TileInfo = {};
    StdVideoAV1Quantization av1Quantization = {};
    StdVideoAV1LoopFilter av1LoopFilter = {};
    StdVideoAV1LoopRestoration av1LoopRestoration = {};
    StdVideoAV1Segmentation av1Segmentation = {};
    StdVideoAV1CDEF av1Cdef = {};
    StdVideoAV1GlobalMotion av1GlobalMotion = {};
    StdVideoAV1FilmGrain av1FilmGrain = {};
#if defined(VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_INLINE_SESSION_PARAMETERS_INFO_KHR)
    VkVideoDecodeAV1InlineSessionParametersInfoKHR av1InlineSessionParameters = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_INLINE_SESSION_PARAMETERS_INFO_KHR};
#endif
    Scratch<uint32_t> av1TileOffsets = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 0);
    Scratch<uint32_t> av1TileSizes = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 0);
    Scratch<uint32_t> h264SliceOffsets = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.h264PictureDesc ? std::max(videoDecodeDesc.h264PictureDesc->sliceOffsetNum, 1u) : 0);
    Scratch<uint32_t> h265SliceSegmentOffsets = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.h265PictureDesc ? std::max(videoDecodeDesc.h265PictureDesc->sliceSegmentOffsetNum, 1u) : 0);
    Scratch<uint16_t> av1MiColStarts = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum + 1, 2u) : 0);
    Scratch<uint16_t> av1MiRowStarts = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum + 1, 2u) : 0);
    Scratch<uint16_t> av1WidthInSbsMinus1 = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 0);
    Scratch<uint16_t> av1HeightInSbsMinus1 = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 0);
    void* codecPictureInfo = nullptr;
    const void* setupReferenceInfo = nullptr;
    bool activatesSetupReferenceSlot = false;
    if (session.GetDesc().codec == VideoCodec::H264) {
        const VideoH264DecodePictureDesc& desc = *videoDecodeDesc.h264PictureDesc;
        h264StdPicture.flags.field_pic_flag = !!(desc.flags & VideoH264DecodePictureBits::FIELD_PICTURE);
        h264StdPicture.flags.is_intra = !!(desc.flags & VideoH264DecodePictureBits::INTRA);
        h264StdPicture.flags.IdrPicFlag = !!(desc.flags & VideoH264DecodePictureBits::IDR);
        h264StdPicture.flags.bottom_field_flag = !!(desc.flags & VideoH264DecodePictureBits::BOTTOM_FIELD);
        h264StdPicture.flags.is_reference = !!(desc.flags & VideoH264DecodePictureBits::REFERENCE);
        h264StdPicture.flags.complementary_field_pair = !!(desc.flags & VideoH264DecodePictureBits::COMPLEMENTARY_FIELD_PAIR);
        h264StdPicture.seq_parameter_set_id = desc.sequenceParameterSetId;
        h264StdPicture.pic_parameter_set_id = desc.pictureParameterSetId;
        h264StdPicture.frame_num = desc.frameNum;
        h264StdPicture.idr_pic_id = desc.idrPictureId;
        h264StdPicture.PicOrderCnt[0] = desc.topFieldOrderCount;
        h264StdPicture.PicOrderCnt[1] = desc.bottomFieldOrderCount;
        for (uint32_t i = 0; i < desc.sliceOffsetNum; i++)
            h264SliceOffsets[i] = desc.sliceOffsets[i] + 4;

        h264Picture.pStdPictureInfo = &h264StdPicture;
        h264Picture.sliceCount = desc.sliceOffsetNum;
        h264Picture.pSliceOffsets = h264SliceOffsets;
        codecPictureInfo = &h264Picture;

        h264StdReference.flags.top_field_flag = !!(desc.flags & VideoH264DecodePictureBits::FIELD_PICTURE) && !(desc.flags & VideoH264DecodePictureBits::BOTTOM_FIELD);
        h264StdReference.flags.bottom_field_flag = !!(desc.flags & VideoH264DecodePictureBits::FIELD_PICTURE) && !!(desc.flags & VideoH264DecodePictureBits::BOTTOM_FIELD);
        h264StdReference.FrameNum = desc.frameNum;
        h264StdReference.PicOrderCnt[0] = desc.topFieldOrderCount;
        h264StdReference.PicOrderCnt[1] = desc.bottomFieldOrderCount;
        h264DpbSlot.pStdReferenceInfo = &h264StdReference;
        setupReferenceInfo = &h264DpbSlot;
        activatesSetupReferenceSlot = (desc.flags & VideoH264DecodePictureBits::REFERENCE) != 0;
    } else if (session.GetDesc().codec == VideoCodec::H265) {
        const VideoH265DecodePictureDesc& desc = *videoDecodeDesc.h265PictureDesc;
        h265StdPicture.flags.IrapPicFlag = !!(desc.flags & VideoH265DecodePictureBits::IRAP);
        h265StdPicture.flags.IdrPicFlag = !!(desc.flags & VideoH265DecodePictureBits::IDR);
        h265StdPicture.flags.IsReference = !!(desc.flags & VideoH265DecodePictureBits::REFERENCE);
        h265StdPicture.flags.short_term_ref_pic_set_sps_flag = !!(desc.flags & VideoH265DecodePictureBits::SHORT_TERM_REF_PIC_SET_SPS);
        h265StdPicture.sps_video_parameter_set_id = desc.videoParameterSetId;
        h265StdPicture.pps_seq_parameter_set_id = desc.sequenceParameterSetId;
        h265StdPicture.pps_pic_parameter_set_id = desc.pictureParameterSetId;
        h265StdPicture.NumDeltaPocsOfRefRpsIdx = desc.numDeltaPocsOfRefRpsIdx;
        h265StdPicture.PicOrderCntVal = desc.pictureOrderCount;
        h265StdPicture.NumBitsForSTRefPicSetInSlice = desc.numBitsForShortTermRefPicSetInSlice;
        for (uint8_t& entry : h265StdPicture.RefPicSetStCurrBefore)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        for (uint8_t& entry : h265StdPicture.RefPicSetStCurrAfter)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        for (uint8_t& entry : h265StdPicture.RefPicSetLtCurr)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;

        uint32_t beforeNum = 0;
        uint32_t afterNum = 0;
        uint32_t longTermNum = 0;
        for (uint32_t i = 0; i < desc.referenceNum; i++) {
            const VideoH265ReferenceDesc& reference = desc.references[i];
            const uint8_t slot = (uint8_t)reference.slot;
            if (reference.longTerm)
                h265StdPicture.RefPicSetLtCurr[longTermNum++] = slot;
            else if (reference.pictureOrderCount < desc.pictureOrderCount)
                h265StdPicture.RefPicSetStCurrBefore[beforeNum++] = slot;
            else if (reference.pictureOrderCount > desc.pictureOrderCount)
                h265StdPicture.RefPicSetStCurrAfter[afterNum++] = slot;
            else {
                NRI_CHECK(false, "Unexpected equal H.265 short-term reference picture order count");
                return;
            }
        }

        for (uint32_t i = 0; i < desc.sliceSegmentOffsetNum; i++)
            h265SliceSegmentOffsets[i] = desc.sliceSegmentOffsets[i] + 4;

        h265Picture.pStdPictureInfo = &h265StdPicture;
        h265Picture.sliceSegmentCount = desc.sliceSegmentOffsetNum;
        h265Picture.pSliceSegmentOffsets = h265SliceSegmentOffsets;
        codecPictureInfo = &h265Picture;

        h265StdReference.PicOrderCntVal = desc.pictureOrderCount;
        h265DpbSlot.pStdReferenceInfo = &h265StdReference;
        setupReferenceInfo = &h265DpbSlot;
        activatesSetupReferenceSlot = (desc.flags & VideoH265DecodePictureBits::REFERENCE) != 0;
    } else if (session.GetDesc().codec == VideoCodec::AV1) {
        const VideoAV1DecodePictureDesc& desc = *videoDecodeDesc.av1PictureDesc;
        for (int32_t& slotIndex : av1Picture.referenceNameSlotIndices)
            slotIndex = -1;

        const VideoAV1PictureBits pictureFlags = desc.flags == VideoAV1PictureBits::NONE ? video::av1::GetDefaultPictureFlags() : desc.flags;
        VideoDecodeAV1ReferenceMappingVK referenceMapping = {};
        const bool isReferenceMappingValid = BuildVideoDecodeAV1ReferenceMapping(desc, referenceMapping);
        NRI_CHECK(isReferenceMappingValid, "AV1 reference mapping is invalid after NRI validation");
        MaybeUnused(isReferenceMappingValid);

        FillVideoDecodeAV1PictureInfo(av1StdPicture, desc, pictureFlags);
        for (uint32_t i = 0; i < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR; i++)
            av1Picture.referenceNameSlotIndices[i] = referenceMapping.referenceNameSlotIndices[i];

        FillVideoAV1DefaultTileInfo(av1TileInfo, av1MiColStarts, av1MiRowStarts, av1WidthInSbsMinus1, av1HeightInSbsMinus1,
            session.GetDesc().width, session.GetDesc().height);
        if (desc.tileLayout) {
            av1TileInfo.flags.uniform_tile_spacing_flag = desc.tileLayout->uniformSpacing != 0;
            av1TileInfo.TileCols = desc.tileLayout->columnNum;
            av1TileInfo.TileRows = desc.tileLayout->rowNum;
            av1TileInfo.context_update_tile_id = desc.tileLayout->contextUpdateTileId;
            av1TileInfo.tile_size_bytes_minus_1 = desc.tileLayout->tileSizeBytesMinus1;
            av1TileInfo.pMiColStarts = desc.tileLayout->miColumnStarts;
            av1TileInfo.pMiRowStarts = desc.tileLayout->miRowStarts;
            av1TileInfo.pWidthInSbsMinus1 = desc.tileLayout->widthInSuperblocksMinus1;
            av1TileInfo.pHeightInSbsMinus1 = desc.tileLayout->heightInSuperblocksMinus1;
        }
        FillVideoDecodeAV1Quantization(av1Quantization, desc);
        FillVideoDecodeAV1LoopFilter(av1LoopFilter, desc);
        FillVideoDecodeAV1Cdef(av1Cdef, desc);
        if (desc.segmentation) {
            std::memcpy(av1Segmentation.FeatureEnabled, desc.segmentation->featureEnabled, sizeof(av1Segmentation.FeatureEnabled));
            std::memcpy(av1Segmentation.FeatureData, desc.segmentation->featureData, sizeof(av1Segmentation.FeatureData));
        }
        FillVideoDecodeAV1LoopRestoration(av1LoopRestoration, desc);
        FillVideoDecodeAV1GlobalMotion(av1GlobalMotion, desc);
        if (desc.filmGrain)
            FillVideoDecodeAV1FilmGrain(av1FilmGrain, *desc.filmGrain);
        av1StdPicture.pTileInfo = &av1TileInfo;
        av1StdPicture.pQuantization = &av1Quantization;
        av1StdPicture.pSegmentation = &av1Segmentation;
        av1StdPicture.pLoopFilter = &av1LoopFilter;
        av1StdPicture.pCDEF = &av1Cdef;
        av1StdPicture.pLoopRestoration = &av1LoopRestoration;
        av1StdPicture.pGlobalMotion = &av1GlobalMotion;
        av1StdPicture.pFilmGrain = ((pictureFlags & VideoAV1PictureBits::APPLY_GRAIN) && desc.filmGrain) ? &av1FilmGrain : nullptr;

        av1Picture.pStdPictureInfo = &av1StdPicture;
#if defined(VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_INLINE_SESSION_PARAMETERS_INFO_KHR)
        if (session.UseInlineSessionParameters()) {
            av1InlineSessionParameters.pStdSequenceHeader = &parameters.GetAV1SequenceHeader();
            av1Picture.pNext = &av1InlineSessionParameters;
        }
#endif
        FillVideoDecodeAV1TilePayload(av1Picture, desc, av1TileOffsets, av1TileSizes);
        codecPictureInfo = &av1Picture;

        FillVideoDecodeAV1SetupReferenceInfo(av1StdReference, desc, pictureFlags);
        av1DpbSlot.pStdReferenceInfo = &av1StdReference;
        setupReferenceInfo = &av1DpbSlot;
        activatesSetupReferenceSlot = desc.refreshFrameFlags != 0;
    }

    VideoPictureVK& dstPicture = *(VideoPictureVK*)videoDecodeDesc.dstPicture;
    VideoPictureVK& setupPicture = videoDecodeDesc.setupPicture ? *(VideoPictureVK*)videoDecodeDesc.setupPicture : dstPicture;
    const bool hasDpbSlots = session.GetDesc().maxReferenceNum != 0;

    VkVideoReferenceSlotInfoKHR setupReferenceSlot = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
    setupReferenceSlot.pNext = setupReferenceInfo;
    const uint32_t setupReferenceSlotIndex = video::GetDecodeSetupSlot(videoDecodeDesc);
    setupReferenceSlot.slotIndex = (hasDpbSlots && activatesSetupReferenceSlot) ? (int32_t)setupReferenceSlotIndex : -1;
    setupReferenceSlot.pPictureResource = &setupPicture.GetResource();

    VkVideoBeginCodingInfoKHR beginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
    const VkVideoSessionParametersKHR sessionParameters = session.UseInlineSessionParameters() ? VK_NULL_HANDLE : parameters.GetHandle();
    beginInfo.videoSession = session.GetHandle();
    beginInfo.videoSessionParameters = sessionParameters;
    beginInfo.referenceSlotCount = videoDecodeDesc.referenceNum;
    beginInfo.pReferenceSlots = referenceSlots;
    if (hasDpbSlots) {
        referenceSlots[beginInfo.referenceSlotCount] = setupReferenceSlot;
        referenceSlots[beginInfo.referenceSlotCount].slotIndex = -1;
        beginInfo.referenceSlotCount++;
    }

    VkVideoDecodeInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR};
    decodeInfo.pNext = codecPictureInfo;
    decodeInfo.srcBuffer = bitstream.GetHandle();
    decodeInfo.srcBufferOffset = videoDecodeDesc.bitstream.offset;
    decodeInfo.srcBufferRange = videoDecodeDesc.bitstream.size;
    decodeInfo.dstPictureResource = dstPicture.GetResource();
    decodeInfo.pSetupReferenceSlot = hasDpbSlots ? &setupReferenceSlot : nullptr;
    decodeInfo.referenceSlotCount = videoDecodeDesc.referenceNum;
    decodeInfo.pReferenceSlots = referenceSlots;

    const auto& vk = m_Device.GetDispatchTable();
    VkVideoEndCodingInfoKHR endInfo = {VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR};
    const bool needsSessionReset = !session.IsResetRecorded();
    vk.CmdBeginVideoCodingKHR(m_Handle, &beginInfo);
    if (needsSessionReset) {
        VkVideoCodingControlInfoKHR controlInfo = {VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR};
        controlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;
        vk.CmdControlVideoCodingKHR(m_Handle, &controlInfo);
        session.SetResetRecorded();
    }
    vk.CmdDecodeVideoKHR(m_Handle, &decodeInfo);
    vk.CmdEndVideoCodingKHR(m_Handle, &endInfo);
}

NRI_INLINE void CommandBufferVK::EncodeVideo(const VideoEncodeDesc& videoEncodeDesc) {
    VideoSessionVK& session = *(VideoSessionVK*)videoEncodeDesc.session;
    VideoSessionParametersVK& parameters = *(VideoSessionParametersVK*)videoEncodeDesc.parameters;

    const VideoEncodePictureDesc defaultPicture = {VideoFrameType::IDR, 0, 0, 0, 0};
    VideoEncodePictureDesc pictureDesc = videoEncodeDesc.pictureDesc ? *videoEncodeDesc.pictureDesc : defaultPicture;
    if (videoEncodeDesc.flags & VideoEncodeBits::FORCE_KEY_FRAME)
        pictureDesc.frameType = VideoFrameType::IDR;
    const VideoEncodeRateControlDesc defaultRateControl = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 0, 51, 30, 1, 0, 0, 0, 0, 0};
    const VideoEncodeRateControlDesc& rateControlDesc = videoEncodeDesc.rateControlDesc ? *videoEncodeDesc.rateControlDesc : defaultRateControl;
    if ((session.GetRateControlModes() & video::GetEncodeRateControlModeMask(rateControlDesc.mode)) == 0) {
        NRI_REPORT_ERROR(&m_Device, "Unsupported Vulkan video encode rate control mode");
        return;
    }
    if (!video::IsFrameTypeSupported(session.GetDesc().codec, pictureDesc.frameType, true)) {
        NRI_REPORT_ERROR(&m_Device, "Vulkan video encode does not support the requested frame type for this codec");
        return;
    }
    if (pictureDesc.frameType == VideoFrameType::B) {
        if (session.GetDesc().codec == VideoCodec::H264 && (!session.GetH264MaxBPictureL0ReferenceCount() || !session.GetH264MaxL1ReferenceCount())) {
            NRI_REPORT_ERROR(&m_Device, "Vulkan H.264 encode session does not support B-frame references");
            return;
        }
        if (session.GetDesc().codec == VideoCodec::H265 && (!session.GetH265MaxBPictureL0ReferenceCount() || !session.GetH265MaxL1ReferenceCount())) {
            NRI_REPORT_ERROR(&m_Device, "Vulkan H.265 encode session does not support B-frame references");
            return;
        }
    }

    VkVideoEncodeH264PictureInfoKHR h264Picture = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PICTURE_INFO_KHR};
    StdVideoEncodeH264PictureInfo h264StdPicture = {};
    StdVideoEncodeH264SliceHeader h264SliceHeader = {};
    VkVideoEncodeH264NaluSliceInfoKHR h264SliceInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_INFO_KHR};
    StdVideoEncodeH264ReferenceInfo h264StdSetupReference = {};
    VkVideoEncodeH264DpbSlotInfoKHR h264SetupReference = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_KHR};
    StdVideoEncodeH264ReferenceListsInfo h264ReferenceLists = {};

    VkVideoEncodeH265PictureInfoKHR h265Picture = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PICTURE_INFO_KHR};
    StdVideoEncodeH265PictureInfo h265StdPicture = {};
    StdVideoEncodeH265SliceSegmentHeader h265SliceHeader = {};
    VkVideoEncodeH265NaluSliceSegmentInfoKHR h265SliceInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_NALU_SLICE_SEGMENT_INFO_KHR};
    StdVideoEncodeH265ReferenceInfo h265StdSetupReference = {};
    VkVideoEncodeH265DpbSlotInfoKHR h265SetupReference = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_KHR};
    StdVideoEncodeH265ReferenceListsInfo h265ReferenceLists = {};
    StdVideoH265ShortTermRefPicSet h265ShortTermRefPicSet = {};

    VkVideoEncodeAV1PictureInfoKHR av1Picture = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_PICTURE_INFO_KHR};
    StdVideoEncodeAV1PictureInfo av1StdPicture = {};
    StdVideoAV1TileInfo av1TileInfo = {};
    StdVideoAV1Quantization av1Quantization = {};
    StdVideoAV1LoopFilter av1LoopFilter = {};
    StdVideoAV1LoopRestoration av1LoopRestoration = {};
    StdVideoAV1CDEF av1Cdef = {};
    StdVideoAV1GlobalMotion av1GlobalMotion = {};
    StdVideoEncodeAV1ExtensionHeader av1ExtensionHeader = {};
    StdVideoEncodeAV1ReferenceInfo av1StdSetupReference = {};
    VkVideoEncodeAV1DpbSlotInfoKHR av1SetupReference = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_DPB_SLOT_INFO_KHR};
    const VideoAV1TileLayoutDesc* encodeAv1TileLayout = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->tileLayout : nullptr;
    std::array<uint16_t, 2> av1MiColStarts = {};
    std::array<uint16_t, 2> av1MiRowStarts = {};
    std::array<uint16_t, 1> av1WidthInSbsMinus1 = {};
    std::array<uint16_t, 1> av1HeightInSbsMinus1 = {};
    const void* codecPictureInfo = nullptr;
    bool isUsedAsReferencePicture = false;
    switch (session.GetDesc().codec) {
        case VideoCodec::H264: {
            for (uint8_t& ref : h264ReferenceLists.RefPicList0)
                ref = STD_VIDEO_H264_NO_REFERENCE_PICTURE;
            for (uint8_t& ref : h264ReferenceLists.RefPicList1)
                ref = STD_VIDEO_H264_NO_REFERENCE_PICTURE;

            if (videoEncodeDesc.referenceNum) {
                const VideoH264EncodePictureDesc* h264PictureDesc = videoEncodeDesc.h264PictureDesc;
                uint8_t list0Num = 0;
                uint8_t list1Num = 0;
                for (uint32_t i = 0; i < h264PictureDesc->referenceNum; i++) {
                    const VideoH264EncodeReferenceDesc& reference = h264PictureDesc->references[i];
                    if (reference.listIndex == 0) {
                        h264ReferenceLists.RefPicList0[list0Num++] = (uint8_t)reference.slot;
                    } else
                        h264ReferenceLists.RefPicList1[list1Num++] = (uint8_t)reference.slot;
                }
                if (pictureDesc.frameType == VideoFrameType::B) {
                    if (list0Num > session.GetH264MaxBPictureL0ReferenceCount()) {
                        NRI_REPORT_ERROR(&m_Device, "H.264 B-frame List0 reference count exceeds Vulkan device limit");
                        return;
                    }
                    if (list1Num > session.GetH264MaxL1ReferenceCount()) {
                        NRI_REPORT_ERROR(&m_Device, "H.264 B-frame List1 reference count exceeds Vulkan device limit");
                        return;
                    }
                }
                h264ReferenceLists.num_ref_idx_l0_active_minus1 = list0Num ? list0Num - 1 : 0;
                h264ReferenceLists.num_ref_idx_l1_active_minus1 = list1Num ? list1Num - 1 : 0;
                h264StdPicture.pRefLists = &h264ReferenceLists;
            }

            h264StdPicture.flags.IdrPicFlag = pictureDesc.frameType == VideoFrameType::IDR;
            isUsedAsReferencePicture = video::IsEncodePictureUsedAsReference(session.GetDesc().codec, pictureDesc.frameType,
                session.GetDesc().maxReferenceNum, videoEncodeDesc.reconstructedPicture != nullptr, 0);
            h264StdPicture.flags.is_reference = isUsedAsReferencePicture;
            h264StdPicture.flags.no_output_of_prior_pics_flag = pictureDesc.frameType == VideoFrameType::IDR;
            h264StdPicture.seq_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->sequenceParameterSetId : 0;
            h264StdPicture.pic_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->pictureParameterSetId : 0;
            h264StdPicture.idr_pic_id = pictureDesc.idrPictureId;
            h264StdPicture.primary_pic_type = GetVideoEncodeH264PictureType(pictureDesc.frameType);
            h264StdPicture.frame_num = pictureDesc.frameIndex;
            h264StdPicture.PicOrderCnt = pictureDesc.pictureOrderCount;
            h264StdPicture.temporal_id = pictureDesc.temporalLayer;
            h264SliceHeader.slice_type = pictureDesc.frameType == VideoFrameType::B ? STD_VIDEO_H264_SLICE_TYPE_B : (pictureDesc.frameType == VideoFrameType::P ? STD_VIDEO_H264_SLICE_TYPE_P : STD_VIDEO_H264_SLICE_TYPE_I);
            h264SliceHeader.disable_deblocking_filter_idc = STD_VIDEO_H264_DISABLE_DEBLOCKING_FILTER_IDC_DISABLED;
            h264SliceInfo.constantQp = video::GetEncodeQPByFrameType(rateControlDesc, pictureDesc.frameType);
            h264SliceInfo.pStdSliceHeader = &h264SliceHeader;
            h264Picture.naluSliceEntryCount = 1;
            h264Picture.pNaluSliceEntries = &h264SliceInfo;
            h264Picture.pStdPictureInfo = &h264StdPicture;
            h264Picture.generatePrefixNalu = false;
            codecPictureInfo = &h264Picture;

            h264StdSetupReference.primary_pic_type = h264StdPicture.primary_pic_type;
            h264StdSetupReference.FrameNum = h264StdPicture.frame_num;
            h264StdSetupReference.PicOrderCnt = h264StdPicture.PicOrderCnt;
            h264StdSetupReference.temporal_id = h264StdPicture.temporal_id;
            h264SetupReference.pStdReferenceInfo = &h264StdSetupReference;
            break;
        }
        case VideoCodec::H265:
            h265StdPicture.pic_type = GetVideoEncodeH265PictureType(pictureDesc.frameType);
            h265StdPicture.sps_video_parameter_set_id = 0;
            h265StdPicture.pps_seq_parameter_set_id = 0;
            h265StdPicture.pps_pic_parameter_set_id = 0;
            h265StdPicture.PicOrderCntVal = pictureDesc.pictureOrderCount;
            h265StdPicture.TemporalId = pictureDesc.temporalLayer;
            h265StdPicture.flags.IrapPicFlag = pictureDesc.frameType == VideoFrameType::IDR || pictureDesc.frameType == VideoFrameType::I;
            isUsedAsReferencePicture = video::IsEncodePictureUsedAsReference(session.GetDesc().codec, pictureDesc.frameType,
                session.GetDesc().maxReferenceNum, videoEncodeDesc.reconstructedPicture != nullptr, 0);
            h265StdPicture.flags.is_reference = isUsedAsReferencePicture;
            h265StdPicture.flags.pic_output_flag = true;
            h265StdPicture.flags.no_output_of_prior_pics_flag = pictureDesc.frameType == VideoFrameType::IDR;
            h265StdPicture.flags.short_term_ref_pic_set_sps_flag = false;
            h265StdPicture.flags.slice_temporal_mvp_enabled_flag = false;
            for (uint8_t& entry : h265ReferenceLists.RefPicList0)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            for (uint8_t& entry : h265ReferenceLists.RefPicList1)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            for (uint8_t& entry : h265ReferenceLists.list_entry_l0)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            for (uint8_t& entry : h265ReferenceLists.list_entry_l1)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            if (videoEncodeDesc.referenceNum) {
                VideoEncodeH265ReferenceListsVK h265Lists = {};
                if (!BuildVideoEncodeH265ReferenceLists(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, pictureDesc.frameType,
                        pictureDesc.pictureOrderCount, h265Lists)) {
                    NRI_CHECK(false, "Failed to build Vulkan H.265 reference lists from validated descriptors");
                    return;
                }

                const uint32_t list0ReferenceNum = h265Lists.list0Num;
                const uint32_t list1ReferenceNum = h265Lists.list1Num;
                if (pictureDesc.frameType == VideoFrameType::B) {
                    if (list0ReferenceNum > session.GetH265MaxBPictureL0ReferenceCount()) {
                        NRI_REPORT_ERROR(&m_Device, "H.265 B-frame List0 reference count exceeds Vulkan device limit");
                        return;
                    }
                    if (list1ReferenceNum > session.GetH265MaxL1ReferenceCount()) {
                        NRI_REPORT_ERROR(&m_Device, "H.265 B-frame List1 reference count exceeds Vulkan device limit");
                        return;
                    }
                }
                h265ReferenceLists.num_ref_idx_l0_active_minus1 = (uint8_t)(list0ReferenceNum - 1);
                h265ReferenceLists.num_ref_idx_l1_active_minus1 = list1ReferenceNum ? (uint8_t)(list1ReferenceNum - 1) : 0;
                for (uint32_t i = 0; i < list0ReferenceNum; i++) {
                    const uint32_t referenceIndex = h265Lists.list0[i];
                    h265ReferenceLists.RefPicList0[i] = (uint8_t)videoEncodeDesc.references[referenceIndex].slot;
                    h265ReferenceLists.list_entry_l0[i] = (uint8_t)GetVideoEncodeH265List0Entry(h265Lists, referenceIndex);
                    h265ReferenceLists.flags.ref_pic_list_modification_flag_l0 |= h265ReferenceLists.list_entry_l0[i] != i;
                }
                for (uint32_t i = 0; i < list1ReferenceNum; i++) {
                    const uint32_t referenceIndex = h265Lists.list1[i];
                    h265ReferenceLists.RefPicList1[i] = (uint8_t)videoEncodeDesc.references[referenceIndex].slot;
                    h265ReferenceLists.list_entry_l1[i] = (uint8_t)GetVideoEncodeH265List1Entry(h265Lists, referenceIndex);
                    h265ReferenceLists.flags.ref_pic_list_modification_flag_l1 |= h265ReferenceLists.list_entry_l1[i] != i;
                }
                h265StdPicture.pRefLists = &h265ReferenceLists;
                h265ShortTermRefPicSet.num_negative_pics = (uint8_t)h265Lists.negativeNum;
                h265ShortTermRefPicSet.used_by_curr_pic_s0_flag = (uint16_t)((1u << h265Lists.negativeNum) - 1u);
                for (uint32_t i = 0; i < h265Lists.negativeNum; i++) {
                    const uint32_t referenceIndex = h265Lists.negative[i];
                    const VideoH265ReferenceDesc* referenceDesc = video::h265::GetReferenceDesc(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, referenceIndex);
                    const int32_t referencePoc = referenceDesc->pictureOrderCount;
                    const int32_t previousPoc = i ? video::h265::GetReferenceDesc(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, h265Lists.negative[i - 1])->pictureOrderCount : pictureDesc.pictureOrderCount;
                    const int32_t deltaPoc = std::max(1, previousPoc - referencePoc);
                    h265ShortTermRefPicSet.delta_poc_s0_minus1[i] = (uint16_t)(deltaPoc - 1);
                }
                h265ShortTermRefPicSet.num_positive_pics = (uint8_t)h265Lists.positiveNum;
                h265ShortTermRefPicSet.used_by_curr_pic_s1_flag = (uint16_t)((1u << h265Lists.positiveNum) - 1u);
                for (uint32_t i = 0; i < h265Lists.positiveNum; i++) {
                    const uint32_t referenceIndex = h265Lists.positive[i];
                    const VideoH265ReferenceDesc* referenceDesc = video::h265::GetReferenceDesc(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, referenceIndex);
                    const int32_t referencePoc = referenceDesc->pictureOrderCount;
                    const int32_t previousPoc = i ? video::h265::GetReferenceDesc(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, h265Lists.positive[i - 1])->pictureOrderCount : pictureDesc.pictureOrderCount;
                    const int32_t deltaPoc = std::max(1, referencePoc - previousPoc);
                    h265ShortTermRefPicSet.delta_poc_s1_minus1[i] = (uint16_t)(deltaPoc - 1);
                }
                h265StdPicture.pShortTermRefPicSet = &h265ShortTermRefPicSet;
            }
            h265SliceHeader.flags.first_slice_segment_in_pic_flag = true;
            h265SliceHeader.flags.slice_sao_luma_flag = true;
            h265SliceHeader.flags.slice_sao_chroma_flag = true;
            h265SliceHeader.flags.num_ref_idx_active_override_flag = h265ReferenceLists.num_ref_idx_l0_active_minus1 != 0 || h265ReferenceLists.num_ref_idx_l1_active_minus1 != 0;
            h265SliceHeader.flags.mvd_l1_zero_flag = false;
            h265SliceHeader.flags.collocated_from_l0_flag = false;
            h265SliceHeader.slice_type = pictureDesc.frameType == VideoFrameType::B ? STD_VIDEO_H265_SLICE_TYPE_B : (pictureDesc.frameType == VideoFrameType::P ? STD_VIDEO_H265_SLICE_TYPE_P : STD_VIDEO_H265_SLICE_TYPE_I);
            h265SliceHeader.MaxNumMergeCand = 5;
            h265SliceInfo.constantQp = video::GetEncodeQPByFrameType(rateControlDesc, pictureDesc.frameType);
            h265SliceInfo.pStdSliceSegmentHeader = &h265SliceHeader;
            h265Picture.naluSliceSegmentEntryCount = 1;
            h265Picture.pNaluSliceSegmentEntries = &h265SliceInfo;
            h265Picture.pStdPictureInfo = &h265StdPicture;
            codecPictureInfo = &h265Picture;

            h265StdSetupReference.pic_type = h265StdPicture.pic_type;
            h265StdSetupReference.PicOrderCntVal = h265StdPicture.PicOrderCntVal;
            h265StdSetupReference.TemporalId = h265StdPicture.TemporalId;
            h265SetupReference.pStdReferenceInfo = &h265StdSetupReference;
            break;
        case VideoCodec::AV1: {
            for (int32_t& slotIndex : av1Picture.referenceNameSlotIndices)
                slotIndex = -1;
            const VideoAV1EncodePictureDesc* av1PictureDesc = videoEncodeDesc.av1PictureDesc;
            av1StdPicture.frame_type = GetVideoAV1FrameType(pictureDesc.frameType);
            av1StdPicture.frame_presentation_time = pictureDesc.frameIndex;
            av1StdPicture.current_frame_id = av1PictureDesc ? av1PictureDesc->currentFrameId : pictureDesc.frameIndex;
            av1StdPicture.order_hint = av1PictureDesc ? av1PictureDesc->orderHint : (uint8_t)pictureDesc.pictureOrderCount;
            av1StdPicture.primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
            av1StdPicture.refresh_frame_flags = av1PictureDesc ? av1PictureDesc->refreshFrameFlags : ((pictureDesc.frameType == VideoFrameType::IDR && session.GetDesc().maxReferenceNum) ? 0xFF : 0);
            av1StdPicture.render_width_minus_1 = (uint16_t)(session.GetDesc().width - 1);
            av1StdPicture.render_height_minus_1 = (uint16_t)(session.GetDesc().height - 1);
            av1StdPicture.interpolation_filter = STD_VIDEO_AV1_INTERPOLATION_FILTER_EIGHTTAP;
            av1StdPicture.TxMode = STD_VIDEO_AV1_TX_MODE_SELECT;
            av1StdPicture.flags.error_resilient_mode = true;
            av1StdPicture.flags.disable_cdf_update = true;
            av1StdPicture.flags.show_frame = true;
            av1StdPicture.flags.showable_frame = true;
            if (av1PictureDesc && av1PictureDesc->flags != VideoAV1PictureBits::NONE) {
                if ((av1PictureDesc->flags & (VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS | VideoAV1PictureBits::FORCE_INTEGER_MV))
                    && parameters.GetAV1SequenceHeader().seq_force_screen_content_tools == 0) {
                    NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode session parameters do not allow screen-content tools");
                    return;
                }
                if ((av1PictureDesc->flags & VideoAV1PictureBits::FORCE_INTEGER_MV) && parameters.GetAV1SequenceHeader().seq_force_integer_mv == 0) {
                    NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode session parameters do not allow integer motion vectors");
                    return;
                }

                FillVideoAV1PictureFlags(av1StdPicture.flags, av1PictureDesc->flags);
                av1StdPicture.render_width_minus_1 = av1PictureDesc->renderWidthMinus1 ? av1PictureDesc->renderWidthMinus1 : av1StdPicture.render_width_minus_1;
                av1StdPicture.render_height_minus_1 = av1PictureDesc->renderHeightMinus1 ? av1PictureDesc->renderHeightMinus1 : av1StdPicture.render_height_minus_1;
                av1StdPicture.interpolation_filter = (StdVideoAV1InterpolationFilter)av1PictureDesc->interpolationFilter;
                av1StdPicture.TxMode = av1PictureDesc->txMode ? (StdVideoAV1TxMode)av1PictureDesc->txMode : STD_VIDEO_AV1_TX_MODE_SELECT;
                av1StdPicture.coded_denom = av1StdPicture.flags.use_superres ? av1PictureDesc->codedDenom : 0;
                av1StdPicture.delta_q_res = av1PictureDesc->deltaQRes;
                av1StdPicture.delta_lf_res = av1PictureDesc->deltaLfRes;
            }
            if (av1StdPicture.flags.frame_size_override_flag && (session.GetAV1CapabilityFlags() & VK_VIDEO_ENCODE_AV1_CAPABILITY_FRAME_SIZE_OVERRIDE_BIT_KHR) == 0) {
                NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode does not support frame size override");
                return;
            }
            for (int8_t& refFrameIndex : av1StdPicture.ref_frame_idx)
                refFrameIndex = -1;
            if (av1StdPicture.frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY) {
                av1StdPicture.primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
                av1StdPicture.refresh_frame_flags = session.GetDesc().maxReferenceNum ? 0xFF : 0;
            } else if (videoEncodeDesc.referenceNum) {
                av1StdPicture.flags.error_resilient_mode = false;
                av1StdPicture.flags.disable_cdf_update = false;
                av1StdPicture.flags.allow_screen_content_tools = false;
                av1StdPicture.flags.force_integer_mv = false;
            }
            av1StdPicture.flags.showable_frame = av1StdPicture.frame_type != STD_VIDEO_AV1_FRAME_TYPE_KEY;

            isUsedAsReferencePicture = video::IsEncodePictureUsedAsReference(session.GetDesc().codec, pictureDesc.frameType,
                session.GetDesc().maxReferenceNum, videoEncodeDesc.reconstructedPicture != nullptr, av1StdPicture.refresh_frame_flags);

            if (av1PictureDesc && av1PictureDesc->referenceNum) {
                VideoEncodeAV1ReferenceMappingVK referenceMapping = {};
                if (!BuildVideoEncodeAV1ReferenceMapping(videoEncodeDesc.references, videoEncodeDesc.referenceNum, *av1PictureDesc, referenceMapping)) {
                    NRI_CHECK(false, "Failed to build Vulkan AV1 reference mapping from validated descriptors");
                    return;
                }

                for (uint32_t i = 0; i < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR; i++) {
                    av1Picture.referenceNameSlotIndices[i] = referenceMapping.referenceNameSlotIndices[i];
                    av1StdPicture.ref_frame_idx[i] = referenceMapping.refFrameIndices[i];
                }
                const uint8_t primaryReferenceIndex = video::av1::GetReferenceNameIndex(av1PictureDesc->primaryReferenceName);
                const int8_t primaryRefFrameIndex = primaryReferenceIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR ? referenceMapping.refFrameIndices[primaryReferenceIndex] : -1;
                if (primaryRefFrameIndex >= 0) {
                    for (int8_t& refFrameIndex : av1StdPicture.ref_frame_idx) {
                        if (refFrameIndex < 0)
                            refFrameIndex = primaryRefFrameIndex;
                    }
                }
                for (uint32_t i = 0; i < av1PictureDesc->referenceNum; i++) {
                    const VideoAV1ReferenceDesc& reference = av1PictureDesc->references[i];
                    av1StdPicture.ref_order_hint[reference.refFrameIndex] = reference.orderHint;
                }
                av1StdPicture.primary_ref_frame = primaryReferenceIndex;
            } else if (videoEncodeDesc.referenceNum) {
                if (videoEncodeDesc.referenceNum > 1) {
                    NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode requires 'av1PictureDesc' for multiple references");
                    return;
                }

                av1Picture.referenceNameSlotIndices[0] = (int32_t)videoEncodeDesc.references[0].slot;
                av1StdPicture.ref_frame_idx[0] = 0;
                av1StdPicture.primary_ref_frame = 0;
            }
            if (av1StdPicture.flags.segmentation_enabled || (av1PictureDesc && av1PictureDesc->segmentation)) {
                NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode does not support segmentation");
                return;
            }
            av1TileInfo.flags.uniform_tile_spacing_flag = true;
            av1TileInfo.TileCols = 1;
            av1TileInfo.TileRows = 1;
            av1TileInfo.tile_size_bytes_minus_1 = 3;
            av1MiColStarts[0] = 0;
            av1MiColStarts[1] = (uint16_t)((session.GetDesc().width + 3) / 4);
            av1MiRowStarts[0] = 0;
            av1MiRowStarts[1] = (uint16_t)((session.GetDesc().height + 3) / 4);
            av1WidthInSbsMinus1[0] = (uint16_t)((session.GetDesc().width + 63) / 64 - 1);
            av1HeightInSbsMinus1[0] = (uint16_t)((session.GetDesc().height + 63) / 64 - 1);
            av1TileInfo.pMiColStarts = av1MiColStarts.data();
            av1TileInfo.pMiRowStarts = av1MiRowStarts.data();
            av1TileInfo.pWidthInSbsMinus1 = av1WidthInSbsMinus1.data();
            av1TileInfo.pHeightInSbsMinus1 = av1HeightInSbsMinus1.data();
            if (encodeAv1TileLayout) {
                if (encodeAv1TileLayout->columnNum > session.GetAV1MaxTiles().width || encodeAv1TileLayout->rowNum > session.GetAV1MaxTiles().height) {
                    NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->tileLayout' exceeds Vulkan AV1 encode tile count limits");
                    return;
                }

                av1TileInfo.flags.uniform_tile_spacing_flag = encodeAv1TileLayout->uniformSpacing != 0;
                av1TileInfo.TileCols = encodeAv1TileLayout->columnNum;
                av1TileInfo.TileRows = encodeAv1TileLayout->rowNum;
                av1TileInfo.context_update_tile_id = encodeAv1TileLayout->contextUpdateTileId;
                av1TileInfo.tile_size_bytes_minus_1 = encodeAv1TileLayout->tileSizeBytesMinus1;
                if (!encodeAv1TileLayout->uniformSpacing) {
                    av1TileInfo.pMiColStarts = encodeAv1TileLayout->miColumnStarts;
                    av1TileInfo.pMiRowStarts = encodeAv1TileLayout->miRowStarts;
                    av1TileInfo.pWidthInSbsMinus1 = encodeAv1TileLayout->widthInSuperblocksMinus1;
                    av1TileInfo.pHeightInSbsMinus1 = encodeAv1TileLayout->heightInSuperblocksMinus1;

                    const uint32_t superblockSize = parameters.GetAV1SequenceHeader().flags.use_128x128_superblock ? 128 : 64;
                    for (uint32_t i = 0; i < encodeAv1TileLayout->columnNum; i++) {
                        const uint32_t tileWidth = uint32_t(encodeAv1TileLayout->widthInSuperblocksMinus1[i] + 1) * superblockSize;
                        if (!IsVideoEncodeAV1TileWidthSupported(tileWidth, session.GetAV1MinTileSize(), session.GetAV1MaxTileSize())) {
                            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->tileLayout->widthInSuperblocksMinus1[%u]' is outside Vulkan AV1 encode tile size limits", i);
                            return;
                        }
                    }
                    for (uint32_t i = 0; i < encodeAv1TileLayout->rowNum; i++) {
                        const uint32_t tileHeight = uint32_t(encodeAv1TileLayout->heightInSuperblocksMinus1[i] + 1) * superblockSize;
                        if (!IsVideoEncodeAV1TileHeightSupported(tileHeight, session.GetAV1MinTileSize(), session.GetAV1MaxTileSize())) {
                            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->tileLayout->heightInSuperblocksMinus1[%u]' is outside Vulkan AV1 encode tile size limits", i);
                            return;
                        }
                    }
                }
            } else if (!IsVideoEncodeAV1TileSizeSupported(session.GetDesc().width, session.GetDesc().height, session.GetAV1MinTileSize(), session.GetAV1MaxTileSize())) {
                NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode does not support the default single-tile size");
                return;
            }
            FillVideoEncodeAV1Quantization(av1Quantization, av1PictureDesc, (av1PictureDesc && av1PictureDesc->baseQIndex) ? av1PictureDesc->baseQIndex : video::GetEncodeQPByFrameType(rateControlDesc, pictureDesc.frameType));
            FillVideoEncodeAV1LoopFilter(av1LoopFilter, av1PictureDesc);
            FillVideoEncodeAV1LoopRestoration(av1LoopRestoration, av1PictureDesc);
            FillVideoEncodeAV1Cdef(av1Cdef, av1PictureDesc);
            FillVideoEncodeAV1GlobalMotion(av1GlobalMotion, av1PictureDesc);
            av1StdPicture.pTileInfo = encodeAv1TileLayout ? &av1TileInfo : nullptr;
            av1StdPicture.pQuantization = &av1Quantization;
            av1StdPicture.pSegmentation = nullptr;
            av1StdPicture.pLoopFilter = &av1LoopFilter;
            av1StdPicture.pLoopRestoration = (av1PictureDesc && av1PictureDesc->loopRestoration) ? &av1LoopRestoration : nullptr;
            av1StdPicture.pCDEF = &av1Cdef;
            av1StdPicture.pGlobalMotion = &av1GlobalMotion;
            av1StdPicture.pExtensionHeader = &av1ExtensionHeader;
            const bool hasActiveAv1References = videoEncodeDesc.referenceNum != 0;
            av1Picture.predictionMode = hasActiveAv1References
                ? (pictureDesc.frameType == VideoFrameType::B ? VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_BIDIRECTIONAL_COMPOUND_KHR : VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_SINGLE_REFERENCE_KHR)
                : VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_INTRA_ONLY_KHR;
            av1Picture.rateControlGroup = hasActiveAv1References
                ? (pictureDesc.frameType == VideoFrameType::B ? VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_BIPREDICTIVE_KHR : VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_PREDICTIVE_KHR)
                : VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_INTRA_KHR;
            if (av1Picture.predictionMode == VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_SINGLE_REFERENCE_KHR) {
                if (session.GetAV1MaxSingleReferenceCount() == 0 || !HasVideoEncodeAV1ReferenceName(av1Picture.referenceNameSlotIndices, session.GetAV1SingleReferenceNameMask())) {
                    NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode does not support the selected single-reference prediction names");
                    return;
                }
            } else if (av1Picture.predictionMode == VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_UNIDIRECTIONAL_COMPOUND_KHR) {
                const bool hasSupportedPair = HasVideoEncodeAV1ReferenceNamePair(av1Picture.referenceNameSlotIndices, session.GetAV1UnidirectionalCompoundReferenceNameMask(), 0, 1)
                    || HasVideoEncodeAV1ReferenceNamePair(av1Picture.referenceNameSlotIndices, session.GetAV1UnidirectionalCompoundReferenceNameMask(), 0, 2)
                    || HasVideoEncodeAV1ReferenceNamePair(av1Picture.referenceNameSlotIndices, session.GetAV1UnidirectionalCompoundReferenceNameMask(), 0, 3)
                    || HasVideoEncodeAV1ReferenceNamePair(av1Picture.referenceNameSlotIndices, session.GetAV1UnidirectionalCompoundReferenceNameMask(), 4, 6);
                if (session.GetAV1MaxUnidirectionalCompoundReferenceCount() == 0 || !hasSupportedPair) {
                    NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode does not support the selected unidirectional compound reference prediction names");
                    return;
                }
            } else if (av1Picture.predictionMode == VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_BIDIRECTIONAL_COMPOUND_KHR) {
                bool hasSupportedPair = false;
                for (uint32_t i = 0; i < 4 && !hasSupportedPair; i++) {
                    for (uint32_t j = 4; j < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR && !hasSupportedPair; j++)
                        hasSupportedPair = HasVideoEncodeAV1ReferenceNamePair(av1Picture.referenceNameSlotIndices, session.GetAV1BidirectionalCompoundReferenceNameMask(), i, j);
                }
                if (session.GetAV1MaxBidirectionalCompoundReferenceCount() == 0 || !hasSupportedPair) {
                    NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode does not support the selected bidirectional compound reference prediction names");
                    return;
                }
            }
            if (rateControlDesc.mode == VideoEncodeRateControlMode::CQP && (av1Quantization.base_q_idx < session.GetAV1MinQIndex() || av1Quantization.base_q_idx > session.GetAV1MaxQIndex())) {
                NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode Q index %u is outside supported range %u..%u", av1Quantization.base_q_idx, session.GetAV1MinQIndex(), session.GetAV1MaxQIndex());
                return;
            }
            av1Picture.constantQIndex = rateControlDesc.mode == VideoEncodeRateControlMode::CQP ? av1Quantization.base_q_idx : 0;
            av1Picture.pStdPictureInfo = &av1StdPicture;
            codecPictureInfo = &av1Picture;

            av1StdSetupReference.RefFrameId = av1PictureDesc ? av1PictureDesc->currentFrameId : pictureDesc.frameIndex;
            av1StdSetupReference.frame_type = av1StdPicture.frame_type;
            av1StdSetupReference.OrderHint = av1StdPicture.order_hint;
            av1StdSetupReference.pExtensionHeader = &av1ExtensionHeader;
            av1SetupReference.pStdReferenceInfo = &av1StdSetupReference;
            break;
        }
        default:
            NRI_CHECK(false, "Unexpected video encode codec");
            return;
    }

    Scratch<VkVideoReferenceSlotInfoKHR> referenceSlots = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoReferenceSlotInfoKHR, videoEncodeDesc.referenceNum + 1);
    const uint32_t h264ReferenceNum = session.GetDesc().codec == VideoCodec::H264 ? videoEncodeDesc.referenceNum : 0;
    const uint32_t h265ReferenceNum = session.GetDesc().codec == VideoCodec::H265 ? videoEncodeDesc.referenceNum : 0;
    const uint32_t av1ReferenceNum = session.GetDesc().codec == VideoCodec::AV1 ? videoEncodeDesc.referenceNum : 0;
    Scratch<StdVideoEncodeH264ReferenceInfo> h264StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoEncodeH264ReferenceInfo, h264ReferenceNum);
    Scratch<VkVideoEncodeH264DpbSlotInfoKHR> h264References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoEncodeH264DpbSlotInfoKHR, h264ReferenceNum);
    Scratch<StdVideoEncodeH265ReferenceInfo> h265StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoEncodeH265ReferenceInfo, h265ReferenceNum);
    Scratch<VkVideoEncodeH265DpbSlotInfoKHR> h265References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoEncodeH265DpbSlotInfoKHR, h265ReferenceNum);
    Scratch<StdVideoEncodeAV1ReferenceInfo> av1StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoEncodeAV1ReferenceInfo, av1ReferenceNum);
    Scratch<VkVideoEncodeAV1DpbSlotInfoKHR> av1References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoEncodeAV1DpbSlotInfoKHR, av1ReferenceNum);
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        VideoPictureVK& picture = *(VideoPictureVK*)videoEncodeDesc.references[i].picture;
        referenceSlots[i] = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
        referenceSlots[i].slotIndex = videoEncodeDesc.references[i].slot;
        referenceSlots[i].pPictureResource = &picture.GetResource();

        if (session.GetDesc().codec == VideoCodec::H264) {
            const VideoH264EncodeReferenceDesc* referenceDesc = video::FindReferenceDesc(videoEncodeDesc.h264PictureDesc, videoEncodeDesc.references[i].slot);
            NRI_CHECK(referenceDesc, "H.264 reference is missing after NRI validation");

            h264StdReferences[i] = {};
            h264StdReferences[i].flags.used_for_long_term_reference = referenceDesc->longTermReference != 0;
            h264StdReferences[i].primary_pic_type = GetVideoEncodeH264PictureType(referenceDesc->frameType);
            h264StdReferences[i].FrameNum = referenceDesc->frameNum;
            h264StdReferences[i].PicOrderCnt = referenceDesc->pictureOrderCount;
            h264StdReferences[i].long_term_pic_num = referenceDesc->longTermPictureIndex;
            h264StdReferences[i].long_term_frame_idx = referenceDesc->longTermFrameIndex;
            h264StdReferences[i].temporal_id = referenceDesc->temporalLayer;
            h264References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_KHR};
            h264References[i].pStdReferenceInfo = &h264StdReferences[i];
            referenceSlots[i].pNext = &h264References[i];
        } else if (session.GetDesc().codec == VideoCodec::H265) {
            const VideoH265ReferenceDesc* referenceDesc = video::h265::GetReferenceDesc(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, i);
            h265StdReferences[i] = {};
            h265StdReferences[i].flags.used_for_long_term_reference = referenceDesc && referenceDesc->longTerm;
            h265StdReferences[i].pic_type = referenceDesc ? GetVideoEncodeH265PictureType(referenceDesc->frameType) : STD_VIDEO_H265_PICTURE_TYPE_P;
            h265StdReferences[i].PicOrderCntVal = referenceDesc ? referenceDesc->pictureOrderCount : (int32_t)videoEncodeDesc.references[i].slot;
            h265StdReferences[i].TemporalId = referenceDesc ? referenceDesc->temporalLayer : 0;
            h265References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_KHR};
            h265References[i].pStdReferenceInfo = &h265StdReferences[i];
            referenceSlots[i].pNext = &h265References[i];
        } else if (session.GetDesc().codec == VideoCodec::AV1) {
            const VideoAV1ReferenceDesc* referenceDesc = video::FindReferenceDesc(videoEncodeDesc.av1PictureDesc, videoEncodeDesc.references[i].slot);
            av1StdReferences[i] = {};
            av1StdReferences[i].frame_type = referenceDesc ? GetVideoAV1FrameType(referenceDesc->frameType) : STD_VIDEO_AV1_FRAME_TYPE_KEY;
            av1StdReferences[i].RefFrameId = referenceDesc ? referenceDesc->frameId : videoEncodeDesc.references[i].slot;
            av1StdReferences[i].OrderHint = referenceDesc ? referenceDesc->orderHint : 0;
            av1StdReferences[i].pExtensionHeader = &av1ExtensionHeader;
            av1References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_DPB_SLOT_INFO_KHR};
            av1References[i].pStdReferenceInfo = &av1StdReferences[i];
            referenceSlots[i].pNext = &av1References[i];
        }
    }

    const bool hasSetupReferenceSlot = isUsedAsReferencePicture;
    VkVideoReferenceSlotInfoKHR setupReferenceSlot = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};

    if (hasSetupReferenceSlot) {
        if (session.GetDesc().codec == VideoCodec::H264)
            setupReferenceSlot.pNext = &h264SetupReference;
        else if (session.GetDesc().codec == VideoCodec::H265)
            setupReferenceSlot.pNext = &h265SetupReference;
        else if (session.GetDesc().codec == VideoCodec::AV1)
            setupReferenceSlot.pNext = &av1SetupReference;
    }
    setupReferenceSlot.slotIndex = hasSetupReferenceSlot ? (int32_t)videoEncodeDesc.reconstructedSlot : -1;

    if (hasSetupReferenceSlot) {
        VideoPictureVK& reconstructedPicture = *(VideoPictureVK*)videoEncodeDesc.reconstructedPicture;
        setupReferenceSlot.pPictureResource = &reconstructedPicture.GetResource();
    }

    VkVideoEncodeInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR};
    encodeInfo.pNext = codecPictureInfo;
    BufferVK& dstBitstream = *(BufferVK*)videoEncodeDesc.dstBitstream.buffer;

    VideoPictureVK& srcPicture = *(VideoPictureVK*)videoEncodeDesc.srcPicture;
    encodeInfo.dstBuffer = dstBitstream.GetHandle();
    encodeInfo.dstBufferOffset = videoEncodeDesc.dstBitstream.offset;
    encodeInfo.dstBufferRange = videoEncodeDesc.dstBitstream.size;
    encodeInfo.precedingExternallyEncodedBytes = (uint32_t)videoEncodeDesc.bitstreamMetadataSize;
    encodeInfo.srcPictureResource = srcPicture.GetResource();
    encodeInfo.pSetupReferenceSlot = hasSetupReferenceSlot ? &setupReferenceSlot : nullptr;
    encodeInfo.referenceSlotCount = videoEncodeDesc.referenceNum;
    encodeInfo.pReferenceSlots = videoEncodeDesc.referenceNum ? (VkVideoReferenceSlotInfoKHR*)referenceSlots : nullptr;

    if (hasSetupReferenceSlot) {
        referenceSlots[videoEncodeDesc.referenceNum] = GetVideoSetupReferenceSlotForBegin(setupReferenceSlot);
    }

    VkVideoEncodeRateControlInfoKHR rateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeRateControlLayerInfoKHR rateControlLayer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR};
    VkVideoEncodeH264RateControlInfoKHR h264RateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeH265RateControlInfoKHR h265RateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeAV1RateControlInfoKHR av1RateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeAV1GopRemainingFrameInfoKHR av1GopRemainingFrameInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_GOP_REMAINING_FRAME_INFO_KHR};
    VkVideoEncodeQualityLevelInfoKHR qualityLevelInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_QUALITY_LEVEL_INFO_KHR};
    const void* beginPNext = &rateControlInfo;
    FillVideoEncodeRateControl(rateControlDesc, rateControlInfo, rateControlLayer);
    if (session.GetDesc().codec == VideoCodec::H264) {
        h264RateControlInfo.gopFrameCount = session.GetDesc().maxReferenceNum ? 60 : 1;
        h264RateControlInfo.idrPeriod = h264RateControlInfo.gopFrameCount;
        h264RateControlInfo.consecutiveBFrameCount = session.GetDesc().maxReferenceNum > 1 ? 1 : 0;
        h264RateControlInfo.temporalLayerCount = 1;
        rateControlInfo.pNext = &h264RateControlInfo;
    } else if (session.GetDesc().codec == VideoCodec::H265) {
        h265RateControlInfo.gopFrameCount = session.GetDesc().maxReferenceNum ? 60 : 1;
        h265RateControlInfo.idrPeriod = h265RateControlInfo.gopFrameCount;
        h265RateControlInfo.consecutiveBFrameCount = session.GetDesc().maxReferenceNum > 1 ? 1 : 0;
        h265RateControlInfo.subLayerCount = 1;
        rateControlInfo.pNext = &h265RateControlInfo;
    } else if (session.GetDesc().codec == VideoCodec::AV1) {
        av1RateControlInfo.flags = VK_VIDEO_ENCODE_AV1_RATE_CONTROL_REGULAR_GOP_BIT_KHR | VK_VIDEO_ENCODE_AV1_RATE_CONTROL_REFERENCE_PATTERN_FLAT_BIT_KHR;
        av1RateControlInfo.gopFrameCount = 300;
        av1RateControlInfo.keyFramePeriod = 300;
        av1RateControlInfo.consecutiveBipredictiveFrameCount = 1;
        rateControlInfo.pNext = &av1RateControlInfo;
        qualityLevelInfo.pNext = &rateControlInfo;
        if (session.DoesAV1RequireGopRemainingFrames() && rateControlDesc.mode != VideoEncodeRateControlMode::CQP) {
            av1GopRemainingFrameInfo.useGopRemainingFrames = VK_TRUE;
            av1GopRemainingFrameInfo.gopRemainingIntra = (pictureDesc.frameType == VideoFrameType::IDR || pictureDesc.frameType == VideoFrameType::I) ? 1 : 0;
            av1GopRemainingFrameInfo.gopRemainingPredictive = av1RateControlInfo.gopFrameCount ? av1RateControlInfo.gopFrameCount - 1 : 0;
            av1GopRemainingFrameInfo.gopRemainingBipredictive = av1RateControlInfo.consecutiveBipredictiveFrameCount;
            av1GopRemainingFrameInfo.pNext = &rateControlInfo;
            beginPNext = &av1GopRemainingFrameInfo;
        }
    }

    VkVideoBeginCodingInfoKHR beginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
    beginInfo.pNext = beginPNext;
    beginInfo.videoSession = session.GetHandle();
    beginInfo.videoSessionParameters = parameters.GetHandle();
    beginInfo.referenceSlotCount = videoEncodeDesc.referenceNum + (hasSetupReferenceSlot ? 1 : 0);
    beginInfo.pReferenceSlots = referenceSlots;

    VkVideoEndCodingInfoKHR endInfo = {VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR};
    BufferVK* resolvedMetadata = (BufferVK*)videoEncodeDesc.resolvedMetadata;
    bool useEncodeFeedback = resolvedMetadata != nullptr && session.GetEncodeFeedbackQueryPool() != VK_NULL_HANDLE;
    uint32_t encodeFeedbackQueryIndex = UINT32_MAX;
    if (useEncodeFeedback) {
        if (session.FindEncodeFeedbackQuery(resolvedMetadata, videoEncodeDesc.resolvedMetadataOffset) != UINT32_MAX) {
            NRI_REPORT_ERROR(&m_Device, "A Vulkan video encode feedback query is already pending for this resolved metadata range");

            return;
        }

        encodeFeedbackQueryIndex = session.AllocateEncodeFeedbackQuery(resolvedMetadata, videoEncodeDesc.resolvedMetadataOffset);

        if (encodeFeedbackQueryIndex == UINT32_MAX) {
            NRI_REPORT_ERROR(&m_Device, "Too many unresolved Vulkan video encode feedback queries are outstanding for this video session");

            return;
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    if (useEncodeFeedback)
        vk.CmdResetQueryPool(m_Handle, session.GetEncodeFeedbackQueryPool(), encodeFeedbackQueryIndex, 1);

    const bool needsSessionReset = !session.IsResetRecorded();
    if (needsSessionReset) {
        VkVideoBeginCodingInfoKHR initBeginInfo = beginInfo;
        initBeginInfo.pNext = nullptr;

        VkVideoCodingControlInfoKHR controlInfo = {VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR};

        vk.CmdBeginVideoCodingKHR(m_Handle, &initBeginInfo);
        controlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR | VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR;
        if (session.GetDesc().codec == VideoCodec::AV1)
            controlInfo.flags |= VK_VIDEO_CODING_CONTROL_ENCODE_QUALITY_LEVEL_BIT_KHR;
        controlInfo.pNext = session.GetDesc().codec == VideoCodec::AV1 ? (const void*)&qualityLevelInfo : (const void*)&rateControlInfo;
        vk.CmdControlVideoCodingKHR(m_Handle, &controlInfo);
        vk.CmdEndVideoCodingKHR(m_Handle, &endInfo);
        session.SetResetRecorded();
    }
    vk.CmdBeginVideoCodingKHR(m_Handle, &beginInfo);

    if (useEncodeFeedback)
        vk.CmdBeginQuery(m_Handle, session.GetEncodeFeedbackQueryPool(), encodeFeedbackQueryIndex, (VkQueryControlFlags)0);

    vk.CmdEncodeVideoKHR(m_Handle, &encodeInfo);

    if (useEncodeFeedback)
        vk.CmdEndQuery(m_Handle, session.GetEncodeFeedbackQueryPool(), encodeFeedbackQueryIndex);

    vk.CmdEndVideoCodingKHR(m_Handle, &endInfo);
}

NRI_INLINE void CommandBufferVK::ResolveVideoEncodeFeedback(VideoSession& videoSession, Buffer& resolvedMetadata, uint64_t resolvedMetadataOffset) {
    VideoSessionVK& session = (VideoSessionVK&)videoSession;
    if (!session.GetEncodeFeedbackQueryPool())
        return;

    BufferVK& feedbackBuffer = (BufferVK&)resolvedMetadata;
    const uint32_t encodeFeedbackQueryIndex = session.FindEncodeFeedbackQuery(&feedbackBuffer, resolvedMetadataOffset);
    if (encodeFeedbackQueryIndex == UINT32_MAX) {
        NRI_REPORT_ERROR(&m_Device, "No unresolved Vulkan video encode feedback query is available for this resolved metadata range");
        return;
    }

    const auto& vk = m_Device.GetDispatchTable();
    constexpr VkDeviceSize queryResultSize = sizeof(uint64_t) * 3;
    const uint64_t queryResultOffset = resolvedMetadataOffset + sizeof(VideoEncodeFeedback);

    vk.CmdCopyQueryPoolResults(m_Handle, session.GetEncodeFeedbackQueryPool(), encodeFeedbackQueryIndex, 1, feedbackBuffer.GetHandle(), queryResultOffset, queryResultSize,
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_WITH_STATUS_BIT_KHR);
    vk.CmdFillBuffer(m_Handle, feedbackBuffer.GetHandle(), queryResultOffset + queryResultSize, sizeof(uint32_t), encodeFeedbackQueryIndex);
    session.SetEncodeFeedbackQueryResolved(encodeFeedbackQueryIndex);
}

NRI_INLINE void CommandBufferVK::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint32_t vkViewportNum = (deviceDesc.features.extendedDynamicState || viewportNum == 0) ? viewportNum : deviceDesc.viewport.maxNum;
    Scratch<VkViewport> vkViewports = NRI_ALLOCATE_SCRATCH(m_Device, VkViewport, vkViewportNum);
    for (uint32_t i = 0; i < viewportNum; i++) {
        const Viewport& in = viewports[i];
        VkViewport& out = vkViewports[i];
        out.x = in.x;
        out.y = in.y;
        out.width = in.width;
        out.height = in.height;
        out.minDepth = in.depthMin;
        out.maxDepth = in.depthMax;

        // Origin top-left requires flipping
        if (!in.originBottomLeft) {
            out.y += in.height;
            out.height = -in.height;
        }
    }

    for (uint32_t i = viewportNum; i < vkViewportNum; i++)
        vkViewports[i] = vkViewports[viewportNum - 1];

    const auto& vk = m_Device.GetDispatchTable();
    if (deviceDesc.features.extendedDynamicState)
        vk.CmdSetViewportWithCount(m_Handle, viewportNum, vkViewports);
    else
        vk.CmdSetViewport(m_Handle, 0, vkViewportNum, vkViewports);
}

NRI_INLINE void CommandBufferVK::SetScissors(const Rect* rects, uint32_t rectNum) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint32_t vkRectNum = (deviceDesc.features.extendedDynamicState || rectNum == 0) ? rectNum : deviceDesc.viewport.maxNum;
    Scratch<VkRect2D> vkRects = NRI_ALLOCATE_SCRATCH(m_Device, VkRect2D, vkRectNum);
    for (uint32_t i = 0; i < rectNum; i++) {
        const Rect& in = rects[i];
        VkRect2D& out = vkRects[i];
        out.offset.x = in.x;
        out.offset.y = in.y;
        out.extent.width = in.width;
        out.extent.height = in.height;
    }

    for (uint32_t i = rectNum; i < vkRectNum; i++)
        vkRects[i] = vkRects[rectNum - 1];

    const auto& vk = m_Device.GetDispatchTable();
    if (deviceDesc.features.extendedDynamicState)
        vk.CmdSetScissorWithCount(m_Handle, rectNum, vkRects);
    else
        vk.CmdSetScissor(m_Handle, 0, vkRectNum, vkRects);
}

NRI_INLINE void CommandBufferVK::SetDepthBounds(float boundsMin, float boundsMax) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetDepthBounds(m_Handle, boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferVK::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    const auto& vk = m_Device.GetDispatchTable();

    if (frontRef == backRef)
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FACE_FRONT_AND_BACK, frontRef);
    else {
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FACE_FRONT_BIT, frontRef);
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FACE_BACK_BIT, backRef);
    }
}

NRI_INLINE void CommandBufferVK::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    Scratch<VkSampleLocationEXT> sampleLocations = NRI_ALLOCATE_SCRATCH(m_Device, VkSampleLocationEXT, locationNum);
    for (uint32_t i = 0; i < locationNum; i++)
        sampleLocations[i] = {(float)(locations[i].x + 8) / 16.0f, (float)(locations[i].y + 8) / 16.0f};

    uint32_t gridDim = (uint32_t)sqrtf((float)locationNum / (float)sampleNum);

    VkSampleLocationsInfoEXT sampleLocationsInfo = {VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT};
    sampleLocationsInfo.sampleLocationsPerPixel = (VkSampleCountFlagBits)sampleNum;
    sampleLocationsInfo.sampleLocationGridSize = {gridDim, gridDim};
    sampleLocationsInfo.sampleLocationsCount = locationNum;
    sampleLocationsInfo.pSampleLocations = sampleLocations;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetSampleLocationsEXT(m_Handle, &sampleLocationsInfo);
}

NRI_INLINE void CommandBufferVK::SetBlendConstants(const Color32f& color) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetBlendConstants(m_Handle, &color.x);
}

NRI_INLINE void CommandBufferVK::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    VkExtent2D shadingRate = GetShadingRate(shadingRateDesc.shadingRate);
    VkFragmentShadingRateCombinerOpKHR combiners[2] = {
        GetShadingRateCombiner(shadingRateDesc.primitiveCombiner),
        GetShadingRateCombiner(shadingRateDesc.attachmentCombiner),
    };

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetFragmentShadingRateKHR(m_Handle, &shadingRate, combiners);
}

NRI_INLINE void CommandBufferVK::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetDepthBias(m_Handle, depthBiasDesc.constant, depthBiasDesc.clamp, depthBiasDesc.slope);
}

NRI_INLINE void CommandBufferVK::ClearAttachments(const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const Rect* rects, uint32_t rectNum) {
    static_assert(sizeof(VkClearValue) == sizeof(ClearValue), "Sizeof mismatch");

    // Attachments
    uint32_t clearAttachmentNum = 0;
    Scratch<VkClearAttachment> clearAttachments = NRI_ALLOCATE_SCRATCH(m_Device, VkClearAttachment, clearAttachmentDescNum);

    for (uint32_t i = 0; i < clearAttachmentDescNum; i++) {
        const ClearAttachmentDesc& clearAttachmentDesc = clearAttachmentDescs[i];

        VkImageAspectFlags aspectMask = 0;
        if (clearAttachmentDesc.planes & PlaneBits::COLOR)
            aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
        if ((clearAttachmentDesc.planes & PlaneBits::DEPTH) && m_DepthStencil->IsDepthWritable())
            aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ((clearAttachmentDesc.planes & PlaneBits::STENCIL) && m_DepthStencil->IsStencilWritable())
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

        if (aspectMask) {
            VkClearAttachment& clearAttachment = clearAttachments[clearAttachmentNum++];

            clearAttachment = {};
            clearAttachment.aspectMask = aspectMask;
            clearAttachment.colorAttachment = clearAttachmentDesc.colorAttachmentIndex;
            clearAttachment.clearValue = *(VkClearValue*)&clearAttachmentDesc.value;
        }
    }

    if (!clearAttachmentNum)
        return;

    // Rects
    bool hasRects = rectNum != 0;
    if (!hasRects)
        rectNum = 1;

    Scratch<VkClearRect> clearRects = NRI_ALLOCATE_SCRATCH(m_Device, VkClearRect, rectNum);
    for (uint32_t i = 0; i < rectNum; i++) {
        VkClearRect& clearRect = clearRects[i];

        clearRect = {};

        // TODO: layer specification for clears? but not supported by D3D12
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = m_ViewMask ? 1 : m_RenderLayerNum; // VUID-vkCmdClearAttachments-baseArrayLayer-00018

        if (hasRects) {
            const Rect& rect = rects[i];
            clearRect.rect = {{rect.x, rect.y}, {rect.width, rect.height}};
        } else
            clearRect.rect = {{0, 0}, {m_RenderWidth, m_RenderHeight}};
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdClearAttachments(m_Handle, clearAttachmentNum, clearAttachments, rectNum, clearRects);
}

NRI_INLINE void CommandBufferVK::ClearStorage(const ClearStorageDesc& clearStorageDesc) {
    const DescriptorVK& descriptorVK = *(DescriptorVK*)clearStorageDesc.descriptor;

    const auto& vk = m_Device.GetDispatchTable();

    DescriptorType descriptorType = descriptorVK.GetType();
    switch (descriptorType) {
        case DescriptorType::STORAGE_TEXTURE: {
            static_assert(sizeof(VkClearColorValue) == sizeof(clearStorageDesc.value), "Unexpected sizeof");

            const VkClearColorValue* value = (VkClearColorValue*)&clearStorageDesc.value;
            const TexViewDesc& texViewDesc = descriptorVK.GetTexViewDesc();
            VkImage image = texViewDesc.texture->GetHandle();

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // TODO: looks like other aspects are unsupported for storage
            subresourceRange.baseMipLevel = texViewDesc.mipOffset;
            subresourceRange.levelCount = texViewDesc.mipNum;
            subresourceRange.baseArrayLayer = texViewDesc.layerOrSliceOffset;
            subresourceRange.layerCount = texViewDesc.layerOrSliceNum;

            vk.CmdClearColorImage(m_Handle, image, VK_IMAGE_LAYOUT_GENERAL, value, 1, &subresourceRange);
        } break;
        case DescriptorType::STORAGE_BUFFER:
        case DescriptorType::STORAGE_STRUCTURED_BUFFER: {
            const VkDescriptorBufferInfo& descriptorBufferInfo = descriptorVK.GetBufferInfo();
            vk.CmdFillBuffer(m_Handle, descriptorBufferInfo.buffer, descriptorBufferInfo.offset, descriptorBufferInfo.range, clearStorageDesc.value.ui.x);
        } break;
        default:
            NRI_CHECK(false, "Unexpected 'descriptorType'");
            break;
    }
}

NRI_INLINE void CommandBufferVK::BeginRendering(const RenderingDesc& renderingDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    Dim_t renderWidth = deviceDesc.dimensions.attachmentMaxDim;
    Dim_t renderHeight = deviceDesc.dimensions.attachmentMaxDim;
    Dim_t renderLayerNum = deviceDesc.dimensions.attachmentLayerMaxNum;

    if (m_Device.m_IsSupported.dynamicRendering) {
        Scratch<VkRenderingAttachmentInfo> colors = NRI_ALLOCATE_SCRATCH(m_Device, VkRenderingAttachmentInfo, renderingDesc.colorNum);

        VkRenderingInfo renderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        renderingInfo.viewMask = renderingDesc.viewMask;
        renderingInfo.colorAttachmentCount = renderingDesc.colorNum;
        renderingInfo.pColorAttachments = colors;

        for (uint32_t i = 0; i < renderingDesc.colorNum; i++)
            FillRenderingAttachmentInfo(colors[i], renderingDesc.colors[i], m_Device.m_IsSupported.storeOpNone, renderWidth, renderHeight, renderLayerNum);

        VkRenderingAttachmentInfo depth = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        if (renderingDesc.depth.descriptor) {
            m_DepthStencil = (DescriptorVK*)renderingDesc.depth.descriptor;

            FillRenderingAttachmentInfo(depth, renderingDesc.depth, m_Device.m_IsSupported.storeOpNone, renderWidth, renderHeight, renderLayerNum);
            renderingInfo.pDepthAttachment = &depth;

            const FormatProps& formatProps = GetFormatProps(m_DepthStencil->GetFormat());
            if (formatProps.isStencil)
                renderingInfo.pStencilAttachment = &depth;
        } else
            m_DepthStencil = nullptr;

        VkRenderingAttachmentInfo stencil = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        if (renderingDesc.stencil.descriptor) { // it's safe to do it this way, since there are no "stencil-only" formats
            m_DepthStencil = (DescriptorVK*)renderingDesc.stencil.descriptor;

            FillRenderingAttachmentInfo(stencil, renderingDesc.stencil, m_Device.m_IsSupported.storeOpNone, renderWidth, renderHeight, renderLayerNum);
            renderingInfo.pStencilAttachment = &stencil;
        }

        // TODO: if there are no attachments, the render area is set to max dims. It may be suboptimal...
        bool hasAttachment = renderingDesc.colors || renderingDesc.depth.descriptor || renderingDesc.stencil.descriptor;
        if (!hasAttachment)
            renderLayerNum = 1;

        renderingInfo.renderArea = {{0, 0}, {renderWidth, renderHeight}};
        renderingInfo.layerCount = renderLayerNum;

        // Shading rate
        VkRenderingFragmentShadingRateAttachmentInfoKHR shadingRate = {VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR};
        if (renderingDesc.shadingRate) {
            uint32_t tileSize = m_Device.GetDesc().other.shadingRateAttachmentTileSize;
            const DescriptorVK& descriptorVK = *(DescriptorVK*)renderingDesc.shadingRate;

            shadingRate.imageView = descriptorVK.GetImageView();
            shadingRate.imageLayout = descriptorVK.GetTexViewDesc().expectedLayout;
            shadingRate.shadingRateAttachmentTexelSize = {tileSize, tileSize};

            renderingInfo.pNext = &shadingRate;
        }

        const auto& vk = m_Device.GetDispatchTable();
        vk.CmdBeginRendering(m_Handle, &renderingInfo);
    } else {
        RenderPassDesc renderPassDesc(m_Device.GetStdAllocator());
        FramebufferDesc framebufferDesc(m_Device.GetStdAllocator());
        Scratch<VkImageView> colorResolves = NRI_ALLOCATE_SCRATCH(m_Device, VkImageView, renderingDesc.colorNum);
        bool hasColorResolve = false;
        uint32_t colorResolveNum = 0;

        renderPassDesc.viewMask = renderingDesc.viewMask;

        for (uint32_t i = 0; i < renderingDesc.colorNum; i++) {
            const AttachmentDesc& color = renderingDesc.colors[i];
            const DescriptorVK& descriptorVK = *(DescriptorVK*)color.descriptor;
            VkImage image = descriptorVK.GetTexViewDesc().texture->GetHandle();
            bool isInputAttachment = HasInputAttachmentRange(m_InputAttachmentRanges, image, GetSubresourceRange(descriptorVK));

            renderPassDesc.colors.push_back(GetRenderPassAttachmentDesc(color, m_Device.m_IsSupported.storeOpNone, isInputAttachment));
            framebufferDesc.attachments.push_back(descriptorVK.GetImageView());
            UpdateRenderingExtent(descriptorVK, renderWidth, renderHeight, renderLayerNum);

            if (isInputAttachment)
                SetRenderPassInputAttachmentIndex(renderPassDesc.inputAttachmentIndices, i);

            if (color.resolveDst) {
                if (color.resolveOp != ResolveOp::AVERAGE) {
                    m_Device.ReportMessage(Message::ERROR, Result::UNSUPPORTED, __FILE__, __LINE__, "CmdBeginRendering(): legacy render passes support only AVERAGE color resolve");
                    return;
                }

                const DescriptorVK& resolveDst = *(DescriptorVK*)color.resolveDst;
                renderPassDesc.colorResolves.push_back(GetRenderPassResolveAttachmentDesc(resolveDst));
                colorResolves[i] = resolveDst.GetImageView();
                hasColorResolve = true;
                colorResolveNum++;
            } else {
                renderPassDesc.colorResolves.push_back({});
                colorResolves[i] = VK_NULL_HANDLE;
            }
        }

        if (!hasColorResolve)
            renderPassDesc.colorResolves.clear();
        else {
            for (uint32_t i = 0; i < renderingDesc.colorNum; i++) {
                VkImageView view = colorResolves[i];
                if (view != VK_NULL_HANDLE)
                    framebufferDesc.attachments.push_back(view);
            }
        }

        if (renderingDesc.depth.descriptor) {
            m_DepthStencil = (DescriptorVK*)renderingDesc.depth.descriptor;
            const FormatProps& formatProps = GetFormatProps(m_DepthStencil->GetFormat());

            renderPassDesc.hasDepth = true;
            renderPassDesc.depth = GetRenderPassAttachmentDesc(renderingDesc.depth, m_Device.m_IsSupported.storeOpNone);
            framebufferDesc.attachments.push_back(m_DepthStencil->GetImageView());
            UpdateRenderingExtent(*m_DepthStencil, renderWidth, renderHeight, renderLayerNum);

            if (renderingDesc.depth.resolveDst) {
                const DescriptorVK& resolveDst = *(DescriptorVK*)renderingDesc.depth.resolveDst;

                renderPassDesc.hasDepthResolve = true;
                renderPassDesc.depthResolve = GetRenderPassResolveAttachmentDesc(resolveDst);
                renderPassDesc.depthResolveMode = GetResolveOp(renderingDesc.depth.resolveOp);
            }

            if (formatProps.isStencil) {
                renderPassDesc.hasStencil = true;
                renderPassDesc.stencil = renderPassDesc.depth;

                if (renderingDesc.depth.resolveDst) {
                    renderPassDesc.hasStencilResolve = true;
                    renderPassDesc.stencilResolve = renderPassDesc.depthResolve;
                    renderPassDesc.stencilResolveMode = renderPassDesc.depthResolveMode;
                }
            }
        } else
            m_DepthStencil = nullptr;

        if (renderingDesc.stencil.descriptor) {
            DescriptorVK& stencil = *(DescriptorVK*)renderingDesc.stencil.descriptor;
            m_DepthStencil = &stencil;

            if (renderingDesc.depth.descriptor && stencil.GetImageView() != ((DescriptorVK*)renderingDesc.depth.descriptor)->GetImageView()) {
                m_Device.ReportMessage(Message::ERROR, Result::UNSUPPORTED, __FILE__, __LINE__, "CmdBeginRendering(): legacy render passes require matching depth and stencil descriptors");
                return;
            }

            renderPassDesc.hasStencil = true;
            renderPassDesc.stencil = GetRenderPassAttachmentDesc(renderingDesc.stencil, m_Device.m_IsSupported.storeOpNone);
            if (renderingDesc.depth.descriptor) {
                renderPassDesc.depth.stencilLoadOp = renderPassDesc.stencil.loadOp;
                renderPassDesc.depth.stencilStoreOp = renderPassDesc.stencil.storeOp;
            }

            if (!renderingDesc.depth.descriptor || stencil.GetImageView() != ((DescriptorVK*)renderingDesc.depth.descriptor)->GetImageView())
                framebufferDesc.attachments.push_back(stencil.GetImageView());

            UpdateRenderingExtent(stencil, renderWidth, renderHeight, renderLayerNum);

            if (renderingDesc.stencil.resolveDst) {
                const DescriptorVK& resolveDst = *(DescriptorVK*)renderingDesc.stencil.resolveDst;

                renderPassDesc.hasStencilResolve = true;
                renderPassDesc.stencilResolve = GetRenderPassResolveAttachmentDesc(resolveDst);
                renderPassDesc.stencilResolveMode = GetResolveOp(renderingDesc.stencil.resolveOp);
            }
        }

        if (renderPassDesc.hasDepthResolve || renderPassDesc.hasStencilResolve) {
            const bool hasSeparateDepthStencilResolve = renderPassDesc.hasDepthResolve && renderPassDesc.hasStencilResolve
                && renderingDesc.depth.resolveDst
                && renderingDesc.stencil.resolveDst
                && ((DescriptorVK*)renderingDesc.depth.resolveDst)->GetImageView() != ((DescriptorVK*)renderingDesc.stencil.resolveDst)->GetImageView();

            if (hasSeparateDepthStencilResolve) {
                m_Device.ReportMessage(Message::ERROR, Result::UNSUPPORTED, __FILE__, __LINE__, "CmdBeginRendering(): legacy render passes require matching depth and stencil resolve descriptors");
                return;
            }

            const DescriptorVK* resolveDst = renderPassDesc.hasDepthResolve ? (DescriptorVK*)renderingDesc.depth.resolveDst : (DescriptorVK*)renderingDesc.stencil.resolveDst;
            framebufferDesc.attachments.push_back(resolveDst->GetImageView());
        }

        if (renderingDesc.shadingRate) {
            const DescriptorVK& descriptorVK = *(DescriptorVK*)renderingDesc.shadingRate;

            renderPassDesc.hasShadingRate = true;
            renderPassDesc.shadingRate.format = GetVkFormat(descriptorVK.GetFormat());
            renderPassDesc.shadingRate.sampleNum = VK_SAMPLE_COUNT_1_BIT;
            renderPassDesc.shadingRate.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            renderPassDesc.shadingRate.layout = descriptorVK.GetTexViewDesc().expectedLayout;
            framebufferDesc.attachments.push_back(descriptorVK.GetImageView());
        }

        bool hasAttachment = renderingDesc.colorNum || renderingDesc.depth.descriptor || renderingDesc.stencil.descriptor;
        if (!hasAttachment)
            renderLayerNum = 1;

        VkRenderPass renderPass = m_Device.GetOrCreateRenderPass(renderPassDesc);
        if (!renderPass)
            return;

        framebufferDesc.renderPass = renderPass;
        framebufferDesc.width = renderWidth;
        framebufferDesc.height = renderHeight;
        framebufferDesc.layerNum = renderingDesc.viewMask ? 1 : renderLayerNum;

        VkFramebuffer framebuffer = m_Device.GetOrCreateFramebuffer(framebufferDesc);
        if (!framebuffer)
            return;

        Scratch<VkClearValue> clearValues = NRI_ALLOCATE_SCRATCH(m_Device, VkClearValue, framebufferDesc.attachments.size());
        for (uint32_t i = 0; i < (uint32_t)framebufferDesc.attachments.size(); i++)
            clearValues[i] = {};

        for (uint32_t i = 0; i < renderingDesc.colorNum; i++)
            clearValues[i] = *(VkClearValue*)&renderingDesc.colors[i].clearValue;

        uint32_t depthStencilClearIndex = renderingDesc.colorNum + colorResolveNum;
        if (renderingDesc.depth.descriptor || renderingDesc.stencil.descriptor) {
            if (renderingDesc.depth.descriptor) {
                const VkClearValue& depthClear = *(VkClearValue*)&renderingDesc.depth.clearValue;
                clearValues[depthStencilClearIndex].depthStencil.depth = depthClear.depthStencil.depth;
            }

            if (renderingDesc.stencil.descriptor) {
                const VkClearValue& stencilClear = *(VkClearValue*)&renderingDesc.stencil.clearValue;
                clearValues[depthStencilClearIndex].depthStencil.stencil = stencilClear.depthStencil.stencil;
            }
        }

        VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        beginInfo.renderPass = renderPass;
        beginInfo.framebuffer = framebuffer;
        beginInfo.renderArea = {{0, 0}, {renderWidth, renderHeight}};
        beginInfo.clearValueCount = (uint32_t)framebufferDesc.attachments.size();
        beginInfo.pClearValues = clearValues;

        const auto& vk = m_Device.GetDispatchTable();
        vk.CmdBeginRenderPass(m_Handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    m_RenderWidth = renderWidth;
    m_RenderHeight = renderHeight;
    m_RenderLayerNum = renderLayerNum;
    m_ViewMask = renderingDesc.viewMask;
    m_RenderPass = true;
}

NRI_INLINE void CommandBufferVK::EndRendering() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.m_IsSupported.dynamicRendering)
        vk.CmdEndRendering(m_Handle);
    else
        vk.CmdEndRenderPass(m_Handle);

    m_DepthStencil = nullptr;
    m_RenderPass = false;
}

NRI_INLINE void CommandBufferVK::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    Scratch<uint8_t> scratch = NRI_ALLOCATE_SCRATCH(m_Device, uint8_t, vertexBufferNum * (sizeof(VkBuffer) + sizeof(VkDeviceSize) * 3));
    uint8_t* ptr = scratch;

    VkBuffer* handles = (VkBuffer*)ptr;
    ptr += vertexBufferNum * sizeof(VkBuffer);

    VkDeviceSize* offsets = (VkDeviceSize*)ptr;
    ptr += vertexBufferNum * sizeof(VkDeviceSize);

    VkDeviceSize* sizes = (VkDeviceSize*)ptr;
    ptr += vertexBufferNum * sizeof(VkDeviceSize);

    VkDeviceSize* strides = (VkDeviceSize*)ptr;

    for (uint32_t i = 0; i < vertexBufferNum; i++) {
        const VertexBufferDesc& vertexBufferDesc = vertexBufferDescs[i];

        const BufferVK* bufferVK = (BufferVK*)vertexBufferDesc.buffer;
        if (bufferVK) {
            handles[i] = bufferVK->GetHandle();
            offsets[i] = vertexBufferDesc.offset;
            sizes[i] = bufferVK->GetDesc().size - vertexBufferDesc.offset;
            strides[i] = vertexBufferDesc.stride;
        } else {
            handles[i] = VK_NULL_HANDLE;
            offsets[i] = 0;
            sizes[i] = 0;
            strides[i] = 0;
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.GetDesc().features.extendedDynamicState)
        vk.CmdBindVertexBuffers2(m_Handle, baseSlot, vertexBufferNum, handles, offsets, sizes, strides);
    else
        vk.CmdBindVertexBuffers(m_Handle, baseSlot, vertexBufferNum, handles, offsets);
}

NRI_INLINE void CommandBufferVK::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const BufferVK& bufferVK = (BufferVK&)buffer;

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.m_IsSupported.maintenance5) {
        uint64_t size = bufferVK.GetDesc().size - offset;
        vk.CmdBindIndexBuffer2(m_Handle, bufferVK.GetHandle(), offset, size, GetIndexType(indexType));
    } else
        vk.CmdBindIndexBuffer(m_Handle, bufferVK.GetHandle(), offset, GetIndexType(indexType));
}

NRI_INLINE void CommandBufferVK::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    m_PipelineLayout = (PipelineLayoutVK*)&pipelineLayout;
    m_PipelineBindPoint = bindPoint;

    { // Push immutable samplers
        const auto& bindingInfo = m_PipelineLayout->GetBindingInfo();

        for (uint32_t i = bindingInfo.rootSamplerBindingOffset; i < (uint32_t)bindingInfo.pushDescriptors.size(); i++) {
            // https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#descriptorsets-push-descriptors
            // To push an immutable sampler...
            VkDescriptorImageInfo imageInfo = {};

            VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrite.dstBinding = bindingInfo.pushDescriptors[i];
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorWrite.pImageInfo = &imageInfo;

            VkPipelineBindPoint vkPipelineBindPoint = GetPipelineBindPoint(bindPoint);

            const auto& vk = m_Device.GetDispatchTable();
            vk.CmdPushDescriptorSet(m_Handle, vkPipelineBindPoint, *m_PipelineLayout, bindingInfo.rootRegisterSpace, 1, &descriptorWrite);
        }
    }
}

NRI_INLINE void CommandBufferVK::SetPipeline(const Pipeline& pipeline) {
    const PipelineVK& pipelineVK = (PipelineVK&)pipeline;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindPipeline(m_Handle, pipelineVK.GetBindPoint(), pipelineVK);

    // Set depth bias provided at pipeline creation time to match D3D12 behavior
    const DepthBiasDesc& depthBias = pipelineVK.GetDepthBias();
    if (IsDepthBiasEnabled(depthBias))
        vk.CmdSetDepthBias(m_Handle, depthBias.constant, depthBias.clamp, depthBias.slope);
}

NRI_INLINE void CommandBufferVK::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    const DescriptorSetVK& descriptorSetVK = *(DescriptorSetVK*)setDescriptorSetDesc.descriptorSet;
    VkDescriptorSet vkDescriptorSet = descriptorSetVK.GetHandle();

    const auto& bindingInfo = m_PipelineLayout->GetBindingInfo();
    uint32_t registerSpace = bindingInfo.sets[setDescriptorSetDesc.setIndex].registerSpace;

    BindPoint bindPoint = setDescriptorSetDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setDescriptorSetDesc.bindPoint;

    const auto& vk = m_Device.GetDispatchTable();
#if 0 // TODO: NV driver can crash if VVL is enabled...
    if (m_Device.m_IsSupported.maintenance6) {
        StageBits shaderStages = StageBits::NONE;
        if (bindPoint == BindPoint::GRAPHICS) {
            shaderStages = StageBits::VERTEX_SHADER
                | StageBits::TESSELLATION_SHADERS
                | StageBits::GEOMETRY_SHADER
                | StageBits::FRAGMENT_SHADER;

            if (m_Device.GetDesc().features.meshShader)
                shaderStages |= StageBits::MESH_SHADERS;
        } else if (bindPoint == BindPoint::COMPUTE)
            shaderStages = StageBits::COMPUTE_SHADER;
        else if (bindPoint == BindPoint::RAY_TRACING)
            shaderStages = StageBits::RAY_TRACING_SHADERS;

        VkBindDescriptorSetsInfo info = {VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO};
        info.stageFlags = GetShaderStageFlags(shaderStages);
        info.layout = *m_PipelineLayout;
        info.firstSet = registerSpace;
        info.descriptorSetCount = 1;
        info.pDescriptorSets = &vkDescriptorSet;

        vk.CmdBindDescriptorSets2(m_Handle, &info);
    } else
#endif
    {
        VkPipelineBindPoint vkPipelineBindPoint = GetPipelineBindPoint(bindPoint);

        vk.CmdBindDescriptorSets(m_Handle, vkPipelineBindPoint, *m_PipelineLayout, registerSpace, 1, &vkDescriptorSet, 0, nullptr);
    }
}

NRI_INLINE void CommandBufferVK::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    const auto& bindingInfo = m_PipelineLayout->GetBindingInfo();
    const PushConstantBindingDesc& pushConstantBindingDesc = bindingInfo.pushConstants[setRootConstantsDesc.rootConstantIndex];
    uint32_t offset = pushConstantBindingDesc.offset + setRootConstantsDesc.offset;

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.m_IsSupported.maintenance6) {
        VkPushConstantsInfo info = {VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO};
        info.layout = *m_PipelineLayout;
        info.stageFlags = pushConstantBindingDesc.stages;
        info.offset = offset;
        info.size = setRootConstantsDesc.size;
        info.pValues = setRootConstantsDesc.data;

        vk.CmdPushConstants2(m_Handle, &info);
    } else
        vk.CmdPushConstants(m_Handle, *m_PipelineLayout, pushConstantBindingDesc.stages, offset, setRootConstantsDesc.size, setRootConstantsDesc.data);
}

NRI_INLINE void CommandBufferVK::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    const DescriptorVK& descriptorVK = *(DescriptorVK*)setRootDescriptorDesc.descriptor;

    VkAccelerationStructureKHR accelerationStructure = descriptorVK.GetAccelerationStructure();

    const auto& bindingInfo = m_PipelineLayout->GetBindingInfo();

    VkDescriptorBufferInfo bufferInfo = descriptorVK.GetBufferInfo();
    bufferInfo.offset += setRootDescriptorDesc.offset;

    VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    accelerationStructureWrite.accelerationStructureCount = 1;
    accelerationStructureWrite.pAccelerationStructures = &accelerationStructure;

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstBinding = bindingInfo.pushDescriptors[setRootDescriptorDesc.rootDescriptorIndex];
    descriptorWrite.descriptorCount = 1;

    // Let's match D3D12 spec (no textures, no typed buffers)
    DescriptorType descriptorType = descriptorVK.GetType();
    switch (descriptorType) {
        case DescriptorType::CONSTANT_BUFFER:
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.pBufferInfo = &bufferInfo;
            break;
        case DescriptorType::STRUCTURED_BUFFER:
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.pBufferInfo = &bufferInfo;
            break;
        case DescriptorType::ACCELERATION_STRUCTURE:
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            descriptorWrite.pNext = &accelerationStructureWrite;
            break;
        default:
            NRI_CHECK(false, "Unexpected 'descriptorType'");
            break;
    }

    BindPoint bindPoint = setRootDescriptorDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setRootDescriptorDesc.bindPoint;
    VkPipelineBindPoint vkPipelineBindPoint = GetPipelineBindPoint(bindPoint);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdPushDescriptorSet(m_Handle, vkPipelineBindPoint, *m_PipelineLayout, bindingInfo.rootRegisterSpace, 1, &descriptorWrite);
}

NRI_INLINE void CommandBufferVK::Draw(const DrawDesc& drawDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDraw(m_Handle, drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
}

NRI_INLINE void CommandBufferVK::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDrawIndexed(m_Handle, drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
}

NRI_INLINE void CommandBufferVK::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const BufferVK& bufferVK = (BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const BufferVK& countBufferVK = *(BufferVK*)countBuffer;
        vk.CmdDrawIndirectCount(m_Handle, bufferVK.GetHandle(), offset, countBufferVK.GetHandle(), countBufferOffset, drawNum, stride);
    } else
        vk.CmdDrawIndirect(m_Handle, bufferVK.GetHandle(), offset, drawNum, stride);
}

NRI_INLINE void CommandBufferVK::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const BufferVK& bufferVK = (BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const BufferVK& countBufferVK = *(BufferVK*)countBuffer;
        vk.CmdDrawIndexedIndirectCount(m_Handle, bufferVK.GetHandle(), offset, countBufferVK.GetHandle(), countBufferOffset, drawNum, stride);
    } else
        vk.CmdDrawIndexedIndirect(m_Handle, bufferVK.GetHandle(), offset, drawNum, stride);
}

NRI_INLINE void CommandBufferVK::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    const BufferVK& src = (BufferVK&)srcBuffer;
    const BufferVK& dstBufferVK = (BufferVK&)dstBuffer;
    const auto& vk = m_Device.GetDispatchTable();

    if (m_Device.m_IsSupported.copyCommands2) {
        VkBufferCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
        region.srcOffset = srcOffset;
        region.dstOffset = dstOffset;
        region.size = size == WHOLE_SIZE ? src.GetDesc().size : size;

        VkCopyBufferInfo2 info = {VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2};
        info.srcBuffer = src.GetHandle();
        info.dstBuffer = dstBufferVK.GetHandle();
        info.regionCount = 1;
        info.pRegions = &region;

        vk.CmdCopyBuffer2(m_Handle, &info);
    } else {
        VkBufferCopy region = {};
        region.srcOffset = srcOffset;
        region.dstOffset = dstOffset;
        region.size = size == WHOLE_SIZE ? src.GetDesc().size : size;

        vk.CmdCopyBuffer(m_Handle, src.GetHandle(), dstBufferVK.GetHandle(), 1, &region);
    }
}

NRI_INLINE void CommandBufferVK::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    const TextureVK& src = (TextureVK&)srcTexture;
    const TextureVK& dst = (TextureVK&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const TextureDesc& srcDesc = src.GetDesc();

    bool isWholeResource = !dstRegion && !srcRegion;
    uint32_t regionNum = isWholeResource ? dstDesc.mipNum : 1;
    Scratch<VkImageCopy2> regions = NRI_ALLOCATE_SCRATCH(m_Device, VkImageCopy2, regionNum);

    if (isWholeResource) {
        for (Dim_t i = 0; i < dstDesc.mipNum; i++) {
            regions[i] = {VK_STRUCTURE_TYPE_IMAGE_COPY_2};
            regions[i].srcSubresource = {GetImageAspectFlags(PlaneBits::ALL, srcDesc.format), i, 0, srcDesc.layerNum};
            regions[i].srcOffset = {};
            regions[i].dstSubresource = {GetImageAspectFlags(PlaneBits::ALL, dstDesc.format), i, 0, dstDesc.layerNum};
            regions[i].dstOffset = {};
            regions[i].extent = {dst.GetSize(0, i), dst.GetSize(1, i), dst.GetSize(2, i)};
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion)
            srcRegion = &wholeResource;
        if (!dstRegion)
            dstRegion = &wholeResource;

        VkImageAspectFlags srcAspectFlags = GetImageAspectFlags(srcRegion->planes, srcDesc.format);
        VkImageAspectFlags dstAspectFlags = GetImageAspectFlags(dstRegion->planes, dstDesc.format);

        regions[0] = {VK_STRUCTURE_TYPE_IMAGE_COPY_2};
        regions[0].srcSubresource = {
            srcAspectFlags,
            srcRegion->mipOffset,
            srcRegion->layerOffset,
            1,
        };
        regions[0].srcOffset = {
            (int32_t)srcRegion->x,
            (int32_t)srcRegion->y,
            (int32_t)srcRegion->z,
        };
        regions[0].dstSubresource = {
            dstAspectFlags,
            dstRegion->mipOffset,
            dstRegion->layerOffset,
            1,
        };
        regions[0].dstOffset = {
            (int32_t)dstRegion->x,
            (int32_t)dstRegion->y,
            (int32_t)dstRegion->z,
        };
        regions[0].extent = {
            (srcRegion->width == WHOLE_SIZE) ? src.GetSize(0, srcRegion->mipOffset) : srcRegion->width,
            (srcRegion->height == WHOLE_SIZE) ? src.GetSize(1, srcRegion->mipOffset) : srcRegion->height,
            (srcRegion->depth == WHOLE_SIZE) ? src.GetSize(2, srcRegion->mipOffset) : srcRegion->depth,
        };
    }

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.m_IsSupported.copyCommands2) {
        VkCopyImageInfo2 info = {VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2};
        info.srcImage = src.GetHandle();
        info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        info.dstImage = dst.GetHandle();
        info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        info.regionCount = regionNum;
        info.pRegions = regions;

        vk.CmdCopyImage2(m_Handle, &info);
    } else {
        Scratch<VkImageCopy> regionsLegacy = NRI_ALLOCATE_SCRATCH(m_Device, VkImageCopy, regionNum);
        for (uint32_t i = 0; i < regionNum; i++) {
            regionsLegacy[i].srcSubresource = regions[i].srcSubresource;
            regionsLegacy[i].srcOffset = regions[i].srcOffset;
            regionsLegacy[i].dstSubresource = regions[i].dstSubresource;
            regionsLegacy[i].dstOffset = regions[i].dstOffset;
            regionsLegacy[i].extent = regions[i].extent;
        }

        vk.CmdCopyImage(m_Handle, src.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionNum, regionsLegacy);
    }
}

NRI_INLINE void CommandBufferVK::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp) {
    const TextureVK& src = (TextureVK&)srcTexture;
    const TextureVK& dst = (TextureVK&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const TextureDesc& srcDesc = src.GetDesc();

    bool isWholeResource = !dstRegion && !srcRegion;
    uint32_t regionNum = isWholeResource ? dstDesc.mipNum : 1;
    Scratch<VkImageResolve2> regions = NRI_ALLOCATE_SCRATCH(m_Device, VkImageResolve2, dstDesc.mipNum);

    if (isWholeResource) {
        for (Dim_t i = 0; i < dstDesc.mipNum; i++) {
            regions[i] = {VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2};
            regions[i].srcSubresource = {GetImageAspectFlags(PlaneBits::ALL, srcDesc.format), i, 0, srcDesc.layerNum};
            regions[i].srcOffset = {};
            regions[i].dstSubresource = {GetImageAspectFlags(PlaneBits::ALL, dstDesc.format), i, 0, dstDesc.layerNum};
            regions[i].dstOffset = {};
            regions[i].extent = dst.GetExtent();
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion)
            srcRegion = &wholeResource;
        if (!dstRegion)
            dstRegion = &wholeResource;

        VkImageAspectFlags srcAspectFlags = GetImageAspectFlags(srcRegion->planes, srcDesc.format);
        VkImageAspectFlags dstAspectFlags = GetImageAspectFlags(dstRegion->planes, dstDesc.format);

        regions[0] = {VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2};
        regions[0].srcSubresource = {
            srcAspectFlags,
            srcRegion->mipOffset,
            srcRegion->layerOffset,
            1,
        };
        regions[0].srcOffset = {
            (int32_t)srcRegion->x,
            (int32_t)srcRegion->y,
            (int32_t)srcRegion->z,
        };
        regions[0].dstSubresource = {
            dstAspectFlags,
            dstRegion->mipOffset,
            dstRegion->layerOffset,
            1,
        };
        regions[0].dstOffset = {
            (int32_t)dstRegion->x,
            (int32_t)dstRegion->y,
            (int32_t)dstRegion->z,
        };
        regions[0].extent = {
            (srcRegion->width == WHOLE_SIZE) ? src.GetSize(0, srcRegion->mipOffset) : srcRegion->width,
            (srcRegion->height == WHOLE_SIZE) ? src.GetSize(1, srcRegion->mipOffset) : srcRegion->height,
            (srcRegion->depth == WHOLE_SIZE) ? src.GetSize(2, srcRegion->mipOffset) : srcRegion->depth,
        };
    }

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.m_IsSupported.copyCommands2) {
        VkResolveImageInfo2 info = {VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2};
        info.srcImage = src.GetHandle();
        info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        info.dstImage = dst.GetHandle();
        info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        info.regionCount = regionNum;
        info.pRegions = regions;

        VkResolveImageModeInfoKHR resolveModeInfo = {VK_STRUCTURE_TYPE_RESOLVE_IMAGE_MODE_INFO_KHR};
        if (m_Device.m_IsSupported.maintenance10) {
            resolveModeInfo.resolveMode = GetResolveOp(resolveOp);
            resolveModeInfo.stencilResolveMode = GetResolveOp(resolveOp);
            // TODO: resolveModeInfo.flags?

            info.pNext = &resolveModeInfo;
        }

        vk.CmdResolveImage2(m_Handle, &info);
    } else {
        Scratch<VkImageResolve> regionsLegacy = NRI_ALLOCATE_SCRATCH(m_Device, VkImageResolve, regionNum);
        for (uint32_t i = 0; i < regionNum; i++) {
            regionsLegacy[i].srcSubresource = regions[i].srcSubresource;
            regionsLegacy[i].srcOffset = regions[i].srcOffset;
            regionsLegacy[i].dstSubresource = regions[i].dstSubresource;
            regionsLegacy[i].dstOffset = regions[i].dstOffset;
            regionsLegacy[i].extent = regions[i].extent;
        }

        vk.CmdResolveImage(m_Handle, src.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionNum, regionsLegacy);
    }
}

NRI_INLINE void CommandBufferVK::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    const BufferVK& src = (BufferVK&)srcBuffer;
    const TextureVK& dst = (TextureVK&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    auto getPlaneLayout = [](Format format, PlaneBits planes, uint32_t& stride, uint32_t& blockWidth, uint32_t& blockHeight) {
        stride = GetFormatProps(format).stride;
        blockWidth = GetFormatProps(format).blockWidth;
        blockHeight = GetFormatProps(format).blockHeight;

        if (format == Format::NV12_UNORM) {
            stride = (planes & PlaneBits::PLANE_1) ? 2 : 1;
            blockWidth = 1;
            blockHeight = 1;
        } else if (format == Format::P010_UNORM || format == Format::P016_UNORM) {
            stride = (planes & PlaneBits::PLANE_1) ? 4 : 2;
            blockWidth = 1;
            blockHeight = 1;
        }
    };
    auto getPlaneDivisor = [](Format format, PlaneBits planes) {
        return ((planes & PlaneBits::PLANE_1) && (format == Format::NV12_UNORM || format == Format::P010_UNORM || format == Format::P016_UNORM)) ? 2u : 1u;
    };

    uint32_t planeStride = 0;
    uint32_t planeBlockWidth = 0;
    uint32_t planeBlockHeight = 0;
    getPlaneLayout(dstDesc.format, dstRegion.planes, planeStride, planeBlockWidth, planeBlockHeight);
    const uint32_t planeDivisor = getPlaneDivisor(dstDesc.format, dstRegion.planes);

    uint32_t rowBlockNum = srcDataLayout.rowPitch / planeStride;
    uint32_t bufferRowLength = rowBlockNum * planeBlockWidth;

    uint32_t sliceRowNum = srcDataLayout.slicePitch / srcDataLayout.rowPitch;
    uint32_t bufferImageHeight = sliceRowNum * planeBlockHeight;

    VkImageAspectFlags dstAspectFlags = GetImageAspectFlags(dstRegion.planes, dstDesc.format);

    VkBufferImageCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2};
    region.bufferOffset = srcDataLayout.offset;
    region.bufferRowLength = bufferRowLength;
    region.bufferImageHeight = bufferImageHeight;
    region.imageSubresource = VkImageSubresourceLayers{
        dstAspectFlags,
        dstRegion.mipOffset,
        dstRegion.layerOffset,
        1,
    };
    region.imageOffset = VkOffset3D{
        (int32_t)(dstRegion.x / planeDivisor),
        (int32_t)(dstRegion.y / planeDivisor),
        dstRegion.z,
    };
    region.imageExtent = VkExtent3D{
        ((dstRegion.width == WHOLE_SIZE) ? dst.GetSize(0, dstRegion.mipOffset) : dstRegion.width) / planeDivisor,
        ((dstRegion.height == WHOLE_SIZE) ? dst.GetSize(1, dstRegion.mipOffset) : dstRegion.height) / planeDivisor,
        (dstRegion.depth == WHOLE_SIZE) ? dst.GetSize(2, dstRegion.mipOffset) : dstRegion.depth,
    };

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.m_IsSupported.copyCommands2) {
        VkCopyBufferToImageInfo2 info = {VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2};
        info.srcBuffer = src.GetHandle();
        info.dstImage = dst.GetHandle();
        info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        info.regionCount = 1;
        info.pRegions = &region;

        vk.CmdCopyBufferToImage2(m_Handle, &info);
    } else {
        VkBufferImageCopy regionLegacy = {};
        regionLegacy.bufferOffset = region.bufferOffset;
        regionLegacy.bufferRowLength = region.bufferRowLength;
        regionLegacy.bufferImageHeight = region.bufferImageHeight;
        regionLegacy.imageSubresource = region.imageSubresource;
        regionLegacy.imageOffset = region.imageOffset;
        regionLegacy.imageExtent = region.imageExtent;

        vk.CmdCopyBufferToImage(m_Handle, src.GetHandle(), dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionLegacy);
    }
}

NRI_INLINE void CommandBufferVK::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    const TextureVK& src = (TextureVK&)srcTexture;
    const BufferVK& dst = (BufferVK&)dstBuffer;
    const TextureDesc& srcDesc = src.GetDesc();
    auto getPlaneLayout = [](Format format, PlaneBits planes, uint32_t& stride, uint32_t& blockWidth, uint32_t& blockHeight) {
        stride = GetFormatProps(format).stride;
        blockWidth = GetFormatProps(format).blockWidth;
        blockHeight = GetFormatProps(format).blockHeight;

        if (format == Format::NV12_UNORM) {
            stride = (planes & PlaneBits::PLANE_1) ? 2 : 1;
            blockWidth = 1;
            blockHeight = 1;
        } else if (format == Format::P010_UNORM || format == Format::P016_UNORM) {
            stride = (planes & PlaneBits::PLANE_1) ? 4 : 2;
            blockWidth = 1;
            blockHeight = 1;
        }
    };
    auto getPlaneDivisor = [](Format format, PlaneBits planes) {
        return ((planes & PlaneBits::PLANE_1) && (format == Format::NV12_UNORM || format == Format::P010_UNORM || format == Format::P016_UNORM)) ? 2u : 1u;
    };

    uint32_t planeStride = 0;
    uint32_t planeBlockWidth = 0;
    uint32_t planeBlockHeight = 0;
    getPlaneLayout(srcDesc.format, srcRegion.planes, planeStride, planeBlockWidth, planeBlockHeight);
    const uint32_t planeDivisor = getPlaneDivisor(srcDesc.format, srcRegion.planes);

    uint32_t rowBlockNum = dstDataLayout.rowPitch / planeStride;
    uint32_t bufferRowLength = rowBlockNum * planeBlockWidth;

    uint32_t sliceRowNum = dstDataLayout.slicePitch / dstDataLayout.rowPitch;
    uint32_t bufferImageHeight = sliceRowNum * planeBlockHeight;

    VkImageAspectFlags srcAspectFlags = GetImageAspectFlags(srcRegion.planes, srcDesc.format);

    VkBufferImageCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2};
    region.bufferOffset = dstDataLayout.offset;
    region.bufferRowLength = bufferRowLength;
    region.bufferImageHeight = bufferImageHeight;
    region.imageSubresource = VkImageSubresourceLayers{
        srcAspectFlags,
        srcRegion.mipOffset,
        srcRegion.layerOffset,
        1,
    };
    region.imageOffset = VkOffset3D{
        (int32_t)(srcRegion.x / planeDivisor),
        (int32_t)(srcRegion.y / planeDivisor),
        srcRegion.z,
    };
    region.imageExtent = VkExtent3D{
        (srcRegion.width == WHOLE_SIZE ? src.GetSize(0, srcRegion.mipOffset) : srcRegion.width) / planeDivisor,
        (srcRegion.height == WHOLE_SIZE ? src.GetSize(1, srcRegion.mipOffset) : srcRegion.height) / planeDivisor,
        srcRegion.depth == WHOLE_SIZE ? src.GetSize(2, srcRegion.mipOffset) : srcRegion.depth,
    };

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Device.m_IsSupported.copyCommands2) {
        VkCopyImageToBufferInfo2 info = {VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2};
        info.srcImage = src.GetHandle();
        info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        info.dstBuffer = dst.GetHandle();
        info.regionCount = 1;
        info.pRegions = &region;

        vk.CmdCopyImageToBuffer2(m_Handle, &info);
    } else {
        VkBufferImageCopy regionLegacy = {};
        regionLegacy.bufferOffset = region.bufferOffset;
        regionLegacy.bufferRowLength = region.bufferRowLength;
        regionLegacy.bufferImageHeight = region.bufferImageHeight;
        regionLegacy.imageSubresource = region.imageSubresource;
        regionLegacy.imageOffset = region.imageOffset;
        regionLegacy.imageExtent = region.imageExtent;

        vk.CmdCopyImageToBuffer(m_Handle, src.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.GetHandle(), 1, &regionLegacy);
    }
}

NRI_INLINE void CommandBufferVK::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    BufferVK& dst = (BufferVK&)buffer;

    if (size == WHOLE_SIZE)
        size = dst.GetDesc().size;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdFillBuffer(m_Handle, dst.GetHandle(), offset, size, 0);
}

NRI_INLINE void CommandBufferVK::Dispatch(const DispatchDesc& dispatchDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDispatch(m_Handle, dispatchDesc.workGroupNumX, dispatchDesc.workGroupNumY, dispatchDesc.workGroupNumZ);
}

NRI_INLINE void CommandBufferVK::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchDesc) == sizeof(VkDispatchIndirectCommand));

    const BufferVK& bufferVK = (BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDispatchIndirect(m_Handle, bufferVK.GetHandle(), offset);
}

NRI_INLINE void CommandBufferVK::Barrier(const BarrierDesc& barrierDesc) {
    // Global
    Scratch<VkMemoryBarrier2> memoryBarriers = NRI_ALLOCATE_SCRATCH(m_Device, VkMemoryBarrier2, barrierDesc.globalNum);
    for (uint32_t i = 0; i < barrierDesc.globalNum; i++) {
        const GlobalBarrierDesc& in = barrierDesc.globals[i];

        VkMemoryBarrier2& out = memoryBarriers[i];
        out = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = GetAccessFlags(in.after.access);
    }

    // Buffer
    Scratch<VkBufferMemoryBarrier2> bufferBarriers = NRI_ALLOCATE_SCRATCH(m_Device, VkBufferMemoryBarrier2, barrierDesc.bufferNum);
    for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
        const BufferBarrierDesc& in = barrierDesc.buffers[i];
        const BufferVK& bufferVK = *(const BufferVK*)in.buffer;

        VkBufferMemoryBarrier2& out = bufferBarriers[i];
        out = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = GetAccessFlags(in.after.access);
        out.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // "VK_SHARING_MODE_CONCURRENT" is intentionally used for buffers to match D3D12 spec
        out.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        out.buffer = bufferVK.GetHandle();
        out.offset = 0;
        out.size = VK_WHOLE_SIZE;
    }

    // Texture
    bool isRegionLocal = false;
    Scratch<VkImageMemoryBarrier2> textureBarriers = NRI_ALLOCATE_SCRATCH(m_Device, VkImageMemoryBarrier2, barrierDesc.textureNum);
    for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
        const TextureBarrierDesc& in = barrierDesc.textures[i];
        const TextureVK& textureVK = *(TextureVK*)in.texture;
        const QueueVK* srcQueue = (QueueVK*)in.srcQueue;
        const QueueVK* dstQueue = (QueueVK*)in.dstQueue;

        VkImageAspectFlags aspectFlags = GetImageAspectFlags(in.planes, textureVK.GetDesc().format);

        VkImageMemoryBarrier2& out = textureBarriers[i];
        out = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = GetAccessFlags(in.after.access);
        out.oldLayout = GetImageLayout(in.before.layout);
        out.newLayout = GetImageLayout(in.after.layout);
        out.srcQueueFamilyIndex = in.srcQueue ? srcQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
        out.dstQueueFamilyIndex = in.dstQueue ? dstQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
        out.image = textureVK.GetHandle();
        out.subresourceRange = {
            aspectFlags,
            in.mipOffset,
            (in.mipNum == REMAINING) ? VK_REMAINING_MIP_LEVELS : in.mipNum,
            in.layerOffset,
            (in.layerNum == REMAINING) ? VK_REMAINING_ARRAY_LAYERS : in.layerNum,
        };

        // Legacy render passes need input attachment references before vkCmdBeginRenderPass.
        if (!m_Device.m_IsSupported.dynamicRendering) {
            VkImageSubresourceRange range = GetSubresourceRange(textureVK, out.subresourceRange);
            if (in.after.layout == Layout::INPUT_ATTACHMENT)
                AddInputAttachmentRange(m_InputAttachmentRanges, textureVK.GetHandle(), range);
            else
                RemoveInputAttachmentRange(m_InputAttachmentRanges, textureVK.GetHandle(), range);
        }

        if (m_RenderPass && in.after.layout == Layout::INPUT_ATTACHMENT)
            isRegionLocal = true;
    }

    // Submit
    VkDependencyInfo dependencyInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependencyInfo.memoryBarrierCount = barrierDesc.globalNum;
    dependencyInfo.pMemoryBarriers = memoryBarriers;
    dependencyInfo.bufferMemoryBarrierCount = barrierDesc.bufferNum;
    dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
    dependencyInfo.imageMemoryBarrierCount = barrierDesc.textureNum;
    dependencyInfo.pImageMemoryBarriers = textureBarriers;

    if (isRegionLocal)
        dependencyInfo.dependencyFlags |= VK_DEPENDENCY_BY_REGION_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdPipelineBarrier2(m_Handle, &dependencyInfo);
}

NRI_INLINE void CommandBufferVK::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBeginQuery(m_Handle, queryPoolVK.GetHandle(), offset, (VkQueryControlFlagBits)0);
}

NRI_INLINE void CommandBufferVK::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;
    const auto& vk = m_Device.GetDispatchTable();

    if (queryPoolVK.GetType() == VK_QUERY_TYPE_TIMESTAMP) {
        // TODO: https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdWriteTimestamp.html
        // https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html
        vk.CmdWriteTimestamp2(m_Handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queryPoolVK.GetHandle(), offset);
    } else
        vk.CmdEndQuery(m_Handle, queryPoolVK.GetHandle(), offset);
}

NRI_INLINE void CommandBufferVK::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    const QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;
    const BufferVK& bufferVK = (BufferVK&)dstBuffer;

    // TODO: wait is questionable here, but it's needed to ensure that the destination buffer gets "complete" values (perf seems unaffected)
    VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyQueryPoolResults(m_Handle, queryPoolVK.GetHandle(), offset, num, bufferVK.GetHandle(), dstOffset, queryPoolVK.GetQuerySize(), flags);
}

NRI_INLINE void CommandBufferVK::ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdResetQueryPool(m_Handle, queryPoolVK.GetHandle(), offset, num);
}

NRI_INLINE void CommandBufferVK::BeginAnnotation(const char* name, uint32_t bgra) {
    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = name;
    info.color[0] = ((bgra >> 16) & 0xFF) / 255.0f;
    info.color[1] = ((bgra >> 8) & 0xFF) / 255.0f;
    info.color[2] = ((bgra >> 0) & 0xFF) / 255.0f;
    info.color[3] = 1.0f; // PIX sets alpha to 1

    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdBeginDebugUtilsLabelEXT)
        vk.CmdBeginDebugUtilsLabelEXT(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::EndAnnotation() {
    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdEndDebugUtilsLabelEXT)
        vk.CmdEndDebugUtilsLabelEXT(m_Handle);
}

NRI_INLINE void CommandBufferVK::Annotation(const char* name, uint32_t bgra) {
    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = name;
    info.color[0] = ((bgra >> 16) & 0xFF) / 255.0f;
    info.color[1] = ((bgra >> 8) & 0xFF) / 255.0f;
    info.color[2] = ((bgra >> 0) & 0xFF) / 255.0f;
    info.color[3] = 1.0f; // PIX sets alpha to 1

    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdInsertDebugUtilsLabelEXT)
        vk.CmdInsertDebugUtilsLabelEXT(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::BuildTopLevelAccelerationStructures(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    static_assert(sizeof(VkAccelerationStructureInstanceKHR) == sizeof(TopLevelInstance), "Mismatched sizeof");

    Scratch<VkAccelerationStructureBuildGeometryInfoKHR> infos = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureBuildGeometryInfoKHR, buildTopLevelAccelerationStructureDescNum);
    Scratch<const VkAccelerationStructureBuildRangeInfoKHR*> pRanges = NRI_ALLOCATE_SCRATCH(m_Device, const VkAccelerationStructureBuildRangeInfoKHR*, buildTopLevelAccelerationStructureDescNum);
    Scratch<VkAccelerationStructureGeometryKHR> geometries = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureGeometryKHR, buildTopLevelAccelerationStructureDescNum);
    Scratch<VkAccelerationStructureBuildRangeInfoKHR> ranges = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureBuildRangeInfoKHR, buildTopLevelAccelerationStructureDescNum);

    for (uint32_t i = 0; i < buildTopLevelAccelerationStructureDescNum; i++) {
        const BuildTopLevelAccelerationStructureDesc& in = buildTopLevelAccelerationStructureDescs[i];

        AccelerationStructureVK* dst = (AccelerationStructureVK*)in.dst;
        AccelerationStructureVK* src = (AccelerationStructureVK*)in.src;
        BufferVK* scratchBuffer = (BufferVK*)in.scratchBuffer;
        BufferVK* instanceBuffer = (BufferVK*)in.instanceBuffer;

        // Range
        VkAccelerationStructureBuildRangeInfoKHR& range = ranges[i];
        range = {};
        range.primitiveCount = in.instanceNum;

        pRanges[i] = &ranges[i];

        // Geometry
        VkAccelerationStructureGeometryKHR& geometry = geometries[i];
        geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.data.deviceAddress = instanceBuffer->GetDeviceAddress() + in.instanceOffset;

        // Info
        VkAccelerationStructureBuildGeometryInfoKHR& info = infos[i];
        info = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        info.flags = GetBuildAccelerationStructureFlags(dst->GetFlags());
        info.dstAccelerationStructure = dst->GetHandle();
        info.geometryCount = 1;
        info.pGeometries = &geometry;
        info.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + in.scratchOffset;

        if (in.src) {
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            info.srcAccelerationStructure = src->GetHandle();
        } else
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, buildTopLevelAccelerationStructureDescNum, infos, pRanges);
}

NRI_INLINE void CommandBufferVK::BuildBottomLevelAccelerationStructures(const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum) {
    // Count
    uint32_t geometryTotalNum = 0;
    uint32_t micromapTotalNum = 0;

    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& desc = buildBottomLevelAccelerationStructureDescs[i];

        for (uint32_t j = 0; j < desc.geometryNum; j++) {
            const BottomLevelGeometryDesc& geometry = desc.geometries[j];

            if (geometry.type == BottomLevelGeometryType::TRIANGLES && geometry.triangles.micromap)
                micromapTotalNum++;
        }

        geometryTotalNum += desc.geometryNum;
    }

    // Convert
    Scratch<VkAccelerationStructureBuildGeometryInfoKHR> infos = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureBuildGeometryInfoKHR, buildBottomLevelAccelerationStructureDescNum);
    Scratch<const VkAccelerationStructureBuildRangeInfoKHR*> pRanges = NRI_ALLOCATE_SCRATCH(m_Device, const VkAccelerationStructureBuildRangeInfoKHR*, buildBottomLevelAccelerationStructureDescNum);
    Scratch<VkAccelerationStructureGeometryKHR> geometriesScratch = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureGeometryKHR, geometryTotalNum);
    Scratch<VkAccelerationStructureBuildRangeInfoKHR> rangesScratch = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureBuildRangeInfoKHR, geometryTotalNum);
    Scratch<VkAccelerationStructureTrianglesOpacityMicromapEXT> trianglesMicromapsScratch = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureTrianglesOpacityMicromapEXT, micromapTotalNum);

    VkAccelerationStructureBuildRangeInfoKHR* ranges = rangesScratch;
    VkAccelerationStructureGeometryKHR* geometries = geometriesScratch;
    VkAccelerationStructureTrianglesOpacityMicromapEXT* trianglesMicromaps = trianglesMicromapsScratch;

    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& in = buildBottomLevelAccelerationStructureDescs[i];

        // Fill ranges and geometries
        pRanges[i] = ranges;

        uint32_t micromapNum = ConvertBottomLevelGeometries(ranges, geometries, trianglesMicromaps, in.geometries, in.geometryNum);

        // Fill info
        AccelerationStructureVK* dst = (AccelerationStructureVK*)in.dst;
        AccelerationStructureVK* src = (AccelerationStructureVK*)in.src;

        BufferVK* scratchBuffer = (BufferVK*)in.scratchBuffer;

        VkAccelerationStructureBuildGeometryInfoKHR& info = infos[i];
        info = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        info.flags = GetBuildAccelerationStructureFlags(dst->GetFlags());
        info.dstAccelerationStructure = dst->GetHandle();
        info.geometryCount = in.geometryNum;
        info.pGeometries = geometries;
        info.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + in.scratchOffset;

        if (in.src) {
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            info.srcAccelerationStructure = src->GetHandle();
        } else
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

        // Increment
        ranges += in.geometryNum;
        geometries += in.geometryNum;
        trianglesMicromaps += micromapNum;
    }

    // Build
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, buildBottomLevelAccelerationStructureDescNum, infos, pRanges);
}

NRI_INLINE void CommandBufferVK::BuildMicromaps(const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum) {
    static_assert(sizeof(MicromapTriangle) == sizeof(VkMicromapTriangleEXT), "Mismatched sizeof");

    Scratch<VkMicromapBuildInfoEXT> infos = NRI_ALLOCATE_SCRATCH(m_Device, VkMicromapBuildInfoEXT, buildMicromapDescNum);
    for (uint32_t i = 0; i < buildMicromapDescNum; i++) {
        const BuildMicromapDesc& in = buildMicromapDescs[i];

        MicromapVK* dst = (MicromapVK*)in.dst;
        BufferVK* scratchBuffer = (BufferVK*)in.scratchBuffer;
        BufferVK* triangleBuffer = (BufferVK*)in.triangleBuffer;
        BufferVK* dataBuffer = (BufferVK*)in.dataBuffer;

        VkMicromapBuildInfoEXT& out = infos[i];
        out = {VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT};
        out.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
        out.flags = GetBuildMicromapFlags(dst->GetFlags());
        out.mode = VK_BUILD_MICROMAP_MODE_BUILD_EXT;
        out.dstMicromap = dst->GetHandle();
        out.usageCountsCount = dst->GetUsageNum();
        out.pUsageCounts = dst->GetUsages();
        out.data.deviceAddress = dataBuffer->GetDeviceAddress() + in.dataOffset;
        out.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + in.scratchOffset;
        out.triangleArray.deviceAddress = triangleBuffer->GetDeviceAddress() + in.triangleOffset;
        out.triangleArrayStride = sizeof(MicromapTriangle);
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildMicromapsEXT(m_Handle, buildMicromapDescNum, infos);
}

NRI_INLINE void CommandBufferVK::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    VkAccelerationStructureKHR dstHandle = ((AccelerationStructureVK&)dst).GetHandle();
    VkAccelerationStructureKHR srcHandle = ((AccelerationStructureVK&)src).GetHandle();

    VkCopyAccelerationStructureInfoKHR info = {VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
    info.src = srcHandle;
    info.dst = dstHandle;
    info.mode = GetAccelerationStructureCopyMode(copyMode);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyAccelerationStructureKHR(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::CopyMicromap(Micromap& dst, const Micromap& src, CopyMode copyMode) {
    VkMicromapEXT dstHandle = ((MicromapVK&)dst).GetHandle();
    VkMicromapEXT srcHandle = ((MicromapVK&)src).GetHandle();

    VkCopyMicromapInfoEXT info = {VK_STRUCTURE_TYPE_COPY_MICROMAP_INFO_EXT};
    info.src = srcHandle;
    info.dst = dstHandle;
    info.mode = GetMicromapCopyMode(copyMode);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyMicromapEXT(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::WriteAccelerationStructureSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<VkAccelerationStructureKHR> handles = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureKHR, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        handles[i] = ((AccelerationStructureVK*)accelerationStructures[i])->GetHandle();

    const QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteAccelerationStructuresPropertiesKHR(m_Handle, accelerationStructureNum, handles, queryPoolVK.GetType(), queryPoolVK.GetHandle(), queryPoolOffset);
}

NRI_INLINE void CommandBufferVK::WriteMicromapSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<VkMicromapEXT> handles = NRI_ALLOCATE_SCRATCH(m_Device, VkMicromapEXT, micromapNum);
    for (uint32_t i = 0; i < micromapNum; i++)
        handles[i] = ((MicromapVK*)micromaps[i])->GetHandle();

    const QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteMicromapsPropertiesEXT(m_Handle, micromapNum, handles, queryPoolVK.GetType(), queryPoolVK.GetHandle(), queryPoolOffset);
}

NRI_INLINE void CommandBufferVK::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    VkStridedDeviceAddressRegionKHR raygen = {};
    raygen.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.raygenShaderRecord.buffer, dispatchRaysDesc.raygenShaderRecord.offset);
    raygen.size = dispatchRaysDesc.raygenShaderRecord.size;
    raygen.stride = dispatchRaysDesc.raygenShaderRecord.stride;

    VkStridedDeviceAddressRegionKHR miss = {};
    miss.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.missShaderBindingTable.buffer, dispatchRaysDesc.missShaderBindingTable.offset);
    miss.size = dispatchRaysDesc.missShaderBindingTable.size;
    miss.stride = dispatchRaysDesc.missShaderBindingTable.stride;

    VkStridedDeviceAddressRegionKHR hit = {};
    hit.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.hitShaderBindingTable.buffer, dispatchRaysDesc.hitShaderBindingTable.offset);
    hit.size = dispatchRaysDesc.hitShaderBindingTable.size;
    hit.stride = dispatchRaysDesc.hitShaderBindingTable.stride;

    VkStridedDeviceAddressRegionKHR callable = {};
    callable.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.callableShaderBindingTable.buffer, dispatchRaysDesc.callableShaderBindingTable.offset);
    callable.size = dispatchRaysDesc.callableShaderBindingTable.size;
    callable.stride = dispatchRaysDesc.callableShaderBindingTable.stride;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdTraceRaysKHR(m_Handle, &raygen, &miss, &hit, &callable, dispatchRaysDesc.width, dispatchRaysDesc.height, dispatchRaysDesc.depth);
}

NRI_INLINE void CommandBufferVK::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchRaysIndirectDesc) == sizeof(VkTraceRaysIndirectCommand2KHR));

    VkDeviceAddress deviceAddress = GetBufferDeviceAddress(&buffer, offset);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdTraceRaysIndirect2KHR(m_Handle, deviceAddress);
}

NRI_INLINE void CommandBufferVK::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDrawMeshTasksEXT(m_Handle, drawMeshTasksDesc.x, drawMeshTasksDesc.y, drawMeshTasksDesc.z);
}

NRI_INLINE void CommandBufferVK::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    static_assert(sizeof(DrawMeshTasksDesc) == sizeof(VkDrawMeshTasksIndirectCommandEXT));

    const BufferVK& bufferVK = (BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const BufferVK& countBufferVK = *(BufferVK*)countBuffer;
        vk.CmdDrawMeshTasksIndirectCountEXT(m_Handle, bufferVK.GetHandle(), offset, countBufferVK.GetHandle(), countBufferOffset, drawNum, stride);
    } else
        vk.CmdDrawMeshTasksIndirectEXT(m_Handle, bufferVK.GetHandle(), offset, drawNum, stride);
}
