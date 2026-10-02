// © 2026 NVIDIA Corporation

#pragma once

#include <algorithm>

namespace nri {

static_assert(video::av1::REFERENCE_NAME_NUM == VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR);
static_assert(video::av1::REFERENCE_NAME_NUM == STD_VIDEO_AV1_PRIMARY_REF_NONE);

static inline VkVideoEncodeRateControlModeFlagBitsKHR GetVideoEncodeRateControlMode(VideoEncodeRateControlMode mode) {
    switch (mode) {
        case VideoEncodeRateControlMode::CQP:
            return VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DISABLED_BIT_KHR;
        case VideoEncodeRateControlMode::CBR:
            return VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR;
        case VideoEncodeRateControlMode::VBR:
            return VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR;
        default:
            return VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DISABLED_BIT_KHR;
    }
}

static inline uint32_t GetSupportedVideoEncodeRateControlModes(VkVideoEncodeRateControlModeFlagsKHR modes) {
    uint32_t result = video::ENCODE_RATE_CONTROL_CQP;
    if (modes & VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR)
        result |= video::ENCODE_RATE_CONTROL_CBR;
    if (modes & VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR)
        result |= video::ENCODE_RATE_CONTROL_VBR;

    return result;
}

static inline void FillVideoEncodeRateControl(const VideoEncodeRateControlDesc& desc, VkVideoEncodeRateControlInfoKHR& info, VkVideoEncodeRateControlLayerInfoKHR& layer) {
    info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR};
    info.rateControlMode = GetVideoEncodeRateControlMode(desc.mode);

    if (desc.mode == VideoEncodeRateControlMode::CQP)
        return;

    layer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR};
    layer.averageBitrate = desc.targetBitrate;
    layer.maxBitrate = (desc.mode == VideoEncodeRateControlMode::VBR && desc.maxBitrate) ? desc.maxBitrate : desc.targetBitrate;
    layer.frameRateNumerator = desc.frameRateNumerator ? desc.frameRateNumerator : 30;
    layer.frameRateDenominator = desc.frameRateDenominator ? desc.frameRateDenominator : 1;

    info.layerCount = 1;
    info.pLayers = &layer;
    info.virtualBufferSizeInMs = desc.virtualBufferSizeMs ? desc.virtualBufferSizeMs : 1000;
    info.initialVirtualBufferSizeInMs = desc.initialVirtualBufferSizeMs ? desc.initialVirtualBufferSizeMs : info.virtualBufferSizeInMs;
}

struct VideoEncodeRateControlVK {
    VkVideoEncodeRateControlInfoKHR info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeRateControlLayerInfoKHR layer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR};
    VkVideoEncodeH264RateControlInfoKHR h264 = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeH265RateControlInfoKHR h265 = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_KHR};
    VkVideoEncodeAV1RateControlInfoKHR av1 = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_RATE_CONTROL_INFO_KHR};
};

// Fills a self-referencing native chain: "rateControl" must not be copied or moved afterwards
static inline void FillVideoEncodeRateControl(VideoEncodeRateControlVK& rateControl, const VideoEncodeRateControlDesc& desc, const VideoSessionDesc& sessionDesc) {
    FillVideoEncodeRateControl(desc, rateControl.info, rateControl.layer);

    if (sessionDesc.codec == VideoCodec::H264) {
        rateControl.h264.gopFrameCount = sessionDesc.maxReferenceNum ? 60 : 1;
        rateControl.h264.idrPeriod = rateControl.h264.gopFrameCount;
        rateControl.h264.consecutiveBFrameCount = sessionDesc.maxReferenceNum > 1 ? 1 : 0;
        rateControl.h264.temporalLayerCount = 1;
        rateControl.info.pNext = &rateControl.h264;
    } else if (sessionDesc.codec == VideoCodec::H265) {
        rateControl.h265.gopFrameCount = sessionDesc.maxReferenceNum ? 60 : 1;
        rateControl.h265.idrPeriod = rateControl.h265.gopFrameCount;
        rateControl.h265.consecutiveBFrameCount = sessionDesc.maxReferenceNum > 1 ? 1 : 0;
        rateControl.h265.subLayerCount = 1;
        rateControl.info.pNext = &rateControl.h265;
    } else if (sessionDesc.codec == VideoCodec::AV1) {
        rateControl.av1.flags = VK_VIDEO_ENCODE_AV1_RATE_CONTROL_REGULAR_GOP_BIT_KHR | VK_VIDEO_ENCODE_AV1_RATE_CONTROL_REFERENCE_PATTERN_FLAT_BIT_KHR;
        rateControl.av1.gopFrameCount = 300;
        rateControl.av1.keyFramePeriod = 300;
        rateControl.av1.consecutiveBipredictiveFrameCount = 1;
        rateControl.info.pNext = &rateControl.av1;
    }
}

// Compares the native rate control state produced by two descriptions (QP values are per-picture, not part of the state)
static inline bool IsVideoEncodeRateControlEqual(const VideoEncodeRateControlDesc& a, const VideoEncodeRateControlDesc& b) {
    VkVideoEncodeRateControlInfoKHR infoA = {};
    VkVideoEncodeRateControlInfoKHR infoB = {};
    VkVideoEncodeRateControlLayerInfoKHR layerA = {};
    VkVideoEncodeRateControlLayerInfoKHR layerB = {};
    FillVideoEncodeRateControl(a, infoA, layerA);
    FillVideoEncodeRateControl(b, infoB, layerB);

    if (infoA.rateControlMode != infoB.rateControlMode || infoA.layerCount != infoB.layerCount)
        return false;

    if (infoA.virtualBufferSizeInMs != infoB.virtualBufferSizeInMs || infoA.initialVirtualBufferSizeInMs != infoB.initialVirtualBufferSizeInMs)
        return false;

    if (!infoA.layerCount)
        return true;

    return layerA.averageBitrate == layerB.averageBitrate && layerA.maxBitrate == layerB.maxBitrate && layerA.frameRateNumerator == layerB.frameRateNumerator
        && layerA.frameRateDenominator == layerB.frameRateDenominator;
}

static inline VkVideoReferenceSlotInfoKHR GetVideoSetupReferenceSlotForBegin(const VkVideoReferenceSlotInfoKHR& setupReferenceSlot) {
    VkVideoReferenceSlotInfoKHR beginReferenceSlot = setupReferenceSlot;
    beginReferenceSlot.slotIndex = -1;
    return beginReferenceSlot;
}

static inline void FillVideoCapabilities(VideoCapabilities& videoCapabilities, const VideoSessionDesc& videoSessionDesc, const VkVideoCapabilitiesKHR& capabilities) {
    videoCapabilities = {};
    videoCapabilities.widthMin = capabilities.minCodedExtent.width;
    videoCapabilities.heightMin = capabilities.minCodedExtent.height;
    videoCapabilities.widthMax = capabilities.maxCodedExtent.width;
    videoCapabilities.heightMax = capabilities.maxCodedExtent.height;
    videoCapabilities.pictureAccessGranularityWidth = capabilities.pictureAccessGranularity.width;
    videoCapabilities.pictureAccessGranularityHeight = capabilities.pictureAccessGranularity.height;
    videoCapabilities.maxReferenceNum = std::min(capabilities.maxActiveReferencePictures, capabilities.maxDpbSlots ? capabilities.maxDpbSlots - 1 : 0);
    videoCapabilities.bitstreamOffsetAlignment = (uint32_t)capabilities.minBitstreamBufferOffsetAlignment;
    videoCapabilities.bitstreamSizeAlignment = (uint32_t)capabilities.minBitstreamBufferSizeAlignment;
    videoCapabilities.bitstreamSizeMax = uint64_t(-1);
    videoCapabilities.metadataOffsetAlignment = 1;
    videoCapabilities.resolvedMetadataOffsetAlignment = 4;
    videoCapabilities.decodeBitstreamSourceMask = videoSessionDesc.type == VideoSessionType::DECODE ? VideoDecodeBitstreamSourceBits::BUFFER : VideoDecodeBitstreamSourceBits::NONE;
    videoCapabilities.encodeBitstreamRangeSizeSupported = true;

    if (!(capabilities.flags & VK_VIDEO_CAPABILITY_SEPARATE_REFERENCE_IMAGES_BIT_KHR)) {
        videoCapabilities.dpbTextureArrayMinLayerNum = videoSessionDesc.maxReferenceNum + 1;
        videoCapabilities.dpbTextureArrayRequired = true;
    }
}

static inline VideoAV1EncodeFeatureBits GetSupportedVideoEncodeAV1FeatureFlags(VkVideoEncodeAV1StdFlagsKHR stdSyntaxFlags) {
    VideoAV1EncodeFeatureBits flags = VideoAV1EncodeFeatureBits::NONE;

    if (stdSyntaxFlags & VK_VIDEO_ENCODE_AV1_STD_DELTA_Q_BIT_KHR)
        flags |= VideoAV1EncodeFeatureBits::QUANTIZATION_DELTAS;

    return flags;
}

static inline uint32_t GetVideoAV1LevelFromSeqIndex(uint32_t seqLevelIdx) {
    return (2 + seqLevelIdx / 4) * 10 + seqLevelIdx % 4;
}

