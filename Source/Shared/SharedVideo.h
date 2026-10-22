// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

namespace video {

constexpr uint32_t ENCODE_RATE_CONTROL_CQP = 1u << (uint32_t)VideoEncodeRateControlMode::CQP;
constexpr uint32_t ENCODE_RATE_CONTROL_CBR = 1u << (uint32_t)VideoEncodeRateControlMode::CBR;
constexpr uint32_t ENCODE_RATE_CONTROL_VBR = 1u << (uint32_t)VideoEncodeRateControlMode::VBR;

static inline uint32_t GetEncodeRateControlModeMask(VideoEncodeRateControlMode mode) {
    return 1u << (uint32_t)mode;
}

static inline uint8_t GetEncodeQPByFrameType(const VideoEncodeRateControlDesc& rateControlDesc, VideoFrameType frameType) {
    return frameType == VideoFrameType::B ? rateControlDesc.qpB : (frameType == VideoFrameType::P ? rateControlDesc.qpP : rateControlDesc.qpI);
}

static inline bool IsFrameTypeSupported(VideoCodec codec, VideoFrameType frameType, bool isBFrameSupported) {
    return frameType != VideoFrameType::B || ((codec == VideoCodec::H264 || codec == VideoCodec::H265) && isBFrameSupported);
}

static inline bool IsEncodePictureUsedAsReference(VideoCodec codec, VideoFrameType frameType, uint32_t maxReferenceNum, bool hasReconstructedPicture, uint8_t av1RefreshFrameFlags) {
    if (!maxReferenceNum || !hasReconstructedPicture)
        return false;

    if ((codec == VideoCodec::H264 || codec == VideoCodec::H265) && frameType == VideoFrameType::B)
        return false;

    return codec != VideoCodec::AV1 || av1RefreshFrameFlags != 0;
}

template <typename T>
static inline const T* FindParameterSet(const T* parameterSets, uint32_t parameterSetNum, uint8_t T::* idMember, uint8_t id) {
    if (!parameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameterSetNum; i++) {
        if (parameterSets[i].*idMember == id)
            return &parameterSets[i];
    }

    return nullptr;
}

template <typename T>
static inline uint32_t FindReferenceIndex(const T* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return UINT32_MAX;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return i;
    }

    return UINT32_MAX;
}

template <typename T>
static inline const T* FindReferenceDesc(const T* references, uint32_t referenceNum, uint32_t slot) {
    const uint32_t index = FindReferenceIndex(references, referenceNum, slot);
    return index != UINT32_MAX ? &references[index] : nullptr;
}

template <typename T>
static inline auto FindReferenceDesc(const T* pictureDesc, uint32_t slot) -> decltype(pictureDesc->references) {
    return pictureDesc ? FindReferenceDesc(pictureDesc->references, pictureDesc->referenceNum, slot) : nullptr;
}

static inline bool HasReferenceSlot(const VideoReference* references, uint32_t referenceNum, uint32_t slot) {
    return FindReferenceIndex(references, referenceNum, slot) != UINT32_MAX;
}

static inline uint32_t GetDecodeSetupSlot(const VideoDecodeDesc& desc) {
    const VideoH264DecodePictureDesc* h264PictureDesc = desc.h264PictureDesc;

    if (h264PictureDesc && h264PictureDesc->hasReferenceSlot)
        return h264PictureDesc->referenceSlot;

    return desc.dstSlot;
}

namespace bitstream {

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

    void WriteUe(uint64_t value) {
        const uint64_t codeNum = value + 1u;
        uint32_t bitNum = 0;

        for (uint64_t temp = codeNum; temp; temp >>= 1)
            bitNum++;

        for (uint32_t i = 1; i < bitNum; i++)
            WriteBit(0);

        WriteBits(codeNum, bitNum);
    }

    void WriteSe(int32_t value) {
        const uint64_t codeNum = value <= 0 ? uint64_t(-int64_t(value) * 2) : uint64_t(int64_t(value) * 2 - 1);
        WriteUe(codeNum);
    }

    void FinishRbsp() {
        WriteBit(1);
        while (bitCount != 0)
            WriteBit(0);
    }
};

} // namespace bitstream

namespace h264 {

static inline void AppendNalHeader(bitstream::ByteWriter& bytes, uint8_t nalHeader) {
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(1);
    bytes.WriteByte(nalHeader);
}

// The cropped frame must not be empty: "CropUnitX * (left + right) < PicWidthInSamplesL", "CropUnitY * (top + bottom) < 16 * FrameHeightInMbs"
static inline bool IsFrameCroppingValid(const VideoH264SequenceParameterSetDesc& sps) {
    if (!(sps.flags & VideoH264SequenceParameterSetBits::FRAME_CROPPING))
        return true;

    const bool frameMbsOnly = !!(sps.flags & VideoH264SequenceParameterSetBits::FRAME_MBS_ONLY);
    const bool hasChroma = sps.chromaFormatIdc != 0 && !(sps.flags & VideoH264SequenceParameterSetBits::SEPARATE_COLOUR_PLANE);
    const uint32_t cropUnitX = (hasChroma && sps.chromaFormatIdc != 3) ? 2 : 1;
    const uint32_t cropUnitY = ((hasChroma && sps.chromaFormatIdc == 1) ? 2 : 1) * (frameMbsOnly ? 1 : 2);
    const uint32_t frameWidth = (sps.pictureWidthInMbsMinus1 + 1u) * 16;
    const uint32_t frameHeight = (sps.pictureHeightInMapUnitsMinus1 + 1u) * 16 * (frameMbsOnly ? 1 : 2);

    return (cropUnitX * (sps.frameCropLeftOffset + sps.frameCropRightOffset)) < frameWidth && (cropUnitY * (sps.frameCropTopOffset + sps.frameCropBottomOffset)) < frameHeight;
}

static inline Result WriteAnnexBParameterSets(const VideoAnnexBParameterSetsDesc& desc, bitstream::ByteWriter& bytes) {
    const bool hasParameterSets = desc.h264Sps && desc.h264Pps;

    if (!hasParameterSets)
        return Result::INVALID_ARGUMENT;

    const VideoH264SequenceParameterSetDesc& sps = *desc.h264Sps;
    const VideoH264PictureParameterSetDesc& pps = *desc.h264Pps;
    const bool highProfileSps = sps.profileIdc == 100 || sps.profileIdc == 110 || sps.profileIdc == 122 || sps.profileIdc == 244 || sps.profileIdc == 44 || sps.profileIdc == 83 || sps.profileIdc == 86 || sps.profileIdc == 118 || sps.profileIdc == 128 || sps.profileIdc == 138 || sps.profileIdc == 139 || sps.profileIdc == 134 || sps.profileIdc == 135;

    if (!highProfileSps || sps.chromaFormatIdc > 3 || sps.pictureOrderCountType > 2 || (pps.flags & VideoH264PictureParameterSetBits::TRANSFORM_8X8_MODE) != 0)
        return Result::UNSUPPORTED;

    if (!IsFrameCroppingValid(sps))
        return Result::INVALID_ARGUMENT;

    AppendNalHeader(bytes, 0x67);
    bitstream::RbspBitWriter spsWriter{bytes};
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

    if (sps.chromaFormatIdc == 3)
        spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::SEPARATE_COLOUR_PLANE));

    spsWriter.WriteUe(sps.bitDepthLumaMinus8);
    spsWriter.WriteUe(sps.bitDepthChromaMinus8);
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::QPPRIME_Y_ZERO_TRANSFORM_BYPASS));
    spsWriter.WriteBit(0);
    spsWriter.WriteUe(sps.log2MaxFrameNumMinus4);
    spsWriter.WriteUe(sps.pictureOrderCountType);

    if (sps.pictureOrderCountType == 0)
        spsWriter.WriteUe(sps.log2MaxPictureOrderCountLsbMinus4);
    else if (sps.pictureOrderCountType == 1) {
        spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::DELTA_PIC_ORDER_ALWAYS_ZERO));
        spsWriter.WriteSe(sps.offsetForNonReferencePicture);
        spsWriter.WriteSe(sps.offsetForTopToBottomField);
        spsWriter.WriteUe(0); // offset_for_ref_frame is not represented by the public descriptor
    }

    spsWriter.WriteUe(sps.referenceFrameNum);
    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::GAPS_IN_FRAME_NUM_ALLOWED));
    spsWriter.WriteUe(sps.pictureWidthInMbsMinus1);
    spsWriter.WriteUe(sps.pictureHeightInMapUnitsMinus1);
    const bool frameMbsOnly = !!(sps.flags & VideoH264SequenceParameterSetBits::FRAME_MBS_ONLY);
    spsWriter.WriteBit(frameMbsOnly);

    if (!frameMbsOnly)
        spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::MB_ADAPTIVE_FRAME_FIELD));

    spsWriter.WriteBit(!!(sps.flags & VideoH264SequenceParameterSetBits::DIRECT_8X8_INFERENCE));
    const bool frameCropping = !!(sps.flags & VideoH264SequenceParameterSetBits::FRAME_CROPPING);
    spsWriter.WriteBit(frameCropping);

    if (frameCropping) {
        spsWriter.WriteUe(sps.frameCropLeftOffset);
        spsWriter.WriteUe(sps.frameCropRightOffset);
        spsWriter.WriteUe(sps.frameCropTopOffset);
        spsWriter.WriteUe(sps.frameCropBottomOffset);
    }

    spsWriter.WriteBit(0); // vui_parameters_present_flag
    spsWriter.FinishRbsp();

    AppendNalHeader(bytes, 0x68);
    bitstream::RbspBitWriter ppsWriter{bytes};
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

    if (pps.secondChromaQpIndexOffset != pps.chromaQpIndexOffset) {
        ppsWriter.WriteBit(0); // transform_8x8_mode_flag
        ppsWriter.WriteBit(0); // pic_scaling_matrix_present_flag
        ppsWriter.WriteSe(pps.secondChromaQpIndexOffset);
    }

    ppsWriter.FinishRbsp();

    return Result::SUCCESS;
}

static inline Result WriteAnnexBEndOfStream(bitstream::ByteWriter& bytes) {
    AppendNalHeader(bytes, 10);
    bytes.WriteByte(0x80);
    AppendNalHeader(bytes, 11);
    bytes.WriteByte(0x80);
    return Result::SUCCESS;
}

} // namespace h264

