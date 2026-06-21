// © 2026 NVIDIA Corporation

static StdVideoH264LevelIdc GetVideoH264LevelIdcVK(uint8_t levelIdc) {
    switch (levelIdc) {
        case 10:
            return STD_VIDEO_H264_LEVEL_IDC_1_0;
        case 11:
            return STD_VIDEO_H264_LEVEL_IDC_1_1;
        case 12:
            return STD_VIDEO_H264_LEVEL_IDC_1_2;
        case 13:
            return STD_VIDEO_H264_LEVEL_IDC_1_3;
        case 20:
            return STD_VIDEO_H264_LEVEL_IDC_2_0;
        case 21:
            return STD_VIDEO_H264_LEVEL_IDC_2_1;
        case 22:
            return STD_VIDEO_H264_LEVEL_IDC_2_2;
        case 30:
            return STD_VIDEO_H264_LEVEL_IDC_3_0;
        case 31:
            return STD_VIDEO_H264_LEVEL_IDC_3_1;
        case 32:
            return STD_VIDEO_H264_LEVEL_IDC_3_2;
        case 40:
            return STD_VIDEO_H264_LEVEL_IDC_4_0;
        case 41:
            return STD_VIDEO_H264_LEVEL_IDC_4_1;
        case 42:
            return STD_VIDEO_H264_LEVEL_IDC_4_2;
        case 50:
            return STD_VIDEO_H264_LEVEL_IDC_5_0;
        case 51:
            return STD_VIDEO_H264_LEVEL_IDC_5_1;
        case 52:
            return STD_VIDEO_H264_LEVEL_IDC_5_2;
        case 60:
            return STD_VIDEO_H264_LEVEL_IDC_6_0;
        case 61:
            return STD_VIDEO_H264_LEVEL_IDC_6_1;
        case 62:
            return STD_VIDEO_H264_LEVEL_IDC_6_2;
    }

    return STD_VIDEO_H264_LEVEL_IDC_INVALID;
}

static StdVideoH264SequenceParameterSet GetVideoH264SequenceParameterSetVK(const VideoH264SequenceParameterSetDesc& desc) {
    StdVideoH264SequenceParameterSet sps = {};
    sps.flags.constraint_set0_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET0);
    sps.flags.constraint_set1_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET1);
    sps.flags.constraint_set2_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET2);
    sps.flags.constraint_set3_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET3);
    sps.flags.constraint_set4_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET4);
    sps.flags.constraint_set5_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET5);
    sps.flags.direct_8x8_inference_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::DIRECT_8X8_INFERENCE);
    sps.flags.mb_adaptive_frame_field_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::MB_ADAPTIVE_FRAME_FIELD);
    sps.flags.frame_mbs_only_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::FRAME_MBS_ONLY);
    sps.flags.delta_pic_order_always_zero_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::DELTA_PIC_ORDER_ALWAYS_ZERO);
    sps.flags.separate_colour_plane_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::SEPARATE_COLOUR_PLANE);
    sps.flags.gaps_in_frame_num_value_allowed_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::GAPS_IN_FRAME_NUM_ALLOWED);
    sps.flags.qpprime_y_zero_transform_bypass_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::QPPRIME_Y_ZERO_TRANSFORM_BYPASS);
    sps.profile_idc = (StdVideoH264ProfileIdc)desc.profileIdc;
    sps.level_idc = GetVideoH264LevelIdcVK(desc.levelIdc);
    sps.chroma_format_idc = (StdVideoH264ChromaFormatIdc)desc.chromaFormatIdc;
    sps.seq_parameter_set_id = desc.sequenceParameterSetId;
    sps.bit_depth_luma_minus8 = desc.bitDepthLumaMinus8;
    sps.bit_depth_chroma_minus8 = desc.bitDepthChromaMinus8;
    sps.log2_max_frame_num_minus4 = desc.log2MaxFrameNumMinus4;
    sps.pic_order_cnt_type = (StdVideoH264PocType)desc.pictureOrderCountType;
    sps.offset_for_non_ref_pic = desc.offsetForNonReferencePicture;
    sps.offset_for_top_to_bottom_field = desc.offsetForTopToBottomField;
    sps.log2_max_pic_order_cnt_lsb_minus4 = desc.log2MaxPictureOrderCountLsbMinus4;
    sps.max_num_ref_frames = desc.referenceFrameNum;
    sps.pic_width_in_mbs_minus1 = desc.pictureWidthInMbsMinus1;
    sps.pic_height_in_map_units_minus1 = desc.pictureHeightInMapUnitsMinus1;
    return sps;
}

