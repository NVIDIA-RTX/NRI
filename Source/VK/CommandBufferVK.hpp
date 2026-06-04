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

static inline void FillRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, const AttachmentDesc& attachmentDesc, Dim_t& renderWidth, Dim_t& renderHeight, Dim_t& layerNum) {
    const DescriptorVK& descriptorVK = *(DescriptorVK*)attachmentDesc.descriptor;
    const TexViewDesc& texViewDesc = descriptorVK.GetTexViewDesc();

    attachmentInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    attachmentInfo.imageView = descriptorVK.GetImageView();
    attachmentInfo.imageLayout = texViewDesc.expectedLayout;
    attachmentInfo.loadOp = GetLoadOp(attachmentDesc.loadOp);
    attachmentInfo.storeOp = GetStoreOp(attachmentDesc.storeOp);
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

static inline StdVideoH264PictureType GetVideoEncodeH264PictureTypeVK(VideoEncodeFrameType frameType) {
    switch (frameType) {
        case VideoEncodeFrameType::IDR:
            return STD_VIDEO_H264_PICTURE_TYPE_IDR;
        case VideoEncodeFrameType::I:
            return STD_VIDEO_H264_PICTURE_TYPE_I;
        case VideoEncodeFrameType::P:
            return STD_VIDEO_H264_PICTURE_TYPE_P;
        case VideoEncodeFrameType::B:
            return STD_VIDEO_H264_PICTURE_TYPE_B;
        case VideoEncodeFrameType::MAX_NUM:
            return STD_VIDEO_H264_PICTURE_TYPE_INVALID;
    }

    return STD_VIDEO_H264_PICTURE_TYPE_INVALID;
}

static inline StdVideoH265PictureType GetVideoEncodeH265PictureTypeVK(VideoEncodeFrameType frameType) {
    switch (frameType) {
        case VideoEncodeFrameType::IDR:
            return STD_VIDEO_H265_PICTURE_TYPE_IDR;
        case VideoEncodeFrameType::I:
            return STD_VIDEO_H265_PICTURE_TYPE_I;
        case VideoEncodeFrameType::P:
            return STD_VIDEO_H265_PICTURE_TYPE_P;
        case VideoEncodeFrameType::B:
            return STD_VIDEO_H265_PICTURE_TYPE_B;
        case VideoEncodeFrameType::MAX_NUM:
            return STD_VIDEO_H265_PICTURE_TYPE_INVALID;
    }

    return STD_VIDEO_H265_PICTURE_TYPE_INVALID;
}

static inline StdVideoAV1FrameType GetVideoEncodeAV1FrameTypeVK(VideoEncodeFrameType frameType) {
    switch (frameType) {
        case VideoEncodeFrameType::IDR:
        case VideoEncodeFrameType::I:
            return STD_VIDEO_AV1_FRAME_TYPE_KEY;
        case VideoEncodeFrameType::P:
        case VideoEncodeFrameType::B:
            return STD_VIDEO_AV1_FRAME_TYPE_INTER;
        case VideoEncodeFrameType::MAX_NUM:
            return STD_VIDEO_AV1_FRAME_TYPE_INVALID;
    }

    return STD_VIDEO_AV1_FRAME_TYPE_INVALID;
}

static inline bool HasVideoEncodeReferenceSlot(const VideoEncodeDesc& videoEncodeDesc, uint32_t slot) {
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        if (videoEncodeDesc.references[i].slot == slot)
            return true;
    }

    return false;
}

static inline const VideoH264ReferenceDesc* FindVideoEncodeH264ReferenceDesc(const VideoH264PictureDesc* h264PictureDesc, uint32_t slot) {
    if (!h264PictureDesc)
        return nullptr;

    for (uint32_t i = 0; i < h264PictureDesc->referenceNum; i++) {
        if (h264PictureDesc->references[i].slot == slot)
            return &h264PictureDesc->references[i];
    }

    return nullptr;
}