namespace h265 {

constexpr uint32_t MAX_REFERENCE_NUM = 15;

static inline const VideoH265ReferenceDesc* GetReferenceDesc(const VideoReference* references, const VideoH265ReferenceDesc* referenceDescs,
    uint32_t referenceNum, uint32_t referenceIndex) {
    if (!references || !referenceDescs || referenceIndex >= referenceNum)
        return nullptr;

    if (referenceDescs[referenceIndex].slot == references[referenceIndex].slot)
        return &referenceDescs[referenceIndex];

    return video::FindReferenceDesc(referenceDescs, referenceNum, references[referenceIndex].slot);
}

struct EncodeReferenceLists {
    std::array<uint32_t, MAX_REFERENCE_NUM> list0 = {};
    std::array<uint32_t, MAX_REFERENCE_NUM> list1 = {};
    uint32_t list0Num = 0;
    uint32_t list1Num = 0;
    uint32_t failingReference = 0;
    bool missingDescriptor = false;
    bool invalidPictureOrderCount = false;
};

static inline bool BuildEncodeReferenceLists(const VideoReference* references, const VideoH265ReferenceDesc* referenceDescs, uint32_t referenceNum,
    VideoFrameType frameType, int32_t currentPictureOrderCount, bool list1MustBeFuture, EncodeReferenceLists& lists) {
    lists = {};

    if (referenceNum > MAX_REFERENCE_NUM) {
        lists.failingReference = MAX_REFERENCE_NUM;
        return false;
    }

    if (referenceNum && !referenceDescs) {
        lists.missingDescriptor = true;
        return false;
    }

    for (uint32_t i = 0; i < referenceNum; i++) {
        const VideoH265ReferenceDesc* referenceDesc = GetReferenceDesc(references, referenceDescs, referenceNum, i);
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
            if (frameType != VideoFrameType::B || referenceDesc->pictureOrderCount == currentPictureOrderCount || (list1MustBeFuture && referenceDesc->pictureOrderCount < currentPictureOrderCount)) {
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

struct EncodeInitialLists {
    std::array<uint32_t, MAX_REFERENCE_NUM> temp0 = {}; // "RefPicListTemp0" as reference indices
    std::array<uint32_t, MAX_REFERENCE_NUM> temp1 = {}; // "RefPicListTemp1" as reference indices
    std::array<uint32_t, MAX_REFERENCE_NUM> list0Entries = {}; // "list_entry_l0" of the List0 entries
    std::array<uint32_t, MAX_REFERENCE_NUM> list1Entries = {}; // "list_entry_l1" of the List1 entries
};

// H.265 8.3.4: "RefPicListTemp0" holds the preceding pictures by descending, the following pictures by ascending picture order count, then the
// long-term pictures ("RefPicListTemp1" the following pictures first). All references are used by the current picture. "list_entry" values index them
static inline bool BuildEncodeInitialLists(const VideoReference* references, const VideoH265ReferenceDesc* referenceDescs, uint32_t referenceNum, int32_t currentPictureOrderCount,
    const EncodeReferenceLists& lists, EncodeInitialLists& initialLists) {
    initialLists = {};

    std::array<uint32_t, MAX_REFERENCE_NUM> before = {};
    std::array<uint32_t, MAX_REFERENCE_NUM> after = {};
    std::array<uint32_t, MAX_REFERENCE_NUM> longTerm = {};
    uint32_t beforeNum = 0;
    uint32_t afterNum = 0;
    uint32_t longTermNum = 0;

    for (uint32_t i = 0; i < referenceNum; i++) {
        const VideoH265ReferenceDesc* referenceDesc = GetReferenceDesc(references, referenceDescs, referenceNum, i);
        if (!referenceDesc)
            return false;

        if (referenceDesc->longTerm)
            longTerm[longTermNum++] = i;
        else if (referenceDesc->pictureOrderCount < currentPictureOrderCount)
            before[beforeNum++] = i;
        else
            after[afterNum++] = i;
    }

    auto getPictureOrderCount = [&](uint32_t referenceIndex) {
        return GetReferenceDesc(references, referenceDescs, referenceNum, referenceIndex)->pictureOrderCount;
    };

    std::sort(before.begin(), before.begin() + beforeNum, [&](uint32_t a, uint32_t b) { return getPictureOrderCount(a) > getPictureOrderCount(b); });
    std::sort(after.begin(), after.begin() + afterNum, [&](uint32_t a, uint32_t b) { return getPictureOrderCount(a) < getPictureOrderCount(b); });

    uint32_t tempNum = 0;
    for (uint32_t i = 0; i < beforeNum; i++)
        initialLists.temp0[tempNum++] = before[i];
    for (uint32_t i = 0; i < afterNum; i++)
        initialLists.temp0[tempNum++] = after[i];
    for (uint32_t i = 0; i < longTermNum; i++)
        initialLists.temp0[tempNum++] = longTerm[i];

    tempNum = 0;
    for (uint32_t i = 0; i < afterNum; i++)
        initialLists.temp1[tempNum++] = after[i];
    for (uint32_t i = 0; i < beforeNum; i++)
        initialLists.temp1[tempNum++] = before[i];
    for (uint32_t i = 0; i < longTermNum; i++)
        initialLists.temp1[tempNum++] = longTerm[i];

    auto findEntry = [&](const std::array<uint32_t, MAX_REFERENCE_NUM>& temp, uint32_t referenceIndex) {
        for (uint32_t i = 0; i < tempNum; i++) {
            if (temp[i] == referenceIndex)
                return i;
        }

        return UINT32_MAX;
    };

    for (uint32_t i = 0; i < lists.list0Num; i++) {
        initialLists.list0Entries[i] = findEntry(initialLists.temp0, lists.list0[i]);
        if (initialLists.list0Entries[i] == UINT32_MAX)
            return false;
    }

    for (uint32_t i = 0; i < lists.list1Num; i++) {
        initialLists.list1Entries[i] = findEntry(initialLists.temp1, lists.list1[i]);
        if (initialLists.list1Entries[i] == UINT32_MAX)
            return false;
    }

    return true;
}

// Without "lists_modification_present_flag" the lists are the initial lists
static inline bool AreEncodeListsInitial(const EncodeReferenceLists& lists, const EncodeInitialLists& initialLists) {
    for (uint32_t i = 0; i < lists.list0Num; i++) {
        if (initialLists.list0Entries[i] != i)
            return false;
    }

    for (uint32_t i = 0; i < lists.list1Num; i++) {
        if (initialLists.list1Entries[i] != i)
            return false;
    }

    return true;
}

static inline void AppendNalHeader(bitstream::ByteWriter& bytes, uint8_t nalUnitType) {
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(0);
    bytes.WriteByte(1);
    bytes.WriteByte(uint8_t(nalUnitType << 1));
    bytes.WriteByte(1);
}

static inline void WriteProfileTierLevel(bitstream::RbspBitWriter& writer, const VideoH265ProfileTierLevelDesc& desc, uint8_t maxSubLayersMinus1) {
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

static inline void WriteSubLayerOrdering(bitstream::RbspBitWriter& writer, const VideoH265DecPicBufMgrDesc& desc, uint8_t maxSubLayersMinus1, bool allSubLayers) {
    const uint32_t firstLayer = allSubLayers ? 0 : maxSubLayersMinus1;
    for (uint32_t i = firstLayer; i <= maxSubLayersMinus1; i++) {
        writer.WriteUe(desc.maxDecPicBufferingMinus1[i]);
        writer.WriteUe(desc.maxNumReorderPics[i]);
        writer.WriteUe(desc.maxLatencyIncreasePlus1[i]);
    }
}

static inline Result WriteAnnexBParameterSets(const VideoAnnexBParameterSetsDesc& desc, bitstream::ByteWriter& bytes) {
    const bool hasParameterSets = desc.h265Vps && desc.h265Sps && desc.h265Pps;

    if (!hasParameterSets)
        return Result::INVALID_ARGUMENT;

    const VideoH265VideoParameterSetDesc& vps = *desc.h265Vps;
    const VideoH265SequenceParameterSetDesc& sps = *desc.h265Sps;
    const VideoH265PictureParameterSetDesc& pps = *desc.h265Pps;

    if (vps.maxSubLayersMinus1 > 6 || sps.maxSubLayersMinus1 > 6)
        return Result::INVALID_ARGUMENT;

    if (sps.numShortTermRefPicSets || (sps.flags & (VideoH265SequenceParameterSetBits::LONG_TERM_REF_PICS_PRESENT | VideoH265SequenceParameterSetBits::VUI_PARAMETERS_PRESENT | VideoH265SequenceParameterSetBits::SCALING_LIST_DATA_PRESENT)) || (pps.flags & (VideoH265PictureParameterSetBits::TILES_ENABLED | VideoH265PictureParameterSetBits::SCALING_LIST_DATA_PRESENT)))
        return Result::UNSUPPORTED;

    const uint8_t vpsMaxSubLayersMinus1 = std::min<uint8_t>(vps.maxSubLayersMinus1, 6);
    const uint8_t spsMaxSubLayersMinus1 = std::min<uint8_t>(sps.maxSubLayersMinus1, 6);

    AppendNalHeader(bytes, 32);
    bitstream::RbspBitWriter vpsWriter{bytes};
    vpsWriter.WriteBits(vps.videoParameterSetId, 4);
    vpsWriter.WriteBit(1);
    vpsWriter.WriteBit(1);
    vpsWriter.WriteBits(0, 6);
    vpsWriter.WriteBits(vpsMaxSubLayersMinus1, 3);
    vpsWriter.WriteBit(!!(vps.flags & VideoH265VideoParameterSetBits::TEMPORAL_ID_NESTING));
    vpsWriter.WriteBits(0xFFFF, 16);
    WriteProfileTierLevel(vpsWriter, vps.profileTierLevel, vpsMaxSubLayersMinus1);
    const bool vpsAllSubLayers = !!(vps.flags & VideoH265VideoParameterSetBits::SUB_LAYER_ORDERING_INFO_PRESENT);
    vpsWriter.WriteBit(vpsAllSubLayers);
    WriteSubLayerOrdering(vpsWriter, vps.decPicBufMgr, vpsMaxSubLayersMinus1, vpsAllSubLayers);
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

    AppendNalHeader(bytes, 33);
    bitstream::RbspBitWriter spsWriter{bytes};
    spsWriter.WriteBits(sps.videoParameterSetId, 4);
    spsWriter.WriteBits(spsMaxSubLayersMinus1, 3);
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::TEMPORAL_ID_NESTING));
    WriteProfileTierLevel(spsWriter, sps.profileTierLevel, spsMaxSubLayersMinus1);
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
    WriteSubLayerOrdering(spsWriter, sps.decPicBufMgr, spsMaxSubLayersMinus1, spsAllSubLayers);
    spsWriter.WriteUe(sps.log2MinLumaCodingBlockSizeMinus3);
    spsWriter.WriteUe(sps.log2DiffMaxMinLumaCodingBlockSize);
    spsWriter.WriteUe(sps.log2MinLumaTransformBlockSizeMinus2);
    spsWriter.WriteUe(sps.log2DiffMaxMinLumaTransformBlockSize);
    spsWriter.WriteUe(sps.maxTransformHierarchyDepthInter);
    spsWriter.WriteUe(sps.maxTransformHierarchyDepthIntra);
    const bool scalingListEnabled = !!(sps.flags & VideoH265SequenceParameterSetBits::SCALING_LIST_ENABLED);
    spsWriter.WriteBit(scalingListEnabled);

    if (scalingListEnabled)
        spsWriter.WriteBit(0); // sps_scaling_list_data_present_flag

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
    spsWriter.WriteUe(0);
    const bool longTermRefsPresent = !!(sps.flags & VideoH265SequenceParameterSetBits::LONG_TERM_REF_PICS_PRESENT);
    spsWriter.WriteBit(longTermRefsPresent);
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::TEMPORAL_MVP_ENABLED));
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED));
    spsWriter.WriteBit(!!(sps.flags & VideoH265SequenceParameterSetBits::VUI_PARAMETERS_PRESENT));
    spsWriter.WriteBit(0);
    spsWriter.FinishRbsp();

    AppendNalHeader(bytes, 34);
    bitstream::RbspBitWriter ppsWriter{bytes};
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
    ppsWriter.WriteBit(0);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::LISTS_MODIFICATION_PRESENT));
    ppsWriter.WriteUe(pps.log2ParallelMergeLevelMinus2);
    ppsWriter.WriteBit(!!(pps.flags & VideoH265PictureParameterSetBits::SLICE_SEGMENT_HEADER_EXTENSION_PRESENT));
    ppsWriter.WriteBit(0);
    ppsWriter.FinishRbsp();

    return Result::SUCCESS;
}

static inline Result WriteAnnexBEndOfStream(bitstream::ByteWriter& bytes) {
    AppendNalHeader(bytes, 36);
    bytes.WriteByte(0x80);
    AppendNalHeader(bytes, 37);
    bytes.WriteByte(0x80);
    return Result::SUCCESS;
}

} // namespace h265

