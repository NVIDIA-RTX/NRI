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
    return desc.h264PictureDesc && !desc.h265PictureDesc && desc.argumentNum == 0 &&
        (desc.referenceNum == 0 || (desc.h264PictureDesc->references && desc.h264PictureDesc->referenceNum == desc.referenceNum));
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
    desc.flags = VideoAV1SequenceBits::ENABLE_ORDER_HINT |
        VideoAV1SequenceBits::ENABLE_CDEF |
        VideoAV1SequenceBits::ENABLE_RESTORATION |
        VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT;
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
    VideoAV1PictureBits flags = VideoAV1PictureBits::ERROR_RESILIENT_MODE |
        VideoAV1PictureBits::DISABLE_CDF_UPDATE |
        VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS |
        VideoAV1PictureBits::FORCE_INTEGER_MV;
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
    constexpr uint32_t supportedFeatureFlags =
        VIDEO_D3D12_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS |
        VIDEO_D3D12_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER |
        VIDEO_D3D12_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS |
        VIDEO_D3D12_AV1_FEATURE_FLAG_AUTO_SEGMENTATION |
        VIDEO_D3D12_AV1_FEATURE_FLAG_CDEF_FILTERING |
        VIDEO_D3D12_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS |
        VIDEO_D3D12_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS;

    return (featureFlags & ~supportedFeatureFlags) == 0;
}

} // namespace nri