static inline void FillVideoDecodeAV1Capabilities(VideoAV1Capabilities& videoAV1Capabilities, const VkVideoDecodeAV1CapabilitiesKHR& capabilities) {
    videoAV1Capabilities = {};
    videoAV1Capabilities.av1MaxLevel = GetVideoAV1LevelFromSeqIndex(capabilities.maxLevel);
}

static inline void FillVideoEncodeAV1Capabilities(VideoAV1Capabilities& videoAV1Capabilities, const VkVideoEncodeAV1CapabilitiesKHR& capabilities) {
    videoAV1Capabilities = {};
    videoAV1Capabilities.av1MaxLevel = GetVideoAV1LevelFromSeqIndex(capabilities.maxLevel);
    videoAV1Capabilities.av1MaxTileColumnNum = capabilities.maxTiles.width;
    videoAV1Capabilities.av1MaxTileRowNum = capabilities.maxTiles.height;
    videoAV1Capabilities.av1MinTileWidth = capabilities.minTileSize.width;
    videoAV1Capabilities.av1MinTileHeight = capabilities.minTileSize.height;
    videoAV1Capabilities.av1MaxTileWidth = capabilities.maxTileSize.width;
    videoAV1Capabilities.av1MaxTileHeight = capabilities.maxTileSize.height;
    videoAV1Capabilities.av1SuperblockSizeMask = capabilities.superblockSizes;
    videoAV1Capabilities.av1MaxSingleReferenceNum = capabilities.maxSingleReferenceCount;
    videoAV1Capabilities.av1SingleReferenceNameMask = capabilities.singleReferenceNameMask;
    videoAV1Capabilities.av1MaxUnidirectionalCompoundReferenceNum = capabilities.maxUnidirectionalCompoundReferenceCount;
    videoAV1Capabilities.av1UnidirectionalCompoundReferenceNameMask = capabilities.unidirectionalCompoundReferenceNameMask;
    videoAV1Capabilities.av1MaxBidirectionalCompoundReferenceNum = capabilities.maxBidirectionalCompoundReferenceCount;
    videoAV1Capabilities.av1BidirectionalCompoundReferenceNameMask = capabilities.bidirectionalCompoundReferenceNameMask;
    videoAV1Capabilities.av1MaxTemporalLayerNum = capabilities.maxTemporalLayerCount;
    videoAV1Capabilities.av1MaxSpatialLayerNum = capabilities.maxSpatialLayerCount;
    videoAV1Capabilities.av1MaxOperatingPointNum = capabilities.maxOperatingPoints;
    videoAV1Capabilities.av1MinQIndex = capabilities.minQIndex;
    videoAV1Capabilities.av1MaxQIndex = capabilities.maxQIndex;
    videoAV1Capabilities.av1EncodeSupportedFeatureFlags = GetSupportedVideoEncodeAV1FeatureFlags(capabilities.stdSyntaxFlags);
}

static inline uint8_t GetVideoAV1SizeBitsMinus1(uint32_t value) {
    uint8_t bits = 0;
    value--;
    do {
        bits++;
        value >>= 1;
    } while (value);

    return bits - 1;
}

// "general_level_idc" is 30 times the level number (H.265 A.4.1)
static inline StdVideoH265LevelIdc GetVideoH265LevelIdc(uint8_t generalLevelIdc) {
    switch (generalLevelIdc) {
        case 30:
            return STD_VIDEO_H265_LEVEL_IDC_1_0;
        case 60:
            return STD_VIDEO_H265_LEVEL_IDC_2_0;
        case 63:
            return STD_VIDEO_H265_LEVEL_IDC_2_1;
        case 90:
            return STD_VIDEO_H265_LEVEL_IDC_3_0;
        case 93:
            return STD_VIDEO_H265_LEVEL_IDC_3_1;
        case 120:
            return STD_VIDEO_H265_LEVEL_IDC_4_0;
        case 123:
            return STD_VIDEO_H265_LEVEL_IDC_4_1;
        case 150:
            return STD_VIDEO_H265_LEVEL_IDC_5_0;
        case 153:
            return STD_VIDEO_H265_LEVEL_IDC_5_1;
        case 156:
            return STD_VIDEO_H265_LEVEL_IDC_5_2;
        case 180:
            return STD_VIDEO_H265_LEVEL_IDC_6_0;
        case 183:
            return STD_VIDEO_H265_LEVEL_IDC_6_1;
        case 186:
            return STD_VIDEO_H265_LEVEL_IDC_6_2;
        default:
            return STD_VIDEO_H265_LEVEL_IDC_INVALID;
    }
}

static inline void FillVideoH265ProfileTierLevel(StdVideoH265ProfileTierLevel& profileTierLevel, const VideoH265ProfileTierLevelDesc& desc) {
    profileTierLevel = {};
    profileTierLevel.flags.general_tier_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::TIER);
    profileTierLevel.flags.general_progressive_source_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::PROGRESSIVE_SOURCE);
    profileTierLevel.flags.general_interlaced_source_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::INTERLACED_SOURCE);
    profileTierLevel.flags.general_non_packed_constraint_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::NON_PACKED_CONSTRAINT);
    profileTierLevel.flags.general_frame_only_constraint_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::FRAME_ONLY_CONSTRAINT);
    profileTierLevel.general_profile_idc = (StdVideoH265ProfileIdc)desc.generalProfileIdc;
    profileTierLevel.general_level_idc = GetVideoH265LevelIdc(desc.generalLevelIdc);
}

static inline void FillVideoH265DecPicBufMgr(StdVideoH265DecPicBufMgr& decPicBufMgr, const VideoH265DecPicBufMgrDesc& desc) {
    decPicBufMgr = {};
    for (uint32_t i = 0; i < STD_VIDEO_H265_SUBLAYERS_LIST_SIZE; i++) {
        decPicBufMgr.max_dec_pic_buffering_minus1[i] = desc.maxDecPicBufferingMinus1[i];
        decPicBufMgr.max_num_reorder_pics[i] = desc.maxNumReorderPics[i];
        decPicBufMgr.max_latency_increase_plus1[i] = desc.maxLatencyIncreasePlus1[i];
    }
}

static inline StdVideoH265ScalingLists GetVideoH265ScalingLists(const VideoH265ScalingListsDesc& desc) {
    StdVideoH265ScalingLists scalingLists = {};
    for (uint32_t i = 0; i < STD_VIDEO_H265_SCALING_LIST_4X4_NUM_LISTS; i++)
        for (uint32_t j = 0; j < STD_VIDEO_H265_SCALING_LIST_4X4_NUM_ELEMENTS; j++)
            scalingLists.ScalingList4x4[i][j] = desc.scalingList4x4[i][j];
    for (uint32_t i = 0; i < STD_VIDEO_H265_SCALING_LIST_8X8_NUM_LISTS; i++)
        for (uint32_t j = 0; j < STD_VIDEO_H265_SCALING_LIST_8X8_NUM_ELEMENTS; j++)
            scalingLists.ScalingList8x8[i][j] = desc.scalingList8x8[i][j];
    for (uint32_t i = 0; i < STD_VIDEO_H265_SCALING_LIST_16X16_NUM_LISTS; i++) {
        for (uint32_t j = 0; j < STD_VIDEO_H265_SCALING_LIST_16X16_NUM_ELEMENTS; j++)
            scalingLists.ScalingList16x16[i][j] = desc.scalingList16x16[i][j];
        scalingLists.ScalingListDCCoef16x16[i] = desc.scalingListDCCoef16x16[i];
    }
    for (uint32_t i = 0; i < STD_VIDEO_H265_SCALING_LIST_32X32_NUM_LISTS; i++) {
        for (uint32_t j = 0; j < STD_VIDEO_H265_SCALING_LIST_32X32_NUM_ELEMENTS; j++)
            scalingLists.ScalingList32x32[i][j] = desc.scalingList32x32[i][j];
        scalingLists.ScalingListDCCoef32x32[i] = desc.scalingListDCCoef32x32[i];
    }

    return scalingLists;
}

static inline StdVideoH265ShortTermRefPicSet GetVideoH265ShortTermRefPicSet(const VideoH265ShortTermRefPicSetDesc& desc) {
    StdVideoH265ShortTermRefPicSet refPicSet = {};
    refPicSet.flags.inter_ref_pic_set_prediction_flag = !!(desc.flags & VideoH265ShortTermRefPicSetBits::INTER_REF_PIC_SET_PREDICTION);
    refPicSet.flags.delta_rps_sign = !!(desc.flags & VideoH265ShortTermRefPicSetBits::DELTA_RPS_SIGN);
    refPicSet.delta_idx_minus1 = desc.deltaIdxMinus1;
    refPicSet.use_delta_flag = desc.useDeltaFlag;
    refPicSet.abs_delta_rps_minus1 = desc.absDeltaRpsMinus1;
    refPicSet.used_by_curr_pic_flag = desc.usedByCurrPicFlag;
    refPicSet.used_by_curr_pic_s0_flag = desc.usedByCurrPicS0Flag;
    refPicSet.used_by_curr_pic_s1_flag = desc.usedByCurrPicS1Flag;
    refPicSet.num_negative_pics = desc.numNegativePics;
    refPicSet.num_positive_pics = desc.numPositivePics;
    for (uint32_t i = 0; i < STD_VIDEO_H265_MAX_DPB_SIZE; i++) {
        refPicSet.delta_poc_s0_minus1[i] = desc.deltaPocS0Minus1[i];
        refPicSet.delta_poc_s1_minus1[i] = desc.deltaPocS1Minus1[i];
    }

    return refPicSet;
}