static inline const VideoAV1ReferenceDesc* FindVideoEncodeAV1ReferenceDesc(const VideoAV1PictureDesc* av1PictureDesc, uint32_t slot) {
    if (!av1PictureDesc)
        return nullptr;

    for (uint32_t i = 0; i < av1PictureDesc->referenceNum; i++) {
        if (av1PictureDesc->references[i].slot == slot)
            return &av1PictureDesc->references[i];
    }

    return nullptr;
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

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferVK::End() {
    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.EndCommandBuffer(m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkEndCommandBuffer");

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferVK::DecodeVideo(const VideoDecodeVKDesc& videoDecodeVKDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDecodeVideoKHR(m_Handle, (const VkVideoDecodeInfoKHR*)videoDecodeVKDesc.vkDecodeInfo);
}

NRI_INLINE void CommandBufferVK::DecodeVideo(const VideoDecodeDesc& videoDecodeDesc) {
    VideoSessionVK& session = *(VideoSessionVK*)videoDecodeDesc.session;
    VideoSessionParametersVK& parameters = *(VideoSessionParametersVK*)videoDecodeDesc.parameters;
    if (parameters.m_Session != &session) {
        NRI_REPORT_ERROR(&m_Device, "'parameters' must belong to 'session'");
        return;
    }

    BufferVK& bitstream = *(BufferVK*)videoDecodeDesc.bitstream.buffer;
    if (videoDecodeDesc.bitstream.offset >= bitstream.GetDesc().size || videoDecodeDesc.bitstream.size > bitstream.GetDesc().size - videoDecodeDesc.bitstream.offset) {
        NRI_REPORT_ERROR(&m_Device, "'bitstream' range is outside of 'bitstream.buffer'");
        return;
    }
    if (!IsAligned(videoDecodeDesc.bitstream.offset, session.m_BitstreamOffsetAlignment) || !IsAligned(videoDecodeDesc.bitstream.size, session.m_BitstreamSizeAlignment)) {
        NRI_REPORT_ERROR(&m_Device, "'bitstream.offset' and 'bitstream.size' must satisfy Vulkan video alignment requirements: offset=%u, size=%u", session.m_BitstreamOffsetAlignment,
            session.m_BitstreamSizeAlignment);
        return;
    }
    if (videoDecodeDesc.referenceNum > session.m_Desc.maxReferenceNum) {
        NRI_REPORT_ERROR(&m_Device, "'referenceNum' exceeds the session DPB slot count");
        return;
    }
    for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
        if (videoDecodeDesc.references[i].slot > session.m_Desc.maxReferenceNum) {
            NRI_REPORT_ERROR(&m_Device, "'references[%u].slot' exceeds the session DPB slot count", i);
            return;
        }
    }

    const uint32_t referenceScratchNum = videoDecodeDesc.referenceNum ? videoDecodeDesc.referenceNum : 1;
    Scratch<VkVideoReferenceSlotInfoKHR> referenceSlots = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoReferenceSlotInfoKHR, videoDecodeDesc.referenceNum + 1);
    Scratch<StdVideoDecodeH264ReferenceInfo> h264StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoDecodeH264ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoDecodeH264DpbSlotInfoKHR> h264References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoDecodeH264DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoDecodeH265ReferenceInfo> h265StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoDecodeH265ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoDecodeH265DpbSlotInfoKHR> h265References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoDecodeH265DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoDecodeAV1ReferenceInfo> av1StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoDecodeAV1ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoDecodeAV1DpbSlotInfoKHR> av1References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoDecodeAV1DpbSlotInfoKHR, referenceScratchNum);
    for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
        if (!videoDecodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&m_Device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureVK& picture = *(VideoPictureVK*)videoDecodeDesc.references[i].picture;
        referenceSlots[i] = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
        referenceSlots[i].slotIndex = videoDecodeDesc.references[i].slot;
        referenceSlots[i].pPictureResource = &picture.m_Resource;
        if (session.m_Desc.codec == VideoCodec::H264) {
            const VideoH264DecodePictureDesc* h264PictureDesc = videoDecodeDesc.h264PictureDesc;
            const VideoH264DecodeReferenceDesc* referenceDesc = h264PictureDesc ? FindVideoH264DecodeReferenceDescVK(h264PictureDesc->references, h264PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->references' must include metadata for each H.264 reference");
                return;
            }

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
        } else if (session.m_Desc.codec == VideoCodec::H265) {
            const VideoH265DecodePictureDesc* h265PictureDesc = videoDecodeDesc.h265PictureDesc;
            const VideoH265ReferenceDesc* referenceDesc = h265PictureDesc ? FindVideoH265ReferenceDescVK(h265PictureDesc->references, h265PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            h265StdReferences[i] = {};
            h265StdReferences[i].flags.used_for_long_term_reference = referenceDesc && referenceDesc->longTerm;
            h265StdReferences[i].PicOrderCntVal = referenceDesc ? referenceDesc->pictureOrderCount : (int32_t)videoDecodeDesc.references[i].slot;
            h265References[i] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR};
            h265References[i].pStdReferenceInfo = &h265StdReferences[i];
            referenceSlots[i].pNext = &h265References[i];
        } else if (session.m_Desc.codec == VideoCodec::AV1) {
            const VideoAV1DecodePictureDesc* av1PictureDesc = videoDecodeDesc.av1PictureDesc;
            const VideoAV1ReferenceDesc* referenceDesc = av1PictureDesc ? FindVideoAV1ReferenceDescVK(av1PictureDesc->references, av1PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references' must include metadata for each AV1 reference");
                return;
            }

            FillVideoDecodeAV1ReferenceInfoVK(av1StdReferences[i], referenceDesc->frameType, referenceDesc->orderHint, referenceDesc->savedOrderHints);
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
    Scratch<uint32_t> av1TileOffsets = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    Scratch<uint32_t> av1TileSizes = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    Scratch<uint32_t> h264SliceOffsets = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.h264PictureDesc ? std::max(videoDecodeDesc.h264PictureDesc->sliceOffsetNum, 1u) : 1u);
    Scratch<uint32_t> h265SliceSegmentOffsets = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoDecodeDesc.h265PictureDesc ? std::max(videoDecodeDesc.h265PictureDesc->sliceSegmentOffsetNum, 1u) : 1u);
    Scratch<uint16_t> av1MiColStarts = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum + 1, 2u) : 2u);
    Scratch<uint16_t> av1MiRowStarts = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum + 1, 2u) : 2u);
    Scratch<uint16_t> av1WidthInSbsMinus1 = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    Scratch<uint16_t> av1HeightInSbsMinus1 = NRI_ALLOCATE_SCRATCH(m_Device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    void* codecPictureInfo = nullptr;
    const void* setupReferenceInfo = nullptr;
    if (session.m_Desc.codec == VideoCodec::H264) {
        if (!videoDecodeDesc.h264PictureDesc) {
            NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc' must be valid for H.264 decode sessions");
            return;
        }

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

        if (desc.flags & VideoH264DecodePictureBits::REFERENCE) {
            h264StdReference.FrameNum = desc.frameNum;
            h264StdReference.PicOrderCnt[0] = desc.topFieldOrderCount;
            h264StdReference.PicOrderCnt[1] = desc.bottomFieldOrderCount;
            h264DpbSlot.pStdReferenceInfo = &h264StdReference;
            setupReferenceInfo = &h264DpbSlot;
        }
    } else if (session.m_Desc.codec == VideoCodec::H265) {
        if (!videoDecodeDesc.h265PictureDesc) {
            NRI_REPORT_ERROR(&m_Device, "'h265PictureDesc' must be valid for H.265 decode sessions");
            return;
        }

        const VideoH265DecodePictureDesc& desc = *videoDecodeDesc.h265PictureDesc;
        if (desc.referenceNum != 0 && !desc.references) {
            NRI_REPORT_ERROR(&m_Device, "'h265PictureDesc->references' is NULL");
            return;
        }
        if (desc.referenceNum > STD_VIDEO_DECODE_H265_REF_PIC_SET_LIST_SIZE) {
            NRI_REPORT_ERROR(&m_Device, "'h265PictureDesc->referenceNum' exceeds the H.265 reference picture set list size");
            return;
        }

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
            else if (reference.pictureOrderCount <= desc.pictureOrderCount)
                h265StdPicture.RefPicSetStCurrBefore[beforeNum++] = slot;
            else
                h265StdPicture.RefPicSetStCurrAfter[afterNum++] = slot;
        }

        for (uint32_t i = 0; i < desc.sliceSegmentOffsetNum; i++)
            h265SliceSegmentOffsets[i] = desc.sliceSegmentOffsets[i] + 4;

        h265Picture.pStdPictureInfo = &h265StdPicture;
        h265Picture.sliceSegmentCount = desc.sliceSegmentOffsetNum;
        h265Picture.pSliceSegmentOffsets = h265SliceSegmentOffsets;
        codecPictureInfo = &h265Picture;

        if (desc.flags & VideoH265DecodePictureBits::REFERENCE) {
            h265StdReference.PicOrderCntVal = desc.pictureOrderCount;
            h265DpbSlot.pStdReferenceInfo = &h265StdReference;
            setupReferenceInfo = &h265DpbSlot;
        }
    } else if (session.m_Desc.codec == VideoCodec::AV1) {
        if (!videoDecodeDesc.av1PictureDesc) {
            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' must be valid for AV1 decode sessions");
            return;
        }

        const VideoAV1DecodePictureDesc& desc = *videoDecodeDesc.av1PictureDesc;
        if ((desc.tileNum != 0 && !desc.tiles) || desc.referenceNum > 8 || (desc.referenceNum != 0 && !desc.references)) {
            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' contains invalid tile or reference data");
            return;
        }
        if (desc.tileLayout && (!desc.tileLayout->columnNum || !desc.tileLayout->rowNum || !desc.tileLayout->miColumnStarts || !desc.tileLayout->miRowStarts || !desc.tileLayout->widthInSuperblocksMinus1 || !desc.tileLayout->heightInSuperblocksMinus1)) {
            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->tileLayout' is invalid");
            return;
        }

        for (int32_t& slotIndex : av1Picture.referenceNameSlotIndices)
            slotIndex = -1;

        const VideoAV1PictureBits pictureFlags = desc.flags == VideoAV1PictureBits::NONE ? GetDefaultVideoAV1PictureFlagsVK() : desc.flags;
        VideoDecodeAV1ReferenceMappingVK referenceMapping = {};
        if (!BuildVideoDecodeAV1ReferenceMappingVK(desc, referenceMapping)) {
            if (referenceMapping.invalidName)
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u].name' or 'av1PictureDesc->primaryReferenceName' is invalid", referenceMapping.failingReference);
            else if (referenceMapping.invalidRefFrameIndex)
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u].refFrameIndex' is invalid", referenceMapping.failingReference);
            else if (referenceMapping.missingPrimaryReference)
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->primaryReferenceName' does not name an active reference");
            else
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->referenceNum' exceeds AV1 DPB slot count");
            return;
        }

        FillVideoDecodeAV1PictureInfoVK(av1StdPicture, desc, pictureFlags);
        for (uint32_t i = 0; i < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR; i++)
            av1Picture.referenceNameSlotIndices[i] = referenceMapping.referenceNameSlotIndices[i];

        FillVideoAV1DefaultTileInfoVK(av1TileInfo, av1MiColStarts, av1MiRowStarts, av1WidthInSbsMinus1, av1HeightInSbsMinus1,
            session.m_Desc.width, session.m_Desc.height);
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
        FillVideoDecodeAV1QuantizationVK(av1Quantization, desc);
        FillVideoDecodeAV1LoopFilterVK(av1LoopFilter, desc);
        FillVideoDecodeAV1CdefVK(av1Cdef, desc);
        if (desc.segmentation) {
            std::memcpy(av1Segmentation.FeatureEnabled, desc.segmentation->featureEnabled, sizeof(av1Segmentation.FeatureEnabled));
            std::memcpy(av1Segmentation.FeatureData, desc.segmentation->featureData, sizeof(av1Segmentation.FeatureData));
        }
        FillVideoDecodeAV1LoopRestorationVK(av1LoopRestoration, desc);
        FillVideoDecodeAV1GlobalMotionVK(av1GlobalMotion, desc);
        if (desc.filmGrain)
            FillVideoDecodeAV1FilmGrainVK(av1FilmGrain, *desc.filmGrain);
        av1StdPicture.pTileInfo = &av1TileInfo;
        av1StdPicture.pQuantization = &av1Quantization;
        av1StdPicture.pSegmentation = &av1Segmentation;
        av1StdPicture.pLoopFilter = &av1LoopFilter;
        av1StdPicture.pCDEF = &av1Cdef;
        av1StdPicture.pLoopRestoration = &av1LoopRestoration;
        av1StdPicture.pGlobalMotion = &av1GlobalMotion;
        av1StdPicture.pFilmGrain = (pictureFlags & VideoAV1PictureBits::APPLY_GRAIN) && desc.filmGrain ? &av1FilmGrain : nullptr;

        av1Picture.pStdPictureInfo = &av1StdPicture;
#if defined(VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_INLINE_SESSION_PARAMETERS_INFO_KHR)
        if (session.m_UseInlineSessionParameters) {
            av1InlineSessionParameters.pStdSequenceHeader = &parameters.m_AV1SequenceHeader;
            av1Picture.pNext = &av1InlineSessionParameters;
        }
