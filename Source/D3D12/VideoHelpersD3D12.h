// © 2021 NVIDIA Corporation

#pragma once

#include "Extensions/NRIVideo.h"

#include <d3d12video.h>
#include <dxva.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace nri {

constexpr uint32_t VIDEO_D3D12_DECODE_MAX_PIC_ENTRY_SLOT = 127;
constexpr uint32_t VIDEO_D3D12_HEVC_MAX_REFERENCE_NUM = 15;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_128x128_SUPERBLOCK = 0x1;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_SUPER_RESOLUTION = 0x200;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER = 0x400;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_CDEF_FILTERING = 0x1000;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS = 0x8000;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_AUTO_SEGMENTATION = 0x10000;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS = 0x40000;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS = 0x80000;
constexpr uint32_t VIDEO_D3D12_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS = 0x100;

// Older Windows SDK headers used by some builds do not name this newer support bit yet.
constexpr D3D12_VIDEO_ENCODER_SUPPORT_FLAGS VIDEO_D3D12_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE = (D3D12_VIDEO_ENCODER_SUPPORT_FLAGS)0x8000;

struct VideoDecodeReferenceLayoutD3D12 {
    uint32_t slotCount = 0;
    uint32_t failingReference = 0;
    bool duplicateSlot = false;
};

inline bool GetVideoDecodeReferenceLayoutD3D12(const VideoReference* references, uint32_t referenceNum, VideoDecodeReferenceLayoutD3D12& layout) {
    layout = {};
    std::array<bool, VIDEO_D3D12_DECODE_MAX_PIC_ENTRY_SLOT + 1> usedSlots = {};

    for (uint32_t i = 0; i < referenceNum; i++) {
        const uint32_t slot = references[i].slot;
        if (slot > VIDEO_D3D12_DECODE_MAX_PIC_ENTRY_SLOT) {
            layout.failingReference = i;
            return false;
        }

        if (usedSlots[slot]) {
            layout.failingReference = i;
            layout.duplicateSlot = true;
            return false;
        }

        usedSlots[slot] = true;
        layout.slotCount = std::max(layout.slotCount, slot + 1);
    }

    return true;
}

inline const VideoH264SequenceParameterSetDesc* FindVideoH264SequenceParameterSetD3D12(const VideoH264SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.sequenceParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.sequenceParameterSetNum; i++) {
        if (parameters.sequenceParameterSets[i].sequenceParameterSetId == id)
            return &parameters.sequenceParameterSets[i];
    }

    return nullptr;
}

inline const VideoH264PictureParameterSetDesc* FindVideoH264PictureParameterSetD3D12(const VideoH264SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.pictureParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.pictureParameterSetNum; i++) {
        if (parameters.pictureParameterSets[i].pictureParameterSetId == id)
            return &parameters.pictureParameterSets[i];
    }

    return nullptr;
}

inline bool CanBuildVideoDecodeH264ArgumentsD3D12(const VideoDecodeDesc& desc) {
    return desc.h264PictureDesc && !desc.h265PictureDesc && desc.argumentNum == 0 && (desc.referenceNum == 0 || (desc.h264PictureDesc->references && desc.h264PictureDesc->referenceNum == desc.referenceNum));
}

inline const VideoH264DecodeReferenceDesc* FindVideoH264DecodeReferenceDescD3D12(const VideoH264DecodeReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return nullptr;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return &references[i];
    }

    return nullptr;
}

inline const VideoH265SequenceParameterSetDesc* FindVideoH265SequenceParameterSetD3D12(const VideoH265SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.sequenceParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.sequenceParameterSetNum; i++) {
        if (parameters.sequenceParameterSets[i].sequenceParameterSetId == id)
            return &parameters.sequenceParameterSets[i];
    }

    return nullptr;
}

inline const VideoH265PictureParameterSetDesc* FindVideoH265PictureParameterSetD3D12(const VideoH265SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.pictureParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.pictureParameterSetNum; i++) {
        if (parameters.pictureParameterSets[i].pictureParameterSetId == id)
            return &parameters.pictureParameterSets[i];
    }

    return nullptr;
}

inline void FillVideoH265ScalingListsD3D12(DXVA_Qmatrix_HEVC& matrix, const VideoH265ScalingListsDesc* scalingLists) {
    matrix = {};
    if (scalingLists) {
        std::memcpy(matrix.ucScalingLists0, scalingLists->scalingList4x4, sizeof(matrix.ucScalingLists0));
        std::memcpy(matrix.ucScalingLists1, scalingLists->scalingList8x8, sizeof(matrix.ucScalingLists1));
        std::memcpy(matrix.ucScalingLists2, scalingLists->scalingList16x16, sizeof(matrix.ucScalingLists2));
        std::memcpy(matrix.ucScalingLists3, scalingLists->scalingList32x32, sizeof(matrix.ucScalingLists3));
        std::memcpy(matrix.ucScalingListDCCoefSizeID2, scalingLists->scalingListDCCoef16x16, sizeof(matrix.ucScalingListDCCoefSizeID2));
        std::memcpy(matrix.ucScalingListDCCoefSizeID3, scalingLists->scalingListDCCoef32x32, sizeof(matrix.ucScalingListDCCoefSizeID3));
        return;
    }

    std::memset(matrix.ucScalingLists0, 16, sizeof(matrix.ucScalingLists0));
    std::memset(matrix.ucScalingLists1, 16, sizeof(matrix.ucScalingLists1));
    std::memset(matrix.ucScalingLists2, 16, sizeof(matrix.ucScalingLists2));
    std::memset(matrix.ucScalingLists3, 16, sizeof(matrix.ucScalingLists3));
    std::memset(matrix.ucScalingListDCCoefSizeID2, 16, sizeof(matrix.ucScalingListDCCoefSizeID2));
    std::memset(matrix.ucScalingListDCCoefSizeID3, 16, sizeof(matrix.ucScalingListDCCoefSizeID3));
}