static inline StdVideoH265LongTermRefPicsSps GetVideoH265LongTermRefPicsSps(const VideoH265LongTermRefPicsSpsDesc& desc) {
    StdVideoH265LongTermRefPicsSps longTermRefPics = {};
    longTermRefPics.used_by_curr_pic_lt_sps_flag = desc.usedByCurrPicLtSpsFlag;
    for (uint32_t i = 0; i < STD_VIDEO_H265_MAX_LONG_TERM_REF_PICS_SPS; i++)
        longTermRefPics.lt_ref_pic_poc_lsb_sps[i] = desc.ltRefPicPocLsbSps[i];

    return longTermRefPics;
}

static inline StdVideoH265VideoParameterSet GetVideoH265VideoParameterSet(const VideoH265VideoParameterSetDesc& desc, const StdVideoH265ProfileTierLevel& profileTierLevel,
    const StdVideoH265DecPicBufMgr& decPicBufMgr) {
    StdVideoH265VideoParameterSet vps = {};
    vps.flags.vps_temporal_id_nesting_flag = !!(desc.flags & VideoH265VideoParameterSetBits::TEMPORAL_ID_NESTING);
    vps.flags.vps_sub_layer_ordering_info_present_flag = !!(desc.flags & VideoH265VideoParameterSetBits::SUB_LAYER_ORDERING_INFO_PRESENT);
    vps.flags.vps_timing_info_present_flag = !!(desc.flags & VideoH265VideoParameterSetBits::TIMING_INFO_PRESENT);
    vps.flags.vps_poc_proportional_to_timing_flag = !!(desc.flags & VideoH265VideoParameterSetBits::POC_PROPORTIONAL_TO_TIMING);
    vps.vps_video_parameter_set_id = desc.videoParameterSetId;
    vps.vps_max_sub_layers_minus1 = desc.maxSubLayersMinus1;
    vps.vps_num_units_in_tick = desc.numUnitsInTick;
    vps.vps_time_scale = desc.timeScale;
    vps.vps_num_ticks_poc_diff_one_minus1 = desc.numTicksPocDiffOneMinus1;
    vps.pDecPicBufMgr = &decPicBufMgr;
    vps.pProfileTierLevel = &profileTierLevel;

    return vps;
}

static inline StdVideoH265SequenceParameterSet GetVideoH265SequenceParameterSet(const VideoH265SequenceParameterSetDesc& desc,
    const StdVideoH265ProfileTierLevel& profileTierLevel, const StdVideoH265DecPicBufMgr& decPicBufMgr, const StdVideoH265ScalingLists* scalingLists,
    const StdVideoH265ShortTermRefPicSet* shortTermRefPicSets, const StdVideoH265LongTermRefPicsSps* longTermRefPicsSps) {
    StdVideoH265SequenceParameterSet sps = {};
    sps.flags.sps_temporal_id_nesting_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::TEMPORAL_ID_NESTING);
    sps.flags.separate_colour_plane_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::SEPARATE_COLOUR_PLANE);
    sps.flags.conformance_window_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::CONFORMANCE_WINDOW);
    sps.flags.sps_sub_layer_ordering_info_present_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::SUB_LAYER_ORDERING_INFO_PRESENT);
    sps.flags.scaling_list_enabled_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::SCALING_LIST_ENABLED);
    sps.flags.sps_scaling_list_data_present_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::SCALING_LIST_DATA_PRESENT);
    sps.flags.amp_enabled_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::AMP_ENABLED);
    sps.flags.sample_adaptive_offset_enabled_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::SAMPLE_ADAPTIVE_OFFSET_ENABLED);
    sps.flags.pcm_enabled_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::PCM_ENABLED);
    sps.flags.pcm_loop_filter_disabled_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::PCM_LOOP_FILTER_DISABLED);
    sps.flags.long_term_ref_pics_present_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::LONG_TERM_REF_PICS_PRESENT);
    sps.flags.sps_temporal_mvp_enabled_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::TEMPORAL_MVP_ENABLED);
    sps.flags.strong_intra_smoothing_enabled_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED);
    sps.flags.vui_parameters_present_flag = !!(desc.flags & VideoH265SequenceParameterSetBits::VUI_PARAMETERS_PRESENT);
    sps.chroma_format_idc = (StdVideoH265ChromaFormatIdc)desc.chromaFormatIdc;
    sps.pic_width_in_luma_samples = desc.pictureWidthInLumaSamples;
    sps.pic_height_in_luma_samples = desc.pictureHeightInLumaSamples;
    sps.sps_video_parameter_set_id = desc.videoParameterSetId;
    sps.sps_max_sub_layers_minus1 = desc.maxSubLayersMinus1;
    sps.sps_seq_parameter_set_id = desc.sequenceParameterSetId;
    sps.bit_depth_luma_minus8 = desc.bitDepthLumaMinus8;
    sps.bit_depth_chroma_minus8 = desc.bitDepthChromaMinus8;
    sps.log2_max_pic_order_cnt_lsb_minus4 = desc.log2MaxPictureOrderCountLsbMinus4;
    sps.log2_min_luma_coding_block_size_minus3 = desc.log2MinLumaCodingBlockSizeMinus3;
    sps.log2_diff_max_min_luma_coding_block_size = desc.log2DiffMaxMinLumaCodingBlockSize;
    sps.log2_min_luma_transform_block_size_minus2 = desc.log2MinLumaTransformBlockSizeMinus2;
    sps.log2_diff_max_min_luma_transform_block_size = desc.log2DiffMaxMinLumaTransformBlockSize;
    sps.max_transform_hierarchy_depth_inter = desc.maxTransformHierarchyDepthInter;
    sps.max_transform_hierarchy_depth_intra = desc.maxTransformHierarchyDepthIntra;
    sps.num_short_term_ref_pic_sets = desc.numShortTermRefPicSets;
    sps.num_long_term_ref_pics_sps = desc.numLongTermRefPicsSps;
    sps.pcm_sample_bit_depth_luma_minus1 = desc.pcmSampleBitDepthLumaMinus1;
    sps.pcm_sample_bit_depth_chroma_minus1 = desc.pcmSampleBitDepthChromaMinus1;
    sps.log2_min_pcm_luma_coding_block_size_minus3 = desc.log2MinPcmLumaCodingBlockSizeMinus3;
    sps.log2_diff_max_min_pcm_luma_coding_block_size = desc.log2DiffMaxMinPcmLumaCodingBlockSize;
    sps.conf_win_left_offset = desc.confWinLeftOffset;
    sps.conf_win_right_offset = desc.confWinRightOffset;
    sps.conf_win_top_offset = desc.confWinTopOffset;
    sps.conf_win_bottom_offset = desc.confWinBottomOffset;
    sps.pProfileTierLevel = &profileTierLevel;
    sps.pDecPicBufMgr = &decPicBufMgr;
    sps.pScalingLists = scalingLists;
    sps.pShortTermRefPicSet = shortTermRefPicSets;
    sps.pLongTermRefPicsSps = longTermRefPicsSps;

    return sps;
}