namespace av1 {

constexpr uint32_t REFERENCE_NAME_NUM = 7;
constexpr uint32_t REFERENCE_FRAME_NUM = 8;
constexpr uint32_t DECODE_REFERENCE_DESC_MAX_NUM = 7 + REFERENCE_FRAME_NUM; // named entries + "NONE" entries of the reference frame map

static constexpr VideoAV1PictureBits GetDefaultPictureFlags() {
    return VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::DISABLE_CDF_UPDATE | VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS | VideoAV1PictureBits::FORCE_INTEGER_MV | VideoAV1PictureBits::SHOW_FRAME | VideoAV1PictureBits::SHOWABLE_FRAME;
}

static inline uint8_t GetReferenceNameIndex(VideoAV1ReferenceName name) {
    switch (name) {
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
        default:
            return REFERENCE_NAME_NUM;
    }
}

static inline uint8_t GetLevelIndex(uint8_t level, uint32_t width, uint32_t height) {
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

constexpr uint32_t SELECT_SCREEN_CONTENT_TOOLS = 2;
constexpr uint32_t PROFILE_HIGH = 1;
constexpr uint8_t INTERPOLATION_FILTER_EIGHTTAP = 0;
constexpr uint8_t INTERPOLATION_FILTER_SWITCHABLE = 4;
constexpr uint8_t TX_MODE_ONLY_4X4 = 0;
constexpr uint8_t TX_MODE_LARGEST = 1;
constexpr uint8_t TX_MODE_SELECT = 2;

enum class ObuType : uint8_t {
    SequenceHeader = 1,
    TemporalDelimiter = 2,
    FrameHeader = 3,
    TileGroup = 4,
    Frame = 6,
    Padding = 15,
};

struct ObuSpan {
    ObuType type = ObuType::Padding;
    size_t payloadOffset = 0;
    size_t payloadSize = 0;
};

struct FramePayloadSpan {
    size_t headerPayloadOffset = 0;
    size_t headerPayloadSize = 0;
    size_t tilePayloadOffset = 0;
    size_t tilePayloadSize = 0;
    bool combinedFrameObu = false;
};

struct BitReader {
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t bitOffset = 0;

    bool ReadBits(uint32_t bitCount, uint32_t& value) {
        if (bitCount > 32 || bitOffset + bitCount > size * 8)
            return false;

        value = 0;
        for (uint32_t i = 0; i < bitCount; i++) {
            const size_t absoluteBit = bitOffset++;
            value = (value << 1) | ((data[absoluteBit / 8] >> (7 - (absoluteBit % 8))) & 1u);
        }
        return true;
    }

    bool ReadFlag(uint8_t& value) {
        uint32_t bit = 0;
        if (!ReadBits(1, bit))
            return false;
        value = (uint8_t)bit;
        return true;
    }

    bool ReadSigned(uint32_t bitCount, int8_t& value) {
        uint32_t bits = 0;
        if (!ReadBits(bitCount, bits))
            return false;

        const uint32_t signBit = 1u << (bitCount - 1u);
        const int32_t signedValue = (bits & signBit) ? (int32_t)bits - (int32_t)(1u << bitCount) : (int32_t)bits;
        value = (int8_t)signedValue;
        return true;
    }

    bool ReadIncrement(uint32_t rangeMin, uint32_t rangeMax, uint32_t& value) {
        value = rangeMin;
        while (value < rangeMax) {
            uint32_t bit = 0;
            if (!ReadBits(1, bit))
                return false;
            if (!bit)
                break;
            ++value;
        }
        return true;
    }

    bool ByteAlign() {
        while (bitOffset % 8) {
            uint32_t bit = 0;

            if (!ReadBits(1, bit) || bit)
                return false;
        }
        return true;
    }

    size_t ByteOffset() const {
        return (bitOffset + 7) / 8;
    }
};

static inline bool ReadLeb128(const uint8_t* data, size_t size, size_t& cursor, size_t& value) {
    value = 0;
    for (uint32_t i = 0; i < 8; i++) {
        if (cursor >= size)
            return false;

        const uint8_t byte = data[cursor++];
        value |= (size_t)(byte & 0x7Fu) << (i * 7u);
        if ((byte & 0x80u) == 0)
            return true;
    }

    return false;
}

static inline bool ReadObuHeader(const uint8_t* data, size_t size, size_t& cursor, ObuSpan& span) {
    if (!data || cursor >= size)
        return false;

    const uint8_t header = data[cursor++];
    if (header & 0x81u)
        return false;

    span.type = (ObuType)((header >> 3u) & 0x0Fu);
    const bool hasExtension = (header & 0x04u) != 0;
    const bool hasSizeField = (header & 0x02u) != 0;
    if (hasExtension) {
        if (cursor >= size)
            return false;

        const uint8_t extension = data[cursor++];
        if (extension & 0x07u)
            return false;
    }

    size_t payloadSize = 0;
    if (hasSizeField) {
        if (!ReadLeb128(data, size, cursor, payloadSize))
            return false;
    } else
        payloadSize = size - cursor;

    if (cursor >= size)
        return false;

    span.payloadOffset = cursor;
    span.payloadSize = payloadSize;
    cursor += std::min(payloadSize, size - cursor);
    return true;
}

static inline bool FindFramePayload(const uint8_t* data, size_t size, FramePayloadSpan& frame) {
    size_t cursor = 0;
    ObuSpan frameHeader = {};
    bool hasFrameHeader = false;
    while (cursor < size) {
        ObuSpan span = {};
        if (!ReadObuHeader(data, size, cursor, span))
            return false;

        if (span.type == ObuType::Frame) {
            frame.headerPayloadOffset = span.payloadOffset;
            frame.headerPayloadSize = span.payloadSize;
            frame.tilePayloadOffset = span.payloadOffset;
            frame.tilePayloadSize = span.payloadSize;
            frame.combinedFrameObu = true;
            return true;
        }

        if (span.type == ObuType::FrameHeader) {
            frameHeader = span;
            hasFrameHeader = true;
            continue;
        }

        if (span.type == ObuType::TileGroup && hasFrameHeader) {
            frame.headerPayloadOffset = frameHeader.payloadOffset;
            frame.headerPayloadSize = frameHeader.payloadSize;
            frame.tilePayloadOffset = span.payloadOffset;
            frame.tilePayloadSize = span.payloadSize;
            frame.combinedFrameObu = false;
            return true;
        }

        if (span.type != ObuType::TemporalDelimiter && span.type != ObuType::SequenceHeader && span.type != ObuType::Padding)
            return false;
    }

    return false;
}

static inline VideoAV1ReferenceName GetReferenceNameFromReferenceIndex(uint32_t referenceIndex) {
    switch (referenceIndex) {
        case 0:
            return VideoAV1ReferenceName::LAST;
        case 1:
            return VideoAV1ReferenceName::LAST2;
        case 2:
            return VideoAV1ReferenceName::LAST3;
        case 3:
            return VideoAV1ReferenceName::GOLDEN;
        case 4:
            return VideoAV1ReferenceName::BWDREF;
        case 5:
            return VideoAV1ReferenceName::ALTREF2;
        case 6:
            return VideoAV1ReferenceName::ALTREF;
        case 7:
        default:
            return VideoAV1ReferenceName::NONE;
    }
}

static inline const VideoAV1ReferenceDesc* FindReferenceByRefFrameIndex(const VideoAV1ReferenceDesc* references, uint32_t referenceNum, uint32_t refFrameIndex) {
    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].refFrameIndex == refFrameIndex)
            return references + i;
    }

    return nullptr;
}

static inline bool BuildInterFrameReferences(const VideoAV1EncodeDecodeInfoDesc& desc, const std::array<uint8_t, 7>& refFrameIndices, VideoAV1EncodeDecodeInfo& info) {
    if (!desc.references || !desc.referenceNum || desc.referenceNum > 8)
        return false;

    uint32_t referenceNum = 0;
    for (uint32_t i = 0; i < 7; i++) {
        const VideoAV1ReferenceDesc* reference = FindReferenceByRefFrameIndex(desc.references, desc.referenceNum, refFrameIndices[i]);
        if (!reference)
            return false;

        info.references[referenceNum] = *reference;
        info.references[referenceNum].name = GetReferenceNameFromReferenceIndex(i);
        referenceNum++;
    }

    // The rest of the reference frame map (D3D12 decode needs the complete map)
    for (uint32_t i = 0; i < desc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = desc.references[i];
        if (reference.refFrameIndex >= REFERENCE_FRAME_NUM)
            return false;

        if (FindReferenceByRefFrameIndex(info.references, referenceNum, reference.refFrameIndex))
            continue;

        info.references[referenceNum] = reference;
        info.references[referenceNum].name = VideoAV1ReferenceName::NONE;
        referenceNum++;
    }

    info.picture.references = info.references;
    info.picture.referenceNum = referenceNum;
    return true;
}

static inline uint32_t TileLog2(uint32_t blockSize, uint32_t target) {
    uint32_t value = 0;
    while ((blockSize << value) < target)
        ++value;
    return value;
}

static inline bool ReadNs(BitReader& reader, uint32_t n, uint32_t& value) {
    uint32_t w = 0;
    uint32_t shifted = n;
    while (shifted) {
        ++w;
        shifted >>= 1;
    }

    const uint32_t m = (1u << w) - n;
    uint32_t v = 0;
    if (w > 1 && !reader.ReadBits(w - 1, v))
        return false;
    if (v < m) {
        value = v;
        return true;
    }

    uint32_t extraBit = 0;
    if (!reader.ReadBits(1, extraBit))
        return false;
    value = (v << 1) - m + extraBit;
    return true;
}

static inline bool ReadDeltaQ(BitReader& reader, int8_t& value) {
    uint8_t deltaCoded = 0;
    if (!reader.ReadFlag(deltaCoded))
        return false;
    if (!deltaCoded) {
        value = 0;
        return true;
    }
    return reader.ReadSigned(7, value);
}

// The saved state of a reference frame slot, if provided and fully determined
static inline const VideoAV1SavedFrameState* GetSavedFrameState(const VideoAV1EncodeDecodeInfoDesc& desc, uint32_t refFrameIndex) {
    if (!desc.savedFrameStates || refFrameIndex >= 8 || !desc.savedFrameStates[refFrameIndex].valid)
        return nullptr;

    return desc.savedFrameStates + refFrameIndex;
}

// "inverse_recenter"
static inline int32_t InverseRecenter(int32_t r, int32_t v) {
    if (v > 2 * r)
        return v;

    return (v & 1) ? r - ((v + 1) >> 1) : r + (v >> 1);
}

// "decode_subexp"
static inline bool ReadSubexp(BitReader& reader, uint32_t symbolNum, uint32_t& value) {
    uint32_t i = 0;
    uint32_t mk = 0;
    constexpr uint32_t k = 3;

    while (true) {
        const uint32_t b2 = i ? k + i - 1 : k;
        const uint32_t a = 1u << b2;

        if (symbolNum <= mk + 3 * a) {
            uint32_t finalBits = 0;

            if (!ReadNs(reader, symbolNum - mk, finalBits))
                return false;

            value = finalBits + mk;
            return true;
        }

        uint8_t moreBits = 0;
        if (!reader.ReadFlag(moreBits))
            return false;

        if (!moreBits) {
            uint32_t bits = 0;

            if (!reader.ReadBits(b2, bits))
                return false;

            value = bits + mk;
            return true;
        }

        i++;
        mk += a;
    }
}

// "setup_shear" ("warpValid"), with "resolve_divisor"
static inline bool IsGlobalMotionShearValid(const int32_t* params) {
    constexpr int32_t WARPEDMODEL_PREC_BITS = 16;
    constexpr int32_t WARP_PARAM_REDUCE_BITS = 6;
    constexpr int32_t DIV_LUT_BITS = 8;
    constexpr int32_t DIV_LUT_PREC_BITS = 14;

    auto clip16 = [](int64_t x) { return x < -32768 ? -32768 : (x > 32767 ? 32767 : x); };
    auto round2Signed = [](int64_t x, int32_t n) { return x >= 0 ? (x + (int64_t(1) << n >> 1)) >> n : -((-x + (int64_t(1) << n >> 1)) >> n); };
    auto abs64 = [](int64_t x) { return x < 0 ? -x : x; };

    // "resolve_divisor(gm_params[2])": "Div_Lut[f]" = Round(2^22 / (256 + f))
    const int64_t d = params[2];
    const int64_t absD = abs64(d);
    if (absD == 0)
        return false;

    int32_t n = 0;
    while ((absD >> (n + 1)) != 0)
        n++;

    const int64_t e = absD - (int64_t(1) << n);
    const int64_t f = n > DIV_LUT_BITS ? round2Signed(e, n - DIV_LUT_BITS) : e << (DIV_LUT_BITS - n);
    const int64_t divLut = ((int64_t(1) << 23) / ((int64_t(1) << DIV_LUT_BITS) + f) + 1) >> 1;
    const int32_t divShift = n + DIV_LUT_PREC_BITS;
    const int64_t divFactor = d < 0 ? -divLut : divLut;

    const int64_t alpha0 = clip16(int64_t(params[2]) - (int64_t(1) << WARPEDMODEL_PREC_BITS));
    const int64_t beta0 = clip16(params[3]);
    const int64_t gamma0 = clip16(round2Signed(int64_t(params[4]) * (int64_t(1) << WARPEDMODEL_PREC_BITS) * divFactor, divShift));
    const int64_t delta0 = clip16(int64_t(params[5]) - round2Signed(int64_t(params[3]) * params[4] * divFactor, divShift) - (int64_t(1) << WARPEDMODEL_PREC_BITS));

    const int64_t alpha = round2Signed(alpha0, WARP_PARAM_REDUCE_BITS) * (1 << WARP_PARAM_REDUCE_BITS);
    const int64_t beta = round2Signed(beta0, WARP_PARAM_REDUCE_BITS) * (1 << WARP_PARAM_REDUCE_BITS);
    const int64_t gamma = round2Signed(gamma0, WARP_PARAM_REDUCE_BITS) * (1 << WARP_PARAM_REDUCE_BITS);
    const int64_t delta = round2Signed(delta0, WARP_PARAM_REDUCE_BITS) * (1 << WARP_PARAM_REDUCE_BITS);

    return 4 * abs64(alpha) + 7 * abs64(beta) < (int64_t(1) << WARPEDMODEL_PREC_BITS) && 4 * abs64(gamma) + 4 * abs64(delta) < (int64_t(1) << WARPEDMODEL_PREC_BITS);
}

// "read_global_param" relative to "PrevGmParams"
static inline bool ReadGlobalParam(BitReader& reader, uint32_t type, uint32_t index, bool allowHighPrecisionMv, int32_t previous, int32_t& value) {
    constexpr uint32_t GM_TYPE_TRANSLATION = 1;
    constexpr uint32_t WARPEDMODEL_PREC_BITS = 16;

    uint32_t absBits = 12; // GM_ABS_ALPHA_BITS
    uint32_t precBits = 15; // GM_ALPHA_PREC_BITS

    if (index < 2) {
        if (type == GM_TYPE_TRANSLATION) {
            absBits = 9 - !allowHighPrecisionMv;  // GM_ABS_TRANS_ONLY_BITS
            precBits = 3 - !allowHighPrecisionMv; // GM_TRANS_ONLY_PREC_BITS
        } else {
            absBits = 12; // GM_ABS_TRANS_BITS
            precBits = 6; // GM_TRANS_PREC_BITS
        }
    }

    const uint32_t precDiff = WARPEDMODEL_PREC_BITS - precBits;
    const int32_t round = (index % 3) == 2 ? (1 << WARPEDMODEL_PREC_BITS) : 0;
    const int32_t sub = (index % 3) == 2 ? (1 << precBits) : 0;
    const int32_t mx = 1 << absBits;
    const int32_t r = (previous >> precDiff) - sub;

    // "decode_signed_subexp_with_ref(-mx, mx + 1, r)" via "decode_unsigned_subexp_with_ref(2 * mx + 1, r + mx)"
    const int32_t symbolNum = 2 * mx + 1;
    const int32_t reference = r + mx;
    uint32_t v = 0;

    if (reference < 0 || reference >= symbolNum || !ReadSubexp(reader, (uint32_t)symbolNum, v))
        return false;

    const int32_t x = (reference << 1) <= symbolNum ? InverseRecenter(reference, (int32_t)v) : symbolNum - 1 - InverseRecenter(symbolNum - 1 - reference, (int32_t)v);
    value = ((x - mx) * (1 << precDiff)) + round;

    return true;
}

