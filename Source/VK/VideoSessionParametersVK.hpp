// © 2026 NVIDIA Corporation

static StdVideoH264LevelIdc GetVideoH264LevelIdc(uint8_t levelIdc) {
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

static StdVideoH264SequenceParameterSet GetVideoH264SequenceParameterSet(const VideoH264SequenceParameterSetDesc& desc) {
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
    sps.flags.frame_cropping_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::FRAME_CROPPING);
    sps.profile_idc = (StdVideoH264ProfileIdc)desc.profileIdc;
    sps.level_idc = GetVideoH264LevelIdc(desc.levelIdc);
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

    if (sps.flags.frame_cropping_flag) {
        sps.frame_crop_left_offset = desc.frameCropLeftOffset;
        sps.frame_crop_right_offset = desc.frameCropRightOffset;
        sps.frame_crop_top_offset = desc.frameCropTopOffset;
        sps.frame_crop_bottom_offset = desc.frameCropBottomOffset;
    }

    return sps;
}

static StdVideoH264PictureParameterSet GetVideoH264PictureParameterSet(const VideoH264PictureParameterSetDesc& desc) {
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

// Sequence header fields affecting frame header syntax or decoding; level, tier, operating points, timing and color description values are not included
struct VideoAV1SequenceSyntaxVK {
    uint32_t seqProfile;
    uint32_t reducedStillPictureHeader;
    uint32_t decoderModelInfoPresent;
    uint32_t equalPictureInterval;
    uint32_t bufferRemovalTimeLengthMinus1;
    uint32_t framePresentationTimeLengthMinus1;
    uint32_t frameWidthBitsMinus1;
    uint32_t frameHeightBitsMinus1;
    uint32_t maxFrameWidthMinus1;
    uint32_t maxFrameHeightMinus1;
    uint32_t frameIdNumbersPresent;
    uint32_t deltaFrameIdLengthMinus2;
    uint32_t additionalFrameIdLengthMinus1;
    uint32_t use128x128Superblock;
    uint32_t enableFilterIntra;
    uint32_t enableIntraEdgeFilter;
    uint32_t enableInterintraCompound;
    uint32_t enableMaskedCompound;
    uint32_t enableWarpedMotion;
    uint32_t enableDualFilter;
    uint32_t enableOrderHint;
    uint32_t enableJntComp;
    uint32_t enableRefFrameMvs;
    uint32_t seqForceScreenContentTools;
    uint32_t seqForceIntegerMv;
    uint32_t orderHintBitsMinus1;
    uint32_t enableSuperres;
    uint32_t enableCdef;
    uint32_t enableRestoration;
    uint32_t bitDepth;
    uint32_t monochrome;
    uint32_t subsamplingX;
    uint32_t subsamplingY;
    uint32_t separateUvDeltaQ;
    uint32_t filmGrainParamsPresent;
};

static bool ReadVideoAV1Uvlc(video::av1::BitReader& reader) {
    uint32_t leadingZeros = 0;
    uint32_t bit = 0;
    while (true) {
        if (!reader.ReadBits(1, bit))
            return false;
        if (bit)
            break;
        if (++leadingZeros >= 32)
            return false;
    }

    uint32_t ignored = 0;
    return reader.ReadBits(leadingZeros, ignored);
}

// AV1 "color_config"
static bool ReadVideoAV1ColorConfig(video::av1::BitReader& reader, VideoAV1SequenceSyntaxVK& syntax) {
    uint32_t highBitdepth = 0;
    uint32_t twelveBit = 0;
    if (!reader.ReadBits(1, highBitdepth) || (syntax.seqProfile == 2 && highBitdepth && !reader.ReadBits(1, twelveBit)))
        return false;

    syntax.bitDepth = twelveBit ? 12 : (highBitdepth ? 10 : 8);
    if (syntax.seqProfile != video::av1::PROFILE_HIGH && !reader.ReadBits(1, syntax.monochrome))
        return false;

    uint32_t colorDescriptionPresent = 0;
    uint32_t colorPrimaries = 2;
    uint32_t transferCharacteristics = 2;
    uint32_t matrixCoefficients = 2;
    if (!reader.ReadBits(1, colorDescriptionPresent))
        return false;
    if (colorDescriptionPresent && (!reader.ReadBits(8, colorPrimaries) || !reader.ReadBits(8, transferCharacteristics) || !reader.ReadBits(8, matrixCoefficients)))
        return false;

    uint32_t ignored = 0;
    if (syntax.monochrome) {
        syntax.subsamplingX = 1;
        syntax.subsamplingY = 1;
        return reader.ReadBits(1, ignored); // color_range
    }

    if (colorPrimaries == 1 && transferCharacteristics == 13 && matrixCoefficients == 0) {
        syntax.subsamplingX = 0;
        syntax.subsamplingY = 0;
    } else {
        if (!reader.ReadBits(1, ignored)) // color_range
            return false;

        if (syntax.seqProfile == 0) {
            syntax.subsamplingX = 1;
            syntax.subsamplingY = 1;
        } else if (syntax.seqProfile == 1) {
            syntax.subsamplingX = 0;
            syntax.subsamplingY = 0;
        } else if (syntax.bitDepth == 12) {
            if (!reader.ReadBits(1, syntax.subsamplingX) || (syntax.subsamplingX && !reader.ReadBits(1, syntax.subsamplingY)))
                return false;
        } else {
            syntax.subsamplingX = 1;
            syntax.subsamplingY = 0;
        }

        if (syntax.subsamplingX && syntax.subsamplingY && !reader.ReadBits(2, ignored)) // chroma_sample_position
            return false;
    }

    return reader.ReadBits(1, syntax.separateUvDeltaQ);
}

// AV1 "sequence_header_obu" payload
static bool ReadVideoAV1SequenceSyntax(const uint8_t* payload, size_t payloadSize, VideoAV1SequenceSyntaxVK& syntax) {
    syntax = {};

    video::av1::BitReader reader{payload, payloadSize, 0};
    uint32_t ignored = 0;
    if (!reader.ReadBits(3, syntax.seqProfile) || !reader.ReadBits(1, ignored) || !reader.ReadBits(1, syntax.reducedStillPictureHeader))
        return false;

    if (syntax.reducedStillPictureHeader) {
        if (!reader.ReadBits(5, ignored))
            return false;
    } else {
        uint32_t timingInfoPresent = 0;
        if (!reader.ReadBits(1, timingInfoPresent))
            return false;

        if (timingInfoPresent) {
            if (!reader.ReadBits(32, ignored) || !reader.ReadBits(32, ignored) || !reader.ReadBits(1, syntax.equalPictureInterval))
                return false;
            if (syntax.equalPictureInterval && !ReadVideoAV1Uvlc(reader))
                return false;
            if (!reader.ReadBits(1, syntax.decoderModelInfoPresent))
                return false;
        }

        uint32_t bufferDelayLengthMinus1 = 0;
        if (syntax.decoderModelInfoPresent) {
            if (!reader.ReadBits(5, bufferDelayLengthMinus1) || !reader.ReadBits(32, ignored) || !reader.ReadBits(5, syntax.bufferRemovalTimeLengthMinus1) || !reader.ReadBits(5, syntax.framePresentationTimeLengthMinus1))
                return false;
        } else
            syntax.equalPictureInterval = 0; // used only with the decoder model

        uint32_t initialDisplayDelayPresent = 0;
        uint32_t operatingPointNumMinus1 = 0;
        if (!reader.ReadBits(1, initialDisplayDelayPresent) || !reader.ReadBits(5, operatingPointNumMinus1))
            return false;

        for (uint32_t i = 0; i <= operatingPointNumMinus1; i++) {
            uint32_t seqLevelIdx = 0;
            if (!reader.ReadBits(12, ignored) || !reader.ReadBits(5, seqLevelIdx))
                return false;
            if (seqLevelIdx > 7 && !reader.ReadBits(1, ignored))
                return false;

            uint32_t isPresent = 0;
            if (syntax.decoderModelInfoPresent) {
                if (!reader.ReadBits(1, isPresent))
                    return false;
                if (isPresent && (!reader.ReadBits(bufferDelayLengthMinus1 + 1, ignored) || !reader.ReadBits(bufferDelayLengthMinus1 + 1, ignored) || !reader.ReadBits(1, ignored)))
                    return false;
            }

            if (initialDisplayDelayPresent) {
                if (!reader.ReadBits(1, isPresent))
                    return false;
                if (isPresent && !reader.ReadBits(4, ignored))
                    return false;
            }
        }
    }

    if (!reader.ReadBits(4, syntax.frameWidthBitsMinus1) || !reader.ReadBits(4, syntax.frameHeightBitsMinus1)
        || !reader.ReadBits(syntax.frameWidthBitsMinus1 + 1, syntax.maxFrameWidthMinus1) || !reader.ReadBits(syntax.frameHeightBitsMinus1 + 1, syntax.maxFrameHeightMinus1))
        return false;

    if (!syntax.reducedStillPictureHeader && !reader.ReadBits(1, syntax.frameIdNumbersPresent))
        return false;
    if (syntax.frameIdNumbersPresent && (!reader.ReadBits(4, syntax.deltaFrameIdLengthMinus2) || !reader.ReadBits(3, syntax.additionalFrameIdLengthMinus1)))
        return false;
    if (!reader.ReadBits(1, syntax.use128x128Superblock) || !reader.ReadBits(1, syntax.enableFilterIntra) || !reader.ReadBits(1, syntax.enableIntraEdgeFilter))
        return false;

    syntax.seqForceScreenContentTools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
    syntax.seqForceIntegerMv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
    if (!syntax.reducedStillPictureHeader) {
        if (!reader.ReadBits(1, syntax.enableInterintraCompound) || !reader.ReadBits(1, syntax.enableMaskedCompound) || !reader.ReadBits(1, syntax.enableWarpedMotion)
            || !reader.ReadBits(1, syntax.enableDualFilter) || !reader.ReadBits(1, syntax.enableOrderHint))
            return false;
        if (syntax.enableOrderHint && (!reader.ReadBits(1, syntax.enableJntComp) || !reader.ReadBits(1, syntax.enableRefFrameMvs)))
            return false;

        uint32_t seqChooseScreenContentTools = 0;
        if (!reader.ReadBits(1, seqChooseScreenContentTools) || (!seqChooseScreenContentTools && !reader.ReadBits(1, syntax.seqForceScreenContentTools)))
            return false;

        if (syntax.seqForceScreenContentTools > 0) {
            uint32_t seqChooseIntegerMv = 0;
            if (!reader.ReadBits(1, seqChooseIntegerMv) || (!seqChooseIntegerMv && !reader.ReadBits(1, syntax.seqForceIntegerMv)))
                return false;
        }

        if (syntax.enableOrderHint && !reader.ReadBits(3, syntax.orderHintBitsMinus1))
            return false;
    }

    if (!reader.ReadBits(1, syntax.enableSuperres) || !reader.ReadBits(1, syntax.enableCdef) || !reader.ReadBits(1, syntax.enableRestoration))
        return false;

    return ReadVideoAV1ColorConfig(reader, syntax) && reader.ReadBits(1, syntax.filmGrainParamsPresent);
}

static bool FindVideoAV1SequenceSyntax(const uint8_t* data, size_t size, VideoAV1SequenceSyntaxVK& syntax) {
    size_t cursor = 0;
    while (cursor < size) {
        video::av1::ObuSpan span = {};
        if (!video::av1::ReadObuHeader(data, size, cursor, span) || span.payloadSize > (size - span.payloadOffset))
            return false;

        if (span.type == video::av1::ObuType::SequenceHeader)
            return ReadVideoAV1SequenceSyntax(data + span.payloadOffset, (size_t)span.payloadSize, syntax);
    }

    return false;
}

static bool ApplyVideoEncodeAV1SequenceHeaderOverride(StdVideoAV1SequenceHeader& header, const uint8_t* data, size_t size) {
    VideoAV1SequenceSyntaxVK syntax = {};
    if (!FindVideoAV1SequenceSyntax(data, size, syntax))
        return false;

    header.frame_width_bits_minus_1 = (uint8_t)syntax.frameWidthBitsMinus1;
    header.frame_height_bits_minus_1 = (uint8_t)syntax.frameHeightBitsMinus1;
    header.max_frame_width_minus_1 = (uint16_t)syntax.maxFrameWidthMinus1;
    header.max_frame_height_minus_1 = (uint16_t)syntax.maxFrameHeightMinus1;
    header.seq_force_screen_content_tools = (uint8_t)syntax.seqForceScreenContentTools;
    header.seq_force_integer_mv = (uint8_t)syntax.seqForceIntegerMv;

    return true;
}

// The app writes the sequence header with "WriteVideoAV1ObuHeaders" from the same desc, so a driver override must not change what frame headers and decoding depend on
static bool IsVideoAV1SequenceHeaderOverridden(const VideoAV1SequenceDesc& desc, const uint8_t* data, size_t size) {
    std::array<uint8_t, 256> written = {};
    video::bitstream::ByteWriter writer = {written.data(), written.size()};
    if (video::av1::WriteSequenceHeaderPayload(desc, writer) != Result::SUCCESS || writer.overflow)
        return true;

    VideoAV1SequenceSyntaxVK expected = {};
    VideoAV1SequenceSyntaxVK overridden = {};
    if (!ReadVideoAV1SequenceSyntax(written.data(), (size_t)writer.writtenSize, expected) || !FindVideoAV1SequenceSyntax(data, size, overridden))
        return true;

    return std::memcmp(&expected, &overridden, sizeof(expected)) != 0;
}

VideoSessionParametersVK::~VideoSessionParametersVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle)
        vk.DestroyVideoSessionParametersKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
}