static inline StdVideoH265PictureParameterSet GetVideoH265PictureParameterSet(const VideoH265PictureParameterSetDesc& desc, const StdVideoH265ScalingLists* scalingLists) {
    StdVideoH265PictureParameterSet pps = {};
    pps.flags.dependent_slice_segments_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::DEPENDENT_SLICE_SEGMENTS_ENABLED);
    pps.flags.output_flag_present_flag = !!(desc.flags & VideoH265PictureParameterSetBits::OUTPUT_FLAG_PRESENT);
    pps.flags.sign_data_hiding_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::SIGN_DATA_HIDING_ENABLED);
    pps.flags.cabac_init_present_flag = !!(desc.flags & VideoH265PictureParameterSetBits::CABAC_INIT_PRESENT);
    pps.flags.constrained_intra_pred_flag = !!(desc.flags & VideoH265PictureParameterSetBits::CONSTRAINED_INTRA_PRED);
    pps.flags.transform_skip_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::TRANSFORM_SKIP_ENABLED);
    pps.flags.cu_qp_delta_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::CU_QP_DELTA_ENABLED);
    pps.flags.pps_slice_chroma_qp_offsets_present_flag = !!(desc.flags & VideoH265PictureParameterSetBits::SLICE_CHROMA_QP_OFFSETS_PRESENT);
    pps.flags.weighted_pred_flag = !!(desc.flags & VideoH265PictureParameterSetBits::WEIGHTED_PRED);
    pps.flags.weighted_bipred_flag = !!(desc.flags & VideoH265PictureParameterSetBits::WEIGHTED_BIPRED);
    pps.flags.transquant_bypass_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::TRANSQUANT_BYPASS_ENABLED);
    pps.flags.tiles_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::TILES_ENABLED);
    pps.flags.entropy_coding_sync_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::ENTROPY_CODING_SYNC_ENABLED);
    pps.flags.uniform_spacing_flag = !!(desc.flags & VideoH265PictureParameterSetBits::UNIFORM_SPACING);
    pps.flags.loop_filter_across_tiles_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_TILES_ENABLED);
    pps.flags.pps_loop_filter_across_slices_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_SLICES_ENABLED);
    pps.flags.deblocking_filter_control_present_flag = !!(desc.flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_CONTROL_PRESENT);
    pps.flags.deblocking_filter_override_enabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_OVERRIDE_ENABLED);
    pps.flags.pps_deblocking_filter_disabled_flag = !!(desc.flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_DISABLED);
    pps.flags.pps_scaling_list_data_present_flag = !!(desc.flags & VideoH265PictureParameterSetBits::SCALING_LIST_DATA_PRESENT);
    pps.flags.lists_modification_present_flag = !!(desc.flags & VideoH265PictureParameterSetBits::LISTS_MODIFICATION_PRESENT);
    pps.flags.slice_segment_header_extension_present_flag = !!(desc.flags & VideoH265PictureParameterSetBits::SLICE_SEGMENT_HEADER_EXTENSION_PRESENT);
    pps.pps_pic_parameter_set_id = desc.pictureParameterSetId;
    pps.pps_seq_parameter_set_id = desc.sequenceParameterSetId;
    pps.sps_video_parameter_set_id = desc.videoParameterSetId;
    pps.num_extra_slice_header_bits = desc.numExtraSliceHeaderBits;
    pps.num_ref_idx_l0_default_active_minus1 = desc.refIndexL0DefaultActiveMinus1;
    pps.num_ref_idx_l1_default_active_minus1 = desc.refIndexL1DefaultActiveMinus1;
    pps.init_qp_minus26 = desc.initQpMinus26;
    pps.diff_cu_qp_delta_depth = desc.diffCuQpDeltaDepth;
    pps.pps_cb_qp_offset = desc.cbQpOffset;
    pps.pps_cr_qp_offset = desc.crQpOffset;
    pps.pps_beta_offset_div2 = desc.betaOffsetDiv2;
    pps.pps_tc_offset_div2 = desc.tcOffsetDiv2;
    pps.log2_parallel_merge_level_minus2 = desc.log2ParallelMergeLevelMinus2;
    pps.num_tile_columns_minus1 = desc.tileColumnNumMinus1;
    pps.num_tile_rows_minus1 = desc.tileRowNumMinus1;
    for (uint32_t i = 0; i < STD_VIDEO_H265_CHROMA_QP_OFFSET_TILE_COLS_LIST_SIZE; i++)
        pps.column_width_minus1[i] = desc.columnWidthMinus1[i];
    for (uint32_t i = 0; i < STD_VIDEO_H265_CHROMA_QP_OFFSET_TILE_ROWS_LIST_SIZE; i++)
        pps.row_height_minus1[i] = desc.rowHeightMinus1[i];
    pps.pScalingLists = scalingLists;

    return pps;
}

static inline void FillVideoAV1ColorConfig(StdVideoAV1ColorConfig& colorConfig, const VideoAV1SequenceDesc& desc) {
    colorConfig = {};
    colorConfig.flags.mono_chrome = !!(desc.flags & VideoAV1SequenceBits::MONO_CHROME);
    colorConfig.flags.color_range = !!(desc.flags & VideoAV1SequenceBits::COLOR_RANGE);
    colorConfig.flags.separate_uv_delta_q = !!(desc.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q);
    colorConfig.flags.color_description_present_flag = !!(desc.flags & VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT);
    colorConfig.BitDepth = desc.bitDepth;
    colorConfig.subsampling_x = desc.subsamplingX;
    colorConfig.subsampling_y = desc.subsamplingY;
    colorConfig.color_primaries = (StdVideoAV1ColorPrimaries)desc.colorPrimaries;
    colorConfig.transfer_characteristics = (StdVideoAV1TransferCharacteristics)desc.transferCharacteristics;
    colorConfig.matrix_coefficients = (StdVideoAV1MatrixCoefficients)desc.matrixCoefficients;
    colorConfig.chroma_sample_position = (StdVideoAV1ChromaSamplePosition)desc.chromaSamplePosition;
}

static inline void FillVideoAV1SequenceHeader(StdVideoAV1SequenceHeader& sequenceHeader, const VideoAV1SequenceDesc& desc, const StdVideoAV1ColorConfig& colorConfig,
    const StdVideoAV1TimingInfo* timingInfo) {
    sequenceHeader = {};
    sequenceHeader.flags.still_picture = !!(desc.flags & VideoAV1SequenceBits::STILL_PICTURE);
    sequenceHeader.flags.reduced_still_picture_header = !!(desc.flags & VideoAV1SequenceBits::REDUCED_STILL_PICTURE_HEADER);
    sequenceHeader.flags.use_128x128_superblock = !!(desc.flags & VideoAV1SequenceBits::USE_128X128_SUPERBLOCK);
    sequenceHeader.flags.enable_filter_intra = !!(desc.flags & VideoAV1SequenceBits::ENABLE_FILTER_INTRA);
    sequenceHeader.flags.enable_intra_edge_filter = !!(desc.flags & VideoAV1SequenceBits::ENABLE_INTRA_EDGE_FILTER);
    sequenceHeader.flags.enable_interintra_compound = !!(desc.flags & VideoAV1SequenceBits::ENABLE_INTERINTRA_COMPOUND);
    sequenceHeader.flags.enable_masked_compound = !!(desc.flags & VideoAV1SequenceBits::ENABLE_MASKED_COMPOUND);
    sequenceHeader.flags.enable_warped_motion = !!(desc.flags & VideoAV1SequenceBits::ENABLE_WARPED_MOTION);
    sequenceHeader.flags.enable_dual_filter = !!(desc.flags & VideoAV1SequenceBits::ENABLE_DUAL_FILTER);
    sequenceHeader.flags.enable_order_hint = !!(desc.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT);
    sequenceHeader.flags.enable_jnt_comp = !!(desc.flags & VideoAV1SequenceBits::ENABLE_JNT_COMP);
    sequenceHeader.flags.enable_ref_frame_mvs = !!(desc.flags & VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS);
    sequenceHeader.flags.frame_id_numbers_present_flag = !!(desc.flags & VideoAV1SequenceBits::FRAME_ID_NUMBERS_PRESENT);
    sequenceHeader.flags.enable_superres = !!(desc.flags & VideoAV1SequenceBits::ENABLE_SUPERRES);
    sequenceHeader.flags.enable_cdef = !!(desc.flags & VideoAV1SequenceBits::ENABLE_CDEF);
    sequenceHeader.flags.enable_restoration = !!(desc.flags & VideoAV1SequenceBits::ENABLE_RESTORATION);
    sequenceHeader.flags.film_grain_params_present = !!(desc.flags & VideoAV1SequenceBits::FILM_GRAIN_PARAMS_PRESENT);
    sequenceHeader.flags.timing_info_present_flag = !!(desc.flags & VideoAV1SequenceBits::TIMING_INFO_PRESENT);
    sequenceHeader.flags.initial_display_delay_present_flag = !!(desc.flags & VideoAV1SequenceBits::INITIAL_DISPLAY_DELAY_PRESENT);
    sequenceHeader.seq_profile = (StdVideoAV1Profile)desc.seqProfile;
    sequenceHeader.frame_width_bits_minus_1 = desc.frameWidthBitsMinus1;
    sequenceHeader.frame_height_bits_minus_1 = desc.frameHeightBitsMinus1;
    sequenceHeader.max_frame_width_minus_1 = desc.maxFrameWidthMinus1;
    sequenceHeader.max_frame_height_minus_1 = desc.maxFrameHeightMinus1;
    sequenceHeader.delta_frame_id_length_minus_2 = desc.deltaFrameIdLengthMinus2;
    sequenceHeader.additional_frame_id_length_minus_1 = desc.additionalFrameIdLengthMinus1;
    sequenceHeader.order_hint_bits_minus_1 = desc.orderHintBitsMinus1;
    sequenceHeader.seq_force_integer_mv = desc.seqForceIntegerMv;
    sequenceHeader.seq_force_screen_content_tools = desc.seqForceScreenContentTools;
    sequenceHeader.pColorConfig = &colorConfig;
    sequenceHeader.pTimingInfo = timingInfo;
}