static StdVideoH264PictureParameterSet GetVideoH264PictureParameterSetVK(const VideoH264PictureParameterSetDesc& desc) {
    StdVideoH264PictureParameterSet pps = {};
    pps.flags.transform_8x8_mode_flag = !!(desc.flags & VideoH264PictureParameterSetBits::TRANSFORM_8X8_MODE);
    pps.flags.redundant_pic_cnt_present_flag = !!(desc.flags & VideoH264PictureParameterSetBits::REDUNDANT_PIC_CNT_PRESENT);
    pps.flags.constrained_intra_pred_flag = !!(desc.flags & VideoH264PictureParameterSetBits::CONSTRAINED_INTRA_PRED);
    pps.flags.deblocking_filter_control_present_flag = !!(desc.flags & VideoH264PictureParameterSetBits::DEBLOCKING_FILTER_CONTROL_PRESENT);
    pps.flags.weighted_pred_flag = !!(desc.flags & VideoH264PictureParameterSetBits::WEIGHTED_PRED);
    pps.flags.bottom_field_pic_order_in_frame_present_flag = !!(desc.flags & VideoH264PictureParameterSetBits::BOTTOM_FIELD_PIC_ORDER_IN_FRAME);
    pps.flags.entropy_coding_mode_flag = !!(desc.flags & VideoH264PictureParameterSetBits::ENTROPY_CODING_MODE);
    pps.seq_parameter_set_id = desc.sequenceParameterSetId;
    pps.pic_parameter_set_id = desc.pictureParameterSetId;
    pps.num_ref_idx_l0_default_active_minus1 = desc.refIndexL0DefaultActiveMinus1;
    pps.num_ref_idx_l1_default_active_minus1 = desc.refIndexL1DefaultActiveMinus1;
    pps.weighted_bipred_idc = (StdVideoH264WeightedBipredIdc)desc.weightedBipredIdc;
    pps.pic_init_qp_minus26 = desc.pictureInitQpMinus26;
    pps.pic_init_qs_minus26 = desc.pictureInitQsMinus26;
    pps.chroma_qp_index_offset = desc.chromaQpIndexOffset;
    pps.second_chroma_qp_index_offset = desc.secondChromaQpIndexOffset;
    return pps;
}

static bool SkipVideoAV1TimingInfoVK(video_av1::BitReader& reader) {
    uint32_t ignored = 0;
    uint8_t equalPictureInterval = 0;
    if (!reader.ReadBits(32, ignored) || !reader.ReadBits(32, ignored) || !reader.ReadFlag(equalPictureInterval))
        return false;
    if (!equalPictureInterval)
        return true;

    uint32_t bit = 0;
    do {
        if (!reader.ReadBits(1, bit))
            return false;
    } while (bit);

    return true;
}

static bool SkipVideoAV1DecoderModelInfoVK(video_av1::BitReader& reader, uint32_t& bufferDelayLength) {
    uint32_t ignored = 0;
    if (!reader.ReadBits(5, bufferDelayLength) || !reader.ReadBits(32, ignored) || !reader.ReadBits(5, ignored) || !reader.ReadBits(5, ignored))
        return false;

    bufferDelayLength++;
    return true;
}