// "film_grain_params" with "update_grain = 1"
static inline bool ReadFilmGrainParams(BitReader& reader, const VideoAV1SequenceDesc& sequence, VideoAV1FilmGrainDesc& filmGrain) {
    const bool isMonochrome = !!(sequence.flags & VideoAV1SequenceBits::MONO_CHROME);
    uint32_t value = 0;

    if (!reader.ReadBits(4, value) || value > 14)
        return false;
    filmGrain.numYPoints = (uint8_t)value;

    for (uint32_t i = 0; i < filmGrain.numYPoints; i++) {
        uint32_t pointValue = 0;
        uint32_t pointScaling = 0;

        if (!reader.ReadBits(8, pointValue) || !reader.ReadBits(8, pointScaling))
            return false;

        filmGrain.pointYValue[i] = (uint8_t)pointValue;
        filmGrain.pointYScaling[i] = (uint8_t)pointScaling;
    }

    if (!isMonochrome && !reader.ReadFlag(filmGrain.chromaScalingFromLuma))
        return false;

    if (!isMonochrome && !filmGrain.chromaScalingFromLuma && !(sequence.subsamplingX == 1 && sequence.subsamplingY == 1 && filmGrain.numYPoints == 0)) {
        if (!reader.ReadBits(4, value) || value > 10)
            return false;
        filmGrain.numCbPoints = (uint8_t)value;

        for (uint32_t i = 0; i < filmGrain.numCbPoints; i++) {
            uint32_t pointValue = 0;
            uint32_t pointScaling = 0;

            if (!reader.ReadBits(8, pointValue) || !reader.ReadBits(8, pointScaling))
                return false;

            filmGrain.pointCbValue[i] = (uint8_t)pointValue;
            filmGrain.pointCbScaling[i] = (uint8_t)pointScaling;
        }

        if (!reader.ReadBits(4, value) || value > 10)
            return false;
        filmGrain.numCrPoints = (uint8_t)value;

        for (uint32_t i = 0; i < filmGrain.numCrPoints; i++) {
            uint32_t pointValue = 0;
            uint32_t pointScaling = 0;

            if (!reader.ReadBits(8, pointValue) || !reader.ReadBits(8, pointScaling))
                return false;

            filmGrain.pointCrValue[i] = (uint8_t)pointValue;
            filmGrain.pointCrScaling[i] = (uint8_t)pointScaling;
        }
    }

    if (!reader.ReadBits(2, value))
        return false;
    filmGrain.grainScalingMinus8 = (uint8_t)value;

    if (!reader.ReadBits(2, value))
        return false;
    filmGrain.arCoeffLag = (uint8_t)value;

    const uint32_t numPosLuma = 2 * filmGrain.arCoeffLag * (filmGrain.arCoeffLag + 1);
    const uint32_t numPosChroma = filmGrain.numYPoints ? numPosLuma + 1 : numPosLuma;

    if (filmGrain.numYPoints) {
        for (uint32_t i = 0; i < numPosLuma; i++) {
            if (!reader.ReadBits(8, value))
                return false;
            filmGrain.arCoeffsYPlus128[i] = (uint8_t)value;
        }
    }

    if (filmGrain.chromaScalingFromLuma || filmGrain.numCbPoints) {
        for (uint32_t i = 0; i < numPosChroma; i++) {
            if (!reader.ReadBits(8, value))
                return false;
            filmGrain.arCoeffsCbPlus128[i] = (uint8_t)value;
        }
    }

    if (filmGrain.chromaScalingFromLuma || filmGrain.numCrPoints) {
        for (uint32_t i = 0; i < numPosChroma; i++) {
            if (!reader.ReadBits(8, value))
                return false;
            filmGrain.arCoeffsCrPlus128[i] = (uint8_t)value;
        }
    }

    if (!reader.ReadBits(2, value))
        return false;
    filmGrain.arCoeffShiftMinus6 = (uint8_t)value;

    if (!reader.ReadBits(2, value))
        return false;
    filmGrain.grainScaleShift = (uint8_t)value;

    if (filmGrain.numCbPoints) {
        uint32_t mult = 0;
        uint32_t lumaMult = 0;
        uint32_t offset = 0;

        if (!reader.ReadBits(8, mult) || !reader.ReadBits(8, lumaMult) || !reader.ReadBits(9, offset))
            return false;

        filmGrain.cbMult = (uint8_t)mult;
        filmGrain.cbLumaMult = (uint8_t)lumaMult;
        filmGrain.cbOffset = (int16_t)offset;
    }

    if (filmGrain.numCrPoints) {
        uint32_t mult = 0;
        uint32_t lumaMult = 0;
        uint32_t offset = 0;

        if (!reader.ReadBits(8, mult) || !reader.ReadBits(8, lumaMult) || !reader.ReadBits(9, offset))
            return false;

        filmGrain.crMult = (uint8_t)mult;
        filmGrain.crLumaMult = (uint8_t)lumaMult;
        filmGrain.crOffset = (int16_t)offset;
    }

    return reader.ReadFlag(filmGrain.overlapFlag) && reader.ReadFlag(filmGrain.clipToRestrictedRange);
}

static inline void BindPointers(VideoAV1EncodeDecodeInfo& info) {
    info.tileLayout.miColumnStarts = info.miColumnStarts;
    info.tileLayout.miRowStarts = info.miRowStarts;
    info.tileLayout.widthInSuperblocksMinus1 = info.widthInSuperblocksMinus1;
    info.tileLayout.heightInSuperblocksMinus1 = info.heightInSuperblocksMinus1;

    info.picture.tileLayout = &info.tileLayout;
    info.picture.quantization = &info.quantization;
    info.picture.loopFilter = &info.loopFilter;
    info.picture.cdef = &info.cdef;
    info.picture.loopRestoration = &info.loopRestoration;
    info.picture.globalMotion = &info.globalMotion;
    info.picture.segmentation = &info.segmentation;
    info.picture.filmGrain = &info.filmGrain;
    info.picture.tiles = info.tiles;
    info.picture.references = info.references;
}

static inline void FillIdentityGlobalMotion(VideoAV1GlobalMotionDesc& globalMotion) {
    for (auto& params : globalMotion.params) {
        params[2] = 1 << 16;
        params[5] = 1 << 16;
    }
}

// The superblock size is taken from "info.sequence"
static inline void FillSingleTileLayout(VideoAV1EncodeDecodeInfo& info, uint32_t width, uint32_t height) {
    const uint32_t sbShift = (info.sequence.flags & VideoAV1SequenceBits::USE_128X128_SUPERBLOCK) ? 5 : 4;
    const uint32_t miCols = 2 * ((width + 7) >> 3);
    const uint32_t miRows = 2 * ((height + 7) >> 3);
    const uint32_t sbCols = (miCols + (1u << sbShift) - 1) >> sbShift;
    const uint32_t sbRows = (miRows + (1u << sbShift) - 1) >> sbShift;

    info.tileLayout.columnNum = 1;
    info.tileLayout.rowNum = 1;
    info.tileLayout.tileSizeBytesMinus1 = 3;
    info.tileLayout.uniformSpacing = 1;
    info.miColumnStarts[0] = 0;
    info.miColumnStarts[1] = (uint16_t)miCols;
    info.miRowStarts[0] = 0;
    info.miRowStarts[1] = (uint16_t)miRows;
    info.widthInSuperblocksMinus1[0] = (uint16_t)(sbCols - 1);
    info.heightInSuperblocksMinus1[0] = (uint16_t)(sbRows - 1);
}

// "get_relative_dist"
static inline int32_t GetRelativeDistance(const VideoAV1SequenceDesc& sequence, uint32_t a, uint32_t b) {
    if (!(sequence.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT))
        return 0;

    const int32_t m = 1 << sequence.orderHintBitsMinus1;
    const int32_t diff = (int32_t)a - (int32_t)b;

    return (diff & (m - 1)) - (diff & m);
}

// "skip_mode_params": returns "skipModeAllowed" and fills "SkipModeFrame" from the references of the current frame, ordered by reference name
static inline bool GetSkipModeFrames(const VideoAV1SequenceDesc& sequence, uint32_t orderHint, const VideoAV1ReferenceDesc* references, VideoAV1ReferenceName* skipModeFrames) {
    int32_t forwardIndex = -1;
    int32_t backwardIndex = -1;
    uint32_t forwardHint = 0;
    uint32_t backwardHint = 0;

    for (int32_t i = 0; i < (int32_t)REFERENCE_NAME_NUM; i++) {
        const uint32_t referenceHint = references[i].orderHint;

        if (GetRelativeDistance(sequence, referenceHint, orderHint) < 0) {
            if (forwardIndex < 0 || GetRelativeDistance(sequence, referenceHint, forwardHint) > 0) {
                forwardIndex = i;
                forwardHint = referenceHint;
            }
        } else if (GetRelativeDistance(sequence, referenceHint, orderHint) > 0) {
            if (backwardIndex < 0 || GetRelativeDistance(sequence, referenceHint, backwardHint) < 0) {
                backwardIndex = i;
                backwardHint = referenceHint;
            }
        }
    }

    if (forwardIndex < 0)
        return false;

    int32_t secondIndex = backwardIndex;

    if (secondIndex < 0) {
        uint32_t secondForwardHint = 0;

        for (int32_t i = 0; i < (int32_t)REFERENCE_NAME_NUM; i++) {
            const uint32_t referenceHint = references[i].orderHint;

            if (GetRelativeDistance(sequence, referenceHint, forwardHint) < 0 && (secondIndex < 0 || GetRelativeDistance(sequence, referenceHint, secondForwardHint) > 0)) {
                secondIndex = i;
                secondForwardHint = referenceHint;
            }
        }

        if (secondIndex < 0)
            return false;
    }

    skipModeFrames[0] = GetReferenceNameFromReferenceIndex((uint32_t)std::min(forwardIndex, secondIndex));
    skipModeFrames[1] = GetReferenceNameFromReferenceIndex((uint32_t)std::max(forwardIndex, secondIndex));

    return true;
}

// "set_frame_refs" (AV1 7.8): "ref_frame_idx[]" from "last_frame_idx", "gold_frame_idx" and the order hints of all 8 slots
static inline bool SetFrameRefs(const VideoAV1EncodeDecodeInfoDesc& desc, uint32_t orderHint, uint32_t lastFrameIdx, uint32_t goldFrameIdx, std::array<uint8_t, 7>& refFrameIndices) {
    const VideoAV1SequenceDesc& sequence = *desc.sequence;
    const int32_t curFrameHint = 1 << sequence.orderHintBitsMinus1;

    std::array<int32_t, 8> shiftedOrderHints = {};
    for (uint32_t i = 0; i < 8; i++) {
        const VideoAV1SavedFrameState* state = GetSavedFrameState(desc, i);
        if (!state)
            return false;

        shiftedOrderHints[i] = curFrameHint + GetRelativeDistance(sequence, state->orderHint, orderHint);
    }

    if (shiftedOrderHints[lastFrameIdx] >= curFrameHint || shiftedOrderHints[goldFrameIdx] >= curFrameHint)
        return false;

    std::array<int32_t, 7> indices = {};
    indices.fill(-1);
    indices[0] = (int32_t)lastFrameIdx;  // LAST_FRAME
    indices[3] = (int32_t)goldFrameIdx;  // GOLDEN_FRAME

    std::array<bool, 8> usedFrame = {};
    usedFrame[lastFrameIdx] = true;
    usedFrame[goldFrameIdx] = true;

    auto findReference = [&](bool isBackward, bool isLatest) {
        int32_t reference = -1;
        int32_t referenceHint = 0;

        for (int32_t i = 0; i < 8; i++) {
            const int32_t hint = shiftedOrderHints[i];

            if (usedFrame[i] || (hint >= curFrameHint) != isBackward)
                continue;

            if (reference < 0 || (isLatest ? hint >= referenceHint : hint < referenceHint)) {
                reference = i;
                referenceHint = hint;
            }
        }

        return reference;
    };

    auto assign = [&](uint32_t nameIndex, int32_t reference) {
        if (reference >= 0) {
            indices[nameIndex] = reference;
            usedFrame[reference] = true;
        }
    };

    assign(6, findReference(true, true));  // ALTREF_FRAME: the latest backward reference
    assign(4, findReference(true, false)); // BWDREF_FRAME: the earliest backward reference
    assign(5, findReference(true, false)); // ALTREF2_FRAME: the next earliest backward reference

    // "Ref_Frame_List": LAST2, LAST3, BWDREF, ALTREF2, ALTREF get the latest remaining forward references
    constexpr uint32_t REF_FRAME_LIST[5] = {1, 2, 4, 5, 6};
    for (uint32_t nameIndex : REF_FRAME_LIST) {
        if (indices[nameIndex] < 0)
            assign(nameIndex, findReference(false, true));
    }

    // The remaining references are the frame with the smallest order hint
    int32_t earliest = 0;
    for (int32_t i = 1; i < 8; i++) {
        if (shiftedOrderHints[i] < shiftedOrderHints[earliest])
            earliest = i;
    }

    for (uint32_t i = 0; i < 7; i++)
        refFrameIndices[i] = (uint8_t)(indices[i] < 0 ? earliest : indices[i]);

    return true;
}