static inline void FillVideoAV1PictureFlags(StdVideoDecodeAV1PictureInfoFlags& flags, VideoAV1PictureBits bits) {
    flags.error_resilient_mode = !!(bits & VideoAV1PictureBits::ERROR_RESILIENT_MODE);
    flags.disable_cdf_update = !!(bits & VideoAV1PictureBits::DISABLE_CDF_UPDATE);
    flags.use_superres = !!(bits & VideoAV1PictureBits::USE_SUPERRES);
    flags.render_and_frame_size_different = !!(bits & VideoAV1PictureBits::RENDER_AND_FRAME_SIZE_DIFFERENT);
    flags.allow_screen_content_tools = !!(bits & VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS);
    flags.is_filter_switchable = !!(bits & VideoAV1PictureBits::IS_FILTER_SWITCHABLE);
    flags.force_integer_mv = !!(bits & VideoAV1PictureBits::FORCE_INTEGER_MV);
    flags.frame_size_override_flag = !!(bits & VideoAV1PictureBits::FRAME_SIZE_OVERRIDE);
    flags.buffer_removal_time_present_flag = !!(bits & VideoAV1PictureBits::BUFFER_REMOVAL_TIME_PRESENT);
    flags.allow_intrabc = !!(bits & VideoAV1PictureBits::ALLOW_INTRABC);
    flags.frame_refs_short_signaling = !!(bits & VideoAV1PictureBits::FRAME_REFS_SHORT_SIGNALING);
    flags.allow_high_precision_mv = !!(bits & VideoAV1PictureBits::ALLOW_HIGH_PRECISION_MV);
    flags.is_motion_mode_switchable = !!(bits & VideoAV1PictureBits::IS_MOTION_MODE_SWITCHABLE);
    flags.use_ref_frame_mvs = !!(bits & VideoAV1PictureBits::USE_REF_FRAME_MVS);
    flags.disable_frame_end_update_cdf = !!(bits & VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF);
    flags.allow_warped_motion = !!(bits & VideoAV1PictureBits::ALLOW_WARPED_MOTION);
    flags.reduced_tx_set = !!(bits & VideoAV1PictureBits::REDUCED_TX_SET);
    flags.reference_select = !!(bits & VideoAV1PictureBits::REFERENCE_SELECT);
    flags.skip_mode_present = !!(bits & VideoAV1PictureBits::SKIP_MODE_PRESENT);
    flags.delta_q_present = !!(bits & VideoAV1PictureBits::DELTA_Q_PRESENT);
    flags.delta_lf_present = !!(bits & VideoAV1PictureBits::DELTA_LF_PRESENT);
    flags.delta_lf_multi = !!(bits & VideoAV1PictureBits::DELTA_LF_MULTI);
    flags.segmentation_enabled = !!(bits & VideoAV1PictureBits::SEGMENTATION_ENABLED);
    flags.segmentation_update_map = !!(bits & VideoAV1PictureBits::SEGMENTATION_UPDATE_MAP);
    flags.segmentation_temporal_update = !!(bits & VideoAV1PictureBits::SEGMENTATION_TEMPORAL_UPDATE);
    flags.segmentation_update_data = !!(bits & VideoAV1PictureBits::SEGMENTATION_UPDATE_DATA);
    flags.UsesLr = !!(bits & VideoAV1PictureBits::USES_LR);
    flags.usesChromaLr = !!(bits & VideoAV1PictureBits::USES_CHROMA_LR);
    flags.apply_grain = !!(bits & VideoAV1PictureBits::APPLY_GRAIN);
}

static inline void FillVideoAV1PictureFlags(StdVideoEncodeAV1PictureInfoFlags& flags, VideoAV1PictureBits bits) {
    flags.error_resilient_mode = !!(bits & VideoAV1PictureBits::ERROR_RESILIENT_MODE);
    flags.disable_cdf_update = !!(bits & VideoAV1PictureBits::DISABLE_CDF_UPDATE);
    flags.use_superres = !!(bits & VideoAV1PictureBits::USE_SUPERRES);
    flags.render_and_frame_size_different = !!(bits & VideoAV1PictureBits::RENDER_AND_FRAME_SIZE_DIFFERENT);
    flags.allow_screen_content_tools = !!(bits & VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS);
    flags.is_filter_switchable = !!(bits & VideoAV1PictureBits::IS_FILTER_SWITCHABLE);
    flags.force_integer_mv = !!(bits & VideoAV1PictureBits::FORCE_INTEGER_MV);
    flags.frame_size_override_flag = !!(bits & VideoAV1PictureBits::FRAME_SIZE_OVERRIDE);
    flags.buffer_removal_time_present_flag = !!(bits & VideoAV1PictureBits::BUFFER_REMOVAL_TIME_PRESENT);
    flags.allow_intrabc = !!(bits & VideoAV1PictureBits::ALLOW_INTRABC);
    flags.frame_refs_short_signaling = !!(bits & VideoAV1PictureBits::FRAME_REFS_SHORT_SIGNALING);
    flags.allow_high_precision_mv = !!(bits & VideoAV1PictureBits::ALLOW_HIGH_PRECISION_MV);
    flags.is_motion_mode_switchable = !!(bits & VideoAV1PictureBits::IS_MOTION_MODE_SWITCHABLE);
    flags.use_ref_frame_mvs = !!(bits & VideoAV1PictureBits::USE_REF_FRAME_MVS);
    flags.disable_frame_end_update_cdf = !!(bits & VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF);
    flags.allow_warped_motion = !!(bits & VideoAV1PictureBits::ALLOW_WARPED_MOTION);
    flags.reduced_tx_set = !!(bits & VideoAV1PictureBits::REDUCED_TX_SET);
    flags.skip_mode_present = !!(bits & VideoAV1PictureBits::SKIP_MODE_PRESENT);
    flags.delta_q_present = !!(bits & VideoAV1PictureBits::DELTA_Q_PRESENT);
    flags.delta_lf_present = !!(bits & VideoAV1PictureBits::DELTA_LF_PRESENT);
    flags.delta_lf_multi = !!(bits & VideoAV1PictureBits::DELTA_LF_MULTI);
    flags.segmentation_enabled = !!(bits & VideoAV1PictureBits::SEGMENTATION_ENABLED);
    flags.segmentation_update_map = !!(bits & VideoAV1PictureBits::SEGMENTATION_UPDATE_MAP);
    flags.segmentation_temporal_update = !!(bits & VideoAV1PictureBits::SEGMENTATION_TEMPORAL_UPDATE);
    flags.segmentation_update_data = !!(bits & VideoAV1PictureBits::SEGMENTATION_UPDATE_DATA);
    flags.UsesLr = !!(bits & VideoAV1PictureBits::USES_LR);
    flags.usesChromaLr = !!(bits & VideoAV1PictureBits::USES_CHROMA_LR);
    flags.show_frame = !!(bits & VideoAV1PictureBits::SHOW_FRAME);
    flags.showable_frame = !!(bits & VideoAV1PictureBits::SHOWABLE_FRAME);
}

static inline StdVideoAV1FrameType GetVideoAV1FrameType(VideoFrameType frameType) {
    switch (frameType) {
        case VideoFrameType::IDR:
        case VideoFrameType::I:
            return STD_VIDEO_AV1_FRAME_TYPE_KEY;
        case VideoFrameType::P:
        case VideoFrameType::B:
            return STD_VIDEO_AV1_FRAME_TYPE_INTER;
        default:
            return STD_VIDEO_AV1_FRAME_TYPE_INVALID;
    }
}

static_assert(video::h265::MAX_REFERENCE_NUM == STD_VIDEO_H265_MAX_NUM_LIST_REF);

struct VideoEncodeH265ReferenceListsVK : video::h265::EncodeReferenceLists {
    std::array<uint32_t, STD_VIDEO_H265_MAX_NUM_LIST_REF> negative = {};
    std::array<uint32_t, STD_VIDEO_H265_MAX_NUM_LIST_REF> positive = {};
    uint32_t negativeNum = 0;
    uint32_t positiveNum = 0;
};

static inline uint32_t FindVideoEncodeH265RpsIndex(const std::array<uint32_t, STD_VIDEO_H265_MAX_NUM_LIST_REF>& references, uint32_t referenceNum, uint32_t referenceIndex) {
    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i] == referenceIndex)
            return i;
    }

    return STD_VIDEO_H265_MAX_NUM_LIST_REF;
}

static inline uint32_t GetVideoEncodeH265List0Entry(const VideoEncodeH265ReferenceListsVK& lists, uint32_t referenceIndex) {
    const uint32_t negativeIndex = FindVideoEncodeH265RpsIndex(lists.negative, lists.negativeNum, referenceIndex);
    if (negativeIndex != STD_VIDEO_H265_MAX_NUM_LIST_REF)
        return negativeIndex;

    return lists.negativeNum + FindVideoEncodeH265RpsIndex(lists.positive, lists.positiveNum, referenceIndex);
}

static inline uint32_t GetVideoEncodeH265List1Entry(const VideoEncodeH265ReferenceListsVK& lists, uint32_t referenceIndex) {
    const uint32_t positiveIndex = FindVideoEncodeH265RpsIndex(lists.positive, lists.positiveNum, referenceIndex);
    if (positiveIndex != STD_VIDEO_H265_MAX_NUM_LIST_REF)
        return positiveIndex;

    return lists.positiveNum + FindVideoEncodeH265RpsIndex(lists.negative, lists.negativeNum, referenceIndex);
}

static inline void AppendVideoEncodeH265RpsReference(VideoEncodeH265ReferenceListsVK& lists, uint32_t referenceIndex, bool negative) {
    std::array<uint32_t, STD_VIDEO_H265_MAX_NUM_LIST_REF>& rps = negative ? lists.negative : lists.positive;
    uint32_t& rpsNum = negative ? lists.negativeNum : lists.positiveNum;

    for (uint32_t i = 0; i < rpsNum; i++) {
        if (rps[i] == referenceIndex)
            return;
    }

    rps[rpsNum++] = referenceIndex;
}

