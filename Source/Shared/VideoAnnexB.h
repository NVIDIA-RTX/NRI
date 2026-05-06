// © 2021 NVIDIA Corporation

#pragma once

#include "Extensions/NRIVideo.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace nri {

namespace video_annex_b {

struct BitWriter {
    std::vector<uint8_t> bytes;
    uint8_t bitCount = 0;

    void WriteBit(uint32_t bit) {
        if (bitCount == 0)
            bytes.push_back(0);
        if (bit & 1)
            bytes.back() |= uint8_t(1u << (7u - bitCount));
        bitCount = (bitCount + 1) & 7u;
    }

    void WriteBits(uint64_t value, uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            WriteBit((uint32_t)((value >> (count - i - 1u)) & 1u));
    }

    void WriteUe(uint32_t value) {
        const uint32_t codeNum = value + 1u;
        uint32_t bitNum = 0;
        for (uint32_t temp = codeNum; temp; temp >>= 1)
            bitNum++;
        for (uint32_t i = 1; i < bitNum; i++)
            WriteBit(0);
        WriteBits(codeNum, bitNum);
    }

    void WriteSe(int32_t value) {
        const uint32_t codeNum = value <= 0 ? uint32_t(-value) * 2u : uint32_t(value) * 2u - 1u;
        WriteUe(codeNum);
    }