// Parses "uncompressed_header" and the following tile group header of a shown KEY or INTER frame. State inherited from reference frames
// ("frame_refs_short_signaling", "frame_size_with_refs", "load_previous", "load_grain_params") comes from "desc.savedFrameStates": a frame
// that needs missing state is rejected, a frame that inherits missing state without needing it gets an invalid "savedFrameState"
static inline bool ParseGeneratedFrameHeader(const VideoAV1EncodeDecodeInfoDesc& desc, const uint8_t* payload, size_t availablePayloadSize, const uint8_t* tilePayload, size_t availableTilePayloadSize,
    size_t fullTilePayloadSize, bool combinedFrameObu, VideoAV1EncodeDecodeInfo& info) {
    constexpr uint32_t MAX_TILE_WIDTH = 4096;
    constexpr uint32_t MAX_TILE_AREA = 4096 * 2304;
    constexpr uint32_t MAX_TILE_COLS = 64;
    constexpr uint32_t MAX_TILE_ROWS = 64;
    constexpr uint32_t PRIMARY_REF_NONE = 7;
    constexpr uint32_t SUPERRES_NUM = 8;
    constexpr uint8_t REMAP_LR_TYPE[4] = {0, 3, 1, 2};

    const VideoAV1SequenceDesc& sequence = *desc.sequence;
    const bool enableOrderHint = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT);
    const bool frameIdNumbersPresent = !!(sequence.flags & VideoAV1SequenceBits::FRAME_ID_NUMBERS_PRESENT);
    const bool use128x128Superblock = !!(sequence.flags & VideoAV1SequenceBits::USE_128X128_SUPERBLOCK);
    const bool separateUvDeltaQ = !!(sequence.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q);
    const uint32_t planeNum = (sequence.flags & VideoAV1SequenceBits::MONO_CHROME) ? 1 : 3;

    if (sequence.flags & VideoAV1SequenceBits::REDUCED_STILL_PICTURE_HEADER)
        return false;

    BitReader reader{payload, availablePayloadSize, 0};
    uint8_t showExistingFrame = 0;
    uint32_t frameType = 0;
    uint8_t showFrame = 0;

    if (!reader.ReadFlag(showExistingFrame) || showExistingFrame || !reader.ReadBits(2, frameType) || !reader.ReadFlag(showFrame))
        return false;

    if (frameType > 1 || !showFrame)
        return false;

    const bool isKeyFrame = frameType == 0;
    VideoAV1PictureBits flags = VideoAV1PictureBits::SHOW_FRAME;
    uint8_t errorResilient = 1; // shown KEY frames

    if (!isKeyFrame) {
        if (!reader.ReadFlag(errorResilient))
            return false;

        flags |= VideoAV1PictureBits::SHOWABLE_FRAME;
    }

    if (errorResilient)
        flags |= VideoAV1PictureBits::ERROR_RESILIENT_MODE;

    uint8_t disableCdfUpdate = 0;
    uint8_t allowScreenContentTools = 0;
    uint8_t forceIntegerMv = 0;

    if (!reader.ReadFlag(disableCdfUpdate))
        return false;

    if (disableCdfUpdate)
        flags |= VideoAV1PictureBits::DISABLE_CDF_UPDATE | VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF;

    if (sequence.seqForceScreenContentTools == SELECT_SCREEN_CONTENT_TOOLS) {
        if (!reader.ReadFlag(allowScreenContentTools))
            return false;
    } else
        allowScreenContentTools = sequence.seqForceScreenContentTools;

    if (allowScreenContentTools) {
        flags |= VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS;

        if (sequence.seqForceIntegerMv == SELECT_SCREEN_CONTENT_TOOLS) {
            if (!reader.ReadFlag(forceIntegerMv))
                return false;
        } else
            forceIntegerMv = sequence.seqForceIntegerMv;
    }

    if (isKeyFrame)
        forceIntegerMv = 1;

    if (forceIntegerMv)
        flags |= VideoAV1PictureBits::FORCE_INTEGER_MV;

    if (frameIdNumbersPresent) {
        uint32_t currentFrameId = 0;

        if (!reader.ReadBits(sequence.additionalFrameIdLengthMinus1 + sequence.deltaFrameIdLengthMinus2 + 3, currentFrameId))
            return false;

        info.picture.currentFrameId = currentFrameId;
    }

    uint8_t frameSizeOverride = 0;
    uint32_t orderHint = 0;
    uint32_t primaryRefFrame = PRIMARY_REF_NONE;
    uint32_t refreshFrameFlags = 0xFF;

    if (!reader.ReadFlag(frameSizeOverride))
        return false;

    if (enableOrderHint && !reader.ReadBits(sequence.orderHintBitsMinus1 + 1, orderHint))
        return false;

    std::array<uint8_t, 7> refFrameIndices = {};

    if (!isKeyFrame) {
        if ((!errorResilient && !reader.ReadBits(3, primaryRefFrame)) || !reader.ReadBits(8, refreshFrameFlags))
            return false;

        // "ref_order_hint": only used by decoders that lost references
        if (errorResilient && enableOrderHint) {
            for (uint32_t i = 0; i < 8; i++) {
                uint32_t refOrderHint = 0;

                if (!reader.ReadBits(sequence.orderHintBitsMinus1 + 1, refOrderHint))
                    return false;
            }
        }

        uint8_t frameRefsShortSignaling = 0;

        if (enableOrderHint && !reader.ReadFlag(frameRefsShortSignaling))
            return false;

        if (frameRefsShortSignaling) {
            uint32_t lastFrameIdx = 0;
            uint32_t goldFrameIdx = 0;

            if (!reader.ReadBits(3, lastFrameIdx) || !reader.ReadBits(3, goldFrameIdx) || !SetFrameRefs(desc, orderHint, lastFrameIdx, goldFrameIdx, refFrameIndices))
                return false;

            flags |= VideoAV1PictureBits::FRAME_REFS_SHORT_SIGNALING;
        }

        for (uint32_t i = 0; i < REFERENCE_NAME_NUM; i++) {
            uint32_t value = 0;

            if (!frameRefsShortSignaling) {
                if (!reader.ReadBits(3, value))
                    return false;

                refFrameIndices[i] = (uint8_t)value;
            }

            // "delta_frame_id_minus_1"
            if (frameIdNumbersPresent && !reader.ReadBits(sequence.deltaFrameIdLengthMinus2 + 2, value))
                return false;
        }

        if (!BuildInterFrameReferences(desc, refFrameIndices, info))
            return false;
    }

    // "load_previous" source, "setup_past_independence" without it
    const VideoAV1SavedFrameState* primaryState = primaryRefFrame != PRIMARY_REF_NONE ? GetSavedFrameState(desc, refFrameIndices[primaryRefFrame]) : nullptr;
    const bool isPrimaryStateMissing = primaryRefFrame != PRIMARY_REF_NONE && !primaryState;

    // "frame_size_with_refs": the size of the first reference with "found_ref"
    uint32_t upscaledWidth = sequence.maxFrameWidthMinus1 + 1u;
    uint32_t frameHeight = sequence.maxFrameHeightMinus1 + 1u;
    const VideoAV1SavedFrameState* sizeState = nullptr;

    if (!isKeyFrame && frameSizeOverride && !errorResilient) {
        for (uint32_t i = 0; i < REFERENCE_NAME_NUM && !sizeState; i++) {
            uint8_t foundRef = 0;

            if (!reader.ReadFlag(foundRef))
                return false;

            if (foundRef) {
                sizeState = GetSavedFrameState(desc, refFrameIndices[i]);

                if (!sizeState)
                    return false;

                upscaledWidth = sizeState->upscaledWidthMinus1 + 1u;
                frameHeight = sizeState->frameHeightMinus1 + 1u;
            }
        }

        flags |= VideoAV1PictureBits::FRAME_SIZE_OVERRIDE;
    }

    // "frame_size" and "superres_params"
    if (frameSizeOverride && !sizeState) {
        uint32_t frameWidthMinus1 = 0;
        uint32_t frameHeightMinus1 = 0;

        if (!reader.ReadBits(sequence.frameWidthBitsMinus1 + 1, frameWidthMinus1) || !reader.ReadBits(sequence.frameHeightBitsMinus1 + 1, frameHeightMinus1))
            return false;

        if (frameWidthMinus1 > sequence.maxFrameWidthMinus1 || frameHeightMinus1 > sequence.maxFrameHeightMinus1)
            return false;

        upscaledWidth = frameWidthMinus1 + 1;
        frameHeight = frameHeightMinus1 + 1;
        flags |= VideoAV1PictureBits::FRAME_SIZE_OVERRIDE;
    }

    uint32_t superresDenom = SUPERRES_NUM;

    if (sequence.flags & VideoAV1SequenceBits::ENABLE_SUPERRES) {
        uint8_t useSuperres = 0;

        if (!reader.ReadFlag(useSuperres))
            return false;

        if (useSuperres) {
            uint32_t codedDenom = 0;

            if (!reader.ReadBits(3, codedDenom))
                return false;

            superresDenom = codedDenom + 9;
            info.picture.codedDenom = (uint8_t)codedDenom;
            info.picture.superresDenom = (uint8_t)superresDenom;
            flags |= VideoAV1PictureBits::USE_SUPERRES;
        }
    }

    const uint32_t frameWidth = (upscaledWidth * SUPERRES_NUM + superresDenom / 2) / superresDenom;

    // "render_size", the render size of the reference with "found_ref"
    uint32_t renderWidthMinus1 = sizeState ? sizeState->renderWidthMinus1 : upscaledWidth - 1;
    uint32_t renderHeightMinus1 = sizeState ? sizeState->renderHeightMinus1 : frameHeight - 1;
    uint8_t renderAndFrameSizeDifferent = 0;

    if (!sizeState && !reader.ReadFlag(renderAndFrameSizeDifferent))
        return false;

    if (renderAndFrameSizeDifferent) {
        if (!reader.ReadBits(16, renderWidthMinus1) || !reader.ReadBits(16, renderHeightMinus1))
            return false;

        flags |= VideoAV1PictureBits::RENDER_AND_FRAME_SIZE_DIFFERENT;
    }

    uint8_t allowIntrabc = 0;
    uint32_t interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP;

    if (isKeyFrame) {
        if (allowScreenContentTools && upscaledWidth == frameWidth && !reader.ReadFlag(allowIntrabc))
            return false;

        if (allowIntrabc)
            flags |= VideoAV1PictureBits::ALLOW_INTRABC;
    } else {
        uint8_t allowHighPrecisionMv = 0;

        if (!forceIntegerMv && !reader.ReadFlag(allowHighPrecisionMv))
            return false;

        if (allowHighPrecisionMv)
            flags |= VideoAV1PictureBits::ALLOW_HIGH_PRECISION_MV;

        uint8_t isFilterSwitchable = 0;

        if (!reader.ReadFlag(isFilterSwitchable))
            return false;

        if (isFilterSwitchable) {
            interpolationFilter = INTERPOLATION_FILTER_SWITCHABLE;
            flags |= VideoAV1PictureBits::IS_FILTER_SWITCHABLE;
        } else if (!reader.ReadBits(2, interpolationFilter))
            return false;

        uint8_t isMotionModeSwitchable = 0;

        if (!reader.ReadFlag(isMotionModeSwitchable))
            return false;

        if (isMotionModeSwitchable)
            flags |= VideoAV1PictureBits::IS_MOTION_MODE_SWITCHABLE;

        // "enable_ref_frame_mvs" is only coded (and the writer only emits it) with "enable_order_hint", "use_ref_frame_mvs" not with "error_resilient_mode"
        if (!errorResilient && enableOrderHint && (sequence.flags & VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS)) {
            uint8_t useRefFrameMvs = 0;

            if (!reader.ReadFlag(useRefFrameMvs))
                return false;

            if (useRefFrameMvs)
                flags |= VideoAV1PictureBits::USE_REF_FRAME_MVS;
        }
    }

    if (!disableCdfUpdate) {
        uint8_t disableFrameEndUpdateCdf = 0;

        if (!reader.ReadFlag(disableFrameEndUpdateCdf))
            return false;

        if (disableFrameEndUpdateCdf)
            flags |= VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF;
    }

    // "tile_info"
    const uint32_t miCols = 2 * ((frameWidth + 7) >> 3);
    const uint32_t miRows = 2 * ((frameHeight + 7) >> 3);
    const uint32_t sbShift = use128x128Superblock ? 5 : 4;
    const uint32_t sbSize = sbShift + 2;
    const uint32_t sbCols = (miCols + (1u << sbShift) - 1) >> sbShift;
    const uint32_t sbRows = (miRows + (1u << sbShift) - 1) >> sbShift;
    const uint32_t minLog2TileCols = TileLog2(MAX_TILE_WIDTH >> sbSize, sbCols);
    const uint32_t maxLog2TileCols = TileLog2(1, std::min(sbCols, MAX_TILE_COLS));
    const uint32_t maxLog2TileRows = TileLog2(1, std::min(sbRows, MAX_TILE_ROWS));
    const uint32_t minLog2Tiles = std::max(minLog2TileCols, TileLog2(MAX_TILE_AREA >> (2 * sbSize), sbRows * sbCols));

    uint8_t uniformTileSpacing = 0;
    if (!reader.ReadFlag(uniformTileSpacing))
        return false;

    uint32_t tileColsLog2 = 0;
    uint32_t tileRowsLog2 = 0;
    uint32_t tileCols = 1;
    uint32_t tileRows = 1;
    std::array<uint32_t, 65> tileStartColSb = {};
    std::array<uint32_t, 65> tileStartRowSb = {};

    if (uniformTileSpacing) {
        if (!reader.ReadIncrement(minLog2TileCols, maxLog2TileCols, tileColsLog2))
            return false;

        const uint32_t tileWidthSb = (sbCols + (1u << tileColsLog2) - 1) >> tileColsLog2;

        for (uint32_t off = 0, i = 0; off < sbCols; off += tileWidthSb)
            tileStartColSb[i++] = off;
        tileCols = (sbCols + tileWidthSb - 1) / tileWidthSb;

        const uint32_t minLog2TileRows = std::max<int32_t>((int32_t)minLog2Tiles - (int32_t)tileColsLog2, 0);

        if (!reader.ReadIncrement(minLog2TileRows, maxLog2TileRows, tileRowsLog2))
            return false;

        const uint32_t tileHeightSb = (sbRows + (1u << tileRowsLog2) - 1) >> tileRowsLog2;

        for (uint32_t off = 0, i = 0; off < sbRows; off += tileHeightSb)
            tileStartRowSb[i++] = off;
        tileRows = (sbRows + tileHeightSb - 1) / tileHeightSb;

        uint32_t i = 0;

        for (; i + 1 < tileCols; i++)
            info.widthInSuperblocksMinus1[i] = (uint16_t)(tileWidthSb - 1);
        info.widthInSuperblocksMinus1[i] = (uint16_t)(sbCols - (tileCols - 1) * tileWidthSb - 1);
        i = 0;

        for (; i + 1 < tileRows; i++)
            info.heightInSuperblocksMinus1[i] = (uint16_t)(tileHeightSb - 1);
        info.heightInSuperblocksMinus1[i] = (uint16_t)(sbRows - (tileRows - 1) * tileHeightSb - 1);
    } else {
        uint32_t startSb = 0;
        uint32_t widestTileSb = 0;

        for (uint32_t i = 0; startSb < sbCols && i < MAX_TILE_COLS; i++) {
            tileStartColSb[i] = startSb;
            const uint32_t maxWidth = std::min(sbCols - startSb, MAX_TILE_WIDTH >> sbSize);
            uint32_t tileWidthMinus1 = 0;

            if (!ReadNs(reader, maxWidth, tileWidthMinus1))
                return false;

            const uint32_t tileWidthSb = tileWidthMinus1 + 1;
            info.widthInSuperblocksMinus1[i] = (uint16_t)tileWidthMinus1;
            widestTileSb = std::max(widestTileSb, tileWidthSb);
            startSb += tileWidthSb;
            tileCols = i + 1;
        }
        tileColsLog2 = TileLog2(1, tileCols);

        if (startSb != sbCols || !tileCols)
            return false;

        startSb = 0;
        uint32_t maxTileAreaSb = MAX_TILE_AREA >> (2 * sbSize);

        if (minLog2Tiles > 0)
            maxTileAreaSb = (sbRows * sbCols) >> (minLog2Tiles + 1);
        else
            maxTileAreaSb = sbRows * sbCols;

        const uint32_t maxTileHeightSb = std::max(maxTileAreaSb / std::max(widestTileSb, 1u), 1u);

        for (uint32_t i = 0; startSb < sbRows && i < MAX_TILE_ROWS; i++) {
            tileStartRowSb[i] = startSb;
            const uint32_t maxHeight = std::min(sbRows - startSb, maxTileHeightSb);
            uint32_t tileHeightMinus1 = 0;

            if (!ReadNs(reader, maxHeight, tileHeightMinus1))
                return false;

            info.heightInSuperblocksMinus1[i] = (uint16_t)tileHeightMinus1;
            const uint32_t tileHeightSb = tileHeightMinus1 + 1;
            startSb += tileHeightSb;
            tileRows = i + 1;
        }
        tileRowsLog2 = TileLog2(1, tileRows);

        if (startSb != sbRows || !tileRows)
            return false;
    }

    uint32_t contextUpdateTileId = 0;
    uint32_t tileSizeBytesMinus1 = 0;
    if (tileColsLog2 > 0 || tileRowsLog2 > 0) {
        if (!reader.ReadBits(tileColsLog2 + tileRowsLog2, contextUpdateTileId) || !reader.ReadBits(2, tileSizeBytesMinus1))
            return false;
    }
    for (uint32_t i = 0; i < tileCols; i++)
        info.miColumnStarts[i] = (uint16_t)(tileStartColSb[i] << sbShift);
    info.miColumnStarts[tileCols] = (uint16_t)miCols;
    for (uint32_t i = 0; i < tileRows; i++)
        info.miRowStarts[i] = (uint16_t)(tileStartRowSb[i] << sbShift);
    info.miRowStarts[tileRows] = (uint16_t)miRows;

    // "quantization_params"
    VideoAV1QuantizationDesc& quantization = info.quantization;
    uint32_t baseQIndex = 0;

    if (!reader.ReadBits(8, baseQIndex) || !ReadDeltaQ(reader, quantization.deltaQYDc))
        return false;

    if (planeNum > 1) {
        if (separateUvDeltaQ && !reader.ReadFlag(quantization.diffUvDelta))
            return false;

        if (!ReadDeltaQ(reader, quantization.deltaQUDc) || !ReadDeltaQ(reader, quantization.deltaQUAc))
            return false;

        if (quantization.diffUvDelta) {
            if (!ReadDeltaQ(reader, quantization.deltaQVDc) || !ReadDeltaQ(reader, quantization.deltaQVAc))
                return false;
        } else {
            quantization.deltaQVDc = quantization.deltaQUDc;
            quantization.deltaQVAc = quantization.deltaQUAc;
        }
    }

    if (!reader.ReadFlag(quantization.usingQmatrix))
        return false;

    if (quantization.usingQmatrix) {
        uint32_t qmY = 0;
        uint32_t qmU = 0;
        uint32_t qmV = 0;

        if (!reader.ReadBits(4, qmY) || !reader.ReadBits(4, qmU))
            return false;

        if (!separateUvDeltaQ)
            qmV = qmU;
        else if (!reader.ReadBits(4, qmV))
            return false;

        quantization.qmY = (uint8_t)qmY;
        quantization.qmU = (uint8_t)qmU;
        quantization.qmV = (uint8_t)qmV;
    }

    // "segmentation_params"
    constexpr uint8_t SEGMENTATION_FEATURE_BITS[8] = {8, 6, 6, 6, 6, 3, 0, 0};
    constexpr bool SEGMENTATION_FEATURE_SIGNED[8] = {true, true, true, true, true, false, false, false};
    constexpr int32_t SEGMENTATION_FEATURE_MAX[8] = {255, 63, 63, 63, 63, 7, 0, 0};

    uint8_t segmentationEnabled = 0;

    if (!reader.ReadFlag(segmentationEnabled))
        return false;

    if (segmentationEnabled) {
        uint8_t updateMap = 1;
        uint8_t temporalUpdate = 0;
        uint8_t updateData = 1;

        if (primaryRefFrame != PRIMARY_REF_NONE) {
            if (!reader.ReadFlag(updateMap) || (updateMap && !reader.ReadFlag(temporalUpdate)) || !reader.ReadFlag(updateData))
                return false;
        }

        flags |= VideoAV1PictureBits::SEGMENTATION_ENABLED;

        if (updateMap)
            flags |= VideoAV1PictureBits::SEGMENTATION_UPDATE_MAP;

        if (temporalUpdate)
            flags |= VideoAV1PictureBits::SEGMENTATION_TEMPORAL_UPDATE;

        if (updateData) {
            flags |= VideoAV1PictureBits::SEGMENTATION_UPDATE_DATA;

            for (uint32_t i = 0; i < 8; i++) {
                for (uint32_t j = 0; j < 8; j++) {
                    uint8_t featureEnabled = 0;

                    if (!reader.ReadFlag(featureEnabled))
                        return false;

                    int32_t clippedValue = 0;

                    if (featureEnabled) {
                        const uint32_t bitNum = SEGMENTATION_FEATURE_BITS[j];
                        const int32_t limit = SEGMENTATION_FEATURE_MAX[j];
                        uint32_t bits = 0;

                        if (SEGMENTATION_FEATURE_SIGNED[j]) {
                            if (!reader.ReadBits(bitNum + 1, bits))
                                return false;

                            const int32_t signedValue = (bits & (1u << bitNum)) ? (int32_t)bits - (int32_t)(1u << (bitNum + 1)) : (int32_t)bits;
                            clippedValue = std::max(-limit, std::min(signedValue, limit));
                        } else {
                            if (!reader.ReadBits(bitNum, bits))
                                return false;

                            clippedValue = std::min((int32_t)bits, limit);
                        }

                        info.segmentation.featureEnabled[i] |= (uint8_t)(1u << j);
                    }

                    info.segmentation.featureData[i][j] = (int16_t)clippedValue;
                }
            }
        } else {
            // "load_previous": the segmentation features of "primary_ref_frame"
            if (!primaryState)
                return false;

            info.segmentation = primaryState->segmentation;
        }
    }

    // "delta_q_params" and "delta_lf_params"
    if (baseQIndex > 0) {
        uint8_t deltaQPresent = 0;

        if (!reader.ReadFlag(deltaQPresent))
            return false;

        if (deltaQPresent) {
            uint32_t deltaQRes = 0;

            if (!reader.ReadBits(2, deltaQRes))
                return false;

            info.picture.deltaQRes = (uint8_t)deltaQRes;
            flags |= VideoAV1PictureBits::DELTA_Q_PRESENT;

            uint8_t deltaLfPresent = 0;

            if (!allowIntrabc && !reader.ReadFlag(deltaLfPresent))
                return false;

            if (deltaLfPresent) {
                uint32_t deltaLfRes = 0;
                uint8_t deltaLfMulti = 0;

                if (!reader.ReadBits(2, deltaLfRes) || !reader.ReadFlag(deltaLfMulti))
                    return false;

                info.picture.deltaLfRes = (uint8_t)deltaLfRes;
                flags |= VideoAV1PictureBits::DELTA_LF_PRESENT;

                if (deltaLfMulti)
                    flags |= VideoAV1PictureBits::DELTA_LF_MULTI;
            }
        }
    }

    // "CodedLossless": "LosslessArray" of every segment, "get_qindex(1, segmentId)" applies "SEG_LVL_ALT_Q"
    bool codedLossless = quantization.deltaQYDc == 0 && quantization.deltaQUDc == 0 && quantization.deltaQUAc == 0 && quantization.deltaQVDc == 0 && quantization.deltaQVAc == 0;
    for (uint32_t segment = 0; segment < 8 && codedLossless; segment++) {
        int32_t qIndex = (int32_t)baseQIndex;
        if (segmentationEnabled && (info.segmentation.featureEnabled[segment] & 0x01))
            qIndex = std::max(0, std::min(255, qIndex + info.segmentation.featureData[segment][0]));

        codedLossless = qIndex == 0;
    }
    const bool allLossless = codedLossless && frameWidth == upscaledWidth;

    // "loop_filter_params": the deltas of "primary_ref_frame" ("load_previous") or the defaults
    const bool isLoopFilterCoded = !codedLossless && !allowIntrabc;
    bool areLoopFilterDeltasKnown = true;

    if (primaryState && isLoopFilterCoded) {
        for (uint32_t i = 0; i < 8; i++)
            info.loopFilter.refDeltas[i] = primaryState->loopFilter.refDeltas[i];
        for (uint32_t i = 0; i < 2; i++)
            info.loopFilter.modeDeltas[i] = primaryState->loopFilter.modeDeltas[i];
    } else {
        info.loopFilter.refDeltas[0] = 1;
        info.loopFilter.refDeltas[4] = -1;
        info.loopFilter.refDeltas[6] = -1;
        info.loopFilter.refDeltas[7] = -1;
        areLoopFilterDeltasKnown = !isPrimaryStateMissing || !isLoopFilterCoded;
    }

    if (isLoopFilterCoded) {
        uint32_t value = 0;

        if (!reader.ReadBits(6, value))
            return false;
        info.loopFilter.level[0] = (uint8_t)value;

        if (!reader.ReadBits(6, value))
            return false;
        info.loopFilter.level[1] = (uint8_t)value;

        if (planeNum > 1 && (info.loopFilter.level[0] || info.loopFilter.level[1])) {
            if (!reader.ReadBits(6, value))
                return false;
            info.loopFilter.level[2] = (uint8_t)value;

            if (!reader.ReadBits(6, value))
                return false;
            info.loopFilter.level[3] = (uint8_t)value;
        }

        if (!reader.ReadBits(3, value))
            return false;
        info.loopFilter.sharpness = (uint8_t)value;

        if (!reader.ReadFlag(info.loopFilter.deltaEnabled))
            return false;

        // The deltas of this frame are inherited from a missing state
        if (info.loopFilter.deltaEnabled && !areLoopFilterDeltasKnown)
            return false;

        if (info.loopFilter.deltaEnabled) {
            if (!reader.ReadFlag(info.loopFilter.deltaUpdate))
                return false;

            for (uint32_t i = 0; i < 8; i++) {
                uint8_t updateRefDelta = 0;

                if (info.loopFilter.deltaUpdate && !reader.ReadFlag(updateRefDelta))
                    return false;

                if (updateRefDelta && !reader.ReadSigned(7, info.loopFilter.refDeltas[i]))
                    return false;
            }

            for (uint32_t i = 0; i < 2; i++) {
                uint8_t updateModeDelta = 0;

                if (info.loopFilter.deltaUpdate && !reader.ReadFlag(updateModeDelta))
                    return false;

                if (updateModeDelta && !reader.ReadSigned(7, info.loopFilter.modeDeltas[i]))
                    return false;
            }
        }
    }

    // "cdef_params"
    if ((sequence.flags & VideoAV1SequenceBits::ENABLE_CDEF) && !codedLossless && !allowIntrabc) {
        uint32_t value = 0;

        if (!reader.ReadBits(2, value))
            return false;
        info.picture.cdefDampingMinus3 = (uint8_t)value;

        if (!reader.ReadBits(2, value))
            return false;
        info.picture.cdefBits = (uint8_t)value;

        const uint32_t cdefStrengthNum = 1u << info.picture.cdefBits;
        for (uint32_t i = 0; i < cdefStrengthNum; i++) {
            if (!reader.ReadBits(4, value))
                return false;
            info.cdef.yPrimaryStrength[i] = (uint8_t)value;

            if (!reader.ReadBits(2, value))
                return false;
            info.cdef.ySecondaryStrength[i] = (uint8_t)value; // as coded: the decoders apply the 3 -> 4 adjustment

            if (planeNum > 1) {
                if (!reader.ReadBits(4, value))
                    return false;
                info.cdef.uvPrimaryStrength[i] = (uint8_t)value;

                if (!reader.ReadBits(2, value))
                    return false;
                info.cdef.uvSecondaryStrength[i] = (uint8_t)value;
            }
        }
    }

    // "lr_params"
    if ((sequence.flags & VideoAV1SequenceBits::ENABLE_RESTORATION) && !allLossless && !allowIntrabc) {
        for (uint32_t plane = 0; plane < planeNum; plane++) {
            uint32_t lrType = 0;

            if (!reader.ReadBits(2, lrType))
                return false;

            info.loopRestoration.frameRestorationType[plane] = REMAP_LR_TYPE[lrType];

            if (lrType) {
                flags |= VideoAV1PictureBits::USES_LR;

                if (plane)
                    flags |= VideoAV1PictureBits::USES_CHROMA_LR;
            }
        }

        if (flags & VideoAV1PictureBits::USES_LR) {
            uint32_t lrUnitShift = 0;

            if (!reader.ReadBits(1, lrUnitShift))
                return false;

            if (use128x128Superblock)
                lrUnitShift++;
            else if (lrUnitShift) {
                uint32_t lrUnitExtraShift = 0;

                if (!reader.ReadBits(1, lrUnitExtraShift))
                    return false;

                lrUnitShift += lrUnitExtraShift;
            }

            info.loopRestoration.lrUnitShift = (uint8_t)lrUnitShift;

            uint32_t lrUvShift = 0;

            if (sequence.subsamplingX && sequence.subsamplingY && (flags & VideoAV1PictureBits::USES_CHROMA_LR) && !reader.ReadBits(1, lrUvShift))
                return false;

            info.loopRestoration.lrUvShift = (uint8_t)lrUvShift;
        }
    }

    // "read_tx_mode"
    uint32_t txMode = TX_MODE_ONLY_4X4;

    if (!codedLossless && !reader.ReadIncrement(TX_MODE_LARGEST, TX_MODE_SELECT, txMode))
        return false;

    // "frame_reference_mode" and "skip_mode_params"
    info.picture.skipModeFrames[0] = VideoAV1ReferenceName::NONE;
    info.picture.skipModeFrames[1] = VideoAV1ReferenceName::NONE;

    if (!isKeyFrame) {
        uint8_t referenceSelect = 0;

        if (!reader.ReadFlag(referenceSelect))
            return false;

        if (referenceSelect)
            flags |= VideoAV1PictureBits::REFERENCE_SELECT;

        uint8_t skipModePresent = 0;

        if (referenceSelect && enableOrderHint && GetSkipModeFrames(sequence, orderHint, info.references, info.picture.skipModeFrames) && !reader.ReadFlag(skipModePresent))
            return false;

        if (skipModePresent)
            flags |= VideoAV1PictureBits::SKIP_MODE_PRESENT;

        uint8_t allowWarpedMotion = 0;

        if (!errorResilient && (sequence.flags & VideoAV1SequenceBits::ENABLE_WARPED_MOTION) && !reader.ReadFlag(allowWarpedMotion))
            return false;

        if (allowWarpedMotion)
            flags |= VideoAV1PictureBits::ALLOW_WARPED_MOTION;
    }

    uint8_t reducedTxSet = 0;

    if (!reader.ReadFlag(reducedTxSet))
        return false;

    if (reducedTxSet)
        flags |= VideoAV1PictureBits::REDUCED_TX_SET;

    // "global_motion_params" relative to "PrevGmParams": the parameters of "primary_ref_frame" or the defaults
    constexpr uint32_t GM_TYPE_TRANSLATION = 1;
    constexpr uint32_t GM_TYPE_ROTZOOM = 2;
    constexpr uint32_t GM_TYPE_AFFINE = 3;

    FillIdentityGlobalMotion(info.globalMotion);

    if (!isKeyFrame) {
        VideoAV1GlobalMotionDesc previous = {};
        FillIdentityGlobalMotion(previous);

        if (primaryState && !errorResilient)
            previous = primaryState->globalMotion;

        const bool isPreviousMissing = isPrimaryStateMissing && !errorResilient;
        const bool allowHighPrecisionMv = !!(flags & VideoAV1PictureBits::ALLOW_HIGH_PRECISION_MV);

        for (uint32_t reference = 1; reference <= REFERENCE_NAME_NUM; reference++) {
            uint8_t isGlobal = 0;
            uint32_t type = 0;

            if (!reader.ReadFlag(isGlobal))
                return false;

            if (isGlobal) {
                uint8_t isRotZoom = 0;
                uint8_t isTranslation = 0;

                if (!reader.ReadFlag(isRotZoom) || (!isRotZoom && !reader.ReadFlag(isTranslation)))
                    return false;

                type = isRotZoom ? GM_TYPE_ROTZOOM : (isTranslation ? GM_TYPE_TRANSLATION : GM_TYPE_AFFINE);

                if (isPreviousMissing)
                    return false;
            }

            info.globalMotion.type[reference] = (uint8_t)type;
            int32_t* params = info.globalMotion.params[reference];
            const int32_t* previousParams = previous.params[reference];

            if (type >= GM_TYPE_ROTZOOM) {
                if (!ReadGlobalParam(reader, type, 2, allowHighPrecisionMv, previousParams[2], params[2]) || !ReadGlobalParam(reader, type, 3, allowHighPrecisionMv, previousParams[3], params[3]))
                    return false;

                if (type == GM_TYPE_AFFINE) {
                    if (!ReadGlobalParam(reader, type, 4, allowHighPrecisionMv, previousParams[4], params[4]) || !ReadGlobalParam(reader, type, 5, allowHighPrecisionMv, previousParams[5], params[5]))
                        return false;
                } else {
                    params[4] = -params[3];
                    params[5] = params[2];
                }
            }

            if (type >= GM_TYPE_TRANSLATION) {
                if (!ReadGlobalParam(reader, type, 0, allowHighPrecisionMv, previousParams[0], params[0]) || !ReadGlobalParam(reader, type, 1, allowHighPrecisionMv, previousParams[1], params[1]))
                    return false;
            }

            // DXVA "wminvalid": the model can't be used for warped blocks
            info.globalMotion.invalid[reference] = type >= GM_TYPE_ROTZOOM && !IsGlobalMotionShearValid(params);
        }
    }

    // "film_grain_params", "load_grain_params" for "update_grain = 0"
    if (sequence.flags & VideoAV1SequenceBits::FILM_GRAIN_PARAMS_PRESENT) {
        uint8_t applyGrain = 0;

        if (!reader.ReadFlag(applyGrain))
            return false;

        if (applyGrain) {
            uint32_t grainSeed = 0;
            uint8_t updateGrain = 1;

            if (!reader.ReadBits(16, grainSeed) || (!isKeyFrame && !reader.ReadFlag(updateGrain)))
                return false;

            if (updateGrain) {
                if (!ReadFilmGrainParams(reader, sequence, info.filmGrain))
                    return false;
            } else {
                uint32_t refIndex = 0;

                if (!reader.ReadBits(3, refIndex))
                    return false;

                // "film_grain_params_ref_idx" must be one of "ref_frame_idx[]"
                const VideoAV1SavedFrameState* grainState = GetSavedFrameState(desc, refIndex);
                bool isReference = false;

                for (uint8_t refFrameIndex : refFrameIndices)
                    isReference |= refFrameIndex == refIndex;

                if (!grainState || !isReference)
                    return false;

                info.filmGrain = grainState->filmGrain;
                applyGrain = grainState->applyGrain;
            }

            // Resolved parameters: backends get the full set
            if (applyGrain) {
                info.filmGrain.grainSeed = (uint16_t)grainSeed;
                info.filmGrain.updateGrain = 1;
                info.filmGrain.filmGrainParamsRefIdx = 0;
                info.filmGrain.matrixCoeffIsIdentity = sequence.matrixCoefficients == 0;
                flags |= VideoAV1PictureBits::APPLY_GRAIN;
            } else
                info.filmGrain = {}; // "reset_grain_params"
        }
    }

    // "frame_obu": "byte_alignment" between "frame_header_obu" and "tile_group_obu"
    if (combinedFrameObu && !reader.ByteAlign())
        return false;

    // "tile_group_obu"
    const uint32_t tileNum = tileCols * tileRows;
    BitReader tileReader{tilePayload, availableTilePayloadSize, 0};
    BitReader& tileGroupReader = combinedFrameObu ? reader : tileReader;

    uint32_t tileGroupStart = 0;
    uint32_t tileGroupEnd = tileNum ? tileNum - 1 : 0;

    if (tileNum > 1) {
        uint8_t tileStartAndEndPresent = 0;

        if (!tileGroupReader.ReadFlag(tileStartAndEndPresent))
            return false;

        if (tileStartAndEndPresent) {
            const uint32_t tileBits = tileColsLog2 + tileRowsLog2;

            if (!tileGroupReader.ReadBits(tileBits, tileGroupStart) || !tileGroupReader.ReadBits(tileBits, tileGroupEnd))
                return false;

            if (tileGroupStart > tileGroupEnd || tileGroupEnd >= tileNum)
                return false;
        }
    }

    if (!tileGroupReader.ByteAlign())
        return false;

    const size_t tileDataOffset = tileGroupReader.ByteOffset();

    if (tileDataOffset >= fullTilePayloadSize)
        return false;

    uint32_t tileGroupTileNum = tileGroupEnd - tileGroupStart + 1;

    if (!tileGroupTileNum || tileGroupTileNum > std::size(info.tiles))
        return false;

    if (tileGroupStart != 0 || tileGroupEnd != tileNum - 1)
        return false;

    const uint32_t tileSizeByteNum = tileSizeBytesMinus1 + 1;
    auto parseTilePayload = [&]() {
        size_t tilePayloadCursor = tileDataOffset;

        for (uint32_t groupTileIndex = 0; groupTileIndex < tileGroupTileNum; groupTileIndex++) {
            const uint32_t tileIndex = tileGroupStart + groupTileIndex;
            uint32_t tileSize = 0;

            if (groupTileIndex + 1 < tileGroupTileNum) {
                if (tilePayloadCursor + tileSizeByteNum > availableTilePayloadSize)
                    return false;

                uint32_t tileSizeMinus1 = 0;

                for (uint32_t byteIndex = 0; byteIndex < tileSizeByteNum; byteIndex++)
                    tileSizeMinus1 |= uint32_t(tilePayload[tilePayloadCursor++]) << (8 * byteIndex);

                tileSize = tileSizeMinus1 + 1;
            } else {
                if (tilePayloadCursor > fullTilePayloadSize || fullTilePayloadSize - tilePayloadCursor > std::numeric_limits<uint32_t>::max())
                    return false;

                tileSize = (uint32_t)(fullTilePayloadSize - tilePayloadCursor);
            }

            if (tilePayloadCursor + tileSize > fullTilePayloadSize)
                return false;

            VideoAV1DecodeTileDesc& tile = info.tiles[groupTileIndex];
            tile.offset = (uint32_t)tilePayloadCursor;
            tile.size = tileSize;
            tile.row = (uint16_t)(tileIndex / tileCols);
            tile.column = (uint16_t)(tileIndex % tileCols);
            tile.anchorFrame = 0xFF;
            tilePayloadCursor += tileSize;
        }

        return tilePayloadCursor == fullTilePayloadSize;
    };

    if (!parseTilePayload())
        return false;

    if (fullTilePayloadSize > std::numeric_limits<uint32_t>::max())
        return false;

    info.sequence = sequence;

    if (!enableOrderHint)
        info.sequence.flags &= ~(VideoAV1SequenceBits::ENABLE_JNT_COMP | VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS);

    info.tileLayout.columnNum = (uint8_t)tileCols;
    info.tileLayout.rowNum = (uint8_t)tileRows;
    info.tileLayout.tileSizeBytesMinus1 = (uint8_t)tileSizeBytesMinus1;
    info.tileLayout.uniformSpacing = uniformTileSpacing;
    info.tileLayout.contextUpdateTileId = (uint16_t)contextUpdateTileId;
    info.picture.frameType = isKeyFrame ? VideoFrameType::IDR : VideoFrameType::P;
    info.picture.orderHint = (uint8_t)orderHint;
    info.picture.refreshFrameFlags = (uint8_t)refreshFrameFlags;
    info.picture.primaryReferenceName = GetReferenceNameFromReferenceIndex(primaryRefFrame);
    info.picture.flags = flags;
    info.picture.renderWidthMinus1 = (uint16_t)renderWidthMinus1;
    info.picture.renderHeightMinus1 = (uint16_t)renderHeightMinus1;
    info.picture.baseQIndex = (uint8_t)baseQIndex;
    info.picture.interpolationFilter = (uint8_t)interpolationFilter;
    info.picture.txMode = (uint8_t)txMode;
    info.picture.tileNum = tileGroupTileNum;

    VideoAV1SavedFrameState& savedFrameState = info.savedFrameState;
    savedFrameState.loopFilter = info.loopFilter;
    savedFrameState.segmentation = info.segmentation;
    savedFrameState.globalMotion = info.globalMotion;
    savedFrameState.filmGrain = info.filmGrain;
    savedFrameState.upscaledWidthMinus1 = (uint16_t)(upscaledWidth - 1);
    savedFrameState.frameHeightMinus1 = (uint16_t)(frameHeight - 1);
    savedFrameState.renderWidthMinus1 = (uint16_t)renderWidthMinus1;
    savedFrameState.renderHeightMinus1 = (uint16_t)renderHeightMinus1;
    savedFrameState.orderHint = (uint8_t)orderHint;
    savedFrameState.applyGrain = (flags & VideoAV1PictureBits::APPLY_GRAIN) ? 1 : 0;
    savedFrameState.valid = areLoopFilterDeltasKnown;

    BindPointers(info);

    return true;
}