void VideoSessionParametersVK::GetH264ReferenceIndexDefaults(uint8_t pictureParameterSetId, uint8_t& l0DefaultActiveMinus1, uint8_t& l1DefaultActiveMinus1) const {
    l0DefaultActiveMinus1 = 0;
    l1DefaultActiveMinus1 = 0;

    for (const ReferenceIndexDefaults& defaults : m_ReferenceIndexDefaults) {
        if (defaults.pictureParameterSetId == pictureParameterSetId) {
            l0DefaultActiveMinus1 = defaults.l0DefaultActiveMinus1;
            l1DefaultActiveMinus1 = defaults.l1DefaultActiveMinus1;
        }
    }
}

void VideoSessionParametersVK::GetH265ReferenceIndexDefaults(uint8_t sequenceParameterSetId, uint8_t pictureParameterSetId, uint8_t& l0DefaultActiveMinus1, uint8_t& l1DefaultActiveMinus1) const {
    l0DefaultActiveMinus1 = 0;
    l1DefaultActiveMinus1 = 0;

    for (const ReferenceIndexDefaults& defaults : m_ReferenceIndexDefaults) {
        if (defaults.sequenceParameterSetId == sequenceParameterSetId && defaults.pictureParameterSetId == pictureParameterSetId) {
            l0DefaultActiveMinus1 = defaults.l0DefaultActiveMinus1;
            l1DefaultActiveMinus1 = defaults.l1DefaultActiveMinus1;
        }
    }
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

NRI_INLINE Result VideoSessionParametersVK::Create(const VideoSessionParametersDesc& videoSessionParametersDesc) {
    VideoSessionVK& session = *(VideoSessionVK*)videoSessionParametersDesc.session;
    m_Session = &session;
    if (session.GetDesc().codec == VideoCodec::H265)
        return CreateH265(session, videoSessionParametersDesc.h265Parameters);
    if (session.GetDesc().codec == VideoCodec::AV1)
        return CreateAV1(session, videoSessionParametersDesc.av1Parameters);
    if (session.GetDesc().codec != VideoCodec::H264)
        return Result::UNSUPPORTED;

    const VideoH264SessionParametersDesc& h264Parameters = *videoSessionParametersDesc.h264Parameters;

    StdVideoH264ScalingLists defaultScalingLists = {};
    for (auto& list : defaultScalingLists.ScalingList4x4) {
        for (uint8_t& entry : list)
            entry = 16;
    }

    for (auto& list : defaultScalingLists.ScalingList8x8) {
        for (uint8_t& entry : list)
            entry = 16;
    }

    Scratch<StdVideoH264SequenceParameterSet> h264Sps = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH264SequenceParameterSet, h264Parameters.sequenceParameterSetNum);
    for (uint32_t i = 0; i < h264Parameters.sequenceParameterSetNum; i++) {
        h264Sps[i] = GetVideoH264SequenceParameterSet(h264Parameters.sequenceParameterSets[i]);
        h264Sps[i].pScalingLists = &defaultScalingLists;
    }

    Scratch<StdVideoH264PictureParameterSet> h264Pps = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH264PictureParameterSet, h264Parameters.pictureParameterSetNum);
    m_ReferenceIndexDefaults.resize(h264Parameters.pictureParameterSetNum);
    for (uint32_t i = 0; i < h264Parameters.pictureParameterSetNum; i++) {
        h264Pps[i] = GetVideoH264PictureParameterSet(h264Parameters.pictureParameterSets[i]);
        h264Pps[i].pScalingLists = &defaultScalingLists;

        m_ReferenceIndexDefaults[i] = {h264Pps[i].seq_parameter_set_id, h264Pps[i].pic_parameter_set_id, h264Pps[i].num_ref_idx_l0_default_active_minus1, h264Pps[i].num_ref_idx_l1_default_active_minus1};
    }

    VkVideoDecodeH264SessionParametersAddInfoKHR decodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR};
    decodeAddInfo.stdSPSCount = h264Parameters.sequenceParameterSetNum;
    decodeAddInfo.pStdSPSs = h264Sps;
    decodeAddInfo.stdPPSCount = h264Parameters.pictureParameterSetNum;
    decodeAddInfo.pStdPPSs = h264Pps;

    VkVideoDecodeH264SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
    decodeInfo.maxStdSPSCount = h264Parameters.maxSequenceParameterSetNum ? h264Parameters.maxSequenceParameterSetNum : h264Parameters.sequenceParameterSetNum;
    decodeInfo.maxStdPPSCount = h264Parameters.maxPictureParameterSetNum ? h264Parameters.maxPictureParameterSetNum : h264Parameters.pictureParameterSetNum;
    decodeInfo.pParametersAddInfo = (h264Parameters.sequenceParameterSetNum || h264Parameters.pictureParameterSetNum) ? &decodeAddInfo : nullptr;

    VkVideoEncodeH264SessionParametersAddInfoKHR encodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR};
    encodeAddInfo.stdSPSCount = h264Parameters.sequenceParameterSetNum;
    encodeAddInfo.pStdSPSs = h264Sps;
    encodeAddInfo.stdPPSCount = h264Parameters.pictureParameterSetNum;
    encodeAddInfo.pStdPPSs = h264Pps;

    VkVideoEncodeH264SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
    encodeInfo.maxStdSPSCount = h264Parameters.maxSequenceParameterSetNum ? h264Parameters.maxSequenceParameterSetNum : h264Parameters.sequenceParameterSetNum;
    encodeInfo.maxStdPPSCount = h264Parameters.maxPictureParameterSetNum ? h264Parameters.maxPictureParameterSetNum : h264Parameters.pictureParameterSetNum;
    encodeInfo.pParametersAddInfo = (h264Parameters.sequenceParameterSetNum || h264Parameters.pictureParameterSetNum) ? &encodeAddInfo : nullptr;

    return CreateNative(session, session.GetDesc().type == VideoSessionType::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
}