static bool ApplyVideoEncodeAV1SequenceHeaderOverrideVK(StdVideoAV1SequenceHeader& header, const uint8_t* data, size_t size) {
    size_t cursor = 0;
    while (cursor < size) {
        video_av1::ObuSpan span = {};
        if (!video_av1::ReadObuHeader(data, size, cursor, span))
            return false;
        if (span.type != video_av1::ObuType::SequenceHeader)
            continue;

        video_av1::BitReader reader{data + span.payloadOffset, span.payloadSize, 0};
        uint32_t ignored = 0;
        uint8_t reducedStillPictureHeader = 0;
        uint8_t ignoredFlag = 0;
        if (!reader.ReadBits(3, ignored) || !reader.ReadFlag(ignoredFlag) || !reader.ReadFlag(reducedStillPictureHeader))
            return false;

        uint8_t decoderModelInfoPresent = 0;
        uint8_t initialDisplayDelayPresent = 0;
        uint32_t bufferDelayLength = 0;
        if (reducedStillPictureHeader) {
            if (!reader.ReadBits(5, ignored))
                return false;
        } else {
            uint8_t timingInfoPresent = 0;
            if (!reader.ReadFlag(timingInfoPresent))
                return false;
            if (timingInfoPresent) {
                if (!SkipVideoAV1TimingInfoVK(reader) || !reader.ReadFlag(decoderModelInfoPresent))
                    return false;
                if (decoderModelInfoPresent && !SkipVideoAV1DecoderModelInfoVK(reader, bufferDelayLength))
                    return false;
            }
            if (!reader.ReadFlag(initialDisplayDelayPresent) || !reader.ReadBits(5, ignored))
                return false;

            const uint32_t operatingPointNum = ignored + 1;
            for (uint32_t i = 0; i < operatingPointNum; i++) {
                uint32_t seqLevelIdx = 0;
                if (!reader.ReadBits(12, ignored) || !reader.ReadBits(5, seqLevelIdx))
                    return false;
                if (seqLevelIdx > 7 && !reader.ReadBits(1, ignored))
                    return false;
                if (decoderModelInfoPresent) {
                    uint8_t decoderModelPresentForThisOp = 0;
                    if (!reader.ReadFlag(decoderModelPresentForThisOp))
                        return false;
                    if (decoderModelPresentForThisOp && (!reader.ReadBits(bufferDelayLength, ignored) || !reader.ReadBits(bufferDelayLength, ignored) || !reader.ReadBits(1, ignored)))
                        return false;
                }
                if (initialDisplayDelayPresent) {
                    uint8_t initialDisplayDelayPresentForThisOp = 0;
                    if (!reader.ReadFlag(initialDisplayDelayPresentForThisOp))
                        return false;
                    if (initialDisplayDelayPresentForThisOp && !reader.ReadBits(4, ignored))
                        return false;
                }
            }
        }

        uint32_t frameWidthBitsMinus1 = 0;
        uint32_t frameHeightBitsMinus1 = 0;
        uint32_t maxFrameWidthMinus1 = 0;
        uint32_t maxFrameHeightMinus1 = 0;
        if (!reader.ReadBits(4, frameWidthBitsMinus1) || !reader.ReadBits(4, frameHeightBitsMinus1) || !reader.ReadBits(frameWidthBitsMinus1 + 1, maxFrameWidthMinus1) || !reader.ReadBits(frameHeightBitsMinus1 + 1, maxFrameHeightMinus1))
            return false;

        uint8_t frameIdNumbersPresent = 0;
        if (!reducedStillPictureHeader && !reader.ReadFlag(frameIdNumbersPresent))
            return false;
        if (frameIdNumbersPresent && (!reader.ReadBits(4, ignored) || !reader.ReadBits(3, ignored)))
            return false;
        if (!reader.ReadBits(1, ignored) || !reader.ReadBits(1, ignored) || !reader.ReadBits(1, ignored))
            return false;

        uint32_t seqForceScreenContentTools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
        uint32_t seqForceIntegerMv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
        if (!reducedStillPictureHeader) {
            uint8_t enableOrderHint = 0;
            if (!reader.ReadBits(1, ignored) || !reader.ReadBits(1, ignored) || !reader.ReadBits(1, ignored) || !reader.ReadBits(1, ignored) || !reader.ReadFlag(enableOrderHint))
                return false;
            if (enableOrderHint && (!reader.ReadBits(1, ignored) || !reader.ReadBits(1, ignored)))
                return false;

            uint8_t seqChooseScreenContentTools = 0;
            if (!reader.ReadFlag(seqChooseScreenContentTools))
                return false;
            if (!seqChooseScreenContentTools && !reader.ReadBits(1, seqForceScreenContentTools))
                return false;
            if (seqForceScreenContentTools > 0) {
                uint8_t seqChooseIntegerMv = 0;
                if (!reader.ReadFlag(seqChooseIntegerMv))
                    return false;
                if (!seqChooseIntegerMv && !reader.ReadBits(1, seqForceIntegerMv))
                    return false;
            }
        }

        header.frame_width_bits_minus_1 = (uint8_t)frameWidthBitsMinus1;
        header.frame_height_bits_minus_1 = (uint8_t)frameHeightBitsMinus1;
        header.max_frame_width_minus_1 = (uint16_t)maxFrameWidthMinus1;
        header.max_frame_height_minus_1 = (uint16_t)maxFrameHeightMinus1;
        header.seq_force_screen_content_tools = (uint8_t)seqForceScreenContentTools;
        header.seq_force_integer_mv = (uint8_t)seqForceIntegerMv;
        return true;
    }

    return false;
}

VideoSessionParametersVK::~VideoSessionParametersVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle)
        vk.DestroyVideoSessionParametersKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
}

NRI_INLINE Result VideoSessionParametersVK::CreateNative(VideoSessionVK& session, const void* pNext) {
    m_Session = &session;

    VkVideoSessionParametersCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR};
    createInfo.pNext = pNext;
    createInfo.videoSession = session.GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateVideoSessionParametersKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateVideoSessionParametersKHR");

    return Result::SUCCESS;
}