static inline Result GetEncodeDecodeInfoFromHeader(const VideoAV1EncodeDecodeInfoDesc& desc, VideoAV1EncodeDecodeInfo& info) {
    const bool hasRequiredInput = desc.feedback && desc.sequence && desc.encodedPayloadHeader && desc.encodedPayloadHeaderSize && desc.feedback->encodedBitstreamWrittenBytes;

    if (!hasRequiredInput)
        return Result::INVALID_ARGUMENT;

    FramePayloadSpan frame = {};
    VideoAV1EncodeDecodeInfo parsedInfo = {};

    if (!FindFramePayload(desc.encodedPayloadHeader, (size_t)desc.encodedPayloadHeaderSize, frame))
        return Result::FAILURE;

    if (frame.headerPayloadOffset >= desc.encodedPayloadHeaderSize || frame.tilePayloadOffset >= desc.encodedPayloadHeaderSize || frame.tilePayloadOffset >= desc.feedback->encodedBitstreamWrittenBytes || !frame.tilePayloadSize)
        return Result::FAILURE;

    const size_t availablePayload = (size_t)desc.encodedPayloadHeaderSize - frame.headerPayloadOffset;
    const uint8_t* tilePayload = desc.encodedPayloadHeader + (frame.combinedFrameObu ? frame.headerPayloadOffset : frame.tilePayloadOffset);
    const size_t availableTilePayload = frame.combinedFrameObu ? availablePayload : (size_t)desc.encodedPayloadHeaderSize - frame.tilePayloadOffset;
    const size_t fullTilePayloadSize = frame.combinedFrameObu ? frame.headerPayloadSize : frame.tilePayloadSize;

    if (!ParseGeneratedFrameHeader(desc, desc.encodedPayloadHeader + frame.headerPayloadOffset, availablePayload, tilePayload, availableTilePayload, fullTilePayloadSize, frame.combinedFrameObu, parsedInfo))
        return Result::FAILURE;

    if (frame.combinedFrameObu) {
        parsedInfo.bitstreamOffset = frame.headerPayloadOffset;
        parsedInfo.bitstreamSize = frame.headerPayloadSize;
        parsedInfo.picture.frameHeaderOffset = 0;
    } else {
        parsedInfo.bitstreamOffset = frame.tilePayloadOffset;
        parsedInfo.bitstreamSize = frame.tilePayloadSize;
        parsedInfo.picture.frameHeaderOffset = 0;
    }

    if (parsedInfo.bitstreamOffset > desc.feedback->encodedBitstreamWrittenBytes || parsedInfo.bitstreamSize > desc.feedback->encodedBitstreamWrittenBytes - parsedInfo.bitstreamOffset)
        return Result::FAILURE;

    if (parsedInfo.bitstreamSize > std::numeric_limits<uint32_t>::max())

        return Result::FAILURE;

    info = parsedInfo;
    BindPointers(info);

    return Result::SUCCESS;
}