NRI_INLINE Result VideoSessionParametersVK::CreateH265(VideoSessionVK& session, const VideoH265SessionParametersDesc* parameters) {
    VideoH265VideoParameterSetDesc defaultVps = {};
    VideoH265SequenceParameterSetDesc defaultSps = {};
    VideoH265PictureParameterSetDesc defaultPps = {};
    VideoH265SessionParametersDesc defaultParameters = {};
    if (!parameters) {
        const uint8_t bitDepthMinus8 = (session.GetDesc().format == Format::P010_UNORM || session.GetDesc().format == Format::P016_UNORM) ? 2 : 0;
        defaultVps.flags = VideoH265VideoParameterSetBits::TEMPORAL_ID_NESTING;
        defaultVps.profileTierLevel.generalProfileIdc = (uint8_t)((session.GetDesc().format == Format::P010_UNORM || session.GetDesc().format == Format::P016_UNORM)
                ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10
                : STD_VIDEO_H265_PROFILE_IDC_MAIN);
        defaultVps.profileTierLevel.generalLevelIdc = GetVideoH265DefaultGeneralLevelIdc(session.GetDesc().width, session.GetDesc().height);
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
        parameters = &defaultParameters;
    }

    uint32_t shortTermRefPicSetNum = 0;
    for (uint32_t i = 0; i < parameters->sequenceParameterSetNum; i++) {
        const VideoH265SequenceParameterSetDesc& sps = parameters->sequenceParameterSets[i];
        shortTermRefPicSetNum += sps.numShortTermRefPicSets;
    }

    Scratch<StdVideoH265ProfileTierLevel> vpsProfileTierLevels = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265ProfileTierLevel, parameters->videoParameterSetNum);
    Scratch<StdVideoH265ProfileTierLevel> spsProfileTierLevels = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265ProfileTierLevel, parameters->sequenceParameterSetNum);
    Scratch<StdVideoH265DecPicBufMgr> vpsDecPicBufMgrs = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265DecPicBufMgr, parameters->videoParameterSetNum);
    Scratch<StdVideoH265DecPicBufMgr> spsDecPicBufMgrs = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265DecPicBufMgr, parameters->sequenceParameterSetNum);
    Scratch<StdVideoH265VideoParameterSet> videoParameterSets = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265VideoParameterSet, parameters->videoParameterSetNum);
    Scratch<StdVideoH265SequenceParameterSet> sequenceParameterSets = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265SequenceParameterSet, parameters->sequenceParameterSetNum);
    Scratch<StdVideoH265PictureParameterSet> pictureParameterSets = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265PictureParameterSet, parameters->pictureParameterSetNum);
    Scratch<StdVideoH265ScalingLists> spsScalingLists = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265ScalingLists, parameters->sequenceParameterSetNum);
    Scratch<StdVideoH265ScalingLists> ppsScalingLists = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265ScalingLists, parameters->pictureParameterSetNum);
    Scratch<StdVideoH265ShortTermRefPicSet> shortTermRefPicSets = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265ShortTermRefPicSet, shortTermRefPicSetNum);
    Scratch<StdVideoH265LongTermRefPicsSps> longTermRefPicsSps = NRI_ALLOCATE_SCRATCH(m_Device, StdVideoH265LongTermRefPicsSps, parameters->sequenceParameterSetNum);

    for (uint32_t i = 0; i < parameters->videoParameterSetNum; i++) {
        const VideoH265VideoParameterSetDesc& vps = parameters->videoParameterSets[i];
        FillVideoH265ProfileTierLevel(vpsProfileTierLevels[i], vps.profileTierLevel);
        FillVideoH265DecPicBufMgr(vpsDecPicBufMgrs[i], vps.decPicBufMgr);
        videoParameterSets[i] = GetVideoH265VideoParameterSet(vps, vpsProfileTierLevels[i], vpsDecPicBufMgrs[i]);
    }

    uint32_t firstShortTermRefPicSet = 0;
    for (uint32_t i = 0; i < parameters->sequenceParameterSetNum; i++) {
        const VideoH265SequenceParameterSetDesc& sps = parameters->sequenceParameterSets[i];
        for (uint32_t j = 0; j < sps.numShortTermRefPicSets; j++)
            shortTermRefPicSets[firstShortTermRefPicSet + j] = GetVideoH265ShortTermRefPicSet(sps.shortTermRefPicSets[j]);

        FillVideoH265ProfileTierLevel(spsProfileTierLevels[i], sps.profileTierLevel);
        FillVideoH265DecPicBufMgr(spsDecPicBufMgrs[i], sps.decPicBufMgr);
        const StdVideoH265ScalingLists* scalingLists = nullptr;
        if (sps.scalingLists) {
            spsScalingLists[i] = GetVideoH265ScalingLists(*sps.scalingLists);
            scalingLists = &spsScalingLists[i];
        }
        const StdVideoH265ShortTermRefPicSet* spsShortTermRefPicSets = sps.numShortTermRefPicSets ? &shortTermRefPicSets[firstShortTermRefPicSet] : nullptr;
        const StdVideoH265LongTermRefPicsSps* spsLongTermRefPics = nullptr;
        if (sps.longTermRefPicsSps) {
            longTermRefPicsSps[i] = GetVideoH265LongTermRefPicsSps(*sps.longTermRefPicsSps);
            spsLongTermRefPics = &longTermRefPicsSps[i];
        }
        sequenceParameterSets[i] = GetVideoH265SequenceParameterSet(sps, spsProfileTierLevels[i], spsDecPicBufMgrs[i], scalingLists, spsShortTermRefPicSets,
            spsLongTermRefPics);
        firstShortTermRefPicSet += sps.numShortTermRefPicSets;
    }

    m_ReferenceIndexDefaults.resize(parameters->pictureParameterSetNum);
    for (uint32_t i = 0; i < parameters->pictureParameterSetNum; i++) {
        const VideoH265PictureParameterSetDesc& pps = parameters->pictureParameterSets[i];
        const StdVideoH265ScalingLists* scalingLists = nullptr;
        if (pps.scalingLists) {
            ppsScalingLists[i] = GetVideoH265ScalingLists(*pps.scalingLists);
            scalingLists = &ppsScalingLists[i];
        }
        pictureParameterSets[i] = GetVideoH265PictureParameterSet(pps, scalingLists);

        m_ReferenceIndexDefaults[i] = {pps.sequenceParameterSetId, pps.pictureParameterSetId, pps.refIndexL0DefaultActiveMinus1, pps.refIndexL1DefaultActiveMinus1};
    }

    VkVideoDecodeH265SessionParametersAddInfoKHR decodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};
    decodeAddInfo.stdVPSCount = parameters->videoParameterSetNum;
    decodeAddInfo.pStdVPSs = videoParameterSets;
    decodeAddInfo.stdSPSCount = parameters->sequenceParameterSetNum;
    decodeAddInfo.pStdSPSs = sequenceParameterSets;
    decodeAddInfo.stdPPSCount = parameters->pictureParameterSetNum;
    decodeAddInfo.pStdPPSs = pictureParameterSets;

    VkVideoDecodeH265SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR};
    decodeInfo.maxStdVPSCount = parameters->maxVideoParameterSetNum ? parameters->maxVideoParameterSetNum : parameters->videoParameterSetNum;
    decodeInfo.maxStdSPSCount = parameters->maxSequenceParameterSetNum ? parameters->maxSequenceParameterSetNum : parameters->sequenceParameterSetNum;
    decodeInfo.maxStdPPSCount = parameters->maxPictureParameterSetNum ? parameters->maxPictureParameterSetNum : parameters->pictureParameterSetNum;
    decodeInfo.pParametersAddInfo = (parameters->videoParameterSetNum || parameters->sequenceParameterSetNum || parameters->pictureParameterSetNum)
        ? &decodeAddInfo
        : nullptr;

    VkVideoEncodeH265SessionParametersAddInfoKHR encodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};
    encodeAddInfo.stdVPSCount = parameters->videoParameterSetNum;
    encodeAddInfo.pStdVPSs = videoParameterSets;
    encodeAddInfo.stdSPSCount = parameters->sequenceParameterSetNum;
    encodeAddInfo.pStdSPSs = sequenceParameterSets;
    encodeAddInfo.stdPPSCount = parameters->pictureParameterSetNum;
    encodeAddInfo.pStdPPSs = pictureParameterSets;

    VkVideoEncodeH265SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR};
    encodeInfo.maxStdVPSCount = parameters->maxVideoParameterSetNum ? parameters->maxVideoParameterSetNum : parameters->videoParameterSetNum;
    encodeInfo.maxStdSPSCount = parameters->maxSequenceParameterSetNum ? parameters->maxSequenceParameterSetNum : parameters->sequenceParameterSetNum;
    encodeInfo.maxStdPPSCount = parameters->maxPictureParameterSetNum ? parameters->maxPictureParameterSetNum : parameters->pictureParameterSetNum;
    encodeInfo.pParametersAddInfo = (parameters->videoParameterSetNum || parameters->sequenceParameterSetNum || parameters->pictureParameterSetNum)
        ? &encodeAddInfo
        : nullptr;

    return CreateNative(session, session.GetDesc().type == VideoSessionType::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
}