inline bool BuildVideoDecodeH264ArgumentsD3D12(const VideoH264SessionParametersDesc& parameters, const VideoH264DecodePictureDesc& pictureDesc, uint64_t bitstreamSize,
    uint32_t dstSlot, DXVA_PicParams_H264& pictureParameters, DXVA_Qmatrix_H264& inverseQuantizationMatrix, DXVA_Slice_H264_Short* slices, uint32_t sliceNum) {
    if (sliceNum == 0 || sliceNum != pictureDesc.sliceOffsetNum || !pictureDesc.sliceOffsets || !slices)
        return false;

    if (pictureDesc.referenceNum > 16 || (pictureDesc.referenceNum && !pictureDesc.references))
        return false;

    uint16_t usedReferenceSlots = 0;
    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoH264DecodeReferenceDesc& reference = pictureDesc.references[i];
        if (reference.slot >= 16 || (usedReferenceSlots & (1u << reference.slot)))
            return false;
        usedReferenceSlots |= 1u << reference.slot;
    }

    const VideoH264PictureParameterSetDesc* pps = FindVideoH264PictureParameterSetD3D12(parameters, pictureDesc.pictureParameterSetId);
    if (!pps)
        return false;

    const VideoH264SequenceParameterSetDesc* sps = FindVideoH264SequenceParameterSetD3D12(parameters, pictureDesc.sequenceParameterSetId);
    if (!sps || pps->sequenceParameterSetId != sps->sequenceParameterSetId)
        return false;

    if (bitstreamSize > UINT32_MAX)
        return false;

    pictureParameters = {};
    pictureParameters.wFrameWidthInMbsMinus1 = sps->pictureWidthInMbsMinus1;
    pictureParameters.wFrameHeightInMbsMinus1 = sps->pictureHeightInMapUnitsMinus1;
    if (dstSlot > 0x7F)
        return false;

    pictureParameters.CurrPic.bPicEntry = (uint8_t)dstSlot;
    pictureParameters.CurrPic.AssociatedFlag = !!(pictureDesc.flags & VideoH264DecodePictureBits::BOTTOM_FIELD);
    pictureParameters.num_ref_frames = sps->referenceFrameNum;
    pictureParameters.field_pic_flag = !!(pictureDesc.flags & VideoH264DecodePictureBits::FIELD_PICTURE);
    pictureParameters.MbaffFrameFlag = !!(sps->flags & VideoH264SequenceParameterSetBits::MB_ADAPTIVE_FRAME_FIELD) && !pictureParameters.field_pic_flag;
    pictureParameters.chroma_format_idc = sps->chromaFormatIdc;
    pictureParameters.RefPicFlag = !!(pictureDesc.flags & VideoH264DecodePictureBits::REFERENCE);
    pictureParameters.constrained_intra_pred_flag = !!(pps->flags & VideoH264PictureParameterSetBits::CONSTRAINED_INTRA_PRED);
    pictureParameters.weighted_pred_flag = !!(pps->flags & VideoH264PictureParameterSetBits::WEIGHTED_PRED);
    pictureParameters.weighted_bipred_idc = pps->weightedBipredIdc;
    pictureParameters.MbsConsecutiveFlag = 1;
    pictureParameters.frame_mbs_only_flag = !!(sps->flags & VideoH264SequenceParameterSetBits::FRAME_MBS_ONLY);
    pictureParameters.transform_8x8_mode_flag = !!(pps->flags & VideoH264PictureParameterSetBits::TRANSFORM_8X8_MODE);
    pictureParameters.MinLumaBipredSize8x8Flag = sps->levelIdc >= 31;
    pictureParameters.IntraPicFlag = !!(pictureDesc.flags & VideoH264DecodePictureBits::INTRA);
    pictureParameters.bit_depth_luma_minus8 = sps->bitDepthLumaMinus8;
    pictureParameters.bit_depth_chroma_minus8 = sps->bitDepthChromaMinus8;
    pictureParameters.Reserved16Bits = 3;
    pictureParameters.StatusReportFeedbackNumber = 1;
    for (uint32_t i = 0; i < 16; i++)
        pictureParameters.RefFrameList[i].bPicEntry = 0xff;
    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoH264DecodeReferenceDesc& reference = pictureDesc.references[i];
        pictureParameters.RefFrameList[reference.slot].Index7Bits = (UCHAR)reference.slot;
        pictureParameters.RefFrameList[reference.slot].AssociatedFlag = !!(reference.flags & VideoH264DecodeReferenceBits::LONG_TERM);
        pictureParameters.FieldOrderCntList[reference.slot][0] = reference.topFieldOrderCount;
        pictureParameters.FieldOrderCntList[reference.slot][1] = reference.bottomFieldOrderCount;
        pictureParameters.FrameNumList[reference.slot] = (USHORT)reference.frameNum;
        if (reference.flags & VideoH264DecodeReferenceBits::TOP_FIELD)
            pictureParameters.UsedForReferenceFlags |= 1u << (reference.slot * 2);
        if (reference.flags & VideoH264DecodeReferenceBits::BOTTOM_FIELD)
            pictureParameters.UsedForReferenceFlags |= 2u << (reference.slot * 2);
        if (reference.flags & VideoH264DecodeReferenceBits::NON_EXISTING)
            pictureParameters.NonExistingFrameFlags |= 1u << reference.slot;
    }
    pictureParameters.CurrFieldOrderCnt[0] = pictureDesc.topFieldOrderCount;
    pictureParameters.CurrFieldOrderCnt[1] = pictureDesc.bottomFieldOrderCount;
    pictureParameters.pic_init_qs_minus26 = pps->pictureInitQsMinus26;
    pictureParameters.chroma_qp_index_offset = pps->chromaQpIndexOffset;
    pictureParameters.second_chroma_qp_index_offset = pps->secondChromaQpIndexOffset;
    pictureParameters.ContinuationFlag = 1;
    pictureParameters.pic_init_qp_minus26 = pps->pictureInitQpMinus26;
    pictureParameters.num_ref_idx_l0_active_minus1 = pps->refIndexL0DefaultActiveMinus1;
    pictureParameters.num_ref_idx_l1_active_minus1 = pps->refIndexL1DefaultActiveMinus1;
    pictureParameters.frame_num = pictureDesc.frameNum;
    pictureParameters.log2_max_frame_num_minus4 = sps->log2MaxFrameNumMinus4;
    pictureParameters.pic_order_cnt_type = sps->pictureOrderCountType;
    pictureParameters.log2_max_pic_order_cnt_lsb_minus4 = sps->log2MaxPictureOrderCountLsbMinus4;
    pictureParameters.delta_pic_order_always_zero_flag = !!(sps->flags & VideoH264SequenceParameterSetBits::DELTA_PIC_ORDER_ALWAYS_ZERO);
    pictureParameters.direct_8x8_inference_flag = !!(sps->flags & VideoH264SequenceParameterSetBits::DIRECT_8X8_INFERENCE);
    pictureParameters.entropy_coding_mode_flag = !!(pps->flags & VideoH264PictureParameterSetBits::ENTROPY_CODING_MODE);
    pictureParameters.pic_order_present_flag = !!(pps->flags & VideoH264PictureParameterSetBits::BOTTOM_FIELD_PIC_ORDER_IN_FRAME);
    pictureParameters.deblocking_filter_control_present_flag = !!(pps->flags & VideoH264PictureParameterSetBits::DEBLOCKING_FILTER_CONTROL_PRESENT);
    pictureParameters.redundant_pic_cnt_present_flag = !!(pps->flags & VideoH264PictureParameterSetBits::REDUNDANT_PIC_CNT_PRESENT);

    inverseQuantizationMatrix = {};
    std::memset(inverseQuantizationMatrix.bScalingLists4x4, 16, sizeof(inverseQuantizationMatrix.bScalingLists4x4));
    std::memset(inverseQuantizationMatrix.bScalingLists8x8, 16, sizeof(inverseQuantizationMatrix.bScalingLists8x8));

    for (uint32_t i = 0; i < sliceNum; i++) {
        const uint32_t offset = pictureDesc.sliceOffsets[i];
        if (offset >= bitstreamSize || (i + 1 < sliceNum && pictureDesc.sliceOffsets[i + 1] <= offset))
            return false;

        const uint64_t nextOffset = i + 1 < sliceNum ? pictureDesc.sliceOffsets[i + 1] : bitstreamSize;
        const uint64_t size = nextOffset - offset;
        if (size > UINT32_MAX)
            return false;

        slices[i] = {};
        slices[i].BSNALunitDataLocation = offset;
        slices[i].SliceBytesInBuffer = (UINT)size;
    }

    return true;
}