struct ObuBitWriter {
    bitstream::ByteWriter& bytes;
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
        const uint64_t codeNum = uint64_t(value) + 1u;
        uint32_t bitNum = 0;

        for (uint64_t temp = codeNum; temp; temp >>= 1)
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

static inline void WriteLeb128(bitstream::ByteWriter& bytes, uint64_t value) {
    do {
        uint8_t byte = uint8_t(value & 0x7F);
        value >>= 7;
        if (value)
            byte |= 0x80;
        bytes.WriteByte(byte);
    } while (value);
}

static inline void AppendObuHeader(bitstream::ByteWriter& bytes, ObuType type, uint64_t payloadSize) {
    bytes.WriteByte((uint8_t(type) << 3) | 0x2);
    WriteLeb128(bytes, payloadSize);
}

static inline bool IsIdentityColorConfig(const VideoAV1SequenceDesc& desc) {
    return (desc.flags & VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT) && desc.colorPrimaries == 1 && desc.transferCharacteristics == 13 && desc.matrixCoefficients == 0;
}

static inline void WriteColorConfig(ObuBitWriter& writer, const VideoAV1SequenceDesc& desc) {
    const bool highBitdepth = desc.bitDepth > 8;
    const bool twelveBit = desc.bitDepth > 10;
    const bool monochrome = !!(desc.flags & VideoAV1SequenceBits::MONO_CHROME);
    const bool colorDescriptionPresent = !!(desc.flags & VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT);

    writer.WriteBit(highBitdepth);

    if (desc.seqProfile == 2 && highBitdepth)
        writer.WriteBit(twelveBit);

    if (desc.seqProfile != PROFILE_HIGH)
        writer.WriteBit(monochrome);

    writer.WriteBit(colorDescriptionPresent);

    if (colorDescriptionPresent) {
        writer.WriteBits(desc.colorPrimaries, 8);
        writer.WriteBits(desc.transferCharacteristics, 8);
        writer.WriteBits(desc.matrixCoefficients, 8);
    }

    if (monochrome) {
        writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::COLOR_RANGE));