#endif
        FillVideoDecodeAV1TilePayloadVK(av1Picture, desc, av1TileOffsets, av1TileSizes);
        codecPictureInfo = &av1Picture;

        if (desc.refreshFrameFlags) {
            FillVideoDecodeAV1SetupReferenceInfoVK(av1StdReference, desc, pictureFlags);
            av1DpbSlot.pStdReferenceInfo = &av1StdReference;
            setupReferenceInfo = &av1DpbSlot;
        }
    }

    VideoPictureVK& dstPicture = *(VideoPictureVK*)videoDecodeDesc.dstPicture;
    VideoPictureVK& setupPicture = videoDecodeDesc.setupPicture ? *(VideoPictureVK*)videoDecodeDesc.setupPicture : dstPicture;

    VkVideoReferenceSlotInfoKHR setupReferenceSlot = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
    setupReferenceSlot.pNext = setupReferenceInfo;
    uint32_t setupReferenceSlotIndex = videoDecodeDesc.dstSlot;
    if (session.m_Desc.codec == VideoCodec::H264 && videoDecodeDesc.h264PictureDesc && videoDecodeDesc.h264PictureDesc->referenceSlot)
        setupReferenceSlotIndex = videoDecodeDesc.h264PictureDesc->referenceSlot;
    if (setupReferenceInfo && setupReferenceSlotIndex > session.m_Desc.maxReferenceNum) {
        NRI_REPORT_ERROR(&m_Device, "The setup reference slot exceeds the session DPB slot count");
        return;
    }
    setupReferenceSlot.slotIndex = setupReferenceInfo ? (int32_t)setupReferenceSlotIndex : -1;
    setupReferenceSlot.pPictureResource = &setupPicture.m_Resource;

    VkVideoBeginCodingInfoKHR beginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
    const VkVideoSessionParametersKHR sessionParameters = session.m_UseInlineSessionParameters ? VK_NULL_HANDLE : parameters.m_Handle;
    beginInfo.videoSession = session.m_Handle;
    beginInfo.videoSessionParameters = sessionParameters;
    beginInfo.referenceSlotCount = videoDecodeDesc.referenceNum;
    beginInfo.pReferenceSlots = referenceSlots;
    if (setupReferenceInfo) {
        referenceSlots[beginInfo.referenceSlotCount] = setupReferenceSlot;
        referenceSlots[beginInfo.referenceSlotCount].slotIndex = -1;
        beginInfo.referenceSlotCount++;
    }

    VkVideoDecodeInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR};
    decodeInfo.pNext = codecPictureInfo;
    decodeInfo.srcBuffer = bitstream.GetHandle();
    decodeInfo.srcBufferOffset = videoDecodeDesc.bitstream.offset;
    decodeInfo.srcBufferRange = videoDecodeDesc.bitstream.size;
    decodeInfo.dstPictureResource = dstPicture.m_Resource;
    decodeInfo.pSetupReferenceSlot = setupReferenceInfo ? &setupReferenceSlot : nullptr;
    decodeInfo.referenceSlotCount = videoDecodeDesc.referenceNum;
    decodeInfo.pReferenceSlots = referenceSlots;

    const auto& vk = m_Device.GetDispatchTable();
    VkVideoEndCodingInfoKHR endInfo = {VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR};
    if (!session.m_ResetRecorded) {
        VkVideoBeginCodingInfoKHR resetBeginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
        resetBeginInfo.videoSession = session.m_Handle;
        resetBeginInfo.videoSessionParameters = sessionParameters;
        VkVideoCodingControlInfoKHR controlInfo = {VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR};
        controlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;
        vk.CmdBeginVideoCodingKHR(m_Handle, &resetBeginInfo);
        vk.CmdControlVideoCodingKHR(m_Handle, &controlInfo);
        vk.CmdEndVideoCodingKHR(m_Handle, &endInfo);
        session.m_ResetRecorded = true;
    }

    vk.CmdBeginVideoCodingKHR(m_Handle, &beginInfo);
    vk.CmdDecodeVideoKHR(m_Handle, &decodeInfo);
    vk.CmdEndVideoCodingKHR(m_Handle, &endInfo);
}

NRI_INLINE void CommandBufferVK::EncodeVideo(const VideoEncodeVKDesc& videoEncodeVKDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdEncodeVideoKHR(m_Handle, (const VkVideoEncodeInfoKHR*)videoEncodeVKDesc.vkEncodeInfo);
}