inline bool BuildVideoDecodeH265ArgumentsD3D12(const VideoH265SessionParametersDesc& parameters, const VideoH265DecodePictureDesc& pictureDesc, uint64_t bitstreamSize,
    uint32_t dstSlot, DXVA_PicParams_HEVC& pictureParameters, DXVA_Qmatrix_HEVC& inverseQuantizationMatrix, DXVA_Slice_HEVC_Short* slices, uint32_t sliceNum) {
    if (sliceNum == 0 || sliceNum != pictureDesc.sliceSegmentOffsetNum || !pictureDesc.sliceSegmentOffsets || !slices)
        return false;

    if (pictureDesc.referenceNum > VIDEO_D3D12_HEVC_MAX_REFERENCE_NUM || (pictureDesc.referenceNum && !pictureDesc.references))
        return false;

    if (dstSlot > VIDEO_D3D12_DECODE_MAX_PIC_ENTRY_SLOT || bitstreamSize > UINT32_MAX)
        return false;

    const VideoH265PictureParameterSetDesc* pps = FindVideoH265PictureParameterSetD3D12(parameters, pictureDesc.pictureParameterSetId);
    if (!pps)
        return false;

    const VideoH265SequenceParameterSetDesc* sps = FindVideoH265SequenceParameterSetD3D12(parameters, pictureDesc.sequenceParameterSetId);
    if (!sps || pps->sequenceParameterSetId != sps->sequenceParameterSetId || pps->videoParameterSetId != sps->videoParameterSetId)
        return false;

    const uint32_t log2MinCbSize = sps->log2MinLumaCodingBlockSizeMinus3 + 3u;
    if (log2MinCbSize >= 16 || sps->pictureWidthInLumaSamples == 0 || sps->pictureHeightInLumaSamples == 0)
        return false;

    pictureParameters = {};
    pictureParameters.PicWidthInMinCbsY = (USHORT)((sps->pictureWidthInLumaSamples + (1u << log2MinCbSize) - 1u) >> log2MinCbSize);
    pictureParameters.PicHeightInMinCbsY = (USHORT)((sps->pictureHeightInLumaSamples + (1u << log2MinCbSize) - 1u) >> log2MinCbSize);
    pictureParameters.CurrPic.Index7Bits = (UCHAR)dstSlot;
    pictureParameters.CurrPic.AssociatedFlag = 0;
    pictureParameters.chroma_format_idc = sps->chromaFormatIdc;
    pictureParameters.separate_colour_plane_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::SEPARATE_COLOUR_PLANE);
    pictureParameters.bit_depth_luma_minus8 = sps->bitDepthLumaMinus8;
    pictureParameters.bit_depth_chroma_minus8 = sps->bitDepthChromaMinus8;
    pictureParameters.log2_max_pic_order_cnt_lsb_minus4 = sps->log2MaxPictureOrderCountLsbMinus4;
    pictureParameters.NoPicReorderingFlag = sps->decPicBufMgr.maxNumReorderPics[0] == 0;
    pictureParameters.sps_max_dec_pic_buffering_minus1 = sps->decPicBufMgr.maxDecPicBufferingMinus1[0];
    pictureParameters.log2_min_luma_coding_block_size_minus3 = sps->log2MinLumaCodingBlockSizeMinus3;
    pictureParameters.log2_diff_max_min_luma_coding_block_size = sps->log2DiffMaxMinLumaCodingBlockSize;
    pictureParameters.log2_min_transform_block_size_minus2 = sps->log2MinLumaTransformBlockSizeMinus2;
    pictureParameters.log2_diff_max_min_transform_block_size = sps->log2DiffMaxMinLumaTransformBlockSize;
    pictureParameters.max_transform_hierarchy_depth_inter = sps->maxTransformHierarchyDepthInter;
    pictureParameters.max_transform_hierarchy_depth_intra = sps->maxTransformHierarchyDepthIntra;
    pictureParameters.num_short_term_ref_pic_sets = sps->numShortTermRefPicSets;
    pictureParameters.num_long_term_ref_pics_sps = sps->numLongTermRefPicsSps;
    pictureParameters.num_ref_idx_l0_default_active_minus1 = pps->refIndexL0DefaultActiveMinus1;
    pictureParameters.num_ref_idx_l1_default_active_minus1 = pps->refIndexL1DefaultActiveMinus1;
    pictureParameters.init_qp_minus26 = pps->initQpMinus26;
    pictureParameters.ucNumDeltaPocsOfRefRpsIdx = pictureDesc.numDeltaPocsOfRefRpsIdx;
    pictureParameters.wNumBitsForShortTermRPSInSlice = pictureDesc.numBitsForShortTermRefPicSetInSlice;
    pictureParameters.scaling_list_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::SCALING_LIST_ENABLED);
    pictureParameters.amp_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::AMP_ENABLED);
    pictureParameters.sample_adaptive_offset_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::SAMPLE_ADAPTIVE_OFFSET_ENABLED);
    pictureParameters.pcm_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::PCM_ENABLED);
    pictureParameters.pcm_sample_bit_depth_luma_minus1 = sps->pcmSampleBitDepthLumaMinus1;
    pictureParameters.pcm_sample_bit_depth_chroma_minus1 = sps->pcmSampleBitDepthChromaMinus1;
    pictureParameters.log2_min_pcm_luma_coding_block_size_minus3 = sps->log2MinPcmLumaCodingBlockSizeMinus3;
    pictureParameters.log2_diff_max_min_pcm_luma_coding_block_size = sps->log2DiffMaxMinPcmLumaCodingBlockSize;
    pictureParameters.pcm_loop_filter_disabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::PCM_LOOP_FILTER_DISABLED);
    pictureParameters.long_term_ref_pics_present_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::LONG_TERM_REF_PICS_PRESENT);
    pictureParameters.sps_temporal_mvp_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::TEMPORAL_MVP_ENABLED);
    pictureParameters.strong_intra_smoothing_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED);
    pictureParameters.dependent_slice_segments_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::DEPENDENT_SLICE_SEGMENTS_ENABLED);
    pictureParameters.output_flag_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::OUTPUT_FLAG_PRESENT);
    pictureParameters.num_extra_slice_header_bits = pps->numExtraSliceHeaderBits;
    pictureParameters.sign_data_hiding_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::SIGN_DATA_HIDING_ENABLED);
    pictureParameters.cabac_init_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::CABAC_INIT_PRESENT);
    pictureParameters.constrained_intra_pred_flag = !!(pps->flags & VideoH265PictureParameterSetBits::CONSTRAINED_INTRA_PRED);
    pictureParameters.transform_skip_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::TRANSFORM_SKIP_ENABLED);
    pictureParameters.cu_qp_delta_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::CU_QP_DELTA_ENABLED);
    pictureParameters.pps_slice_chroma_qp_offsets_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::SLICE_CHROMA_QP_OFFSETS_PRESENT);
    pictureParameters.weighted_pred_flag = !!(pps->flags & VideoH265PictureParameterSetBits::WEIGHTED_PRED);
    pictureParameters.weighted_bipred_flag = !!(pps->flags & VideoH265PictureParameterSetBits::WEIGHTED_BIPRED);
    pictureParameters.transquant_bypass_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::TRANSQUANT_BYPASS_ENABLED);
    pictureParameters.tiles_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::TILES_ENABLED);
    pictureParameters.entropy_coding_sync_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::ENTROPY_CODING_SYNC_ENABLED);
    pictureParameters.uniform_spacing_flag = !!(pps->flags & VideoH265PictureParameterSetBits::UNIFORM_SPACING);
    pictureParameters.loop_filter_across_tiles_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_TILES_ENABLED);
    pictureParameters.pps_loop_filter_across_slices_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_SLICES_ENABLED);
    pictureParameters.deblocking_filter_override_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_OVERRIDE_ENABLED);
    pictureParameters.pps_deblocking_filter_disabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_DISABLED);
    pictureParameters.lists_modification_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::LISTS_MODIFICATION_PRESENT);
    pictureParameters.slice_segment_header_extension_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::SLICE_SEGMENT_HEADER_EXTENSION_PRESENT);
    pictureParameters.IrapPicFlag = !!(pictureDesc.flags & VideoH265DecodePictureBits::IRAP);
    pictureParameters.IdrPicFlag = !!(pictureDesc.flags & VideoH265DecodePictureBits::IDR);
    pictureParameters.IntraPicFlag = pictureDesc.referenceNum == 0;
    pictureParameters.pps_cb_qp_offset = pps->cbQpOffset;
    pictureParameters.pps_cr_qp_offset = pps->crQpOffset;
    pictureParameters.num_tile_columns_minus1 = pps->tileColumnNumMinus1;
    pictureParameters.num_tile_rows_minus1 = pps->tileRowNumMinus1;
    std::memcpy(pictureParameters.column_width_minus1, pps->columnWidthMinus1, sizeof(pictureParameters.column_width_minus1));
    std::memcpy(pictureParameters.row_height_minus1, pps->rowHeightMinus1, sizeof(pictureParameters.row_height_minus1));
    pictureParameters.diff_cu_qp_delta_depth = pps->diffCuQpDeltaDepth;
    pictureParameters.pps_beta_offset_div2 = pps->betaOffsetDiv2;
    pictureParameters.pps_tc_offset_div2 = pps->tcOffsetDiv2;
    pictureParameters.log2_parallel_merge_level_minus2 = pps->log2ParallelMergeLevelMinus2;
    pictureParameters.CurrPicOrderCntVal = pictureDesc.pictureOrderCount;
    pictureParameters.StatusReportFeedbackNumber = 1;

    for (DXVA_PicEntry_HEVC& entry : pictureParameters.RefPicList)
        entry.bPicEntry = 0xFF;
    std::memset(pictureParameters.RefPicSetStCurrBefore, 0xFF, sizeof(pictureParameters.RefPicSetStCurrBefore));
    std::memset(pictureParameters.RefPicSetStCurrAfter, 0xFF, sizeof(pictureParameters.RefPicSetStCurrAfter));
    std::memset(pictureParameters.RefPicSetLtCurr, 0xFF, sizeof(pictureParameters.RefPicSetLtCurr));

    uint32_t beforeNum = 0;
    uint32_t afterNum = 0;
    uint32_t longTermNum = 0;
    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoH265ReferenceDesc& reference = pictureDesc.references[i];
        if (reference.slot > VIDEO_D3D12_DECODE_MAX_PIC_ENTRY_SLOT)
            return false;

        pictureParameters.RefPicList[i].Index7Bits = (UCHAR)reference.slot;
        pictureParameters.RefPicList[i].AssociatedFlag = reference.longTerm != 0;
        pictureParameters.PicOrderCntValList[i] = reference.pictureOrderCount;

        if (reference.longTerm) {
            if (longTermNum >= sizeof(pictureParameters.RefPicSetLtCurr))
                return false;
            pictureParameters.RefPicSetLtCurr[longTermNum++] = (UCHAR)i;
        } else if (reference.pictureOrderCount < pictureDesc.pictureOrderCount) {
            if (beforeNum >= sizeof(pictureParameters.RefPicSetStCurrBefore))
                return false;
            pictureParameters.RefPicSetStCurrBefore[beforeNum++] = (UCHAR)i;
        } else {
            if (afterNum >= sizeof(pictureParameters.RefPicSetStCurrAfter))
                return false;
            pictureParameters.RefPicSetStCurrAfter[afterNum++] = (UCHAR)i;
        }
    }

    FillVideoH265ScalingListsD3D12(inverseQuantizationMatrix, pps->scalingLists ? pps->scalingLists : sps->scalingLists);

    for (uint32_t i = 0; i < sliceNum; i++) {
        const uint32_t offset = pictureDesc.sliceSegmentOffsets[i];
        if (offset >= bitstreamSize || (i + 1 < sliceNum && pictureDesc.sliceSegmentOffsets[i + 1] <= offset))
            return false;

        const uint64_t nextOffset = i + 1 < sliceNum ? pictureDesc.sliceSegmentOffsets[i + 1] : bitstreamSize;
        const uint64_t size = nextOffset - offset;
        if (size > UINT32_MAX)
            return false;

        slices[i] = {};
        slices[i].BSNALunitDataLocation = offset;
        slices[i].SliceBytesInBuffer = (UINT)size;
    }

    return true;
}

