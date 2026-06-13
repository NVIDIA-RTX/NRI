// © 2026 NVIDIA Corporation

#pragma once

#include "VideoAV1.h"

namespace nri {

namespace video_annex_b {

struct ByteWriter {
    uint8_t* dst;
    uint64_t dstSize;
    uint64_t writtenSize = 0;
    bool overflow = false;

    void WriteByte(uint8_t byte) {
        if (dst) {
            if (writtenSize < dstSize)
                dst[writtenSize] = byte;
            else
                overflow = true;
        }
        writtenSize++;
    }

    Result Finish(uint64_t& size) const {
        size = writtenSize;
        return overflow ? Result::INVALID_ARGUMENT : Result::SUCCESS;
    }
};

struct RbspBitWriter {
    ByteWriter& bytes;
    uint32_t zeroRun = 0;
    uint8_t byte = 0;
    uint8_t bitCount = 0;

    void WriteRbspByte(uint8_t value) {
        if (zeroRun >= 2 && value <= 3) {
            bytes.WriteByte(3);
            zeroRun = 0;
        }

        bytes.WriteByte(value);
        zeroRun = value == 0 ? zeroRun + 1 : 0;
    }

    void WriteBit(uint32_t bit) {
        if (bitCount == 0)
            byte = 0;
        if (bit & 1)
            byte |= uint8_t(1u << (7u - bitCount));
        bitCount++;
        if (bitCount == 8) {
            WriteRbspByte(byte);
            bitCount = 0;
        }
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

inline void AppendH264NalHeader(ByteWriter& bytes, uint8_t nalHeader) {
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(1);
    bytes.WriteByte(nalHeader);
}

inline void AppendH265NalHeader(ByteWriter& bytes, uint8_t nalUnitType) {
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(1);
    bytes.WriteByte(uint8_t(nalUnitType << 1));
    bytes.WriteByte(1);
}

inline void WriteH265ProfileTierLevel(RbspBitWriter& writer, const VideoH265ProfileTierLevelDesc& desc, uint8_t maxSubLayersMinus1) {
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

inline void WriteH265SubLayerOrdering(RbspBitWriter& writer, const VideoH265DecPicBufMgrDesc& desc, uint8_t maxSubLayersMinus1, bool allSubLayers) {
    const uint32_t firstLayer = allSubLayers ? 0 : maxSubLayersMinus1;
    for (uint32_t i = firstLayer; i <= maxSubLayersMinus1; i++) {
        writer.WriteUe(desc.maxDecPicBufferingMinus1[i]);
        writer.WriteUe(desc.maxNumReorderPics[i]);
        writer.WriteUe(desc.maxLatencyIncreasePlus1[i]);
    }
}

inline Result WriteH264AnnexBParameterSets(const VideoAnnexBParameterSetsDesc& desc, ByteWriter& bytes) {
    if (!desc.h264Sps || !desc.h264Pps)
        return Result::INVALID_ARGUMENT;

    const VideoH264SequenceParameterSetDesc& sps = *desc.h264Sps;
    const VideoH264PictureParameterSetDesc& pps = *desc.h264Pps;
    const bool highProfileSps = sps.profileIdc == 100 || sps.profileIdc == 110 || sps.profileIdc == 122 || sps.profileIdc == 244 || sps.profileIdc == 44 || sps.profileIdc == 83 || sps.profileIdc == 86 || sps.profileIdc == 118 || sps.profileIdc == 128 || sps.profileIdc == 138 || sps.profileIdc == 139 || sps.profileIdc == 134 || sps.profileIdc == 135;
    if (!highProfileSps || (pps.flags & VideoH264PictureParameterSetBits::TRANSFORM_8X8_MODE) != 0)
        return Result::UNSUPPORTED;

    AppendH264NalHeader(bytes, 0x67);
    RbspBitWriter spsWriter {bytes};
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

    AppendH264NalHeader(bytes, 0x68);
    RbspBitWriter ppsWriter {bytes};
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

    return Result::SUCCESS;
}

inline Result WriteH265AnnexBParameterSets(const VideoAnnexBParameterSetsDesc& desc, ByteWriter& bytes) {
    if (!desc.h265Vps || !desc.h265Sps || !desc.h265Pps)
        return Result::INVALID_ARGUMENT;

    const VideoH265VideoParameterSetDesc& vps = *desc.h265Vps;
    const VideoH265SequenceParameterSetDesc& sps = *desc.h265Sps;
    const VideoH265PictureParameterSetDesc& pps = *desc.h265Pps;
    const uint8_t vpsMaxSubLayersMinus1 = std::min<uint8_t>(vps.maxSubLayersMinus1, 7);
    const uint8_t spsMaxSubLayersMinus1 = std::min<uint8_t>(sps.maxSubLayersMinus1, 7);

    AppendH265NalHeader(bytes, 32);
    RbspBitWriter vpsWriter {bytes};
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

    AppendH265NalHeader(bytes, 33);
    RbspBitWriter spsWriter {bytes};
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

    AppendH265NalHeader(bytes, 34);
    RbspBitWriter ppsWriter {bytes};
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

    return Result::SUCCESS;
}

inline Result WriteH264AnnexBEndOfStream(ByteWriter& bytes) {
    AppendH264NalHeader(bytes, 10);
    bytes.WriteByte(0x80);
    AppendH264NalHeader(bytes, 11);
    bytes.WriteByte(0x80);
    return Result::SUCCESS;
}

inline Result WriteH265AnnexBEndOfStream(ByteWriter& bytes) {
    AppendH265NalHeader(bytes, 36);
    bytes.WriteByte(0x80);
    AppendH265NalHeader(bytes, 37);
    bytes.WriteByte(0x80);
    return Result::SUCCESS;
}

struct ObuBitWriter {
    ByteWriter& bytes;
    uint8_t byte = 0;
    uint8_t bitCount = 0;

    void WriteBit(uint32_t bit) {
        if (bitCount == 0)
            byte = 0;
        if (bit & 1)
            byte |= uint8_t(1u << (7u - bitCount));
        bitCount++;
        if (bitCount == 8) {
            bytes.WriteByte(byte);
            bitCount = 0;
        }
    }

    void WriteBits(uint64_t value, uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            WriteBit((uint32_t)((value >> (count - i - 1u)) & 1u));
    }

    void WriteUvlc(uint32_t value) {
        const uint32_t codeNum = value + 1u;
        uint32_t bitNum = 0;
        for (uint32_t temp = codeNum; temp; temp >>= 1)
            bitNum++;
        for (uint32_t i = 1; i < bitNum; i++)
            WriteBit(0);
        WriteBits(codeNum, bitNum);
    }

    void FinishBits() {
        WriteBit(1);
        while (bitCount != 0)
            WriteBit(0);
    }
};

inline void WriteLeb128(ByteWriter& bytes, uint64_t value) {
    do {
        uint8_t byte = uint8_t(value & 0x7F);
        value >>= 7;
        if (value)
            byte |= 0x80;
        bytes.WriteByte(byte);
    } while (value);
}

inline void AppendAv1ObuHeader(ByteWriter& bytes, video_av1::ObuType type, uint64_t payloadSize) {
    bytes.WriteByte((uint8_t(type) << 3) | 0x2);
    WriteLeb128(bytes, payloadSize);
}

inline uint8_t GetAv1LevelIndex(uint8_t level, uint32_t width, uint32_t height) {
    switch (level) {
        case 20:
            return 0;
        case 21:
            return 1;
        case 30:
            return 4;
        case 31:
            return 5;
        case 40:
            return 8;
        case 41:
            return 9;
        case 50:
            return 12;
        case 51:
            return 13;
        case 52:
            return 14;
        case 53:
            return 15;
        case 60:
            return 16;
        case 61:
            return 17;
        case 62:
            return 18;
        case 63:
            return 19;
        case 70:
            return 20;
        case 71:
            return 21;
        case 72:
            return 22;
        case 73:
            return 23;
        default:
            break;
    }

    const uint64_t samples = uint64_t(width) * height;
    if (samples <= 512ull * 288ull)
        return 0;
    if (samples <= 704ull * 396ull)
        return 1;
    if (samples <= 1088ull * 612ull)
        return 4;
    if (samples <= 1376ull * 774ull)
        return 5;
    if (samples <= 2048ull * 1152ull)
        return 8;
    if (samples <= 4096ull * 2176ull)
        return 12;

    return 13;
}

inline void WriteAv1ColorConfig(ObuBitWriter& writer, const VideoAV1SequenceDesc& desc) {
    const bool highBitdepth = desc.bitDepth > 8;
    const bool twelveBit = desc.bitDepth > 10;
    const bool monochrome = !!(desc.flags & VideoAV1SequenceBits::MONO_CHROME);
    const bool colorDescriptionPresent = !!(desc.flags & VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT);

    writer.WriteBit(highBitdepth);
    if (desc.seqProfile == 2 && highBitdepth)
        writer.WriteBit(twelveBit);
    if (desc.seqProfile != video_av1::PROFILE_HIGH)
        writer.WriteBit(monochrome);

    writer.WriteBit(colorDescriptionPresent);
    if (colorDescriptionPresent) {
        writer.WriteBits(desc.colorPrimaries, 8);
        writer.WriteBits(desc.transferCharacteristics, 8);
        writer.WriteBits(desc.matrixCoefficients, 8);
    }

    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::COLOR_RANGE));
    if (monochrome) {
        writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q));
        return;
    }