    void FinishRbsp() {
        WriteBit(1);
        while (bitCount != 0)
            WriteBit(0);
    }
};

inline void AppendNalRbsp(std::vector<uint8_t>& bytes, const std::vector<uint8_t>& rbsp) {
    uint32_t zeroRun = 0;
    for (uint8_t byte : rbsp) {
        if (zeroRun >= 2 && byte <= 3) {
            bytes.push_back(3);
            zeroRun = 0;
        }

        bytes.push_back(byte);
        zeroRun = byte == 0 ? zeroRun + 1 : 0;
    }
}

inline void AppendH264NalHeader(std::vector<uint8_t>& bytes, uint8_t nalHeader) {
    bytes.insert(bytes.end(), {0, 0, 0, 1, nalHeader});
}

inline void AppendH265NalHeader(std::vector<uint8_t>& bytes, uint8_t nalUnitType) {
    bytes.insert(bytes.end(), {0, 0, 0, 1, uint8_t(nalUnitType << 1), 1});
}

inline void WriteH265ProfileTierLevel(BitWriter& writer, const VideoH265ProfileTierLevelDesc& desc, uint8_t maxSubLayersMinus1) {
    writer.WriteBits(0, 2); // general_profile_space
    writer.WriteBit(!!(desc.flags & VideoH265ProfileTierLevelBits::TIER));
    writer.WriteBits(desc.generalProfileIdc, 5);

    uint32_t compatibilityFlags = 0;
    if (desc.generalProfileIdc >= 1 && desc.generalProfileIdc <= 32)
        compatibilityFlags = 1u << (31u - desc.generalProfileIdc);
    writer.WriteBits(compatibilityFlags, 32);

    writer.WriteBit(!!(desc.flags & VideoH265ProfileTierLevelBits::PROGRESSIVE_SOURCE));
    writer.WriteBit(!!(desc.flags & VideoH265ProfileTierLevelBits::INTERLACED_SOURCE));
    writer.WriteBit(!!(desc.flags & VideoH265ProfileTierLevelBits::NON_PACKED_CONSTRAINT));
    writer.WriteBit(!!(desc.flags & VideoH265ProfileTierLevelBits::FRAME_ONLY_CONSTRAINT));
    writer.WriteBits(0, 44); // general_reserved_zero_44bits
    writer.WriteBits(desc.generalLevelIdc, 8);

    for (uint32_t i = 0; i < maxSubLayersMinus1; i++) {
        writer.WriteBit(0); // sub_layer_profile_present_flag
        writer.WriteBit(0); // sub_layer_level_present_flag
    }
    if (maxSubLayersMinus1 > 0) {
        for (uint32_t i = maxSubLayersMinus1; i < 8; i++)
            writer.WriteBits(0, 2);
    }
}

inline void WriteH265SubLayerOrdering(BitWriter& writer, const VideoH265DecPicBufMgrDesc& desc, uint8_t maxSubLayersMinus1, bool allSubLayers) {
    const uint32_t firstLayer = allSubLayers ? 0 : maxSubLayersMinus1;
    for (uint32_t i = firstLayer; i <= maxSubLayersMinus1; i++) {
        writer.WriteUe(desc.maxDecPicBufferingMinus1[i]);
        writer.WriteUe(desc.maxNumReorderPics[i]);
        writer.WriteUe(desc.maxLatencyIncreasePlus1[i]);
    }
}

inline Result WriteH264AnnexBParameterSets(const VideoAnnexBParameterSetsDesc& desc, std::vector<uint8_t>& headers) {
    if (!desc.h264Sps || !desc.h264Pps)
        return Result::INVALID_ARGUMENT;

    const VideoH264SequenceParameterSetDesc& sps = *desc.h264Sps;
    const VideoH264PictureParameterSetDesc& pps = *desc.h264Pps;
    const bool highProfileSps = sps.profileIdc == 100 || sps.profileIdc == 110 || sps.profileIdc == 122 || sps.profileIdc == 244 || sps.profileIdc == 44 || sps.profileIdc == 83 || sps.profileIdc == 86 || sps.profileIdc == 118 || sps.profileIdc == 128 || sps.profileIdc == 138 || sps.profileIdc == 139 || sps.profileIdc == 134 || sps.profileIdc == 135;
    if (!highProfileSps || (pps.flags & VideoH264PictureParameterSetBits::TRANSFORM_8X8_MODE) != 0)
        return Result::UNSUPPORTED;

    AppendH264NalHeader(headers, 0x67);
    BitWriter spsWriter;
    spsWriter.WriteBits(sps.profileIdc, 8);
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET0));
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET1));
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET2));
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET3));
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET4));
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET5));
    spsWriter.WriteBits(0, 2);
    spsWriter.WriteBits(sps.levelIdc, 8);
    spsWriter.WriteUe(sps.sequenceParameterSetId);
    spsWriter.WriteUe(sps.chromaFormatIdc);
    spsWriter.WriteUe(sps.bitDepthLumaMinus8);
    spsWriter.WriteUe(sps.bitDepthChromaMinus8);
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::QPPRIME_Y_ZERO_TRANSFORM_BYPASS));
    spsWriter.WriteBit(0);
    spsWriter.WriteUe(sps.log2MaxFrameNumMinus4);
    spsWriter.WriteUe(sps.pictureOrderCountType);
    if (sps.pictureOrderCountType == 0)
        spsWriter.WriteUe(sps.log2MaxPictureOrderCountLsbMinus4);
    spsWriter.WriteUe(sps.referenceFrameNum);
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::GAPS_IN_FRAME_NUM_ALLOWED));
    spsWriter.WriteUe(sps.pictureWidthInMbsMinus1);
    spsWriter.WriteUe(sps.pictureHeightInMapUnitsMinus1);
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::FRAME_MBS_ONLY));
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::DIRECT_8X8_INFERENCE));
    spsWriter.WriteBit(0);
    spsWriter.WriteBit(0);
    spsWriter.FinishRbsp();
    AppendNalRbsp(headers, spsWriter.bytes);

    AppendH264NalHeader(headers, 0x68);
    BitWriter ppsWriter;
    ppsWriter.WriteUe(pps.pictureParameterSetId);
    ppsWriter.WriteUe(pps.sequenceParameterSetId);
    ppsWriter.WriteBit(!!(pps.flags & VideoH264PictureParameterSetBits::ENTROPY_CODING_MODE));
    ppsWriter.WriteBit(!!(pps.flags & VideoH264PictureParameterSetBits::BOTTOM_FIELD_PIC_ORDER_IN_FRAME));
    ppsWriter.WriteUe(0);
    ppsWriter.WriteUe(pps.refIndexL0DefaultActiveMinus1);
    ppsWriter.WriteUe(pps.refIndexL1DefaultActiveMinus1);
    ppsWriter.WriteBit(!!(pps.flags & VideoH264PictureParameterSetBits::WEIGHTED_PRED));
    ppsWriter.WriteBits(pps.weightedBipredIdc, 2);
    ppsWriter.WriteSe(pps.pictureInitQpMinus26);
    ppsWriter.WriteSe(pps.pictureInitQsMinus26);
    ppsWriter.WriteSe(pps.chromaQpIndexOffset);
    ppsWriter.WriteBit(!!(pps.flags & VideoH264PictureParameterSetBits::DEBLOCKING_FILTER_CONTROL_PRESENT));
    ppsWriter.WriteBit(!!(pps.flags & VideoH264PictureParameterSetBits::CONSTRAINED_INTRA_PRED));
    ppsWriter.WriteBit(!!(pps.flags & VideoH264PictureParameterSetBits::REDUNDANT_PIC_CNT_PRESENT));
    ppsWriter.FinishRbsp();
    AppendNalRbsp(headers, ppsWriter.bytes);

    return Result::SUCCESS;
}