inline bool IsVideoEncodeFrameTypeSupportedByD3D12NoBGop(VideoCodec codec, VideoEncodeFrameType frameType) {
    return frameType != VideoEncodeFrameType::B || codec != VideoCodec::H264;
}

inline bool IsVideoEncodePictureUsedAsReferenceD3D12(VideoCodec codec, uint32_t maxReferenceNum, bool hasReconstructedPicture, uint8_t av1RefreshFrameFlags) {
    if (!maxReferenceNum || !hasReconstructedPicture)
        return false;

    return codec != VideoCodec::AV1 || av1RefreshFrameFlags != 0;
}

inline uint8_t GetVideoEncodeQPByFrameTypeD3D12(const VideoEncodeRateControlDesc& rateControlDesc, VideoEncodeFrameType frameType) {
    return frameType == VideoEncodeFrameType::B ? rateControlDesc.qpB : (frameType == VideoEncodeFrameType::P ? rateControlDesc.qpP : rateControlDesc.qpI);
}

inline VideoAV1SequenceDesc GetDefaultVideoAV1SequenceDescD3D12(uint32_t width, uint32_t height, Format format) {
    VideoAV1SequenceDesc desc = {};
    desc.flags = VideoAV1SequenceBits::ENABLE_ORDER_HINT | VideoAV1SequenceBits::ENABLE_CDEF | VideoAV1SequenceBits::ENABLE_RESTORATION | VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT;
    desc.bitDepth = format == Format::P010_UNORM || format == Format::P016_UNORM ? 10 : 8;
    desc.subsamplingX = 1;
    desc.subsamplingY = 1;
    desc.maxFrameWidthMinus1 = (uint16_t)(width - 1);
    desc.maxFrameHeightMinus1 = (uint16_t)(height - 1);
    desc.frameWidthBitsMinus1 = 15;
    desc.frameHeightBitsMinus1 = 15;
    desc.orderHintBitsMinus1 = 7;
    desc.seqForceIntegerMv = 2;
    desc.seqForceScreenContentTools = 2;
    desc.colorPrimaries = 1;
    desc.transferCharacteristics = 1;
    desc.matrixCoefficients = 1;
    desc.chromaSamplePosition = 1;
    return desc;
}