    if (desc.seqProfile == 2) {
        if (twelveBit) {
            writer.WriteBit(desc.subsamplingX);
            if (desc.subsamplingX)
                writer.WriteBit(desc.subsamplingY);
        }
    }

    if (desc.subsamplingX && desc.subsamplingY)
        writer.WriteBits(desc.chromaSamplePosition, 2);

    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q));
}

inline Result WriteAv1SequenceHeaderPayload(const VideoAV1SequenceDesc& desc, ByteWriter& bytes) {
    if (desc.seqProfile > 2 || (desc.bitDepth != 8 && desc.bitDepth != 10 && desc.bitDepth != 12) || desc.frameWidthBitsMinus1 > 15 || desc.frameHeightBitsMinus1 > 15)
        return Result::INVALID_ARGUMENT;
    if ((uint32_t)desc.maxFrameWidthMinus1 >= (1u << (desc.frameWidthBitsMinus1 + 1u)) || (uint32_t)desc.maxFrameHeightMinus1 >= (1u << (desc.frameHeightBitsMinus1 + 1u)))
        return Result::INVALID_ARGUMENT;

    const bool reducedStillPictureHeader = !!(desc.flags & VideoAV1SequenceBits::REDUCED_STILL_PICTURE_HEADER);
    const bool timingInfoPresent = !!(desc.flags & VideoAV1SequenceBits::TIMING_INFO_PRESENT);
    const bool initialDisplayDelayPresent = !!(desc.flags & VideoAV1SequenceBits::INITIAL_DISPLAY_DELAY_PRESENT);
    const bool enableOrderHint = !!(desc.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT);
    const bool frameIdNumbersPresent = !!(desc.flags & VideoAV1SequenceBits::FRAME_ID_NUMBERS_PRESENT);
    if (desc.seqForceScreenContentTools > video_av1::SELECT_SCREEN_CONTENT_TOOLS || desc.seqForceIntegerMv > video_av1::SELECT_SCREEN_CONTENT_TOOLS)
        return Result::INVALID_ARGUMENT;

    const bool selectScreenContentTools = desc.seqForceScreenContentTools == video_av1::SELECT_SCREEN_CONTENT_TOOLS;
    const bool selectIntegerMv = desc.seqForceIntegerMv == video_av1::SELECT_SCREEN_CONTENT_TOOLS;
    const uint8_t levelIndex = GetAv1LevelIndex(desc.level, uint32_t(desc.maxFrameWidthMinus1) + 1u, uint32_t(desc.maxFrameHeightMinus1) + 1u);

    ObuBitWriter writer = {bytes};
    writer.WriteBits(desc.seqProfile, 3);
    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::STILL_PICTURE));
    writer.WriteBit(reducedStillPictureHeader);
    if (reducedStillPictureHeader) {
        writer.WriteBits(levelIndex, 5);
    } else {
        writer.WriteBit(timingInfoPresent);
        if (timingInfoPresent) {
            writer.WriteBits(desc.numUnitsInDisplayTick, 32);
            writer.WriteBits(desc.timeScale, 32);
            writer.WriteBit(desc.numTicksPerPictureMinus1 != 0);
            if (desc.numTicksPerPictureMinus1 != 0)
                writer.WriteUvlc(desc.numTicksPerPictureMinus1);
            writer.WriteBit(0); // decoder_model_info_present_flag
        }

        writer.WriteBit(initialDisplayDelayPresent);
        writer.WriteBits(0, 5);  // operating_points_cnt_minus_1
        writer.WriteBits(0, 12); // operating_point_idc[0]
        writer.WriteBits(levelIndex, 5);
        if (levelIndex > 7)
            writer.WriteBit(0); // seq_tier[0]
        if (initialDisplayDelayPresent)
            writer.WriteBit(0); // initial_display_delay_present_for_this_op[0]
    }

    writer.WriteBits(desc.frameWidthBitsMinus1, 4);
    writer.WriteBits(desc.frameHeightBitsMinus1, 4);
    writer.WriteBits(desc.maxFrameWidthMinus1, desc.frameWidthBitsMinus1 + 1u);
    writer.WriteBits(desc.maxFrameHeightMinus1, desc.frameHeightBitsMinus1 + 1u);

    if (!reducedStillPictureHeader) {
        writer.WriteBit(frameIdNumbersPresent);
        if (frameIdNumbersPresent) {
            writer.WriteBits(desc.deltaFrameIdLengthMinus2, 4);
            writer.WriteBits(desc.additionalFrameIdLengthMinus1, 3);
        }
    }

    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::USE_128X128_SUPERBLOCK));
    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_FILTER_INTRA));
    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_INTRA_EDGE_FILTER));
    if (!reducedStillPictureHeader) {
        writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_INTERINTRA_COMPOUND));
        writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_MASKED_COMPOUND));
        writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_WARPED_MOTION));
        writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_DUAL_FILTER));
        writer.WriteBit(enableOrderHint);
        if (enableOrderHint) {
            writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_JNT_COMP));
            writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS));
        }

        writer.WriteBit(selectScreenContentTools);
        if (!selectScreenContentTools)
            writer.WriteBit(desc.seqForceScreenContentTools);
        if (desc.seqForceScreenContentTools != 0) {
            writer.WriteBit(selectIntegerMv);
            if (!selectIntegerMv)
                writer.WriteBit(desc.seqForceIntegerMv);
        }
        if (enableOrderHint)
            writer.WriteBits(desc.orderHintBitsMinus1, 3);
    }

    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_SUPERRES));
    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_CDEF));
    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::ENABLE_RESTORATION));
    WriteAv1ColorConfig(writer, desc);
    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::FILM_GRAIN_PARAMS_PRESENT));
    writer.FinishBits();

    return Result::SUCCESS;
}