NRI_INLINE Result VideoSessionParametersVK::CreateNative(VideoSessionVK& session, const VkVideoSessionParametersCreateInfoKHR* nativeCreateInfo) {
    m_Session = &session;

    VkVideoSessionParametersCreateInfoKHR createInfo = GetVideoSessionParametersCreateInfoVK(session.GetHandle(), nativeCreateInfo);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateVideoSessionParametersKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateVideoSessionParametersKHR");

    return Result::SUCCESS;
}

NRI_INLINE Result VideoSessionParametersVK::Create(const VideoSessionParametersDesc& videoSessionParametersDesc) {
    if (!videoSessionParametersDesc.session)
        return Result::INVALID_ARGUMENT;

    VideoSessionVK& session = *(VideoSessionVK*)videoSessionParametersDesc.session;
    m_Session = &session;
    if (session.GetDesc().codec == VideoCodec::H265) {
        m_H265Parameters = videoSessionParametersDesc.h265Parameters;
        return CreateH265(session);
    }
    if (session.GetDesc().codec == VideoCodec::AV1) {
        m_AV1Parameters = videoSessionParametersDesc.av1Parameters;
        return CreateAV1(session);
    }
    if (session.GetDesc().codec != VideoCodec::H264)
        return Result::UNSUPPORTED;

    const VideoH264SessionParametersDesc emptyH264Parameters = {};
    const VideoH264SessionParametersDesc& h264Parameters = videoSessionParametersDesc.h264Parameters ? *videoSessionParametersDesc.h264Parameters : emptyH264Parameters;

    m_H264Sps.resize(h264Parameters.sequenceParameterSetNum);
    for (uint32_t i = 0; i < h264Parameters.sequenceParameterSetNum; i++) {
        m_H264Sps[i] = GetVideoH264SequenceParameterSetVK(h264Parameters.sequenceParameterSets[i]);
        m_H264Sps[i].pScalingLists = &m_H264DefaultScalingLists;
    }

    m_H264Pps.resize(h264Parameters.pictureParameterSetNum);
    for (uint32_t i = 0; i < h264Parameters.pictureParameterSetNum; i++) {
        m_H264Pps[i] = GetVideoH264PictureParameterSetVK(h264Parameters.pictureParameterSets[i]);
        m_H264Pps[i].pScalingLists = &m_H264DefaultScalingLists;
    }

    VkVideoDecodeH264SessionParametersAddInfoKHR decodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR};
    decodeAddInfo.stdSPSCount = h264Parameters.sequenceParameterSetNum;
    decodeAddInfo.pStdSPSs = m_H264Sps.data();
    decodeAddInfo.stdPPSCount = h264Parameters.pictureParameterSetNum;
    decodeAddInfo.pStdPPSs = m_H264Pps.data();

    VkVideoDecodeH264SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
    decodeInfo.maxStdSPSCount = h264Parameters.maxSequenceParameterSetNum ? h264Parameters.maxSequenceParameterSetNum : h264Parameters.sequenceParameterSetNum;
    decodeInfo.maxStdPPSCount = h264Parameters.maxPictureParameterSetNum ? h264Parameters.maxPictureParameterSetNum : h264Parameters.pictureParameterSetNum;
    decodeInfo.pParametersAddInfo = h264Parameters.sequenceParameterSetNum || h264Parameters.pictureParameterSetNum ? &decodeAddInfo : nullptr;

    VkVideoEncodeH264SessionParametersAddInfoKHR encodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR};
    encodeAddInfo.stdSPSCount = h264Parameters.sequenceParameterSetNum;
    encodeAddInfo.pStdSPSs = m_H264Sps.data();
    encodeAddInfo.stdPPSCount = h264Parameters.pictureParameterSetNum;
    encodeAddInfo.pStdPPSs = m_H264Pps.data();

    VkVideoEncodeH264SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
    encodeInfo.maxStdSPSCount = h264Parameters.maxSequenceParameterSetNum ? h264Parameters.maxSequenceParameterSetNum : h264Parameters.sequenceParameterSetNum;
    encodeInfo.maxStdPPSCount = h264Parameters.maxPictureParameterSetNum ? h264Parameters.maxPictureParameterSetNum : h264Parameters.pictureParameterSetNum;
    encodeInfo.pParametersAddInfo = h264Parameters.sequenceParameterSetNum || h264Parameters.pictureParameterSetNum ? &encodeAddInfo : nullptr;

    return CreateNative(session, session.GetDesc().type == VideoSessionType::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
}