NRI_INLINE Result VideoSessionParametersVK::CreateAV1(VideoSessionVK& session, const VideoAV1SessionParametersDesc* parameters) {
    if (parameters) {
        FillVideoAV1ColorConfig(m_AV1ColorConfig, parameters->sequence);
        m_AV1TimingInfo = {};
        const StdVideoAV1TimingInfo* timingInfo = nullptr;
        if (parameters->sequence.numUnitsInDisplayTick && parameters->sequence.timeScale) {
            m_AV1TimingInfo.num_units_in_display_tick = parameters->sequence.numUnitsInDisplayTick;
            m_AV1TimingInfo.time_scale = parameters->sequence.timeScale;
            m_AV1TimingInfo.num_ticks_per_picture_minus_1 = parameters->sequence.numTicksPerPictureMinus1;
            timingInfo = &m_AV1TimingInfo;
        }
        if (session.GetDesc().type == VideoSessionType::DECODE) {
            m_AV1ColorConfig.flags.color_description_present_flag = false;
            m_AV1ColorConfig.chroma_sample_position = {};
            timingInfo = &m_AV1TimingInfo;
        }
        FillVideoAV1SequenceHeader(m_AV1SequenceHeader, parameters->sequence, m_AV1ColorConfig, timingInfo);
        m_AV1OperatingPoint.seq_level_idx = video::av1::GetLevelIndex(parameters->sequence.level, session.GetDesc().width, session.GetDesc().height);
    } else {
        m_AV1ColorConfig.BitDepth = (session.GetDesc().format == Format::P010_UNORM || session.GetDesc().format == Format::P016_UNORM) ? 10 : 8;
        m_AV1ColorConfig.subsampling_x = 1;
        m_AV1ColorConfig.subsampling_y = 1;
        m_AV1ColorConfig.flags.color_description_present_flag = true;
        m_AV1ColorConfig.color_primaries = STD_VIDEO_AV1_COLOR_PRIMARIES_BT_709;
        m_AV1ColorConfig.transfer_characteristics = STD_VIDEO_AV1_TRANSFER_CHARACTERISTICS_BT_709;
        m_AV1ColorConfig.matrix_coefficients = STD_VIDEO_AV1_MATRIX_COEFFICIENTS_BT_709;
        m_AV1ColorConfig.chroma_sample_position = STD_VIDEO_AV1_CHROMA_SAMPLE_POSITION_VERTICAL;
        m_AV1SequenceHeader.seq_profile = STD_VIDEO_AV1_PROFILE_MAIN;
        m_AV1SequenceHeader.flags.enable_order_hint = true;
        m_AV1SequenceHeader.frame_width_bits_minus_1 = GetVideoAV1SizeBitsMinus1(session.GetDesc().width);
        m_AV1SequenceHeader.frame_height_bits_minus_1 = GetVideoAV1SizeBitsMinus1(session.GetDesc().height);
        m_AV1SequenceHeader.max_frame_width_minus_1 = (uint16_t)(session.GetDesc().width - 1);
        m_AV1SequenceHeader.max_frame_height_minus_1 = (uint16_t)(session.GetDesc().height - 1);
        m_AV1SequenceHeader.order_hint_bits_minus_1 = 7;
        m_AV1SequenceHeader.seq_force_integer_mv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
        m_AV1SequenceHeader.seq_force_screen_content_tools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
        m_AV1SequenceHeader.pColorConfig = &m_AV1ColorConfig;
        m_AV1OperatingPoint.seq_level_idx = video::av1::GetLevelIndex(0, session.GetDesc().width, session.GetDesc().height);
    }

    if (session.GetDesc().type == VideoSessionType::ENCODE) {
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

    Scratch<uint8_t> data = NRI_ALLOCATE_SCRATCH(m_Device, uint8_t, dataSize);
    vkResult = vk.GetEncodedVideoSessionParametersKHR(m_Device, &getInfo, &feedbackInfo, &dataSize, data);
    if (vkResult != VK_SUCCESS || !feedbackInfo.hasOverrides)
        return Result::SUCCESS;

    // The app emits the sequence header with "WriteVideoAV1ObuHeaders" from the same desc, which can't reproduce driver overrides
    if (parameters) {
        if (IsVideoAV1SequenceHeaderOverridden(parameters->sequence, data, dataSize)) {
            NRI_REPORT_ERROR(&m_Device, "Vulkan AV1 encode driver overrides the sequence header, which 'WriteVideoAV1ObuHeaders' can't reproduce");

            return Result::UNSUPPORTED;
        }

        return Result::SUCCESS;
    }

    if (!ApplyVideoEncodeAV1SequenceHeaderOverride(m_AV1SequenceHeader, data, dataSize))
        return Result::SUCCESS;

    vk.DestroyVideoSessionParametersKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    m_Handle = VK_NULL_HANDLE;

    return CreateNative(session, &encodeQualityInfo);
}