NRI_INLINE void CommandBufferVK::EncodeVideo(const VideoEncodeDesc& videoEncodeDesc) {
    VideoSessionVK& session = *(VideoSessionVK*)videoEncodeDesc.session;
    VideoSessionParametersVK& parameters = *(VideoSessionParametersVK*)videoEncodeDesc.parameters;
    if (parameters.m_Session != &session) {
        NRI_REPORT_ERROR(&m_Device, "'parameters' must belong to 'session'");
        return;
    }
    if (videoEncodeDesc.av1PictureDesc && session.m_Desc.codec != VideoCodec::AV1) {
        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' can only be used with AV1 sessions");
        return;
    }
    if (videoEncodeDesc.h264PictureDesc && session.m_Desc.codec != VideoCodec::H264) {
        NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc' can only be used with H.264 sessions");
        return;
    }
    if (videoEncodeDesc.h265ReferenceDescs && session.m_Desc.codec != VideoCodec::H265) {
        NRI_REPORT_ERROR(&m_Device, "'h265ReferenceDescs' can only be used with H.265 sessions");
        return;
    }
    if (videoEncodeDesc.referenceNum > session.m_Desc.maxReferenceNum) {
        NRI_REPORT_ERROR(&m_Device, "'referenceNum' exceeds the session DPB slot count");
        return;
    }
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        if (videoEncodeDesc.references[i].slot > session.m_Desc.maxReferenceNum) {
            NRI_REPORT_ERROR(&m_Device, "'references[%u].slot' exceeds the session DPB slot count", i);
            return;
        }
    }
    if (session.m_Desc.maxReferenceNum != 0 && videoEncodeDesc.reconstructedSlot > session.m_Desc.maxReferenceNum) {
        NRI_REPORT_ERROR(&m_Device, "'reconstructedSlot' exceeds the session DPB slot count");
        return;
    }

    const VideoEncodePictureDesc defaultPicture = {VideoEncodeFrameType::IDR, 0, 0, 0, 0};
    VideoEncodePictureDesc pictureDesc = videoEncodeDesc.pictureDesc ? *videoEncodeDesc.pictureDesc : defaultPicture;
    if (videoEncodeDesc.flags & VideoEncodeBits::FORCE_KEY_FRAME)
        pictureDesc.frameType = VideoEncodeFrameType::IDR;
    const VideoEncodeRateControlDesc defaultRateControl = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 0, 51, 30, 1, 0, 0, 0, 0, 0};
    const VideoEncodeRateControlDesc& rateControlDesc = videoEncodeDesc.rateControlDesc ? *videoEncodeDesc.rateControlDesc : defaultRateControl;
    if ((uint32_t)rateControlDesc.mode >= (uint32_t)VideoEncodeRateControlMode::MAX_NUM || (rateControlDesc.mode != VideoEncodeRateControlMode::CQP && !rateControlDesc.targetBitrate)
        || (rateControlDesc.qpMax && rateControlDesc.qpMin > rateControlDesc.qpMax)) {
        NRI_REPORT_ERROR(&m_Device, "'rateControlDesc' is invalid");
        return;
    }
    if ((session.m_RateControlModes & GetVideoEncodeRateControlModeMask(rateControlDesc.mode)) == 0) {
        NRI_REPORT_ERROR(&m_Device, "Unsupported Vulkan video encode rate control mode");
        return;
    }
    if (!IsVideoEncodeFrameTypeSupportedByVK(session.m_Desc.codec, pictureDesc.frameType)) {
        NRI_REPORT_ERROR(&m_Device, "Vulkan video encode sessions are aligned with the no-B-frame parity target");
        return;
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
    StdVideoAV1CDEF av1Cdef = {};
    StdVideoAV1GlobalMotion av1GlobalMotion = {};
    StdVideoEncodeAV1ReferenceInfo av1StdSetupReference = {};
    VkVideoEncodeAV1DpbSlotInfoKHR av1SetupReference = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_DPB_SLOT_INFO_KHR};
    const VideoAV1TileLayoutDesc* encodeAv1TileLayout = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->tileLayout : nullptr;
    if (encodeAv1TileLayout && (!encodeAv1TileLayout->columnNum || !encodeAv1TileLayout->rowNum || !encodeAv1TileLayout->miColumnStarts || !encodeAv1TileLayout->miRowStarts || !encodeAv1TileLayout->widthInSuperblocksMinus1 || !encodeAv1TileLayout->heightInSuperblocksMinus1)) {
        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->tileLayout' is invalid");
        return;
    }
    std::array<uint16_t, 2> av1MiColStarts = {};
    std::array<uint16_t, 2> av1MiRowStarts = {};
    std::array<uint16_t, 1> av1WidthInSbsMinus1 = {};
    std::array<uint16_t, 1> av1HeightInSbsMinus1 = {};

    const void* codecPictureInfo = nullptr;
    bool isUsedAsReferencePicture = false;
    switch (session.m_Desc.codec) {
        case VideoCodec::H264: {
            for (uint8_t& ref : h264ReferenceLists.RefPicList0)
                ref = STD_VIDEO_H264_NO_REFERENCE_PICTURE;
            for (uint8_t& ref : h264ReferenceLists.RefPicList1)
                ref = STD_VIDEO_H264_NO_REFERENCE_PICTURE;

            if (videoEncodeDesc.referenceNum) {
                const VideoH264PictureDesc* h264PictureDesc = videoEncodeDesc.h264PictureDesc;
                if (!h264PictureDesc) {
                    NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc' must be valid when H.264 encode uses references");
                    return;
                }
                if (h264PictureDesc->referenceNum != videoEncodeDesc.referenceNum) {
                    NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->referenceNum' must match 'referenceNum'");
                    return;
                }

                uint8_t list0Num = 0;
                uint8_t list1Num = 0;
                for (uint32_t i = 0; i < h264PictureDesc->referenceNum; i++) {
                    const VideoH264ReferenceDesc& reference = h264PictureDesc->references[i];
                    if (!HasVideoEncodeReferenceSlot(videoEncodeDesc, reference.slot)) {
                        NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->references[%u].slot' is not present in 'references'", i);
                        return;
                    }
                    if (reference.slot > UINT8_MAX) {
                        NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->references[%u].slot' exceeds the H.264 reference list slot range", i);
                        return;
                    }

                    if (reference.listIndex == 0) {
                        if (list0Num >= STD_VIDEO_H264_MAX_NUM_LIST_REF) {
                            NRI_REPORT_ERROR(&m_Device, "H.264 List0 reference count exceeds STD_VIDEO_H264_MAX_NUM_LIST_REF");
                            return;
                        }
                        h264ReferenceLists.RefPicList0[list0Num++] = (uint8_t)reference.slot;
                    } else if (reference.listIndex == 1) {
                        if (list1Num >= STD_VIDEO_H264_MAX_NUM_LIST_REF) {
                            NRI_REPORT_ERROR(&m_Device, "H.264 List1 reference count exceeds STD_VIDEO_H264_MAX_NUM_LIST_REF");
                            return;
                        }
                        h264ReferenceLists.RefPicList1[list1Num++] = (uint8_t)reference.slot;
                    } else {
                        NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->references[%u].listIndex' must be 0 or 1", i);
                        return;
                    }
                }
                h264ReferenceLists.num_ref_idx_l0_active_minus1 = list0Num ? list0Num - 1 : 0;
                h264ReferenceLists.num_ref_idx_l1_active_minus1 = list1Num ? list1Num - 1 : 0;
                h264StdPicture.pRefLists = &h264ReferenceLists;
            }

            h264StdPicture.flags.IdrPicFlag = pictureDesc.frameType == VideoEncodeFrameType::IDR;
            isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceVK(session.m_Desc.codec, session.m_Desc.maxReferenceNum,
                videoEncodeDesc.reconstructedPicture != nullptr, 0);
            h264StdPicture.flags.is_reference = isUsedAsReferencePicture;
            h264StdPicture.flags.no_output_of_prior_pics_flag = pictureDesc.frameType == VideoEncodeFrameType::IDR;
            h264StdPicture.seq_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->sequenceParameterSetId : 0;
            h264StdPicture.pic_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->pictureParameterSetId : 0;
            h264StdPicture.idr_pic_id = pictureDesc.idrPictureId;
            h264StdPicture.primary_pic_type = GetVideoEncodeH264PictureTypeVK(pictureDesc.frameType);
            h264StdPicture.frame_num = pictureDesc.frameIndex;
            h264StdPicture.PicOrderCnt = pictureDesc.pictureOrderCount;
            h264StdPicture.temporal_id = pictureDesc.temporalLayer;
            h264SliceHeader.slice_type = pictureDesc.frameType == VideoEncodeFrameType::B ? STD_VIDEO_H264_SLICE_TYPE_B : (pictureDesc.frameType == VideoEncodeFrameType::P ? STD_VIDEO_H264_SLICE_TYPE_P : STD_VIDEO_H264_SLICE_TYPE_I);
            h264SliceHeader.disable_deblocking_filter_idc = STD_VIDEO_H264_DISABLE_DEBLOCKING_FILTER_IDC_DISABLED;
            h264SliceInfo.constantQp = pictureDesc.frameType == VideoEncodeFrameType::B ? rateControlDesc.qpB : (pictureDesc.frameType == VideoEncodeFrameType::P ? rateControlDesc.qpP : rateControlDesc.qpI);
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
            if (videoEncodeDesc.referenceNum > STD_VIDEO_H265_MAX_NUM_LIST_REF) {
                NRI_REPORT_ERROR(&m_Device, "'referenceNum' exceeds the H.265 reference list size");
                return;
            }

            h265StdPicture.pic_type = GetVideoEncodeH265PictureTypeVK(pictureDesc.frameType);
            h265StdPicture.sps_video_parameter_set_id = 0;
            h265StdPicture.pps_seq_parameter_set_id = 0;
            h265StdPicture.pps_pic_parameter_set_id = 0;
            h265StdPicture.PicOrderCntVal = pictureDesc.pictureOrderCount;
            h265StdPicture.TemporalId = pictureDesc.temporalLayer;
            h265StdPicture.flags.IrapPicFlag = pictureDesc.frameType == VideoEncodeFrameType::IDR || pictureDesc.frameType == VideoEncodeFrameType::I;
            isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceVK(session.m_Desc.codec, session.m_Desc.maxReferenceNum,
                videoEncodeDesc.reconstructedPicture != nullptr, 0);
            h265StdPicture.flags.is_reference = isUsedAsReferencePicture;
            h265StdPicture.flags.pic_output_flag = true;
            h265StdPicture.flags.no_output_of_prior_pics_flag = pictureDesc.frameType == VideoEncodeFrameType::IDR;
            h265StdPicture.flags.short_term_ref_pic_set_sps_flag = false;
            for (uint8_t& entry : h265ReferenceLists.RefPicList0)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            for (uint8_t& entry : h265ReferenceLists.RefPicList1)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            for (uint8_t& entry : h265ReferenceLists.list_entry_l0)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            for (uint8_t& entry : h265ReferenceLists.list_entry_l1)
                entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
            if (videoEncodeDesc.referenceNum) {
                VideoEncodeHEVCReferenceListsVK hevcLists = {};
                if (!BuildVideoEncodeHEVCReferenceListsVK(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, pictureDesc.frameType,
                        pictureDesc.pictureOrderCount, hevcLists)) {
                    if (hevcLists.missingDescriptor)
                        NRI_REPORT_ERROR(&m_Device, "'h265ReferenceDescs' must include an entry for each H.265 reference");
                    else if (hevcLists.invalidPictureOrderCount)
                        NRI_REPORT_ERROR(&m_Device, "'h265ReferenceDescs[%u].pictureOrderCount' is not valid for the current frame type", hevcLists.failingReference);
                    else
                        NRI_REPORT_ERROR(&m_Device, "'referenceNum' exceeds the H.265 reference list size");
                    return;
                }

                const uint32_t list0ReferenceNum = hevcLists.list0Num;
                const uint32_t list1ReferenceNum = hevcLists.list1Num;
                h265ReferenceLists.num_ref_idx_l0_active_minus1 = (uint8_t)(list0ReferenceNum - 1);
                h265ReferenceLists.num_ref_idx_l1_active_minus1 = list1ReferenceNum ? (uint8_t)(list1ReferenceNum - 1) : 0;
                for (uint32_t i = 0; i < list0ReferenceNum; i++) {
                    const uint32_t referenceIndex = hevcLists.list0[i];
                    h265ReferenceLists.RefPicList0[i] = (uint8_t)videoEncodeDesc.references[referenceIndex].slot;
                    h265ReferenceLists.list_entry_l0[i] = (uint8_t)referenceIndex;
                }
                for (uint32_t i = 0; i < list1ReferenceNum; i++) {
                    const uint32_t referenceIndex = hevcLists.list1[i];
                    h265ReferenceLists.RefPicList1[i] = (uint8_t)videoEncodeDesc.references[referenceIndex].slot;
                    h265ReferenceLists.list_entry_l1[i] = (uint8_t)referenceIndex;
                }
                h265StdPicture.pRefLists = &h265ReferenceLists;
                h265ShortTermRefPicSet.num_negative_pics = (uint8_t)list0ReferenceNum;
                h265ShortTermRefPicSet.used_by_curr_pic_s0_flag = (uint16_t)((1u << list0ReferenceNum) - 1u);
                for (uint32_t i = 0; i < list0ReferenceNum; i++) {
                    const uint32_t referenceIndex = hevcLists.list0[i];
                    const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescVK(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[referenceIndex].slot);
                    const int32_t referencePoc = referenceDesc->pictureOrderCount;
                    const int32_t deltaPoc = std::max(1, pictureDesc.pictureOrderCount - referencePoc);
                    h265ShortTermRefPicSet.delta_poc_s0_minus1[i] = (uint16_t)(deltaPoc - 1);
                }
                h265ShortTermRefPicSet.num_positive_pics = (uint8_t)list1ReferenceNum;
                h265ShortTermRefPicSet.used_by_curr_pic_s1_flag = (uint16_t)((1u << list1ReferenceNum) - 1u);
                for (uint32_t i = 0; i < list1ReferenceNum; i++) {
                    const uint32_t referenceIndex = hevcLists.list1[i];
                    const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescVK(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[referenceIndex].slot);
                    const int32_t referencePoc = referenceDesc->pictureOrderCount;
                    const int32_t deltaPoc = std::max(1, referencePoc - pictureDesc.pictureOrderCount);
                    h265ShortTermRefPicSet.delta_poc_s1_minus1[i] = (uint16_t)(deltaPoc - 1);
                }
                h265StdPicture.pShortTermRefPicSet = &h265ShortTermRefPicSet;
            }
            h265SliceHeader.flags.first_slice_segment_in_pic_flag = true;
            h265SliceHeader.flags.slice_sao_luma_flag = true;
            h265SliceHeader.flags.slice_sao_chroma_flag = true;
            h265SliceHeader.flags.num_ref_idx_active_override_flag = videoEncodeDesc.referenceNum != 0;
            h265SliceHeader.slice_type = pictureDesc.frameType == VideoEncodeFrameType::B ? STD_VIDEO_H265_SLICE_TYPE_B : (pictureDesc.frameType == VideoEncodeFrameType::P ? STD_VIDEO_H265_SLICE_TYPE_P : STD_VIDEO_H265_SLICE_TYPE_I);
            h265SliceHeader.MaxNumMergeCand = 5;
            h265SliceInfo.constantQp = GetVideoEncodeQPByFrameTypeVK(rateControlDesc, pictureDesc.frameType);
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
            const VideoAV1PictureDesc* av1PictureDesc = videoEncodeDesc.av1PictureDesc;
            if (!IsVideoEncodeAV1KeyFrameReferenceStateValidVK(pictureDesc.frameType, videoEncodeDesc.referenceNum)) {
                NRI_REPORT_ERROR(&m_Device, "AV1 key frames must not reference previous pictures");
                return;
            }
            av1StdPicture.frame_type = GetVideoEncodeAV1FrameTypeVK(pictureDesc.frameType);
            av1StdPicture.frame_presentation_time = pictureDesc.frameIndex;
            av1StdPicture.current_frame_id = videoEncodeDesc.reconstructedSlot;
            av1StdPicture.order_hint = av1PictureDesc ? av1PictureDesc->orderHint : (uint8_t)pictureDesc.pictureOrderCount;
            av1StdPicture.primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
            av1StdPicture.refresh_frame_flags = av1PictureDesc ? av1PictureDesc->refreshFrameFlags : (pictureDesc.frameType == VideoEncodeFrameType::IDR ? 0xFF : 0);
            av1StdPicture.render_width_minus_1 = (uint16_t)(session.m_Desc.width - 1);
            av1StdPicture.render_height_minus_1 = (uint16_t)(session.m_Desc.height - 1);
            av1StdPicture.interpolation_filter = STD_VIDEO_AV1_INTERPOLATION_FILTER_EIGHTTAP;
            av1StdPicture.TxMode = STD_VIDEO_AV1_TX_MODE_SELECT;
            av1StdPicture.flags.error_resilient_mode = true;
            av1StdPicture.flags.disable_cdf_update = true;
            av1StdPicture.flags.allow_screen_content_tools = true;
            av1StdPicture.flags.force_integer_mv = true;
            av1StdPicture.flags.show_frame = true;
            av1StdPicture.flags.showable_frame = true;
            if (av1PictureDesc && av1PictureDesc->flags != VideoAV1PictureBits::NONE) {
                FillVideoAV1PictureFlagsVK(av1StdPicture.flags, av1PictureDesc->flags);
                av1StdPicture.render_width_minus_1 = av1PictureDesc->renderWidthMinus1 ? av1PictureDesc->renderWidthMinus1 : av1StdPicture.render_width_minus_1;
                av1StdPicture.render_height_minus_1 = av1PictureDesc->renderHeightMinus1 ? av1PictureDesc->renderHeightMinus1 : av1StdPicture.render_height_minus_1;
                av1StdPicture.interpolation_filter = (StdVideoAV1InterpolationFilter)av1PictureDesc->interpolationFilter;
                av1StdPicture.TxMode = (StdVideoAV1TxMode)(av1PictureDesc->txMode ? av1PictureDesc->txMode : STD_VIDEO_AV1_TX_MODE_SELECT);
                av1StdPicture.coded_denom = av1PictureDesc->codedDenom;
                av1StdPicture.delta_q_res = av1PictureDesc->deltaQRes;
                av1StdPicture.delta_lf_res = av1PictureDesc->deltaLfRes;
            }
            for (int8_t& refFrameIndex : av1StdPicture.ref_frame_idx)
                refFrameIndex = -1;
            if (av1StdPicture.frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY) {
                av1StdPicture.primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
                av1StdPicture.refresh_frame_flags = 0xFF;
            } else if (videoEncodeDesc.referenceNum) {
                av1StdPicture.flags.error_resilient_mode = false;
                av1StdPicture.flags.disable_cdf_update = false;
                av1StdPicture.flags.allow_screen_content_tools = false;
                av1StdPicture.flags.force_integer_mv = false;
            }
            av1StdPicture.flags.showable_frame = av1StdPicture.frame_type != STD_VIDEO_AV1_FRAME_TYPE_KEY;
            if (av1StdPicture.refresh_frame_flags && !videoEncodeDesc.reconstructedPicture) {
                NRI_REPORT_ERROR(&m_Device, "AV1 frames that refresh DPB slots require 'reconstructedPicture'");
                return;
            }
            isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceVK(session.m_Desc.codec, session.m_Desc.maxReferenceNum,
                videoEncodeDesc.reconstructedPicture != nullptr, av1StdPicture.refresh_frame_flags);

            if (av1PictureDesc) {
                VideoEncodeAV1ReferenceMappingVK referenceMapping = {};
                if (!BuildVideoEncodeAV1ReferenceMappingVK(videoEncodeDesc.references, videoEncodeDesc.referenceNum, *av1PictureDesc, referenceMapping)) {
                    if (referenceMapping.missingResource)
                        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u].slot' is not present in 'references'", referenceMapping.failingReference);
                    else if (referenceMapping.invalidName)
                        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u].name' is invalid", referenceMapping.failingReference);
                    else if (referenceMapping.missingPrimaryReference)
                        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->primaryReferenceName' does not name an active reference");
                    else
                        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->referenceNum' exceeds AV1 DPB slot count or contains an invalid refFrameIndex");
                    return;
                }

                for (uint32_t i = 0; i < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR; i++) {
                    av1Picture.referenceNameSlotIndices[i] = referenceMapping.referenceNameSlotIndices[i];
                    av1StdPicture.ref_frame_idx[i] = referenceMapping.refFrameIndices[i];
                }
                const uint8_t primaryReferenceIndex = GetVideoAV1ReferenceNameIndexVK(av1PictureDesc->primaryReferenceName);
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
                av1StdPicture.primary_ref_frame = primaryRefFrameIndex >= 0 ? (uint8_t)primaryRefFrameIndex : primaryReferenceIndex;
            } else if (videoEncodeDesc.referenceNum) {
                av1Picture.referenceNameSlotIndices[0] = (int32_t)videoEncodeDesc.references[0].slot;
                av1StdPicture.ref_frame_idx[0] = 0;
                av1StdPicture.primary_ref_frame = 0;
            }
            av1TileInfo.flags.uniform_tile_spacing_flag = true;
            av1TileInfo.TileCols = 1;
            av1TileInfo.TileRows = 1;
            av1TileInfo.tile_size_bytes_minus_1 = 3;
            av1MiColStarts[0] = 0;
            av1MiColStarts[1] = (uint16_t)((session.m_Desc.width + 3) / 4);
            av1MiRowStarts[0] = 0;
            av1MiRowStarts[1] = (uint16_t)((session.m_Desc.height + 3) / 4);
            av1WidthInSbsMinus1[0] = (uint16_t)((session.m_Desc.width + 63) / 64 - 1);
            av1HeightInSbsMinus1[0] = (uint16_t)((session.m_Desc.height + 63) / 64 - 1);
            av1TileInfo.pMiColStarts = av1MiColStarts.data();
            av1TileInfo.pMiRowStarts = av1MiRowStarts.data();
            av1TileInfo.pWidthInSbsMinus1 = av1WidthInSbsMinus1.data();
            av1TileInfo.pHeightInSbsMinus1 = av1HeightInSbsMinus1.data();
            if (encodeAv1TileLayout) {
                av1TileInfo.flags.uniform_tile_spacing_flag = encodeAv1TileLayout->uniformSpacing != 0;
                av1TileInfo.TileCols = encodeAv1TileLayout->columnNum;
                av1TileInfo.TileRows = encodeAv1TileLayout->rowNum;
                av1TileInfo.context_update_tile_id = encodeAv1TileLayout->contextUpdateTileId;
                av1TileInfo.tile_size_bytes_minus_1 = encodeAv1TileLayout->tileSizeBytesMinus1;
                av1TileInfo.pMiColStarts = encodeAv1TileLayout->miColumnStarts;
                av1TileInfo.pMiRowStarts = encodeAv1TileLayout->miRowStarts;
                av1TileInfo.pWidthInSbsMinus1 = encodeAv1TileLayout->widthInSuperblocksMinus1;
                av1TileInfo.pHeightInSbsMinus1 = encodeAv1TileLayout->heightInSuperblocksMinus1;
            }
            av1Quantization.base_q_idx = av1PictureDesc && av1PictureDesc->baseQIndex ? av1PictureDesc->baseQIndex : GetVideoEncodeQPByFrameTypeVK(rateControlDesc, pictureDesc.frameType);
            av1Cdef.cdef_damping_minus_3 = av1PictureDesc && av1PictureDesc->cdefDampingMinus3 ? av1PictureDesc->cdefDampingMinus3 : 3;
            av1Cdef.cdef_bits = av1PictureDesc ? av1PictureDesc->cdefBits : 0;
            av1StdPicture.pTileInfo = &av1TileInfo;
            av1StdPicture.pQuantization = &av1Quantization;
            av1StdPicture.pLoopFilter = &av1LoopFilter;
            av1StdPicture.pCDEF = &av1Cdef;
            av1StdPicture.pGlobalMotion = &av1GlobalMotion;
            const bool hasActiveAv1References = videoEncodeDesc.referenceNum != 0;
            av1Picture.predictionMode = hasActiveAv1References
                ? (pictureDesc.frameType == VideoEncodeFrameType::B ? VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_BIDIRECTIONAL_COMPOUND_KHR : VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_SINGLE_REFERENCE_KHR)
                : VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_INTRA_ONLY_KHR;
            av1Picture.rateControlGroup = hasActiveAv1References
                ? (pictureDesc.frameType == VideoEncodeFrameType::B ? VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_BIPREDICTIVE_KHR : VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_PREDICTIVE_KHR)
                : VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_INTRA_KHR;
            av1Picture.constantQIndex = GetVideoEncodeQPByFrameTypeVK(rateControlDesc, pictureDesc.frameType);
            av1Picture.pStdPictureInfo = &av1StdPicture;
            codecPictureInfo = &av1Picture;

            av1StdSetupReference.RefFrameId = videoEncodeDesc.reconstructedSlot;
            av1StdSetupReference.frame_type = av1StdPicture.frame_type;
            av1StdSetupReference.OrderHint = av1StdPicture.order_hint;
            av1SetupReference.pStdReferenceInfo = &av1StdSetupReference;
            break;
        }
        case VideoCodec::NONE:
        case VideoCodec::MAX_NUM:
            NRI_REPORT_ERROR(&m_Device, "Unsupported video encode codec");
            return;
    }

    Scratch<VkVideoReferenceSlotInfoKHR> referenceSlots = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoReferenceSlotInfoKHR, videoEncodeDesc.referenceNum + 1);
    const uint32_t referenceScratchNum = videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1;
    Scratch<StdVideoEncodeH264ReferenceInfo> h264StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoEncodeH264ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoEncodeH264DpbSlotInfoKHR> h264References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoEncodeH264DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoEncodeH265ReferenceInfo> h265StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoEncodeH265ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoEncodeH265DpbSlotInfoKHR> h265References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoEncodeH265DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoEncodeAV1ReferenceInfo> av1StdReferences = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoEncodeAV1ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoEncodeAV1DpbSlotInfoKHR> av1References = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoEncodeAV1DpbSlotInfoKHR, referenceScratchNum);
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        if (!videoEncodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&m_Device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureVK& picture = *(VideoPictureVK*)videoEncodeDesc.references[i].picture;
        referenceSlots[i] = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
        referenceSlots[i].slotIndex = videoEncodeDesc.references[i].slot;
        referenceSlots[i].pPictureResource = &picture.m_Resource;

        if (session.m_Desc.codec == VideoCodec::H264) {
            const VideoH264ReferenceDesc* referenceDesc = FindVideoEncodeH264ReferenceDesc(videoEncodeDesc.h264PictureDesc, videoEncodeDesc.references[i].slot);
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&m_Device, "'references[%u].slot' is not described by 'h264PictureDesc'", i);
                return;
            }

            h264StdReferences[i] = {};
            h264StdReferences[i].flags.used_for_long_term_reference = referenceDesc->longTermReference != 0;
            h264StdReferences[i].primary_pic_type = GetVideoEncodeH264PictureTypeVK(referenceDesc->frameType);
            h264StdReferences[i].FrameNum = referenceDesc->frameNum;
            h264StdReferences[i].PicOrderCnt = referenceDesc->pictureOrderCount;
            h264StdReferences[i].long_term_pic_num = referenceDesc->longTermPictureIndex;
            h264StdReferences[i].long_term_frame_idx = referenceDesc->longTermFrameIndex;
            h264StdReferences[i].temporal_id = referenceDesc->temporalLayer;
            h264References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_KHR};
            h264References[i].pStdReferenceInfo = &h264StdReferences[i];
            referenceSlots[i].pNext = &h264References[i];
        } else if (session.m_Desc.codec == VideoCodec::H265) {
            const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescVK(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[i].slot);
            h265StdReferences[i] = {};
            h265StdReferences[i].flags.used_for_long_term_reference = referenceDesc && referenceDesc->longTerm;
            h265StdReferences[i].pic_type = referenceDesc ? GetVideoEncodeH265PictureTypeVK(referenceDesc->frameType) : STD_VIDEO_H265_PICTURE_TYPE_P;
            h265StdReferences[i].PicOrderCntVal = referenceDesc ? referenceDesc->pictureOrderCount : (int32_t)videoEncodeDesc.references[i].slot;
            h265StdReferences[i].TemporalId = referenceDesc ? referenceDesc->temporalLayer : 0;
            h265References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_KHR};
            h265References[i].pStdReferenceInfo = &h265StdReferences[i];
            referenceSlots[i].pNext = &h265References[i];
        } else if (session.m_Desc.codec == VideoCodec::AV1) {
            const VideoAV1ReferenceDesc* referenceDesc = FindVideoEncodeAV1ReferenceDesc(videoEncodeDesc.av1PictureDesc, videoEncodeDesc.references[i].slot);
            av1StdReferences[i] = {};
            av1StdReferences[i].frame_type = referenceDesc ? GetVideoEncodeAV1FrameTypeVK(referenceDesc->frameType) : STD_VIDEO_AV1_FRAME_TYPE_KEY;
            av1StdReferences[i].RefFrameId = videoEncodeDesc.references[i].slot;
            av1StdReferences[i].OrderHint = referenceDesc ? referenceDesc->orderHint : 0;
            av1References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_DPB_SLOT_INFO_KHR};
            av1References[i].pStdReferenceInfo = &av1StdReferences[i];
            referenceSlots[i].pNext = &av1References[i];
        }
    }

    VkVideoReferenceSlotInfoKHR setupReferenceSlot = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
    if (isUsedAsReferencePicture) {
        if (session.m_Desc.codec == VideoCodec::H264)
            setupReferenceSlot.pNext = &h264SetupReference;
        else if (session.m_Desc.codec == VideoCodec::H265)
            setupReferenceSlot.pNext = &h265SetupReference;
        else if (session.m_Desc.codec == VideoCodec::AV1)
            setupReferenceSlot.pNext = &av1SetupReference;
    }
    setupReferenceSlot.slotIndex = isUsedAsReferencePicture ? (int32_t)videoEncodeDesc.reconstructedSlot : -1;
    if (isUsedAsReferencePicture) {
        VideoPictureVK& reconstructedPicture = *(VideoPictureVK*)videoEncodeDesc.reconstructedPicture;
        setupReferenceSlot.pPictureResource = &reconstructedPicture.m_Resource;
    }

    VkVideoEncodeInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR};
    encodeInfo.pNext = codecPictureInfo;
    BufferVK& dstBitstream = *(BufferVK*)videoEncodeDesc.dstBitstream.buffer;
    if (videoEncodeDesc.dstBitstream.offset >= dstBitstream.GetDesc().size || videoEncodeDesc.dstBitstream.size > dstBitstream.GetDesc().size - videoEncodeDesc.dstBitstream.offset) {
        NRI_REPORT_ERROR(&m_Device, "'dstBitstream' range is outside of 'dstBitstream.buffer'");
        return;
    }
    if (!IsAligned(videoEncodeDesc.dstBitstream.offset, session.m_BitstreamOffsetAlignment) || !IsAligned(videoEncodeDesc.dstBitstream.size, session.m_BitstreamSizeAlignment)) {
        NRI_REPORT_ERROR(&m_Device, "'dstBitstream.offset' and 'dstBitstream.size' must satisfy Vulkan video alignment requirements: offset=%u, size=%u", session.m_BitstreamOffsetAlignment,
            session.m_BitstreamSizeAlignment);
        return;
    }

    VideoPictureVK& srcPicture = *(VideoPictureVK*)videoEncodeDesc.srcPicture;
    encodeInfo.dstBuffer = dstBitstream.GetHandle();
    encodeInfo.dstBufferOffset = videoEncodeDesc.dstBitstream.offset;
    encodeInfo.dstBufferRange = videoEncodeDesc.dstBitstream.size;
    encodeInfo.srcPictureResource = srcPicture.m_Resource;
    encodeInfo.pSetupReferenceSlot = isUsedAsReferencePicture ? &setupReferenceSlot : nullptr;
    encodeInfo.referenceSlotCount = videoEncodeDesc.referenceNum;
    encodeInfo.pReferenceSlots = referenceSlots;

    if (isUsedAsReferencePicture) {
        referenceSlots[videoEncodeDesc.referenceNum] = GetVideoEncodeSetupReferenceSlotForBeginVK(setupReferenceSlot);
    }

    VkVideoEncodeRateControlInfoKHR rateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeRateControlLayerInfoKHR rateControlLayer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR};
    FillVideoEncodeRateControlVK(rateControlDesc, rateControlInfo, rateControlLayer);

    VkVideoBeginCodingInfoKHR beginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
    beginInfo.pNext = &rateControlInfo;
    beginInfo.videoSession = session.m_Handle;
    beginInfo.videoSessionParameters = parameters.m_Handle;
    beginInfo.referenceSlotCount = videoEncodeDesc.referenceNum + (isUsedAsReferencePicture ? 1 : 0);
    beginInfo.pReferenceSlots = referenceSlots;

    VkVideoEndCodingInfoKHR endInfo = {VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR};
    BufferVK* resolvedMetadata = (BufferVK*)videoEncodeDesc.resolvedMetadata;
    bool useEncodeFeedback = resolvedMetadata != nullptr && session.m_EncodeFeedbackQueryPool != VK_NULL_HANDLE;
    uint32_t encodeFeedbackQueryIndex = UINT32_MAX;
    if (useEncodeFeedback) {
        for (uint32_t i = 0; i < VideoSessionVK::ENCODE_FEEDBACK_QUERY_NUM; i++) {
            const VideoSessionVK::EncodeFeedbackPayloadReadback& payloadReadback = session.m_EncodeFeedbackPayloadReadbacks[i];
            if (payloadReadback.active && payloadReadback.resolvedMetadata == resolvedMetadata && payloadReadback.resolvedMetadataOffset == videoEncodeDesc.resolvedMetadataOffset) {
                NRI_REPORT_ERROR(&m_Device, "A Vulkan video encode feedback query is already pending for this resolved metadata range");
                useEncodeFeedback = false;
                break;
            }
        }

        for (uint32_t i = 0; useEncodeFeedback && i < VideoSessionVK::ENCODE_FEEDBACK_QUERY_NUM; i++) {
            if (!session.m_EncodeFeedbackPayloadReadbacks[i].active) {
                encodeFeedbackQueryIndex = i;
                break;
            }
        }

        if (useEncodeFeedback && encodeFeedbackQueryIndex == UINT32_MAX) {
            NRI_REPORT_ERROR(&m_Device, "Too many unresolved Vulkan video encode feedback queries are outstanding for this video session");
            useEncodeFeedback = false;
        } else if (useEncodeFeedback) {
            VideoSessionVK::EncodeFeedbackPayloadReadback& payloadReadback = session.m_EncodeFeedbackPayloadReadbacks[encodeFeedbackQueryIndex];
            payloadReadback.active = true;
            payloadReadback.resolvedMetadata = resolvedMetadata;
            payloadReadback.resolvedMetadataOffset = videoEncodeDesc.resolvedMetadataOffset;
            payloadReadback.bitstream = (BufferVK*)videoEncodeDesc.dstBitstream.buffer;
            payloadReadback.dstBitstreamOffset = videoEncodeDesc.dstBitstream.offset;
            payloadReadback.size = videoEncodeDesc.dstBitstream.size;
            payloadReadback.copyHeader = videoEncodeDesc.av1PictureDesc != nullptr;
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    if (useEncodeFeedback)
        vk.CmdResetQueryPool(m_Handle, session.m_EncodeFeedbackQueryPool, encodeFeedbackQueryIndex, 1);

    if (!session.m_ResetRecorded) {
        VkVideoBeginCodingInfoKHR initBeginInfo = beginInfo;
        initBeginInfo.pNext = nullptr;

        VkVideoCodingControlInfoKHR controlInfo = {VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR};

        vk.CmdBeginVideoCodingKHR(m_Handle, &initBeginInfo);
        controlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR | VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR;
        controlInfo.pNext = &rateControlInfo;
        vk.CmdControlVideoCodingKHR(m_Handle, &controlInfo);
        vk.CmdEndVideoCodingKHR(m_Handle, &endInfo);
        session.m_ResetRecorded = true;
    }
    vk.CmdBeginVideoCodingKHR(m_Handle, &beginInfo);

    if (useEncodeFeedback)
        vk.CmdBeginQuery(m_Handle, session.m_EncodeFeedbackQueryPool, encodeFeedbackQueryIndex, (VkQueryControlFlags)0);

    vk.CmdEncodeVideoKHR(m_Handle, &encodeInfo);

    if (useEncodeFeedback)
        vk.CmdEndQuery(m_Handle, session.m_EncodeFeedbackQueryPool, encodeFeedbackQueryIndex);

    vk.CmdEndVideoCodingKHR(m_Handle, &endInfo);
}

NRI_INLINE void CommandBufferVK::ResolveVideoEncodeFeedback(VideoSession& videoSession, Buffer& resolvedMetadata, uint64_t resolvedMetadataOffset) {
    VideoSessionVK& session = (VideoSessionVK&)videoSession;
    if (!session.m_EncodeFeedbackQueryPool)
        return;

    BufferVK& feedbackBuffer = (BufferVK&)resolvedMetadata;
    uint32_t encodeFeedbackQueryIndex = UINT32_MAX;
    for (uint32_t i = 0; i < VideoSessionVK::ENCODE_FEEDBACK_QUERY_NUM; i++) {
        const VideoSessionVK::EncodeFeedbackPayloadReadback& payloadReadback = session.m_EncodeFeedbackPayloadReadbacks[i];
        if (payloadReadback.active && payloadReadback.resolvedMetadata == &feedbackBuffer && payloadReadback.resolvedMetadataOffset == resolvedMetadataOffset) {
            encodeFeedbackQueryIndex = i;
            break;
        }
    }

    if (encodeFeedbackQueryIndex == UINT32_MAX) {
        NRI_REPORT_ERROR(&m_Device, "No unresolved Vulkan video encode feedback query is available for this resolved metadata range");
        return;
    }

    const VideoSessionVK::EncodeFeedbackPayloadReadback payloadReadback = session.m_EncodeFeedbackPayloadReadbacks[encodeFeedbackQueryIndex];
    session.m_EncodeFeedbackPayloadReadbacks[encodeFeedbackQueryIndex] = {};

    const auto& vk = m_Device.GetDispatchTable();
    uint64_t queryResult[2] = {};
    VkResult result = vk.GetQueryPoolResults(session.m_Device, session.m_EncodeFeedbackQueryPool, encodeFeedbackQueryIndex, 1, sizeof(queryResult), queryResult, sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_STATUS_BIT_KHR);

    VideoEncodeFeedback feedback = {};
    feedback.encodedBitstreamOffset = 0;
    if (result == VK_SUCCESS) {
        feedback.encodedBitstreamWrittenBytes = queryResult[0];
        feedback.writtenSubregionNum = 1;
        if ((int64_t)queryResult[1] < 0)
            feedback.errorFlags = queryResult[1];
    } else
        feedback.errorFlags = (uint64_t)result;

    vk.CmdUpdateBuffer(m_Handle, feedbackBuffer.GetHandle(), resolvedMetadataOffset, sizeof(feedback), &feedback);

    if (payloadReadback.copyHeader && payloadReadback.bitstream && feedback.encodedBitstreamWrittenBytes) {
        const uint64_t dstOffset = resolvedMetadataOffset + sizeof(VideoEncodeFeedback);
        if (dstOffset < feedbackBuffer.GetDesc().size) {
            VkBufferCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
            region.srcOffset = payloadReadback.dstBitstreamOffset;
            region.dstOffset = dstOffset;
            region.size = std::min(feedback.encodedBitstreamWrittenBytes, std::min(payloadReadback.size, feedbackBuffer.GetDesc().size - dstOffset));

            if (region.size) {
                VkCopyBufferInfo2 copyInfo = {VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2};
                copyInfo.srcBuffer = payloadReadback.bitstream->GetHandle();
                copyInfo.dstBuffer = feedbackBuffer.GetHandle();
                copyInfo.regionCount = 1;
                copyInfo.pRegions = &region;

                vk.CmdCopyBuffer2(m_Handle, &copyInfo);
            }
        }
    }
}

NRI_INLINE void CommandBufferVK::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    Scratch<VkViewport> vkViewports = NRI_ALLOCATE_SCRATCH(m_Device, VkViewport, viewportNum);
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

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetViewportWithCount(m_Handle, viewportNum, vkViewports);
}