inline Result WriteH265AnnexBParameterSets(const VideoAnnexBParameterSetsDesc& desc, std::vector<uint8_t>& headers) {
    if (!desc.h265Vps || !desc.h265Sps || !desc.h265Pps)
        return Result::INVALID_ARGUMENT;

    const VideoH265VideoParameterSetDesc& vps = *desc.h265Vps;
    const VideoH265SequenceParameterSetDesc& sps = *desc.h265Sps;
    const VideoH265PictureParameterSetDesc& pps = *desc.h265Pps;
    const uint8_t vpsMaxSubLayersMinus1 = std::min<uint8_t>(vps.maxSubLayersMinus1, 7);
    const uint8_t spsMaxSubLayersMinus1 = std::min<uint8_t>(sps.maxSubLayersMinus1, 7);

    AppendH265NalHeader(headers, 32);
    BitWriter vpsWriter;
    vpsWriter.WriteBits(vps.videoParameterSetId, 4);
    vpsWriter.WriteBit(1);
    vpsWriter.WriteBit(1);
    vpsWriter.WriteBits(0, 6);
    vpsWriter.WriteBits(vpsMaxSubLayersMinus1, 3);
    vpsWriter.WriteBit(!!(vps.flags & VideoH265VideoParameterSetBits::TEMPORAL_ID_NESTING));
    vpsWriter.WriteBits(0xFFFF, 16);
    WriteH265ProfileTierLevel(vpsWriter, vps.profileTierLevel, vpsMaxSubLayersMinus1);
    const bool vpsAllSubLayers = !!(vps.flags & VideoH265VideoParameterSetBits::SUB_LAYER_ORDERING_INFO_PRESENT);
    vpsWriter.WriteBit(vpsAllSubLayers);
    WriteH265SubLayerOrdering(vpsWriter, vps.decPicBufMgr, vpsMaxSubLayersMinus1, vpsAllSubLayers);
    vpsWriter.WriteBits(0, 6);
    vpsWriter.WriteUe(0);
    const bool vpsTimingInfoPresent = !!(vps.flags & VideoH265VideoParameterSetBits::TIMING_INFO_PRESENT);
    vpsWriter.WriteBit(vpsTimingInfoPresent);
    if (vpsTimingInfoPresent) {
        vpsWriter.WriteBits(vps.numUnitsInTick, 32);
        vpsWriter.WriteBits(vps.timeScale, 32);
        const bool pocProportional = !!(vps.flags & VideoH265VideoParameterSetBits::POC_PROPORTIONAL_TO_TIMING);
        vpsWriter.WriteBit(pocProportional);
        if (pocProportional)
            vpsWriter.WriteUe(vps.numTicksPocDiffOneMinus1);
        vpsWriter.WriteUe(0);
    }
    vpsWriter.WriteBit(0);
    vpsWriter.FinishRbsp();
    AppendNalRbsp(headers, vpsWriter.bytes);

    AppendH265NalHeader(headers, 33);
    BitWriter spsWriter;
    spsWriter.WriteBits(sps.videoParameterSetId, 4);
    spsWriter.WriteBits(spsMaxSubLayersMinus1, 3);
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::TEMPORAL_ID_NESTING));
    WriteH265ProfileTierLevel(spsWriter, sps.profileTierLevel, spsMaxSubLayersMinus1);
    spsWriter.WriteUe(sps.sequenceParameterSetId);
    spsWriter.WriteUe(sps.chromaFormatIdc);
    if (sps.chromaFormatIdc == 3)
        spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::SEPARATE_COLOUR_PLANE));
    spsWriter.WriteUe(sps.pictureWidthInLumaSamples);
    spsWriter.WriteUe(sps.pictureHeightInLumaSamples);
    const bool conformanceWindow = !!(sps.flags & VideoH265SequenceParameterSetBits::CONFORMANCE_WINDOW);
    spsWriter.WriteBit(conformanceWindow);
    if (conformanceWindow) {
        spsWriter.WriteUe(sps.confWinLeftOffset);
        spsWriter.WriteUe(sps.confWinRightOffset);
        spsWriter.WriteUe(sps.confWinTopOffset);
        spsWriter.WriteUe(sps.confWinBottomOffset);
    }
    spsWriter.WriteUe(sps.bitDepthLumaMinus8);
    spsWriter.WriteUe(sps.bitDepthChromaMinus8);
    spsWriter.WriteUe(sps.log2MaxPictureOrderCountLsbMinus4);
    const bool spsAllSubLayers = !!(sps.flags & VideoH265SequenceParameterSetBits::SUB_LAYER_ORDERING_INFO_PRESENT);
    spsWriter.WriteBit(spsAllSubLayers);
    WriteH265SubLayerOrdering(spsWriter, sps.decPicBufMgr, spsMaxSubLayersMinus1, spsAllSubLayers);
    spsWriter.WriteUe(sps.log2MinLumaCodingBlockSizeMinus3);
    spsWriter.WriteUe(sps.log2DiffMaxMinLumaCodingBlockSize);
    spsWriter.WriteUe(sps.log2MinLumaTransformBlockSizeMinus2);
    spsWriter.WriteUe(sps.log2DiffMaxMinLumaTransformBlockSize);
    spsWriter.WriteUe(sps.maxTransformHierarchyDepthInter);
    spsWriter.WriteUe(sps.maxTransformHierarchyDepthIntra);
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::SCALING_LIST_ENABLED));
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::AMP_ENABLED));
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::SAMPLE_ADAPTIVE_OFFSET_ENABLED));
    const bool pcmEnabled = !!(sps.flags & VideoH265SequenceParameterSetBits::PCM_ENABLED);
    spsWriter.WriteBit(pcmEnabled);
    if (pcmEnabled) {
        spsWriter.WriteBits(sps.pcmSampleBitDepthLumaMinus1, 4);
        spsWriter.WriteBits(sps.pcmSampleBitDepthChromaMinus1, 4);
        spsWriter.WriteUe(sps.log2MinPcmLumaCodingBlockSizeMinus3);
        spsWriter.WriteUe(sps.log2DiffMaxMinPcmLumaCodingBlockSize);
        spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::PCM_LOOP_FILTER_DISABLED));
    }
    spsWriter.WriteUe(sps.numShortTermRefPicSets);
    if (sps.numShortTermRefPicSets != 0)
        return Result::UNSUPPORTED;
    const bool longTermRefsPresent = !!(sps.flags & VideoH265SequenceParameterSetBits::LONG_TERM_REF_PICS_PRESENT);
    spsWriter.WriteBit(longTermRefsPresent);
    if (longTermRefsPresent)
        return Result::UNSUPPORTED;
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::TEMPORAL_MVP_ENABLED));
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED));
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::VUI_PARAMETERS_PRESENT));
    if (sps.flags & VideoH265SequenceParameterSetBits::VUI_PARAMETERS_PRESENT)
        return Result::UNSUPPORTED;
    spsWriter.WriteBit(0);
    spsWriter.FinishRbsp();
    AppendNalRbsp(headers, spsWriter.bytes);

    AppendH265NalHeader(headers, 34);
    BitWriter ppsWriter;
    ppsWriter.WriteUe(pps.pictureParameterSetId);
    ppsWriter.WriteUe(pps.sequenceParameterSetId);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::DEPENDENT_SLICE_SEGMENTS_ENABLED));
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::OUTPUT_FLAG_PRESENT));
    ppsWriter.WriteBits(pps.numExtraSliceHeaderBits, 3);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::SIGN_DATA_HIDING_ENABLED));
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::CABAC_INIT_PRESENT));
    ppsWriter.WriteUe(pps.refIndexL0DefaultActiveMinus1);
    ppsWriter.WriteUe(pps.refIndexL1DefaultActiveMinus1);
    ppsWriter.WriteSe(pps.initQpMinus26);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::CONSTRAINED_INTRA_PRED));
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::TRANSFORM_SKIP_ENABLED));
    const bool cuQpDelta = !!(pps.flags & VideoH265PictureParameterSetBits::CU_QP_DELTA_ENABLED);
    ppsWriter.WriteBit(cuQpDelta);
    if (cuQpDelta)
        ppsWriter.WriteUe(pps.diffCuQpDeltaDepth);
    ppsWriter.WriteSe(pps.cbQpOffset);
    ppsWriter.WriteSe(pps.crQpOffset);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::SLICE_CHROMA_QP_OFFSETS_PRESENT));
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::WEIGHTED_PRED));
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::WEIGHTED_BIPRED));
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::TRANSQUANT_BYPASS_ENABLED));
    if (pps.flags & VideoH265PictureParameterSetBits::TILES_ENABLED)
        return Result::UNSUPPORTED;
    ppsWriter.WriteBit(0);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::ENTROPY_CODING_SYNC_ENABLED));
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_SLICES_ENABLED));
    const bool deblockingControl = !!(pps.flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_CONTROL_PRESENT);
    ppsWriter.WriteBit(deblockingControl);
    if (deblockingControl) {
        ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_OVERRIDE_ENABLED));
        const bool deblockingDisabled = !!(pps.flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_DISABLED);
        ppsWriter.WriteBit(deblockingDisabled);
        if (!deblockingDisabled) {
            ppsWriter.WriteSe(pps.betaOffsetDiv2);
            ppsWriter.WriteSe(pps.tcOffsetDiv2);
        }
    }
    if (pps.flags & VideoH265PictureParameterSetBits::SCALING_LIST_DATA_PRESENT)
        return Result::UNSUPPORTED;
    ppsWriter.WriteBit(0);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::LISTS_MODIFICATION_PRESENT));
    ppsWriter.WriteUe(pps.log2ParallelMergeLevelMinus2);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::SLICE_SEGMENT_HEADER_EXTENSION_PRESENT));
    ppsWriter.WriteBit(0);
    ppsWriter.FinishRbsp();
    AppendNalRbsp(headers, ppsWriter.bytes);

    return Result::SUCCESS;
}

} // namespace video_annex_b

inline Result WriteVideoAnnexBParameterSetsShared(VideoAnnexBParameterSetsDesc& desc) {
    std::vector<uint8_t> headers;
    Result result = Result::UNSUPPORTED;

    if (desc.codec == VideoCodec::H264)
        result = video_annex_b::WriteH264AnnexBParameterSets(desc, headers);
    else if (desc.codec == VideoCodec::H265)
        result = video_annex_b::WriteH265AnnexBParameterSets(desc, headers);

    if (result != Result::SUCCESS)
        return result;

    desc.writtenSize = headers.size();
    if (!desc.dst)
        return Result::SUCCESS;

    if (desc.dstSize < headers.size())
        return Result::INVALID_ARGUMENT;

    std::memcpy(desc.dst, headers.data(), headers.size());
    return Result::SUCCESS;
}

} // namespace nri