NRI_INLINE Result VideoSessionParametersVK::CreateH265(VideoSessionVK& session) {
    VideoH265VideoParameterSetDesc defaultVps = {};
    VideoH265SequenceParameterSetDesc defaultSps = {};
    VideoH265PictureParameterSetDesc defaultPps = {};
    VideoH265SessionParametersDesc defaultParameters = {};
    if (!m_H265Parameters) {
        const uint8_t bitDepthMinus8 = session.GetDesc().format == Format::P010_UNORM || session.GetDesc().format == Format::P016_UNORM ? 2 : 0;
        defaultVps.flags = VideoH265VideoParameterSetBits::TEMPORAL_ID_NESTING;
        defaultVps.profileTierLevel.generalProfileIdc = (uint8_t)(session.GetDesc().format == Format::P010_UNORM || session.GetDesc().format == Format::P016_UNORM
                ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10
                : STD_VIDEO_H265_PROFILE_IDC_MAIN);
        defaultVps.profileTierLevel.generalLevelIdc = (uint8_t)GetVideoH265LevelIdcVK(session.GetDesc().width, session.GetDesc().height);
        defaultVps.decPicBufMgr.maxDecPicBufferingMinus1[0] = (uint8_t)std::min(session.GetDesc().maxReferenceNum ? session.GetDesc().maxReferenceNum : 1u, 15u);
        defaultVps.decPicBufMgr.maxNumReorderPics[0] = session.GetDesc().maxReferenceNum > 1 ? 1 : 0;

        defaultSps.flags = VideoH265SequenceParameterSetBits::TEMPORAL_ID_NESTING | VideoH265SequenceParameterSetBits::SAMPLE_ADAPTIVE_OFFSET_ENABLED | VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED;
        defaultSps.chromaFormatIdc = STD_VIDEO_H265_CHROMA_FORMAT_IDC_420;
        defaultSps.pictureWidthInLumaSamples = session.GetDesc().width;
        defaultSps.pictureHeightInLumaSamples = session.GetDesc().height;
        defaultSps.bitDepthLumaMinus8 = bitDepthMinus8;
        defaultSps.bitDepthChromaMinus8 = bitDepthMinus8;
        defaultSps.log2MaxPictureOrderCountLsbMinus4 = 4;
        defaultSps.log2MinLumaCodingBlockSizeMinus3 = 0;
        defaultSps.log2DiffMaxMinLumaCodingBlockSize = 2;
        defaultSps.log2MinLumaTransformBlockSizeMinus2 = 0;
        defaultSps.log2DiffMaxMinLumaTransformBlockSize = 2;
        defaultSps.maxTransformHierarchyDepthInter = 2;
        defaultSps.maxTransformHierarchyDepthIntra = 2;
        defaultSps.profileTierLevel = defaultVps.profileTierLevel;
        defaultSps.decPicBufMgr = defaultVps.decPicBufMgr;

        defaultPps.flags = VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_SLICES_ENABLED | VideoH265PictureParameterSetBits::LISTS_MODIFICATION_PRESENT;
        defaultPps.log2ParallelMergeLevelMinus2 = 2;

        defaultParameters.videoParameterSets = &defaultVps;
        defaultParameters.videoParameterSetNum = 1;
        defaultParameters.sequenceParameterSets = &defaultSps;
        defaultParameters.sequenceParameterSetNum = 1;
        defaultParameters.pictureParameterSets = &defaultPps;
        defaultParameters.pictureParameterSetNum = 1;
        m_H265Parameters = &defaultParameters;
    }

    if ((m_H265Parameters->videoParameterSetNum && !m_H265Parameters->videoParameterSets) || (m_H265Parameters->sequenceParameterSetNum && !m_H265Parameters->sequenceParameterSets) || (m_H265Parameters->pictureParameterSetNum && !m_H265Parameters->pictureParameterSets))
        return Result::INVALID_ARGUMENT;

    m_H265VpsProfileTierLevels.resize(m_H265Parameters->videoParameterSetNum);
    m_H265SpsProfileTierLevels.resize(m_H265Parameters->sequenceParameterSetNum);
    m_H265VpsDecPicBufMgrs.resize(m_H265Parameters->videoParameterSetNum);
    m_H265SpsDecPicBufMgrs.resize(m_H265Parameters->sequenceParameterSetNum);
    m_H265Vps.resize(m_H265Parameters->videoParameterSetNum);
    m_H265Sps.resize(m_H265Parameters->sequenceParameterSetNum);
    m_H265Pps.resize(m_H265Parameters->pictureParameterSetNum);
    m_H265SpsScalingLists.resize(m_H265Parameters->sequenceParameterSetNum);
    m_H265PpsScalingLists.resize(m_H265Parameters->pictureParameterSetNum);
    uint32_t shortTermRefPicSetNum = 0;
    for (uint32_t i = 0; i < m_H265Parameters->sequenceParameterSetNum; i++) {
        const VideoH265SequenceParameterSetDesc& sps = m_H265Parameters->sequenceParameterSets[i];
        if (sps.numShortTermRefPicSets && !sps.shortTermRefPicSets)
            return Result::INVALID_ARGUMENT;
        shortTermRefPicSetNum += sps.numShortTermRefPicSets;
    }
    m_H265ShortTermRefPicSets.resize(shortTermRefPicSetNum);
    m_H265LongTermRefPicsSps.resize(m_H265Parameters->sequenceParameterSetNum);

    for (uint32_t i = 0; i < m_H265Parameters->videoParameterSetNum; i++) {
        const VideoH265VideoParameterSetDesc& vps = m_H265Parameters->videoParameterSets[i];
        FillVideoH265ProfileTierLevelVK(m_H265VpsProfileTierLevels[i], vps.profileTierLevel);
        FillVideoH265DecPicBufMgrVK(m_H265VpsDecPicBufMgrs[i], vps.decPicBufMgr);
        m_H265Vps[i] = GetVideoH265VideoParameterSetVK(vps, m_H265VpsProfileTierLevels[i], m_H265VpsDecPicBufMgrs[i]);
    }

    uint32_t firstShortTermRefPicSet = 0;
    for (uint32_t i = 0; i < m_H265Parameters->sequenceParameterSetNum; i++) {
        const VideoH265SequenceParameterSetDesc& sps = m_H265Parameters->sequenceParameterSets[i];
        for (uint32_t j = 0; j < sps.numShortTermRefPicSets; j++)
            m_H265ShortTermRefPicSets[firstShortTermRefPicSet + j] = GetVideoH265ShortTermRefPicSetVK(sps.shortTermRefPicSets[j]);

        FillVideoH265ProfileTierLevelVK(m_H265SpsProfileTierLevels[i], sps.profileTierLevel);
        FillVideoH265DecPicBufMgrVK(m_H265SpsDecPicBufMgrs[i], sps.decPicBufMgr);
        const StdVideoH265ScalingLists* scalingLists = nullptr;
        if (sps.scalingLists) {
            m_H265SpsScalingLists[i] = GetVideoH265ScalingListsVK(*sps.scalingLists);
            scalingLists = &m_H265SpsScalingLists[i];
        }
        const StdVideoH265ShortTermRefPicSet* shortTermRefPicSets = sps.numShortTermRefPicSets ? &m_H265ShortTermRefPicSets[firstShortTermRefPicSet] : nullptr;
        const StdVideoH265LongTermRefPicsSps* longTermRefPicsSps = nullptr;
        if (sps.longTermRefPicsSps) {
            m_H265LongTermRefPicsSps[i] = GetVideoH265LongTermRefPicsSpsVK(*sps.longTermRefPicsSps);
            longTermRefPicsSps = &m_H265LongTermRefPicsSps[i];
        }
        m_H265Sps[i] = GetVideoH265SequenceParameterSetVK(sps, m_H265SpsProfileTierLevels[i], m_H265SpsDecPicBufMgrs[i], scalingLists, shortTermRefPicSets,
            longTermRefPicsSps);
        firstShortTermRefPicSet += sps.numShortTermRefPicSets;
    }

    for (uint32_t i = 0; i < m_H265Parameters->pictureParameterSetNum; i++) {
        const VideoH265PictureParameterSetDesc& pps = m_H265Parameters->pictureParameterSets[i];
        const StdVideoH265ScalingLists* scalingLists = nullptr;
        if (pps.scalingLists) {
            m_H265PpsScalingLists[i] = GetVideoH265ScalingListsVK(*pps.scalingLists);
            scalingLists = &m_H265PpsScalingLists[i];
        }
        m_H265Pps[i] = GetVideoH265PictureParameterSetVK(pps, scalingLists);
    }

    VkVideoDecodeH265SessionParametersAddInfoKHR decodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};
    decodeAddInfo.stdVPSCount = m_H265Parameters->videoParameterSetNum;
    decodeAddInfo.pStdVPSs = m_H265Vps.data();
    decodeAddInfo.stdSPSCount = m_H265Parameters->sequenceParameterSetNum;
    decodeAddInfo.pStdSPSs = m_H265Sps.data();
    decodeAddInfo.stdPPSCount = m_H265Parameters->pictureParameterSetNum;
    decodeAddInfo.pStdPPSs = m_H265Pps.data();

    VkVideoDecodeH265SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR};
    decodeInfo.maxStdVPSCount = m_H265Parameters->maxVideoParameterSetNum ? m_H265Parameters->maxVideoParameterSetNum : m_H265Parameters->videoParameterSetNum;
    decodeInfo.maxStdSPSCount = m_H265Parameters->maxSequenceParameterSetNum ? m_H265Parameters->maxSequenceParameterSetNum : m_H265Parameters->sequenceParameterSetNum;
    decodeInfo.maxStdPPSCount = m_H265Parameters->maxPictureParameterSetNum ? m_H265Parameters->maxPictureParameterSetNum : m_H265Parameters->pictureParameterSetNum;
    decodeInfo.pParametersAddInfo = m_H265Parameters->videoParameterSetNum || m_H265Parameters->sequenceParameterSetNum || m_H265Parameters->pictureParameterSetNum
        ? &decodeAddInfo
        : nullptr;

    VkVideoEncodeH265SessionParametersAddInfoKHR encodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};
    encodeAddInfo.stdVPSCount = m_H265Parameters->videoParameterSetNum;
    encodeAddInfo.pStdVPSs = m_H265Vps.data();
    encodeAddInfo.stdSPSCount = m_H265Parameters->sequenceParameterSetNum;
    encodeAddInfo.pStdSPSs = m_H265Sps.data();
    encodeAddInfo.stdPPSCount = m_H265Parameters->pictureParameterSetNum;
    encodeAddInfo.pStdPPSs = m_H265Pps.data();

    VkVideoEncodeH265SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR};
    encodeInfo.maxStdVPSCount = m_H265Parameters->maxVideoParameterSetNum ? m_H265Parameters->maxVideoParameterSetNum : m_H265Parameters->videoParameterSetNum;
    encodeInfo.maxStdSPSCount = m_H265Parameters->maxSequenceParameterSetNum ? m_H265Parameters->maxSequenceParameterSetNum : m_H265Parameters->sequenceParameterSetNum;
    encodeInfo.maxStdPPSCount = m_H265Parameters->maxPictureParameterSetNum ? m_H265Parameters->maxPictureParameterSetNum : m_H265Parameters->pictureParameterSetNum;
    encodeInfo.pParametersAddInfo = m_H265Parameters->videoParameterSetNum || m_H265Parameters->sequenceParameterSetNum || m_H265Parameters->pictureParameterSetNum
        ? &encodeAddInfo
        : nullptr;

    return CreateNative(session, session.GetDesc().type == VideoSessionType::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
}