static inline void FillVideoDecodeAV1ReferenceInfo(StdVideoDecodeAV1ReferenceInfo& info, VideoFrameType frameType, uint8_t orderHint, const uint8_t* savedOrderHints = nullptr) {
    info = {};
    info.frame_type = (uint8_t)GetVideoAV1FrameType(frameType);
    info.OrderHint = orderHint;
    if (savedOrderHints)
        std::memcpy(info.SavedOrderHints, savedOrderHints, sizeof(info.SavedOrderHints));
    else {
        for (uint8_t& savedOrderHint : info.SavedOrderHints)
            savedOrderHint = orderHint;
    }
}

// "OrderHints[]" is indexed by reference name: "OrderHints[LAST_FRAME + i]" (AV1 7.20)
static inline void FillVideoDecodeAV1OrderHints(uint8_t* orderHints, const VideoAV1DecodePictureDesc& desc) {
    if (desc.orderHints)
        std::memcpy(orderHints, desc.orderHints, STD_VIDEO_AV1_NUM_REF_FRAMES);
    else
        orderHints[0] = desc.orderHint;

    for (uint32_t i = 0; i < desc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = desc.references[i];
        const uint8_t referenceNameIndex = video::av1::GetReferenceNameIndex(reference.name);

        if (referenceNameIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR)
            orderHints[STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME + referenceNameIndex] = reference.orderHint;
    }
}

static inline void FillVideoDecodeAV1SetupReferenceInfo(StdVideoDecodeAV1ReferenceInfo& info, const VideoAV1DecodePictureDesc& desc, VideoAV1PictureBits pictureFlags) {
    info = {};
    info.frame_type = (uint8_t)GetVideoAV1FrameType(desc.frameType);
    info.OrderHint = desc.orderHint;
    FillVideoDecodeAV1OrderHints(info.SavedOrderHints, desc); // "SavedOrderHints[i][j] = OrderHints[j]" (AV1 7.20)
    info.flags.disable_frame_end_update_cdf = !!(pictureFlags & VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF);
    info.flags.segmentation_enabled = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_ENABLED);
}

static inline void FillVideoAV1DefaultTileInfo(StdVideoAV1TileInfo& info, uint16_t* miColStarts, uint16_t* miRowStarts, uint16_t* widthInSbsMinus1,
    uint16_t* heightInSbsMinus1, uint32_t width, uint32_t height) {
    info = {};
    info.flags.uniform_tile_spacing_flag = true;
    info.TileCols = 1;
    info.TileRows = 1;
    info.tile_size_bytes_minus_1 = 3;
    miColStarts[0] = 0;
    miColStarts[1] = (uint16_t)((width + 3) / 4);
    miRowStarts[0] = 0;
    miRowStarts[1] = (uint16_t)((height + 3) / 4);
    widthInSbsMinus1[0] = (uint16_t)((width + 63) / 64 - 1);
    heightInSbsMinus1[0] = (uint16_t)((height + 63) / 64 - 1);
    info.pMiColStarts = miColStarts;
    info.pMiRowStarts = miRowStarts;
    info.pWidthInSbsMinus1 = widthInSbsMinus1;
    info.pHeightInSbsMinus1 = heightInSbsMinus1;
}

static inline bool BuildVideoEncodeH265ReferenceLists(const VideoReference* references, const VideoH265ReferenceDesc* referenceDescs, uint32_t referenceNum,
    VideoFrameType frameType, int32_t currentPictureOrderCount, VideoEncodeH265ReferenceListsVK& lists) {
    lists = {};
    if (!video::h265::BuildEncodeReferenceLists(references, referenceDescs, referenceNum, frameType, currentPictureOrderCount, false, lists))
        return false;

    for (uint32_t i = 0; i < referenceNum; i++) {
        const VideoH265ReferenceDesc* referenceDesc = video::h265::GetReferenceDesc(references, referenceDescs, referenceNum, i);
        AppendVideoEncodeH265RpsReference(lists, i, referenceDesc->pictureOrderCount < currentPictureOrderCount);
    }

    auto getPictureOrderCount = [&](uint32_t referenceIndex) {
        const VideoH265ReferenceDesc* referenceDesc = video::h265::GetReferenceDesc(references, referenceDescs, referenceNum, referenceIndex);
        return referenceDesc->pictureOrderCount;
    };

    std::sort(lists.negative.begin(), lists.negative.begin() + lists.negativeNum,
        [&](uint32_t a, uint32_t b) { return getPictureOrderCount(a) > getPictureOrderCount(b); });
    std::sort(lists.positive.begin(), lists.positive.begin() + lists.positiveNum,
        [&](uint32_t a, uint32_t b) { return getPictureOrderCount(a) < getPictureOrderCount(b); });

    return true;
}

struct VideoEncodeAV1ReferenceMappingVK {
    std::array<int32_t, VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR> referenceNameSlotIndices = {};
    std::array<int8_t, VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR> refFrameIndices = {};
    uint32_t failingReference = 0;
    bool invalidName = false;
    bool missingResource = false;
    bool missingPrimaryReference = false;
};

struct VideoDecodeAV1ReferenceMappingVK {
    std::array<int32_t, VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR> referenceNameSlotIndices = {};
    uint32_t failingReference = 0;
    bool invalidName = false;
    bool invalidRefFrameIndex = false;
    bool missingPrimaryReference = false;
};

static inline bool BuildVideoEncodeAV1ReferenceMapping(const VideoReference* references, uint32_t referenceNum, const VideoAV1EncodePictureDesc& pictureDesc,
    VideoEncodeAV1ReferenceMappingVK& mapping) {
    for (int32_t& slotIndex : mapping.referenceNameSlotIndices)
        slotIndex = -1;
    for (int8_t& refFrameIndex : mapping.refFrameIndices)
        refFrameIndex = -1;
    mapping.failingReference = 0;
    mapping.invalidName = false;
    mapping.missingResource = false;
    mapping.missingPrimaryReference = false;

    if (pictureDesc.referenceNum > video::av1::REFERENCE_FRAME_NUM) {
        mapping.failingReference = video::av1::REFERENCE_FRAME_NUM;
        return false;
    }

    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = pictureDesc.references[i];
        if (!video::HasReferenceSlot(references, referenceNum, reference.slot)) {
            mapping.failingReference = i;
            mapping.missingResource = true;
            return false;
        }

        if (reference.refFrameIndex >= video::av1::REFERENCE_FRAME_NUM) {
            mapping.failingReference = i;
            return false;
        }

        const uint8_t referenceNameIndex = video::av1::GetReferenceNameIndex(reference.name);
        if (reference.name == VideoAV1ReferenceName::NONE)
            continue;

        if (referenceNameIndex >= VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR) {
            mapping.failingReference = i;
            mapping.invalidName = true;
            return false;
        }

        mapping.referenceNameSlotIndices[referenceNameIndex] = (int32_t)reference.slot;
        mapping.refFrameIndices[referenceNameIndex] = (int8_t)reference.refFrameIndex;
    }

    const uint8_t primaryReferenceIndex = video::av1::GetReferenceNameIndex(pictureDesc.primaryReferenceName);
    if (primaryReferenceIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR && mapping.referenceNameSlotIndices[primaryReferenceIndex] < 0) {
        mapping.missingPrimaryReference = true;
        return false;
    }

    return true;
}

static inline uint32_t GetVideoEncodeAV1ReferenceNameNum(const int32_t* referenceNameSlotIndices, uint32_t mask) {
    uint32_t num = 0;
    for (uint32_t i = 0; i < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR; i++) {
        if (referenceNameSlotIndices[i] >= 0 && (mask & (1u << i)))
            num++;
    }

    return num;
}

static inline bool HasVideoEncodeAV1ReferenceName(const int32_t* referenceNameSlotIndices, uint32_t mask) {
    return GetVideoEncodeAV1ReferenceNameNum(referenceNameSlotIndices, mask) != 0;
}

static inline bool HasVideoEncodeAV1ReferenceNamePair(const int32_t* referenceNameSlotIndices, uint32_t mask, uint32_t a, uint32_t b) {
    return referenceNameSlotIndices[a] >= 0 && referenceNameSlotIndices[b] >= 0 && (mask & (1u << a)) && (mask & (1u << b));
}

static inline bool IsVideoEncodeAV1TileWidthSupported(uint32_t tileWidth, const VkExtent2D& minTileSize, const VkExtent2D& maxTileSize) {
    return tileWidth >= minTileSize.width && tileWidth <= maxTileSize.width;
}

static inline bool IsVideoEncodeAV1TileHeightSupported(uint32_t tileHeight, const VkExtent2D& minTileSize, const VkExtent2D& maxTileSize) {
    return tileHeight >= minTileSize.height && tileHeight <= maxTileSize.height;
}

static inline bool IsVideoEncodeAV1TileSizeSupported(uint32_t tileWidth, uint32_t tileHeight, const VkExtent2D& minTileSize, const VkExtent2D& maxTileSize) {
    return IsVideoEncodeAV1TileWidthSupported(tileWidth, minTileSize, maxTileSize) && IsVideoEncodeAV1TileHeightSupported(tileHeight, minTileSize, maxTileSize);
}