NRI_INLINE void CommandBufferVK::SetScissors(const Rect* rects, uint32_t rectNum) {
    Scratch<VkRect2D> vkRects = NRI_ALLOCATE_SCRATCH(m_Device, VkRect2D, rectNum);
    for (uint32_t i = 0; i < rectNum; i++) {
        const Rect& in = rects[i];
        VkRect2D& out = vkRects[i];
        out.offset.x = in.x;
        out.offset.y = in.y;
        out.extent.width = in.width;
        out.extent.height = in.height;
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetScissorWithCount(m_Handle, rectNum, vkRects);
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

    Scratch<VkRenderingAttachmentInfo> colors = NRI_ALLOCATE_SCRATCH(m_Device, VkRenderingAttachmentInfo, renderingDesc.colorNum);

    VkRenderingInfo renderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderingInfo.viewMask = renderingDesc.viewMask;
    renderingInfo.colorAttachmentCount = renderingDesc.colorNum;
    renderingInfo.pColorAttachments = colors;

    for (uint32_t i = 0; i < renderingDesc.colorNum; i++)
        FillRenderingAttachmentInfo(colors[i], renderingDesc.colors[i], renderWidth, renderHeight, renderLayerNum);

    VkRenderingAttachmentInfo depth = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    if (renderingDesc.depth.descriptor) {
        m_DepthStencil = (DescriptorVK*)renderingDesc.depth.descriptor;

        FillRenderingAttachmentInfo(depth, renderingDesc.depth, renderWidth, renderHeight, renderLayerNum);
        renderingInfo.pDepthAttachment = &depth;

        const FormatProps& formatProps = GetFormatProps(m_DepthStencil->GetFormat());
        if (formatProps.isStencil)
            renderingInfo.pStencilAttachment = &depth;
    } else
        m_DepthStencil = nullptr;

    VkRenderingAttachmentInfo stencil = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    if (renderingDesc.stencil.descriptor) { // it's safe to do it this way, since there are no "stencil-only" formats
        m_DepthStencil = (DescriptorVK*)renderingDesc.stencil.descriptor;

        FillRenderingAttachmentInfo(stencil, renderingDesc.stencil, renderWidth, renderHeight, renderLayerNum);
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

    m_RenderWidth = renderWidth;
    m_RenderHeight = renderHeight;
    m_RenderLayerNum = renderLayerNum;
    m_ViewMask = renderingDesc.viewMask;
    m_RenderPass = true;
}

NRI_INLINE void CommandBufferVK::EndRendering() {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdEndRendering(m_Handle);

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
    vk.CmdBindVertexBuffers2(m_Handle, baseSlot, vertexBufferNum, handles, offsets, sizes, strides);
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
    bufferInfo.offset += setRootDescriptorDesc.offset; // TODO: adjust "size"?

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

    VkBufferCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
    region.srcOffset = srcOffset;
    region.dstOffset = dstOffset;
    region.size = size == WHOLE_SIZE ? src.GetDesc().size : size;

    VkCopyBufferInfo2 info = {VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2};
    info.srcBuffer = src.GetHandle();
    info.dstBuffer = dstBufferVK.GetHandle();
    info.regionCount = 1;
    info.pRegions = &region;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyBuffer2(m_Handle, &info);
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

    VkCopyImageInfo2 info = {VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2};
    info.srcImage = src.GetHandle();
    info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    info.dstImage = dst.GetHandle();
    info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    info.regionCount = regionNum;
    info.pRegions = regions;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyImage2(m_Handle, &info);
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

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdResolveImage2(m_Handle, &info);
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
        return (planes & PlaneBits::PLANE_1) && (format == Format::NV12_UNORM || format == Format::P010_UNORM || format == Format::P016_UNORM) ? 2u : 1u;
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

    VkCopyBufferToImageInfo2 info = {VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2};
    info.srcBuffer = src.GetHandle();
    info.dstImage = dst.GetHandle();
    info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    info.regionCount = 1;
    info.pRegions = &region;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyBufferToImage2(m_Handle, &info);
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
        return (planes & PlaneBits::PLANE_1) && (format == Format::NV12_UNORM || format == Format::P010_UNORM || format == Format::P016_UNORM) ? 2u : 1u;
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

    VkCopyImageToBufferInfo2 info = {VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2};
    info.srcImage = src.GetHandle();
    info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    info.dstBuffer = dst.GetHandle();
    info.regionCount = 1;
    info.pRegions = &region;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyImageToBuffer2(m_Handle, &info);
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
    vk.CmdDispatch(m_Handle, dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

NRI_INLINE void CommandBufferVK::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchDesc) == sizeof(VkDispatchIndirectCommand));

    const BufferVK& bufferVK = (BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDispatchIndirect(m_Handle, bufferVK.GetHandle(), offset);
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

        uint32_t micromapNum = ConvertBotomLevelGeometries(ranges, geometries, trianglesMicromaps, in.geometries, in.geometryNum);

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

NRI_INLINE void CommandBufferVK::WriteAccelerationStructuresSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<VkAccelerationStructureKHR> handles = NRI_ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureKHR, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        handles[i] = ((AccelerationStructureVK*)accelerationStructures[i])->GetHandle();

    const QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteAccelerationStructuresPropertiesKHR(m_Handle, accelerationStructureNum, handles, queryPoolVK.GetType(), queryPoolVK.GetHandle(), queryPoolOffset);
}

NRI_INLINE void CommandBufferVK::WriteMicromapsSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<VkMicromapEXT> handles = NRI_ALLOCATE_SCRATCH(m_Device, VkMicromapEXT, micromapNum);
    for (uint32_t i = 0; i < micromapNum; i++)
        handles[i] = ((MicromapVK*)micromaps[i])->GetHandle();

    const QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteMicromapsPropertiesEXT(m_Handle, micromapNum, handles, queryPoolVK.GetType(), queryPoolVK.GetHandle(), queryPoolOffset);
}

NRI_INLINE void CommandBufferVK::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    VkStridedDeviceAddressRegionKHR raygen = {};
    raygen.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.raygenShader.buffer, dispatchRaysDesc.raygenShader.offset);
    raygen.size = dispatchRaysDesc.raygenShader.size;
    raygen.stride = dispatchRaysDesc.raygenShader.stride;

    VkStridedDeviceAddressRegionKHR miss = {};
    miss.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.missShaders.buffer, dispatchRaysDesc.missShaders.offset);
    miss.size = dispatchRaysDesc.missShaders.size;
    miss.stride = dispatchRaysDesc.missShaders.stride;

    VkStridedDeviceAddressRegionKHR hit = {};
    hit.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.hitShaderGroups.buffer, dispatchRaysDesc.hitShaderGroups.offset);
    hit.size = dispatchRaysDesc.hitShaderGroups.size;
    hit.stride = dispatchRaysDesc.hitShaderGroups.stride;

    VkStridedDeviceAddressRegionKHR callable = {};
    callable.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.callableShaders.buffer, dispatchRaysDesc.callableShaders.offset);
    callable.size = dispatchRaysDesc.callableShaders.size;
    callable.stride = dispatchRaysDesc.callableShaders.stride;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdTraceRaysKHR(m_Handle, &raygen, &miss, &hit, &callable, dispatchRaysDesc.x, dispatchRaysDesc.y, dispatchRaysDesc.z);
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
