// © 2021 NVIDIA Corporation

#pragma once

#include "Extensions/NRIVideo.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace nri {

inline uint8_t GetVideoAV1ReferenceNameIndexVK(VideoAV1ReferenceName name) {
    switch (name) {
        case VideoAV1ReferenceName::NONE:
            return STD_VIDEO_AV1_PRIMARY_REF_NONE;
        case VideoAV1ReferenceName::LAST:
            return 0;
        case VideoAV1ReferenceName::LAST2:
            return 1;
        case VideoAV1ReferenceName::LAST3:
            return 2;
        case VideoAV1ReferenceName::GOLDEN:
            return 3;
        case VideoAV1ReferenceName::BWDREF:
            return 4;
        case VideoAV1ReferenceName::ALTREF2:
            return 5;
        case VideoAV1ReferenceName::ALTREF:
            return 6;
        case VideoAV1ReferenceName::MAX_NUM:
            return STD_VIDEO_AV1_PRIMARY_REF_NONE;
    }

    return STD_VIDEO_AV1_PRIMARY_REF_NONE;
}

inline uint8_t GetVideoAV1NamedReferenceIndexVK(VideoAV1ReferenceName name) {
    const uint8_t referenceNameIndex = GetVideoAV1ReferenceNameIndexVK(name);
    return name != VideoAV1ReferenceName::NONE && referenceNameIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR
        ? referenceNameIndex
        : VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR;
}

inline uint8_t GetVideoEncodeAV1ReferenceNameIndexVK(VideoAV1ReferenceName name) {
    return GetVideoAV1ReferenceNameIndexVK(name);
}

inline uint8_t GetVideoEncodeQPByFrameTypeVK(const VideoEncodeRateControlDesc& rateControlDesc, VideoEncodeFrameType frameType) {
    return frameType == VideoEncodeFrameType::B ? rateControlDesc.qpB : (frameType == VideoEncodeFrameType::P ? rateControlDesc.qpP : rateControlDesc.qpI);
}

inline bool IsVideoEncodePictureUsedAsReferenceVK(VideoCodec codec, uint32_t maxReferenceNum, bool hasReconstructedPicture, uint8_t av1RefreshFrameFlags) {
    if (!maxReferenceNum || !hasReconstructedPicture)
        return false;

    return codec != VideoCodec::AV1 || av1RefreshFrameFlags != 0;
}

inline StdVideoAV1Level GetVideoAV1LevelVK(uint32_t width, uint32_t height) {
    const uint64_t samples = uint64_t(width) * height;
    if (samples <= 512ull * 288ull)
        return STD_VIDEO_AV1_LEVEL_2_0;
    if (samples <= 704ull * 396ull)
        return STD_VIDEO_AV1_LEVEL_2_1;
    if (samples <= 1088ull * 612ull)
        return STD_VIDEO_AV1_LEVEL_3_0;
    if (samples <= 1376ull * 774ull)
        return STD_VIDEO_AV1_LEVEL_3_1;
    if (samples <= 2048ull * 1152ull)
        return STD_VIDEO_AV1_LEVEL_4_0;
    if (samples <= 4096ull * 2176ull)
        return STD_VIDEO_AV1_LEVEL_5_0;

    return STD_VIDEO_AV1_LEVEL_5_1;
}

inline StdVideoAV1Level GetVideoAV1LevelVK(uint8_t level, uint32_t width, uint32_t height) {
    switch (level) {
        case 20:
            return STD_VIDEO_AV1_LEVEL_2_0;
        case 21:
            return STD_VIDEO_AV1_LEVEL_2_1;
        case 30:
            return STD_VIDEO_AV1_LEVEL_3_0;
        case 31:
            return STD_VIDEO_AV1_LEVEL_3_1;
        case 40:
            return STD_VIDEO_AV1_LEVEL_4_0;
        case 41:
            return STD_VIDEO_AV1_LEVEL_4_1;
        case 50:
            return STD_VIDEO_AV1_LEVEL_5_0;
        case 51:
            return STD_VIDEO_AV1_LEVEL_5_1;
        case 52:
            return STD_VIDEO_AV1_LEVEL_5_2;
        case 53:
            return STD_VIDEO_AV1_LEVEL_5_3;
        case 60:
            return STD_VIDEO_AV1_LEVEL_6_0;
        case 61:
            return STD_VIDEO_AV1_LEVEL_6_1;
        case 62:
            return STD_VIDEO_AV1_LEVEL_6_2;
        case 63:
            return STD_VIDEO_AV1_LEVEL_6_3;
        case 70:
            return STD_VIDEO_AV1_LEVEL_7_0;
        case 71:
            return STD_VIDEO_AV1_LEVEL_7_1;
        case 72:
            return STD_VIDEO_AV1_LEVEL_7_2;
        case 73:
            return STD_VIDEO_AV1_LEVEL_7_3;
        default:
            return GetVideoAV1LevelVK(width, height);
    }
}

