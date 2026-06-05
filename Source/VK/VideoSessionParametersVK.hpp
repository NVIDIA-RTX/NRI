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

VideoSessionParametersVK::~VideoSessionParametersVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle)
        vk.DestroyVideoSessionParametersKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
}

NRI_INLINE Result VideoSessionParametersVK::CreateNative(VideoSessionVK& session, const void* pNext) {
    m_Session = &session;

    VkVideoSessionParametersCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR};
    createInfo.pNext = pNext;
    createInfo.videoSession = session.m_Handle;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateVideoSessionParametersKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateVideoSessionParametersKHR");

    return Result::SUCCESS;
}

NRI_INLINE Result VideoSessionParametersVK::CreateNative(VideoSessionVK& session, const VkVideoSessionParametersCreateInfoKHR* nativeCreateInfo) {
    m_Session = &session;

    VkVideoSessionParametersCreateInfoKHR createInfo = GetVideoSessionParametersCreateInfoVK(session.m_Handle, nativeCreateInfo);

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
    if (session.m_Desc.codec == VideoCodec::H265) {
        m_H265Parameters = videoSessionParametersDesc.h265Parameters;
        return CreateH265(session);
    }
    if (session.m_Desc.codec == VideoCodec::AV1) {
        m_AV1Parameters = videoSessionParametersDesc.av1Parameters;
        return CreateAV1(session);
    }
    if (session.m_Desc.codec != VideoCodec::H264)
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

    return CreateNative(session, session.m_Desc.type == VideoSessionType::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
}

NRI_INLINE Result VideoSessionParametersVK::CreateH265(VideoSessionVK& session) {
    VideoH265VideoParameterSetDesc defaultVps = {};
    VideoH265SequenceParameterSetDesc defaultSps = {};
    VideoH265PictureParameterSetDesc defaultPps = {};
    VideoH265SessionParametersDesc defaultParameters = {};
    if (!m_H265Parameters) {
        const uint8_t bitDepthMinus8 = session.m_Desc.format == Format::P010_UNORM || session.m_Desc.format == Format::P016_UNORM ? 2 : 0;
        defaultVps.flags = VideoH265VideoParameterSetBits::TEMPORAL_ID_NESTING;
        defaultVps.profileTierLevel.generalProfileIdc = (uint8_t)(session.m_Desc.format == Format::P010_UNORM || session.m_Desc.format == Format::P016_UNORM
                ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10
                : STD_VIDEO_H265_PROFILE_IDC_MAIN);
        defaultVps.profileTierLevel.generalLevelIdc = (uint8_t)GetVideoH265LevelIdcVK(session.m_Desc.width, session.m_Desc.height);
        defaultVps.decPicBufMgr.maxDecPicBufferingMinus1[0] = (uint8_t)std::min(session.m_Desc.maxReferenceNum ? session.m_Desc.maxReferenceNum : 1u, 15u);

        defaultSps.flags = VideoH265SequenceParameterSetBits::TEMPORAL_ID_NESTING | VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED;
        defaultSps.chromaFormatIdc = STD_VIDEO_H265_CHROMA_FORMAT_IDC_420;
        defaultSps.pictureWidthInLumaSamples = session.m_Desc.width;
        defaultSps.pictureHeightInLumaSamples = session.m_Desc.height;
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

        defaultPps.flags = VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_SLICES_ENABLED;
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

    return CreateNative(session, session.m_Desc.type == VideoSessionType::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
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
        if (session.m_Desc.type == VideoSessionType::DECODE) {
            m_AV1ColorConfig.flags.color_description_present_flag = false;
            m_AV1ColorConfig.chroma_sample_position = {};
            timingInfo = &m_AV1TimingInfo;
        }
        FillVideoAV1SequenceHeaderVK(m_AV1SequenceHeader, m_AV1Parameters->sequence, m_AV1ColorConfig, timingInfo);
        m_AV1OperatingPoint.seq_level_idx = (uint8_t)GetVideoAV1LevelVK(m_AV1Parameters->sequence.level, session.m_Desc.width, session.m_Desc.height);
    } else {
        m_AV1ColorConfig.BitDepth = session.m_Desc.format == Format::P010_UNORM || session.m_Desc.format == Format::P016_UNORM ? 10 : 8;
        m_AV1ColorConfig.subsampling_x = 1;
        m_AV1ColorConfig.subsampling_y = 1;
        m_AV1ColorConfig.flags.color_description_present_flag = true;
        m_AV1ColorConfig.color_primaries = STD_VIDEO_AV1_COLOR_PRIMARIES_BT_709;
        m_AV1ColorConfig.transfer_characteristics = STD_VIDEO_AV1_TRANSFER_CHARACTERISTICS_BT_709;
        m_AV1ColorConfig.matrix_coefficients = STD_VIDEO_AV1_MATRIX_COEFFICIENTS_BT_709;
        m_AV1ColorConfig.chroma_sample_position = STD_VIDEO_AV1_CHROMA_SAMPLE_POSITION_VERTICAL;
        m_AV1SequenceHeader.seq_profile = STD_VIDEO_AV1_PROFILE_MAIN;
        m_AV1SequenceHeader.flags.enable_order_hint = true;
        m_AV1SequenceHeader.frame_width_bits_minus_1 = GetVideoAV1SizeBitsMinus1VK(session.m_Desc.width);
        m_AV1SequenceHeader.frame_height_bits_minus_1 = GetVideoAV1SizeBitsMinus1VK(session.m_Desc.height);
        m_AV1SequenceHeader.max_frame_width_minus_1 = (uint16_t)(session.m_Desc.width - 1);
        m_AV1SequenceHeader.max_frame_height_minus_1 = (uint16_t)(session.m_Desc.height - 1);
        m_AV1SequenceHeader.order_hint_bits_minus_1 = 7;
        m_AV1SequenceHeader.seq_force_integer_mv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
        m_AV1SequenceHeader.seq_force_screen_content_tools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
        m_AV1SequenceHeader.pColorConfig = &m_AV1ColorConfig;
        m_AV1OperatingPoint.seq_level_idx = (uint8_t)GetVideoAV1LevelVK(session.m_Desc.width, session.m_Desc.height);
    }

    VkVideoDecodeAV1SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR};
    decodeInfo.pStdSequenceHeader = &m_AV1SequenceHeader;

    VkVideoEncodeAV1SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR};
    encodeInfo.pStdSequenceHeader = &m_AV1SequenceHeader;

    if (session.m_UseInlineSessionParameters && session.m_Desc.type == VideoSessionType::DECODE)
        return Result::SUCCESS;

    return CreateNative(session, session.m_Desc.type == VideoSessionType::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
}