inline Result WriteAv1SequenceHeaderObu(const VideoAV1SequenceDesc& desc, ByteWriter& bytes) {
    ByteWriter payloadCounter = {};
    Result result = WriteAv1SequenceHeaderPayload(desc, payloadCounter);
    if (result != Result::SUCCESS)
        return result;

    AppendAv1ObuHeader(bytes, video_av1::ObuType::SequenceHeader, payloadCounter.writtenSize);

    return WriteAv1SequenceHeaderPayload(desc, bytes);
}

inline Result WriteAv1ObuHeaders(const VideoAV1SequenceDesc& desc, ByteWriter& bytes) {
    AppendAv1ObuHeader(bytes, video_av1::ObuType::TemporalDelimiter, 0);
    return WriteAv1SequenceHeaderObu(desc, bytes);
}

} // namespace video_annex_b

inline Result WriteVideoAnnexBParameterSetsShared(VideoAnnexBParameterSetsDesc& desc) {
    video_annex_b::ByteWriter byteCounter = {};
    Result result = Result::UNSUPPORTED;

    if (desc.codec == VideoCodec::H264)
        result = video_annex_b::WriteH264AnnexBParameterSets(desc, byteCounter);
    else if (desc.codec == VideoCodec::H265)
        result = video_annex_b::WriteH265AnnexBParameterSets(desc, byteCounter);

    if (result != Result::SUCCESS)
        return result;

    desc.writtenSize = byteCounter.writtenSize;
    if (!desc.dst)
        return Result::SUCCESS;

    if (desc.dstSize < byteCounter.writtenSize)
        return Result::INVALID_ARGUMENT;

    video_annex_b::ByteWriter byteWriter = {desc.dst, desc.dstSize};
    if (desc.codec == VideoCodec::H264)
        result = video_annex_b::WriteH264AnnexBParameterSets(desc, byteWriter);
    else
        result = video_annex_b::WriteH265AnnexBParameterSets(desc, byteWriter);

    if (result != Result::SUCCESS)
        return result;

    return byteWriter.Finish(desc.writtenSize);
}