NRI_INLINE Result VideoSessionParametersVK::CreateAV1(VideoSessionVK& session) {
    if (m_AV1Parameters) {
        FillVideoAV1ColorConfigVK(m_AV1ColorConfig, m_AV1Parameters->sequence);
        m_AV1TimingInfo = {};
        const StdVideoAV1TimingInfo* timingInfo = nullptr;
        if (m_AV1Parameters->sequence.numUnitsInDisplayTick && m_AV1Parameters->sequence.timeScale) {
            m_AV1TimingInfo.num_units_in_display_tick = m_AV1Parameters->sequence.numUnitsInDisplayTick;
            m_AV1TimingInfo.time_scale = m_AV1Parameters->sequence.timeScale;
            m_AV1TimingInfo.num_ticks_per_picture_minus_1 = m_AV1Parameters->sequence.numTicksPerPictureMinus1;
            timingInfo = &m_AV1TimingInfo;
        }
        if (session.GetDesc().type == VideoSessionType::DECODE) {
            m_AV1ColorConfig.flags.color_description_present_flag = false;
            m_AV1ColorConfig.chroma_sample_position = {};
            timingInfo = &m_AV1TimingInfo;
        }
        FillVideoAV1SequenceHeaderVK(m_AV1SequenceHeader, m_AV1Parameters->sequence, m_AV1ColorConfig, timingInfo);
        m_AV1OperatingPoint.seq_level_idx = (uint8_t)GetVideoAV1LevelVK(m_AV1Parameters->sequence.level, session.GetDesc().width, session.GetDesc().height);
    } else {
        m_AV1ColorConfig.BitDepth = session.GetDesc().format == Format::P010_UNORM || session.GetDesc().format == Format::P016_UNORM ? 10 : 8;
        m_AV1ColorConfig.subsampling_x = 1;
        m_AV1ColorConfig.subsampling_y = 1;
        m_AV1ColorConfig.flags.color_description_present_flag = true;
        m_AV1ColorConfig.color_primaries = STD_VIDEO_AV1_COLOR_PRIMARIES_BT_709;
        m_AV1ColorConfig.transfer_characteristics = STD_VIDEO_AV1_TRANSFER_CHARACTERISTICS_BT_709;
        m_AV1ColorConfig.matrix_coefficients = STD_VIDEO_AV1_MATRIX_COEFFICIENTS_BT_709;
        m_AV1ColorConfig.chroma_sample_position = STD_VIDEO_AV1_CHROMA_SAMPLE_POSITION_VERTICAL;
        m_AV1SequenceHeader.seq_profile = STD_VIDEO_AV1_PROFILE_MAIN;
        m_AV1SequenceHeader.flags.enable_order_hint = true;
        m_AV1SequenceHeader.frame_width_bits_minus_1 = GetVideoAV1SizeBitsMinus1VK(session.GetDesc().width);
        m_AV1SequenceHeader.frame_height_bits_minus_1 = GetVideoAV1SizeBitsMinus1VK(session.GetDesc().height);
        m_AV1SequenceHeader.max_frame_width_minus_1 = (uint16_t)(session.GetDesc().width - 1);
        m_AV1SequenceHeader.max_frame_height_minus_1 = (uint16_t)(session.GetDesc().height - 1);
        m_AV1SequenceHeader.order_hint_bits_minus_1 = 7;
        m_AV1SequenceHeader.seq_force_integer_mv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
        m_AV1SequenceHeader.seq_force_screen_content_tools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
        m_AV1SequenceHeader.pColorConfig = &m_AV1ColorConfig;
        m_AV1OperatingPoint.seq_level_idx = (uint8_t)GetVideoAV1LevelVK(session.GetDesc().width, session.GetDesc().height);
    }

    if (session.GetDesc().type == VideoSessionType::ENCODE) {
        m_AV1SequenceHeader.seq_force_screen_content_tools = 0;
        m_AV1OperatingPoint.decoder_buffer_delay = 1;
        m_AV1OperatingPoint.encoder_buffer_delay = 2;
    }

    VkVideoDecodeAV1SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR};
    decodeInfo.pStdSequenceHeader = &m_AV1SequenceHeader;

    VkVideoEncodeAV1SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR};
    encodeInfo.pStdSequenceHeader = &m_AV1SequenceHeader;
    encodeInfo.pStdDecoderModelInfo = &m_AV1DecoderModelInfo;
    encodeInfo.stdOperatingPointCount = 1;
    encodeInfo.pStdOperatingPoints = &m_AV1OperatingPoint;
    VkVideoEncodeQualityLevelInfoKHR encodeQualityInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_QUALITY_LEVEL_INFO_KHR};
    encodeQualityInfo.pNext = &encodeInfo;
    if (session.UseInlineSessionParameters() && session.GetDesc().type == VideoSessionType::DECODE)
        return Result::SUCCESS;

    if (session.GetDesc().type == VideoSessionType::DECODE)
        return CreateNative(session, &decodeInfo);

    Result result = CreateNative(session, &encodeQualityInfo);
    if (result != Result::SUCCESS)
        return result;

    if (!m_Device.GetDispatchTable().GetEncodedVideoSessionParametersKHR)
        return Result::SUCCESS;

    VkVideoEncodeSessionParametersGetInfoKHR getInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_GET_INFO_KHR};
    getInfo.videoSessionParameters = m_Handle;
    VkVideoEncodeSessionParametersFeedbackInfoKHR feedbackInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_FEEDBACK_INFO_KHR};
    size_t dataSize = 0;
    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.GetEncodedVideoSessionParametersKHR(m_Device, &getInfo, &feedbackInfo, &dataSize, nullptr);
    if (vkResult != VK_SUCCESS || !feedbackInfo.hasOverrides || !dataSize)
        return Result::SUCCESS;

    std::vector<uint8_t> data(dataSize);
    vkResult = vk.GetEncodedVideoSessionParametersKHR(m_Device, &getInfo, &feedbackInfo, &dataSize, data.data());
    if (vkResult != VK_SUCCESS || !feedbackInfo.hasOverrides || !ApplyVideoEncodeAV1SequenceHeaderOverrideVK(m_AV1SequenceHeader, data.data(), dataSize))
        return Result::SUCCESS;

    vk.DestroyVideoSessionParametersKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    m_Handle = VK_NULL_HANDLE;

    return CreateNative(session, &encodeQualityInfo);
}