inline uint8_t GetVideoAV1SizeBitsMinus1VK(uint32_t value) {
    uint8_t bits = 0;
    value--;
    do {
        bits++;
        value >>= 1;
    } while (value);

    return bits - 1;
}

inline VideoAV1SequenceDesc GetDefaultVideoAV1SequenceDescVK(uint32_t width, uint32_t height, Format format) {
    VideoAV1SequenceDesc desc = {};
    desc.flags = VideoAV1SequenceBits::ENABLE_ORDER_HINT | VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT;
    desc.bitDepth = format == Format::P010_UNORM || format == Format::P016_UNORM ? 10 : 8;
    desc.subsamplingX = 1;
    desc.subsamplingY = 1;
    desc.maxFrameWidthMinus1 = (uint16_t)(width - 1);
    desc.maxFrameHeightMinus1 = (uint16_t)(height - 1);
    desc.frameWidthBitsMinus1 = GetVideoAV1SizeBitsMinus1VK(width);
    desc.frameHeightBitsMinus1 = GetVideoAV1SizeBitsMinus1VK(height);
    desc.orderHintBitsMinus1 = 7;
    desc.seqForceIntegerMv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
    desc.seqForceScreenContentTools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
    desc.level = 0;
    desc.colorPrimaries = STD_VIDEO_AV1_COLOR_PRIMARIES_BT_709;
    desc.transferCharacteristics = STD_VIDEO_AV1_TRANSFER_CHARACTERISTICS_BT_709;
    desc.matrixCoefficients = STD_VIDEO_AV1_MATRIX_COEFFICIENTS_BT_709;
    desc.chromaSamplePosition = STD_VIDEO_AV1_CHROMA_SAMPLE_POSITION_VERTICAL;
    return desc;
}

inline VideoAV1PictureBits GetDefaultVideoAV1PictureFlagsVK() {
    return VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::DISABLE_CDF_UPDATE | VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS | VideoAV1PictureBits::FORCE_INTEGER_MV | VideoAV1PictureBits::SHOW_FRAME | VideoAV1PictureBits::SHOWABLE_FRAME;
}

inline void FillVideoH265ProfileTierLevelVK(StdVideoH265ProfileTierLevel& profileTierLevel, const VideoH265ProfileTierLevelDesc& desc) {
    profileTierLevel = {};
    profileTierLevel.flags.general_tier_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::TIER);
    profileTierLevel.flags.general_progressive_source_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::PROGRESSIVE_SOURCE);
    profileTierLevel.flags.general_interlaced_source_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::INTERLACED_SOURCE);
    profileTierLevel.flags.general_non_packed_constraint_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::NON_PACKED_CONSTRAINT);
    profileTierLevel.flags.general_frame_only_constraint_flag = !!(desc.flags & VideoH265ProfileTierLevelBits::FRAME_ONLY_CONSTRAINT);
    profileTierLevel.general_profile_idc = (StdVideoH265ProfileIdc)desc.generalProfileIdc;
    profileTierLevel.general_level_idc = (StdVideoH265LevelIdc)desc.generalLevelIdc;
}

inline void FillVideoH265DecPicBufMgrVK(StdVideoH265DecPicBufMgr& decPicBufMgr, const VideoH265DecPicBufMgrDesc& desc) {
    decPicBufMgr = {};
    for (uint32_t i = 0; i < STD_VIDEO_H265_SUBLAYERS_LIST_SIZE; i++) {
        decPicBufMgr.max_dec_pic_buffering_minus1[i] = desc.maxDecPicBufferingMinus1[i];
        decPicBufMgr.max_num_reorder_pics[i] = desc.maxNumReorderPics[i];
        decPicBufMgr.max_latency_increase_plus1[i] = desc.maxLatencyIncreasePlus1[i];
    }
}