inline Result WriteVideoAV1ObuHeadersShared(VideoAV1ObuHeadersDesc& desc) {
    video_annex_b::ByteWriter byteCounter = {};
    Result result = video_annex_b::WriteAv1ObuHeaders(desc.sequence, byteCounter);
    if (result != Result::SUCCESS)
        return result;

    desc.writtenSize = byteCounter.writtenSize;
    if (!desc.dst)
        return Result::SUCCESS;

    if (desc.dstSize < byteCounter.writtenSize)
        return Result::INVALID_ARGUMENT;

    video_annex_b::ByteWriter byteWriter = {desc.dst, desc.dstSize};
    result = video_annex_b::WriteAv1ObuHeaders(desc.sequence, byteWriter);
    if (result != Result::SUCCESS)
        return result;

    return byteWriter.Finish(desc.writtenSize);
}

inline Result WriteVideoAnnexBEndOfStreamShared(VideoAnnexBEndOfStreamDesc& desc) {
    video_annex_b::ByteWriter byteCounter = {};
    Result result = Result::UNSUPPORTED;

    if (desc.codec == VideoCodec::H264)
        result = video_annex_b::WriteH264AnnexBEndOfStream(byteCounter);
    else if (desc.codec == VideoCodec::H265)
        result = video_annex_b::WriteH265AnnexBEndOfStream(byteCounter);

    if (result != Result::SUCCESS)
        return result;

    desc.writtenSize = byteCounter.writtenSize;
    if (!desc.dst)
        return Result::SUCCESS;

    if (desc.dstSize < byteCounter.writtenSize)
        return Result::INVALID_ARGUMENT;

    video_annex_b::ByteWriter byteWriter = {desc.dst, desc.dstSize};
    if (desc.codec == VideoCodec::H264)
        result = video_annex_b::WriteH264AnnexBEndOfStream(byteWriter);
    else
        result = video_annex_b::WriteH265AnnexBEndOfStream(byteWriter);

    if (result != Result::SUCCESS)
        return result;

    return byteWriter.Finish(desc.writtenSize);
}

} // namespace nri