        return;
    }

    if (!IsIdentityColorConfig(desc)) {
        writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::COLOR_RANGE));

        if (desc.seqProfile == 2 && twelveBit) {
            writer.WriteBit(desc.subsamplingX);

            if (desc.subsamplingX)
                writer.WriteBit(desc.subsamplingY);
        }

        if (desc.subsamplingX && desc.subsamplingY)
            writer.WriteBits(desc.chromaSamplePosition, 2);
    }

    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q));
}

static inline Result WriteSequenceHeaderPayload(const VideoAV1SequenceDesc& desc, bitstream::ByteWriter& bytes) {
    constexpr uint8_t maxSequenceProfile = 2;

    if (desc.seqProfile > maxSequenceProfile || (desc.bitDepth != 8 && desc.bitDepth != 10 && desc.bitDepth != 12) || desc.frameWidthBitsMinus1 > 15 || desc.frameHeightBitsMinus1 > 15)

        return Result::INVALID_ARGUMENT;

    if ((uint32_t)desc.maxFrameWidthMinus1 >= (1u << (desc.frameWidthBitsMinus1 + 1u)) || (uint32_t)desc.maxFrameHeightMinus1 >= (1u << (desc.frameHeightBitsMinus1 + 1u)))

        return Result::INVALID_ARGUMENT;

    const bool reducedStillPictureHeader = !!(desc.flags & VideoAV1SequenceBits::REDUCED_STILL_PICTURE_HEADER);
    const bool stillPicture = !!(desc.flags & VideoAV1SequenceBits::STILL_PICTURE);
    const bool timingInfoPresent = !!(desc.flags & VideoAV1SequenceBits::TIMING_INFO_PRESENT);
    const bool initialDisplayDelayPresent = !!(desc.flags & VideoAV1SequenceBits::INITIAL_DISPLAY_DELAY_PRESENT);
    const bool enableOrderHint = !!(desc.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT);
    const bool frameIdNumbersPresent = !!(desc.flags & VideoAV1SequenceBits::FRAME_ID_NUMBERS_PRESENT);
    const bool monochrome = !!(desc.flags & VideoAV1SequenceBits::MONO_CHROME);
    const bool identityColorConfig = !monochrome && IsIdentityColorConfig(desc);

    if (desc.seqForceScreenContentTools > SELECT_SCREEN_CONTENT_TOOLS || desc.seqForceIntegerMv > SELECT_SCREEN_CONTENT_TOOLS || (enableOrderHint && desc.orderHintBitsMinus1 > 7))

        return Result::INVALID_ARGUMENT;

    if (desc.subsamplingX > 1 || desc.subsamplingY > 1 || desc.chromaSamplePosition > 3)

        return Result::INVALID_ARGUMENT;

    if ((desc.seqProfile == 0 && desc.bitDepth == 12) || (desc.seqProfile == 1 && (desc.bitDepth == 12 || monochrome || desc.subsamplingX || desc.subsamplingY)))

        return Result::INVALID_ARGUMENT;

    if (monochrome && (desc.subsamplingX != 1 || desc.subsamplingY != 1 || desc.chromaSamplePosition != 0 || (desc.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q)))

        return Result::INVALID_ARGUMENT;

    if (identityColorConfig && (!(desc.flags & VideoAV1SequenceBits::COLOR_RANGE) || desc.subsamplingX || desc.subsamplingY || desc.chromaSamplePosition))

        return Result::INVALID_ARGUMENT;

    if (!monochrome && !identityColorConfig && ((desc.seqProfile == 0 && (!desc.subsamplingX || !desc.subsamplingY)) || (desc.seqProfile == 2 && desc.bitDepth != 12 && (!desc.subsamplingX || desc.subsamplingY)) || (desc.seqProfile == 2 && desc.bitDepth == 12 && !desc.subsamplingX && desc.subsamplingY) || (!(desc.subsamplingX && desc.subsamplingY) && desc.chromaSamplePosition)))

        return Result::INVALID_ARGUMENT;

    if (reducedStillPictureHeader && (!stillPicture || timingInfoPresent || initialDisplayDelayPresent || frameIdNumbersPresent || enableOrderHint))
        return Result::INVALID_ARGUMENT;

    // "frame_id_length" (additional_frame_id_length_minus_1 + delta_frame_id_length_minus_2 + 3) must not exceed 16
    if (frameIdNumbersPresent && (desc.additionalFrameIdLengthMinus1 > 7 || (desc.additionalFrameIdLengthMinus1 + desc.deltaFrameIdLengthMinus2 + 3) > 16))
        return Result::INVALID_ARGUMENT;

    if (timingInfoPresent && (!desc.numUnitsInDisplayTick || !desc.timeScale || desc.numTicksPerPictureMinus1 == UINT32_MAX))
        return Result::INVALID_ARGUMENT;

    const bool selectScreenContentTools = desc.seqForceScreenContentTools == SELECT_SCREEN_CONTENT_TOOLS;
    const bool selectIntegerMv = desc.seqForceIntegerMv == SELECT_SCREEN_CONTENT_TOOLS;
    const uint8_t levelIndex = GetLevelIndex(desc.level, uint32_t(desc.maxFrameWidthMinus1) + 1u, uint32_t(desc.maxFrameHeightMinus1) + 1u);

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
    WriteColorConfig(writer, desc);
    writer.WriteBit(!!(desc.flags & VideoAV1SequenceBits::FILM_GRAIN_PARAMS_PRESENT));
    writer.FinishBits();

    return Result::SUCCESS;
}

static inline Result WriteSequenceHeaderObu(const VideoAV1SequenceDesc& desc, bitstream::ByteWriter& bytes) {
    bitstream::ByteWriter payloadCounter = {};
    Result result = WriteSequenceHeaderPayload(desc, payloadCounter);
    if (result != Result::SUCCESS)
        return result;

    AppendObuHeader(bytes, ObuType::SequenceHeader, payloadCounter.writtenSize);

    return WriteSequenceHeaderPayload(desc, bytes);
}

static inline Result WriteObuHeaders(const VideoAV1SequenceDesc& desc, bitstream::ByteWriter& bytes) {
    AppendObuHeader(bytes, ObuType::TemporalDelimiter, 0);
    return WriteSequenceHeaderObu(desc, bytes);
}

} // namespace av1

template <typename T, typename F>
static inline Result WriteSizedPayload(T& desc, const F& writePayload) {
    bitstream::ByteWriter byteCounter = {};
    Result result = writePayload(byteCounter);
    if (result != Result::SUCCESS)
        return result;

    desc.writtenSize = byteCounter.writtenSize;
    if (!desc.dst)
        return Result::SUCCESS;

    if (desc.dstSize < byteCounter.writtenSize)
        return Result::INVALID_ARGUMENT;

    bitstream::ByteWriter byteWriter = {desc.dst, desc.dstSize};
    result = writePayload(byteWriter);
    if (result != Result::SUCCESS)
        return result;

    return byteWriter.Finish(desc.writtenSize);
}

static inline Result WriteAnnexBParameterSets(VideoAnnexBParameterSetsDesc& desc) {
    return WriteSizedPayload(desc, [&](bitstream::ByteWriter& bytes) {
        if (desc.codec == VideoCodec::H264)
            return h264::WriteAnnexBParameterSets(desc, bytes);
        if (desc.codec == VideoCodec::H265)
            return h265::WriteAnnexBParameterSets(desc, bytes);

        return Result::UNSUPPORTED;
    });
}

static inline Result WriteAV1ObuHeaders(VideoAV1ObuHeadersDesc& desc) {
    return WriteSizedPayload(desc, [&](bitstream::ByteWriter& bytes) {
        return av1::WriteObuHeaders(desc.sequence, bytes);
    });
}

static inline Result WriteAnnexBEndOfStream(VideoAnnexBEndOfStreamDesc& desc) {
    return WriteSizedPayload(desc, [&](bitstream::ByteWriter& bytes) {
        if (desc.codec == VideoCodec::H264)
            return h264::WriteAnnexBEndOfStream(bytes);
        if (desc.codec == VideoCodec::H265)
            return h265::WriteAnnexBEndOfStream(bytes);

        return Result::UNSUPPORTED;
    });
}

} // namespace video

} // namespace nri