inline StdVideoH265ScalingLists GetVideoH265ScalingListsVK(const VideoH265ScalingListsDesc& desc) {
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

inline StdVideoH265ShortTermRefPicSet GetVideoH265ShortTermRefPicSetVK(const VideoH265ShortTermRefPicSetDesc& desc) {
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

inline StdVideoH265LongTermRefPicsSps GetVideoH265LongTermRefPicsSpsVK(const VideoH265LongTermRefPicsSpsDesc& desc) {
    StdVideoH265LongTermRefPicsSps longTermRefPics = {};
    longTermRefPics.used_by_curr_pic_lt_sps_flag = desc.usedByCurrPicLtSpsFlag;
    for (uint32_t i = 0; i < STD_VIDEO_H265_MAX_LONG_TERM_REF_PICS_SPS; i++)
        longTermRefPics.lt_ref_pic_poc_lsb_sps[i] = desc.ltRefPicPocLsbSps[i];

    return longTermRefPics;
}

inline StdVideoH265VideoParameterSet GetVideoH265VideoParameterSetVK(const VideoH265VideoParameterSetDesc& desc, const StdVideoH265ProfileTierLevel& profileTierLevel,
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

inline StdVideoH265SequenceParameterSet GetVideoH265SequenceParameterSetVK(const VideoH265SequenceParameterSetDesc& desc,
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

inline StdVideoH265PictureParameterSet GetVideoH265PictureParameterSetVK(const VideoH265PictureParameterSetDesc& desc, const StdVideoH265ScalingLists* scalingLists) {
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

inline void FillVideoAV1ColorConfigVK(StdVideoAV1ColorConfig& colorConfig, const VideoAV1SequenceDesc& desc) {
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

inline void FillVideoAV1SequenceHeaderVK(StdVideoAV1SequenceHeader& sequenceHeader, const VideoAV1SequenceDesc& desc, const StdVideoAV1ColorConfig& colorConfig,
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

inline void FillVideoAV1PictureFlagsVK(StdVideoDecodeAV1PictureInfoFlags& flags, VideoAV1PictureBits bits) {
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

inline void FillVideoAV1PictureFlagsVK(StdVideoEncodeAV1PictureInfoFlags& flags, VideoAV1PictureBits bits) {
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

inline StdVideoAV1FrameType GetVideoAV1FrameTypeVK(VideoEncodeFrameType frameType) {
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

struct VideoEncodeHEVCReferenceListsVK {
    std::array<uint32_t, STD_VIDEO_H265_MAX_NUM_LIST_REF> list0 = {};
    std::array<uint32_t, STD_VIDEO_H265_MAX_NUM_LIST_REF> list1 = {};
    uint32_t list0Num = 0;
    uint32_t list1Num = 0;
    uint32_t failingReference = 0;
    bool missingDescriptor = false;
    bool invalidPictureOrderCount = false;
};

inline const VideoH265ReferenceDesc* FindVideoH265ReferenceDescVK(const VideoH265ReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return nullptr;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return &references[i];
    }

    return nullptr;
}

inline const VideoH264DecodeReferenceDesc* FindVideoH264DecodeReferenceDescVK(const VideoH264DecodeReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return nullptr;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return &references[i];
    }

    return nullptr;
}

inline const VideoAV1ReferenceDesc* FindVideoAV1ReferenceDescVK(const VideoAV1ReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return nullptr;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return &references[i];
    }

    return nullptr;
}

inline void FillVideoDecodeAV1ReferenceInfoVK(StdVideoDecodeAV1ReferenceInfo& info, VideoEncodeFrameType frameType, uint8_t orderHint, const uint8_t* savedOrderHints = nullptr) {
    info = {};
    info.frame_type = (uint8_t)GetVideoAV1FrameTypeVK(frameType);
    info.OrderHint = orderHint;
    if (savedOrderHints)
        std::memcpy(info.SavedOrderHints, savedOrderHints, sizeof(info.SavedOrderHints));
    else {
        for (uint8_t& savedOrderHint : info.SavedOrderHints)
            savedOrderHint = orderHint;
    }
}

inline void FillVideoDecodeAV1SetupReferenceInfoVK(StdVideoDecodeAV1ReferenceInfo& info, const VideoAV1DecodePictureDesc& desc, VideoAV1PictureBits pictureFlags) {
    FillVideoDecodeAV1ReferenceInfoVK(info, desc.frameType, desc.orderHint);
    info.flags.disable_frame_end_update_cdf = !!(pictureFlags & VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF);
    info.flags.segmentation_enabled = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_ENABLED);
}

inline void FillVideoAV1DefaultTileInfoVK(StdVideoAV1TileInfo& info, uint16_t* miColStarts, uint16_t* miRowStarts, uint16_t* widthInSbsMinus1,
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

inline bool BuildVideoEncodeHEVCReferenceListsVK(const VideoReference* references, const VideoH265ReferenceDesc* referenceDescs, uint32_t referenceNum,
    VideoEncodeFrameType frameType, int32_t currentPictureOrderCount, VideoEncodeHEVCReferenceListsVK& lists) {
    lists = {};

    if (referenceNum > STD_VIDEO_H265_MAX_NUM_LIST_REF) {
        lists.failingReference = STD_VIDEO_H265_MAX_NUM_LIST_REF;
        return false;
    }

    if (referenceNum && !referenceDescs) {
        lists.missingDescriptor = true;
        return false;
    }

    for (uint32_t i = 0; i < referenceNum; i++) {
        const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescVK(referenceDescs, referenceNum, references[i].slot);
        if (!referenceDesc) {
            lists.failingReference = i;
            lists.missingDescriptor = true;
            return false;
        }

        if (referenceDesc->listIndex == 0) {
            if (referenceDesc->pictureOrderCount >= currentPictureOrderCount) {
                lists.failingReference = i;
                lists.invalidPictureOrderCount = true;
                return false;
            }

            lists.list0[lists.list0Num++] = i;
        } else if (referenceDesc->listIndex == 1) {
            if (frameType != VideoEncodeFrameType::B) {
                lists.failingReference = i;
                lists.invalidPictureOrderCount = true;
                return false;
            }
            if (referenceDesc->pictureOrderCount <= currentPictureOrderCount) {
                lists.failingReference = i;
                lists.invalidPictureOrderCount = true;
                return false;
            }

            lists.list1[lists.list1Num++] = i;
        } else {
            lists.failingReference = i;
            lists.invalidPictureOrderCount = true;
            return false;
        }
    }

    if (referenceNum && !lists.list0Num) {
        lists.invalidPictureOrderCount = true;
        return false;
    }

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

inline bool HasVideoReferenceSlotVK(const VideoReference* references, uint32_t referenceNum, uint32_t slot) {
    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return true;
    }

    return false;
}

inline bool BuildVideoEncodeAV1ReferenceMappingVK(const VideoReference* references, uint32_t referenceNum, const VideoAV1PictureDesc& pictureDesc,
    VideoEncodeAV1ReferenceMappingVK& mapping) {
    for (int32_t& slotIndex : mapping.referenceNameSlotIndices)
        slotIndex = -1;
    for (int8_t& refFrameIndex : mapping.refFrameIndices)
        refFrameIndex = -1;
    mapping.failingReference = 0;
    mapping.invalidName = false;
    mapping.missingResource = false;
    mapping.missingPrimaryReference = false;

    if (pictureDesc.referenceNum > 8) {
        mapping.failingReference = 8;
        return false;
    }

    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = pictureDesc.references[i];
        if (!HasVideoReferenceSlotVK(references, referenceNum, reference.slot)) {
            mapping.failingReference = i;
            mapping.missingResource = true;
            return false;
        }

        if (reference.refFrameIndex >= 8) {
            mapping.failingReference = i;
            return false;
        }

        const uint8_t referenceNameIndex = GetVideoAV1NamedReferenceIndexVK(reference.name);
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

    const uint8_t primaryReferenceIndex = GetVideoAV1ReferenceNameIndexVK(pictureDesc.primaryReferenceName);
    if (primaryReferenceIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR && mapping.referenceNameSlotIndices[primaryReferenceIndex] < 0) {
        mapping.missingPrimaryReference = true;
        return false;
    }

    return true;
}

inline bool BuildVideoDecodeAV1ReferenceMappingVK(const VideoAV1DecodePictureDesc& pictureDesc, VideoDecodeAV1ReferenceMappingVK& mapping) {
    for (int32_t& slotIndex : mapping.referenceNameSlotIndices)
        slotIndex = -1;
    mapping.failingReference = 0;
    mapping.invalidName = false;
    mapping.invalidRefFrameIndex = false;
    mapping.missingPrimaryReference = false;

    if (pictureDesc.referenceNum > 8) {
        mapping.failingReference = 8;
        return false;
    }

    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = pictureDesc.references[i];
        if (reference.refFrameIndex >= 8) {
            mapping.failingReference = i;
            mapping.invalidRefFrameIndex = true;
            return false;
        }

        const uint8_t referenceNameIndex = GetVideoAV1NamedReferenceIndexVK(reference.name);
        if (reference.name == VideoAV1ReferenceName::NONE)
            continue;

        if (referenceNameIndex >= VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR) {
            mapping.failingReference = i;
            mapping.invalidName = true;
            return false;
        }

        mapping.referenceNameSlotIndices[referenceNameIndex] = (int32_t)reference.slot;
    }

    const uint8_t primaryReferenceIndex = GetVideoAV1ReferenceNameIndexVK(pictureDesc.primaryReferenceName);
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

inline void FillVideoDecodeAV1PictureInfoVK(StdVideoDecodeAV1PictureInfo& info, const VideoAV1DecodePictureDesc& desc, VideoAV1PictureBits pictureFlags) {
    info = {};

    FillVideoAV1PictureFlagsVK(info.flags, pictureFlags);
    info.frame_type = GetVideoAV1FrameTypeVK(desc.frameType);
    info.current_frame_id = desc.currentFrameId;
    info.OrderHint = desc.orderHint;
    info.primary_ref_frame = GetVideoAV1ReferenceNameIndexVK(desc.primaryReferenceName);
    info.refresh_frame_flags = desc.refreshFrameFlags;
    info.interpolation_filter = (StdVideoAV1InterpolationFilter)(desc.interpolationFilter ? desc.interpolationFilter : STD_VIDEO_AV1_INTERPOLATION_FILTER_SWITCHABLE);
    info.TxMode = (StdVideoAV1TxMode)(desc.txMode ? desc.txMode : STD_VIDEO_AV1_TX_MODE_SELECT);
    info.delta_q_res = desc.deltaQRes;
    info.delta_lf_res = desc.deltaLfRes;
    info.coded_denom = desc.codedDenom;

    if (desc.orderHints)
        std::memcpy(info.OrderHints, desc.orderHints, sizeof(info.OrderHints));
    else
        info.OrderHints[0] = desc.orderHint;
    info.expectedFrameId[0] = desc.currentFrameId;

    for (uint32_t i = 0; i < desc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = desc.references[i];
        info.OrderHints[reference.refFrameIndex] = reference.orderHint;
        info.expectedFrameId[reference.refFrameIndex] = reference.frameId;
    }
}

inline void FillVideoDecodeAV1TilePayloadVK(VkVideoDecodeAV1PictureInfoKHR& info, const VideoAV1DecodePictureDesc& desc, uint32_t* tileOffsets, uint32_t* tileSizes) {
    for (uint32_t i = 0; i < desc.tileNum; i++) {
        tileOffsets[i] = desc.tiles[i].offset;
        tileSizes[i] = desc.tiles[i].size;
    }

    info.frameHeaderOffset = desc.frameHeaderOffset;
    info.tileCount = desc.tileNum;
    info.pTileOffsets = desc.tileNum ? tileOffsets : nullptr;
    info.pTileSizes = desc.tileNum ? tileSizes : nullptr;
}

inline void FillVideoDecodeAV1QuantizationVK(StdVideoAV1Quantization& info, const VideoAV1DecodePictureDesc& desc) {
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

inline void FillVideoDecodeAV1LoopFilterVK(StdVideoAV1LoopFilter& info, const VideoAV1DecodePictureDesc& desc) {
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

inline void FillVideoDecodeAV1CdefVK(StdVideoAV1CDEF& info, const VideoAV1DecodePictureDesc& desc) {
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

inline void FillVideoDecodeAV1LoopRestorationVK(StdVideoAV1LoopRestoration& info, const VideoAV1DecodePictureDesc& desc) {
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

inline void FillVideoDecodeAV1GlobalMotionVK(StdVideoAV1GlobalMotion& info, const VideoAV1DecodePictureDesc& desc) {
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

inline void FillVideoDecodeAV1FilmGrainVK(StdVideoAV1FilmGrain& info, const VideoAV1FilmGrainDesc& desc) {
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

inline bool IsVideoEncodeAV1KeyFrameReferenceStateValidVK(VideoEncodeFrameType frameType, uint32_t referenceNum) {
    return (frameType != VideoEncodeFrameType::IDR && frameType != VideoEncodeFrameType::I) || referenceNum == 0;
}

} // namespace nri