static inline bool BuildVideoDecodeAV1ReferenceMapping(const VideoAV1DecodePictureDesc& pictureDesc, VideoDecodeAV1ReferenceMappingVK& mapping) {
    for (int32_t& slotIndex : mapping.referenceNameSlotIndices)
        slotIndex = -1;
    mapping.failingReference = 0;
    mapping.invalidName = false;
    mapping.invalidRefFrameIndex = false;
    mapping.missingPrimaryReference = false;

    if (pictureDesc.referenceNum > video::av1::REFERENCE_FRAME_NUM) {
        mapping.failingReference = video::av1::REFERENCE_FRAME_NUM;
        return false;
    }

    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = pictureDesc.references[i];
        if (reference.refFrameIndex >= video::av1::REFERENCE_FRAME_NUM) {
            mapping.failingReference = i;
            mapping.invalidRefFrameIndex = true;
            return false;
        }

        const uint8_t referenceNameIndex = video::av1::GetReferenceNameIndex(reference.name);
        if (reference.name == VideoAV1ReferenceName::NONE)
            continue;

        if (referenceNameIndex >= VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR) {
            mapping.failingReference = i;
            mapping.invalidName = true;
            return false;
        }

        mapping.referenceNameSlotIndices[referenceNameIndex] = (int32_t)reference.slot;
    }

    const uint8_t primaryReferenceIndex = video::av1::GetReferenceNameIndex(pictureDesc.primaryReferenceName);
    if (pictureDesc.primaryReferenceName == VideoAV1ReferenceName::MAX_NUM) {
        mapping.invalidName = true;
        return false;
    }
    if (primaryReferenceIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR && mapping.referenceNameSlotIndices[primaryReferenceIndex] < 0) {
        mapping.missingPrimaryReference = true;
        return false;
    }

    return true;
}

static inline void FillVideoDecodeAV1PictureInfo(StdVideoDecodeAV1PictureInfo& info, const VideoAV1DecodePictureDesc& desc, VideoAV1PictureBits pictureFlags) {
    info = {};

    FillVideoAV1PictureFlags(info.flags, pictureFlags);
    info.frame_type = GetVideoAV1FrameType(desc.frameType);
    info.current_frame_id = desc.currentFrameId;
    info.OrderHint = desc.orderHint;
    info.primary_ref_frame = video::av1::GetReferenceNameIndex(desc.primaryReferenceName);
    info.refresh_frame_flags = desc.refreshFrameFlags;
    info.interpolation_filter = (StdVideoAV1InterpolationFilter)desc.interpolationFilter; // 0 = EIGHTTAP
    info.TxMode = (StdVideoAV1TxMode)desc.txMode;                                         // 0 = ONLY_4X4
    info.delta_q_res = desc.deltaQRes;
    info.delta_lf_res = desc.deltaLfRes;
    info.coded_denom = desc.codedDenom;

    info.SkipModeFrame[0] = (uint8_t)desc.skipModeFrames[0];
    info.SkipModeFrame[1] = (uint8_t)desc.skipModeFrames[1];

    static_assert((uint8_t)VideoAV1ReferenceName::LAST == STD_VIDEO_AV1_REFERENCE_NAME_LAST_FRAME, "Unexpected");
    static_assert((uint8_t)VideoAV1ReferenceName::ALTREF == STD_VIDEO_AV1_REFERENCE_NAME_ALTREF_FRAME, "Unexpected");

    // "OrderHints[]" is indexed by reference name, "expectedFrameId[]" by reference name index ("i" in "ref_frame_idx[i]")
    FillVideoDecodeAV1OrderHints(info.OrderHints, desc);
    for (uint32_t i = 0; i < desc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = desc.references[i];
        const uint8_t referenceNameIndex = video::av1::GetReferenceNameIndex(reference.name);

        if (referenceNameIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR)
            info.expectedFrameId[referenceNameIndex] = reference.frameId;
    }
}

static inline void FillVideoDecodeAV1TilePayload(VkVideoDecodeAV1PictureInfoKHR& info, const VideoAV1DecodePictureDesc& desc, uint32_t* tileOffsets, uint32_t* tileSizes) {
    for (uint32_t i = 0; i < desc.tileNum; i++) {
        tileOffsets[i] = desc.tiles[i].offset;
        tileSizes[i] = desc.tiles[i].size;
    }

    info.frameHeaderOffset = desc.frameHeaderOffset;
    info.tileCount = desc.tileNum;
    info.pTileOffsets = desc.tileNum ? tileOffsets : nullptr;
    info.pTileSizes = desc.tileNum ? tileSizes : nullptr;
}

static inline void FillVideoDecodeAV1Quantization(StdVideoAV1Quantization& info, const VideoAV1DecodePictureDesc& desc) {
    info = {};
    info.base_q_idx = desc.baseQIndex;
    if (!desc.quantization)
        return;

    info.flags.using_qmatrix = desc.quantization->usingQmatrix != 0;
    info.flags.diff_uv_delta = desc.quantization->diffUvDelta != 0;
    info.DeltaQYDc = desc.quantization->deltaQYDc;
    info.DeltaQUDc = desc.quantization->deltaQUDc;
    info.DeltaQUAc = desc.quantization->deltaQUAc;
    info.DeltaQVDc = desc.quantization->deltaQVDc;
    info.DeltaQVAc = desc.quantization->deltaQVAc;
    info.qm_y = desc.quantization->qmY;
    info.qm_u = desc.quantization->qmU;
    info.qm_v = desc.quantization->qmV;
}

static inline void FillVideoDecodeAV1LoopFilter(StdVideoAV1LoopFilter& info, const VideoAV1DecodePictureDesc& desc) {
    info = {};
    if (desc.loopFilter) {
        info.flags.loop_filter_delta_enabled = desc.loopFilter->deltaEnabled != 0;
        info.flags.loop_filter_delta_update = desc.loopFilter->deltaUpdate != 0;
        std::memcpy(info.loop_filter_level, desc.loopFilter->level, sizeof(info.loop_filter_level));
        info.loop_filter_sharpness = desc.loopFilter->sharpness;
        info.update_mode_delta = desc.loopFilter->updateModeDelta;
        std::memcpy(info.loop_filter_ref_deltas, desc.loopFilter->refDeltas, sizeof(info.loop_filter_ref_deltas));
        std::memcpy(info.loop_filter_mode_deltas, desc.loopFilter->modeDeltas, sizeof(info.loop_filter_mode_deltas));
        return;
    }

    info.loop_filter_ref_deltas[0] = 1;
    info.loop_filter_ref_deltas[4] = -1;
    info.loop_filter_ref_deltas[6] = -1;
    info.loop_filter_ref_deltas[7] = -1;
}

static inline void FillVideoDecodeAV1Cdef(StdVideoAV1CDEF& info, const VideoAV1DecodePictureDesc& desc) {
    info = {};
    info.cdef_damping_minus_3 = desc.cdefDampingMinus3;
    info.cdef_bits = desc.cdefBits;
    if (!desc.cdef)
        return;

    std::memcpy(info.cdef_y_pri_strength, desc.cdef->yPrimaryStrength, sizeof(info.cdef_y_pri_strength));
    std::memcpy(info.cdef_y_sec_strength, desc.cdef->ySecondaryStrength, sizeof(info.cdef_y_sec_strength));
    std::memcpy(info.cdef_uv_pri_strength, desc.cdef->uvPrimaryStrength, sizeof(info.cdef_uv_pri_strength));
    std::memcpy(info.cdef_uv_sec_strength, desc.cdef->uvSecondaryStrength, sizeof(info.cdef_uv_sec_strength));
}

static inline void FillVideoDecodeAV1LoopRestoration(StdVideoAV1LoopRestoration& info, const VideoAV1DecodePictureDesc& desc) {
    info = {};
    if (!desc.loopRestoration) {
        info.LoopRestorationSize[0] = 1;
        info.LoopRestorationSize[1] = 1;
        info.LoopRestorationSize[2] = 1;
        return;
    }

    info.FrameRestorationType[0] = (StdVideoAV1FrameRestorationType)desc.loopRestoration->frameRestorationType[0];
    info.FrameRestorationType[1] = (StdVideoAV1FrameRestorationType)desc.loopRestoration->frameRestorationType[1];
    info.FrameRestorationType[2] = (StdVideoAV1FrameRestorationType)desc.loopRestoration->frameRestorationType[2];
    info.LoopRestorationSize[0] = 1 + desc.loopRestoration->lrUnitShift;
    info.LoopRestorationSize[1] = 1 + desc.loopRestoration->lrUnitShift - desc.loopRestoration->lrUvShift;
    info.LoopRestorationSize[2] = 1 + desc.loopRestoration->lrUnitShift - desc.loopRestoration->lrUvShift;
}