inline VideoAV1PictureBits GetDefaultVideoAV1PictureFlags(bool) {
    VideoAV1PictureBits flags = VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::DISABLE_CDF_UPDATE | VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS | VideoAV1PictureBits::FORCE_INTEGER_MV;
    return flags;
}

struct VideoEncodeHEVCReferenceListsD3D12 {
    std::array<uint32_t, VIDEO_D3D12_HEVC_MAX_REFERENCE_NUM> list0 = {};
    std::array<uint32_t, VIDEO_D3D12_HEVC_MAX_REFERENCE_NUM> list1 = {};
    uint32_t list0Num = 0;
    uint32_t list1Num = 0;
    uint32_t failingReference = 0;
    bool missingDescriptor = false;
    bool invalidPictureOrderCount = false;
};

inline const VideoH265ReferenceDesc* FindVideoH265ReferenceDescD3D12(const VideoH265ReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return nullptr;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return &references[i];
    }

    return nullptr;
}

inline bool BuildVideoEncodeHEVCReferenceListsD3D12(const VideoReference* references, const VideoH265ReferenceDesc* referenceDescs, uint32_t referenceNum,
    VideoEncodeFrameType frameType, int32_t currentPictureOrderCount, VideoEncodeHEVCReferenceListsD3D12& lists) {
    lists = {};

    if (referenceNum > VIDEO_D3D12_HEVC_MAX_REFERENCE_NUM) {
        lists.failingReference = VIDEO_D3D12_HEVC_MAX_REFERENCE_NUM;
        return false;
    }

    if (referenceNum && !referenceDescs) {
        lists.missingDescriptor = true;
        return false;
    }

    for (uint32_t i = 0; i < referenceNum; i++) {
        const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescD3D12(referenceDescs, referenceNum, references[i].slot);
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

inline bool IsVideoEncodeAV1RequiredFeatureSetSupportedD3D12(uint32_t featureFlags) {
    constexpr uint32_t supportedFeatureFlags = VIDEO_D3D12_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS | VIDEO_D3D12_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER | VIDEO_D3D12_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS | VIDEO_D3D12_AV1_FEATURE_FLAG_AUTO_SEGMENTATION | VIDEO_D3D12_AV1_FEATURE_FLAG_CDEF_FILTERING | VIDEO_D3D12_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS | VIDEO_D3D12_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS;

    return (featureFlags & ~supportedFeatureFlags) == 0;
}

inline GUID GetVideoDecodeProfileD3D12(VideoCodec codec, Format format) {
    switch (codec) {
        case VideoCodec::H264:
            return D3D12_VIDEO_DECODE_PROFILE_H264;
        case VideoCodec::H265:
            return format == Format::P010_UNORM || format == Format::P016_UNORM ? D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
        case VideoCodec::AV1:
            return D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE0;
        case VideoCodec::NONE:
        case VideoCodec::MAX_NUM:
            return {};
    }

    return {};
}

inline D3D12_VIDEO_ENCODER_CODEC GetVideoEncodeCodecD3D12(VideoCodec codec) {
    switch (codec) {
        case VideoCodec::H264:
            return D3D12_VIDEO_ENCODER_CODEC_H264;
        case VideoCodec::H265:
            return D3D12_VIDEO_ENCODER_CODEC_HEVC;
        case VideoCodec::AV1:
            return D3D12_VIDEO_ENCODER_CODEC_AV1;
        case VideoCodec::NONE:
        case VideoCodec::MAX_NUM:
            return (D3D12_VIDEO_ENCODER_CODEC)-1;
    }

    return (D3D12_VIDEO_ENCODER_CODEC)-1;
}

inline bool IsVideoDecodeCodecSupportedD3D12(ID3D12VideoDevice& videoDevice, VideoCodec codec) {
    constexpr uint32_t width = 128;
    constexpr uint32_t height = 128;

    D3D12_VIDEO_DECODE_CONFIGURATION configuration = {};
    configuration.DecodeProfile = GetVideoDecodeProfileD3D12(codec, Format::NV12_UNORM);
    configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
    configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
    if (configuration.DecodeProfile == GUID{})
        return false;

    D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport = {};
    decodeSupport.Configuration = configuration;
    decodeSupport.Width = width;
    decodeSupport.Height = height;
    decodeSupport.DecodeFormat = DXGI_FORMAT_NV12;
    decodeSupport.FrameRate = {30, 1};

    HRESULT hr = videoDevice.CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT, &decodeSupport, sizeof(decodeSupport));
    if (FAILED(hr))
        return false;

    if ((decodeSupport.SupportFlags & D3D12_VIDEO_DECODE_SUPPORT_FLAG_SUPPORTED) == 0)
        return false;

    return (decodeSupport.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED) == 0;
}

inline bool IsVideoEncodeCodecSupportedD3D12(ID3D12VideoDevice3& videoDevice, VideoCodec codec) {
    constexpr uint32_t width = 128;
    constexpr uint32_t height = 128;

    D3D12_VIDEO_ENCODER_CODEC d3d12Codec = GetVideoEncodeCodecD3D12(codec);
    if (d3d12Codec == (D3D12_VIDEO_ENCODER_CODEC)-1)
        return false;

    D3D12_VIDEO_ENCODER_PROFILE_H264 h264Profile = D3D12_VIDEO_ENCODER_PROFILE_H264_HIGH;
    D3D12_VIDEO_ENCODER_PROFILE_HEVC hevcProfile = D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    D3D12_VIDEO_ENCODER_AV1_PROFILE av1Profile = D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN;
    D3D12_VIDEO_ENCODER_PROFILE_DESC profile = {};
    if (codec == VideoCodec::H264) {
        profile.DataSize = sizeof(h264Profile);
        profile.pH264Profile = &h264Profile;
    } else if (codec == VideoCodec::H265) {
        profile.DataSize = sizeof(hevcProfile);
        profile.pHEVCProfile = &hevcProfile;
    } else {
        profile.DataSize = sizeof(av1Profile);
        profile.pAV1Profile = &av1Profile;
    }

    D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264 h264Config = {};
    h264Config.DirectModeConfig = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_DIRECT_MODES_DISABLED;
    h264Config.DisableDeblockingFilterConfig = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_SLICES_DEBLOCKING_MODE_0_ALL_LUMA_CHROMA_SLICE_BLOCK_EDGES_ALWAYS_FILTERED;

    D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC hevcConfig = {};
    hevcConfig.MinLumaCodingUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8;
    hevcConfig.MaxLumaCodingUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_32x32;
    hevcConfig.MinLumaTransformUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4;
    hevcConfig.MaxLumaTransformUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32;
    hevcConfig.max_transform_hierarchy_depth_inter = 3;
    hevcConfig.max_transform_hierarchy_depth_intra = 3;
    if (codec == VideoCodec::H265) {
        D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC hevcCaps = {};
        hevcCaps.MinLumaCodingUnitSize = hevcConfig.MinLumaCodingUnitSize;
        hevcCaps.MaxLumaCodingUnitSize = hevcConfig.MaxLumaCodingUnitSize;
        hevcCaps.MinLumaTransformUnitSize = hevcConfig.MinLumaTransformUnitSize;
        hevcCaps.MaxLumaTransformUnitSize = hevcConfig.MaxLumaTransformUnitSize;
        hevcCaps.max_transform_hierarchy_depth_inter = hevcConfig.max_transform_hierarchy_depth_inter;
        hevcCaps.max_transform_hierarchy_depth_intra = hevcConfig.max_transform_hierarchy_depth_intra;

        D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT hevcConfigSupport = {};
        hevcConfigSupport.Codec = d3d12Codec;
        hevcConfigSupport.Profile = profile;
        hevcConfigSupport.CodecSupportLimits.DataSize = sizeof(hevcCaps);
        hevcConfigSupport.CodecSupportLimits.pHEVCSupport = &hevcCaps;
        HRESULT hr = videoDevice.CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &hevcConfigSupport, sizeof(hevcConfigSupport));
        if (FAILED(hr) || !hevcConfigSupport.IsSupported)
            return false;

        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_SUPPORT || hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_REQUIRED)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_ASYMETRIC_MOTION_PARTITION;
        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_SAO_FILTER_SUPPORT)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_SAO_FILTER;
        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_DISABLING_LOOP_FILTER_ACROSS_SLICES_SUPPORT)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_DISABLE_LOOP_FILTER_ACROSS_SLICES;
        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_TRANSFORM_SKIP_SUPPORT)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_TRANSFORM_SKIPPING;
    }

    D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION av1Config = {};
    av1Config.FeatureFlags = D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_NONE;
    av1Config.OrderHintBitsMinus1 = 7;
    if (codec == VideoCodec::AV1) {
        D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT av1Caps = {};
        D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT av1ConfigSupport = {};
        av1ConfigSupport.Codec = d3d12Codec;
        av1ConfigSupport.Profile = profile;
        av1ConfigSupport.CodecSupportLimits.DataSize = sizeof(av1Caps);
        av1ConfigSupport.CodecSupportLimits.pAV1Support = &av1Caps;
        HRESULT hr = videoDevice.CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &av1ConfigSupport, sizeof(av1ConfigSupport));
        if (FAILED(hr) || !av1ConfigSupport.IsSupported || !IsVideoEncodeAV1RequiredFeatureSetSupportedD3D12(av1Caps.RequiredFeatureFlags))
            return false;

        av1Config.FeatureFlags = av1Caps.RequiredFeatureFlags;
    }

    D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfig = {};
    if (codec == VideoCodec::H264) {
        codecConfig.DataSize = sizeof(h264Config);
        codecConfig.pH264Config = &h264Config;
    } else if (codec == VideoCodec::H265) {
        codecConfig.DataSize = sizeof(hevcConfig);
        codecConfig.pHEVCConfig = &hevcConfig;
    } else {
        codecConfig.DataSize = sizeof(av1Config);
        codecConfig.pAV1Config = &av1Config;
    }

    D3D12_FEATURE_DATA_VIDEO_ENCODER_RATE_CONTROL_MODE rateControlMode = {};
    rateControlMode.Codec = d3d12Codec;
    rateControlMode.RateControlMode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
    HRESULT hr = videoDevice.CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_RATE_CONTROL_MODE, &rateControlMode, sizeof(rateControlMode));
    if (FAILED(hr) || !rateControlMode.IsSupported)
        return false;

    D3D12_VIDEO_ENCODER_RATE_CONTROL_CQP cqp = {26, 28, 30};
    D3D12_VIDEO_ENCODER_RATE_CONTROL rateControl = {};
    rateControl.Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
    rateControl.ConfigParams.DataSize = sizeof(cqp);
    rateControl.ConfigParams.pConfiguration_CQP = &cqp;
    rateControl.TargetFrameRate = {30, 1};

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = {};
    h264Gop.GOPLength = 1;
    h264Gop.PPicturePeriod = 1;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = {};
    hevcGop.GOPLength = 1;
    hevcGop.PPicturePeriod = 1;

    D3D12_VIDEO_ENCODER_AV1_SEQUENCE_STRUCTURE av1Sequence = {};
    av1Sequence.IntraDistance = 1;
    av1Sequence.InterFramePeriod = 0;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE gop = {};
    if (codec == VideoCodec::H264) {
        gop.DataSize = sizeof(h264Gop);
        gop.pH264GroupOfPictures = &h264Gop;
    } else if (codec == VideoCodec::H265) {
        gop.DataSize = sizeof(hevcGop);
        gop.pHEVCGroupOfPictures = &hevcGop;
    } else {
        gop.DataSize = sizeof(av1Sequence);
        gop.pAV1SequenceStructure = &av1Sequence;
    }

    D3D12_VIDEO_ENCODER_LEVELS_H264 suggestedH264Level = D3D12_VIDEO_ENCODER_LEVELS_H264_41;
    D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC suggestedHevcLevel = {D3D12_VIDEO_ENCODER_LEVELS_HEVC_41, D3D12_VIDEO_ENCODER_TIER_HEVC_MAIN};
    D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS suggestedAv1Level = {D3D12_VIDEO_ENCODER_AV1_LEVELS_4_1, D3D12_VIDEO_ENCODER_AV1_TIER_MAIN};
    D3D12_VIDEO_ENCODER_LEVEL_SETTING suggestedLevel = {};
    if (codec == VideoCodec::H264) {
        suggestedLevel.DataSize = sizeof(suggestedH264Level);
        suggestedLevel.pH264LevelSetting = &suggestedH264Level;
    } else if (codec == VideoCodec::H265) {
        suggestedLevel.DataSize = sizeof(suggestedHevcLevel);
        suggestedLevel.pHEVCLevelSetting = &suggestedHevcLevel;
    } else {
        suggestedLevel.DataSize = sizeof(suggestedAv1Level);
        suggestedLevel.pAV1LevelSetting = &suggestedAv1Level;
    }

    D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC resolution = {width, height};
    D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionLimits = {};
    if (codec == VideoCodec::AV1) {
        D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES tiles = {};
        tiles.RowCount = 1;
        tiles.ColCount = 1;

        D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 encoderSupport = {};
        encoderSupport.Codec = d3d12Codec;
        encoderSupport.InputFormat = DXGI_FORMAT_NV12;
        encoderSupport.CodecConfiguration = codecConfig;
        encoderSupport.CodecGopSequence = gop;
        encoderSupport.RateControl = rateControl;
        encoderSupport.IntraRefresh = D3D12_VIDEO_ENCODER_INTRA_REFRESH_MODE_NONE;
        encoderSupport.SubregionFrameEncoding = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
        encoderSupport.ResolutionsListCount = 1;
        encoderSupport.pResolutionList = &resolution;
        encoderSupport.MaxReferenceFramesInDPB = 8;
        encoderSupport.SuggestedProfile = profile;
        encoderSupport.SuggestedLevel = suggestedLevel;
        encoderSupport.pResolutionDependentSupport = &resolutionLimits;
        encoderSupport.SubregionFrameEncodingData.DataSize = sizeof(tiles);
        encoderSupport.SubregionFrameEncodingData.pTilesPartition_AV1 = &tiles;
        hr = videoDevice.CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1, &encoderSupport, sizeof(encoderSupport));
        if (FAILED(hr) || (encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0)
            return false;

        return (encoderSupport.SupportFlags & VIDEO_D3D12_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) != 0;
    }

    D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT encoderSupport = {};
    encoderSupport.Codec = d3d12Codec;
    encoderSupport.InputFormat = DXGI_FORMAT_NV12;
    encoderSupport.CodecConfiguration = codecConfig;
    encoderSupport.CodecGopSequence = gop;
    encoderSupport.RateControl = rateControl;
    encoderSupport.IntraRefresh = D3D12_VIDEO_ENCODER_INTRA_REFRESH_MODE_NONE;
    encoderSupport.SubregionFrameEncoding = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
    encoderSupport.ResolutionsListCount = 1;
    encoderSupport.pResolutionList = &resolution;
    encoderSupport.MaxReferenceFramesInDPB = 0;
    encoderSupport.SuggestedProfile = profile;
    encoderSupport.SuggestedLevel = suggestedLevel;
    encoderSupport.pResolutionDependentSupport = &resolutionLimits;
    hr = videoDevice.CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT, &encoderSupport, sizeof(encoderSupport));
    if (FAILED(hr) || (encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0)
        return false;

    return (encoderSupport.SupportFlags & VIDEO_D3D12_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) != 0;
}

} // namespace nri