static inline void FillVideoDecodeAV1GlobalMotion(StdVideoAV1GlobalMotion& info, const VideoAV1DecodePictureDesc& desc) {
    info = {};
    for (uint32_t i = 0; i < 8; i++) {
        info.gm_params[i][2] = 1 << 16;
        info.gm_params[i][5] = 1 << 16;
    }

    if (!desc.globalMotion)
        return;

    std::memcpy(info.GmType, desc.globalMotion->type, sizeof(info.GmType));
    std::memcpy(info.gm_params, desc.globalMotion->params, sizeof(info.gm_params));
}

static inline void FillVideoEncodeAV1Quantization(StdVideoAV1Quantization& info, const VideoAV1EncodePictureDesc* desc, uint32_t baseQIndex) {
    info = {};
    info.base_q_idx = (uint8_t)baseQIndex;
    if (!desc || !desc->quantization)
        return;

    info.flags.using_qmatrix = desc->quantization->usingQmatrix != 0;
    info.flags.diff_uv_delta = desc->quantization->diffUvDelta != 0;
    info.DeltaQYDc = desc->quantization->deltaQYDc;
    info.DeltaQUDc = desc->quantization->deltaQUDc;
    info.DeltaQUAc = desc->quantization->deltaQUAc;
    info.DeltaQVDc = desc->quantization->deltaQVDc;
    info.DeltaQVAc = desc->quantization->deltaQVAc;
    info.qm_y = desc->quantization->qmY;
    info.qm_u = desc->quantization->qmU;
    info.qm_v = desc->quantization->qmV;
}

static inline void FillVideoEncodeAV1LoopFilter(StdVideoAV1LoopFilter& info, const VideoAV1EncodePictureDesc* desc) {
    info = {};
    if (desc && desc->loopFilter) {
        info.flags.loop_filter_delta_enabled = desc->loopFilter->deltaEnabled != 0;
        info.flags.loop_filter_delta_update = desc->loopFilter->deltaUpdate != 0;
        std::memcpy(info.loop_filter_level, desc->loopFilter->level, sizeof(info.loop_filter_level));
        info.loop_filter_sharpness = desc->loopFilter->sharpness;

        // Signal all ref/mode deltas on update, otherwise the provided deltas are not coded
        if (info.flags.loop_filter_delta_enabled && info.flags.loop_filter_delta_update) {
            info.update_ref_delta = (uint8_t)((1u << STD_VIDEO_AV1_TOTAL_REFS_PER_FRAME) - 1);
            info.update_mode_delta = (uint8_t)((1u << STD_VIDEO_AV1_LOOP_FILTER_ADJUSTMENTS) - 1);
        }
        std::memcpy(info.loop_filter_ref_deltas, desc->loopFilter->refDeltas, sizeof(info.loop_filter_ref_deltas));
        std::memcpy(info.loop_filter_mode_deltas, desc->loopFilter->modeDeltas, sizeof(info.loop_filter_mode_deltas));
        return;
    }

    info.loop_filter_ref_deltas[0] = 1;
    info.loop_filter_ref_deltas[4] = -1;
    info.loop_filter_ref_deltas[6] = -1;
    info.loop_filter_ref_deltas[7] = -1;
}

static inline void FillVideoEncodeAV1Cdef(StdVideoAV1CDEF& info, const VideoAV1EncodePictureDesc* desc) {
    info = {};
    if (!desc)
        return;

    info.cdef_damping_minus_3 = desc->cdefDampingMinus3;
    info.cdef_bits = desc->cdefBits;
    if (!desc->cdef)
        return;

    std::memcpy(info.cdef_y_pri_strength, desc->cdef->yPrimaryStrength, sizeof(info.cdef_y_pri_strength));
    std::memcpy(info.cdef_y_sec_strength, desc->cdef->ySecondaryStrength, sizeof(info.cdef_y_sec_strength));
    std::memcpy(info.cdef_uv_pri_strength, desc->cdef->uvPrimaryStrength, sizeof(info.cdef_uv_pri_strength));
    std::memcpy(info.cdef_uv_sec_strength, desc->cdef->uvSecondaryStrength, sizeof(info.cdef_uv_sec_strength));
}

static inline void FillVideoEncodeAV1LoopRestoration(StdVideoAV1LoopRestoration& info, const VideoAV1EncodePictureDesc* desc) {
    info = {};
    if (!desc || !desc->loopRestoration) {
        info.LoopRestorationSize[0] = 1;
        info.LoopRestorationSize[1] = 1;
        info.LoopRestorationSize[2] = 1;
        return;
    }

    info.FrameRestorationType[0] = (StdVideoAV1FrameRestorationType)desc->loopRestoration->frameRestorationType[0];
    info.FrameRestorationType[1] = (StdVideoAV1FrameRestorationType)desc->loopRestoration->frameRestorationType[1];
    info.FrameRestorationType[2] = (StdVideoAV1FrameRestorationType)desc->loopRestoration->frameRestorationType[2];
    info.LoopRestorationSize[0] = 1 + desc->loopRestoration->lrUnitShift;
    info.LoopRestorationSize[1] = 1 + desc->loopRestoration->lrUnitShift - desc->loopRestoration->lrUvShift;
    info.LoopRestorationSize[2] = 1 + desc->loopRestoration->lrUnitShift - desc->loopRestoration->lrUvShift;
}

static inline void FillVideoEncodeAV1GlobalMotion(StdVideoAV1GlobalMotion& info, const VideoAV1EncodePictureDesc* desc) {
    info = {};
    for (uint32_t i = 0; i < 8; i++) {
        info.gm_params[i][2] = 1 << 16;
        info.gm_params[i][5] = 1 << 16;
    }

    if (!desc || !desc->globalMotion)
        return;

    std::memcpy(info.GmType, desc->globalMotion->type, sizeof(info.GmType));
    std::memcpy(info.gm_params, desc->globalMotion->params, sizeof(info.gm_params));
}

static inline void FillVideoDecodeAV1FilmGrain(StdVideoAV1FilmGrain& info, const VideoAV1FilmGrainDesc& desc) {
    info = {};
    info.flags.chroma_scaling_from_luma = desc.chromaScalingFromLuma != 0;
    info.flags.overlap_flag = desc.overlapFlag != 0;
    info.flags.clip_to_restricted_range = desc.clipToRestrictedRange != 0;
    info.flags.update_grain = desc.updateGrain != 0;
    info.grain_scaling_minus_8 = desc.grainScalingMinus8;
    info.ar_coeff_lag = desc.arCoeffLag;
    info.ar_coeff_shift_minus_6 = desc.arCoeffShiftMinus6;
    info.grain_scale_shift = desc.grainScaleShift;
    info.grain_seed = desc.grainSeed;
    info.film_grain_params_ref_idx = desc.filmGrainParamsRefIdx;
    info.num_y_points = desc.numYPoints;
    info.num_cb_points = desc.numCbPoints;
    info.num_cr_points = desc.numCrPoints;
    info.cb_mult = desc.cbMult;
    info.cb_luma_mult = desc.cbLumaMult;
    info.cb_offset = desc.cbOffset;
    info.cr_mult = desc.crMult;
    info.cr_luma_mult = desc.crLumaMult;
    info.cr_offset = desc.crOffset;
    std::memcpy(info.point_y_value, desc.pointYValue, sizeof(info.point_y_value));
    std::memcpy(info.point_y_scaling, desc.pointYScaling, sizeof(info.point_y_scaling));
    std::memcpy(info.point_cb_value, desc.pointCbValue, sizeof(info.point_cb_value));
    std::memcpy(info.point_cb_scaling, desc.pointCbScaling, sizeof(info.point_cb_scaling));
    std::memcpy(info.point_cr_value, desc.pointCrValue, sizeof(info.point_cr_value));
    std::memcpy(info.point_cr_scaling, desc.pointCrScaling, sizeof(info.point_cr_scaling));
    std::memcpy(info.ar_coeffs_y_plus_128, desc.arCoeffsYPlus128, sizeof(info.ar_coeffs_y_plus_128));
    std::memcpy(info.ar_coeffs_cb_plus_128, desc.arCoeffsCbPlus128, sizeof(info.ar_coeffs_cb_plus_128));
    std::memcpy(info.ar_coeffs_cr_plus_128, desc.arCoeffsCrPlus128, sizeof(info.ar_coeffs_cr_plus_128));
}

// Returns the lowest of levels 3.1, 4.1, 5.1 and 6.1 (as "general_level_idc") whose Table A.8 limits allow the picture size at up to 60 frames per second
static inline uint8_t GetVideoH265DefaultGeneralLevelIdc(uint32_t width, uint32_t height) {
    struct Level {
        uint8_t generalLevelIdc;
        uint64_t maxLumaPs;
        uint64_t maxLumaSr;
    };

    static constexpr Level levels[] = {
        {93, 983040, 33177600},
        {123, 2228224, 133693440},
        {153, 8912896, 534773760},
        {183, 35651584, 2139095040},
    };

    const uint64_t samples = uint64_t(width) * height;

    for (const Level& level : levels) {
        const uint64_t maxDimSquared = level.maxLumaPs * 8;

        if (samples <= level.maxLumaPs && (uint64_t(width) * width) <= maxDimSquared && (uint64_t(height) * height) <= maxDimSquared && (samples * 60) <= level.maxLumaSr)
            return level.generalLevelIdc;
    }

    return levels[std::size(levels) - 1].generalLevelIdc;
}

} // namespace nri
