// © 2026 NVIDIA Corporation

#pragma once

#include <limits>

namespace nri {

namespace video_av1 {

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
            if (!ReadBits(1, bit))
                return false;
        }
        return true;
    }

    size_t ByteOffset() const {
        return (bitOffset + 7) / 8;
    }
};

inline bool ReadLeb128(const uint8_t* data, size_t size, size_t& cursor, size_t& value) {
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

inline bool ReadObuHeader(const uint8_t* data, size_t size, size_t& cursor, ObuSpan& span) {
    if (!data || !size)
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
        cursor++;
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

inline bool FindFrameObu(const uint8_t* data, size_t size, ObuSpan& frame) {
    size_t cursor = 0;
    while (cursor < size) {
        ObuSpan span = {};
        if (!ReadObuHeader(data, size, cursor, span))
            return false;
        if (span.type == ObuType::Frame) {
            frame = span;
            return true;
        }
        if (span.type != ObuType::TemporalDelimiter && span.type != ObuType::SequenceHeader && span.type != ObuType::Padding)
            return false;
    }

    return false;
}

inline bool FindFramePayload(const uint8_t* data, size_t size, FramePayloadSpan& frame) {
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

inline bool PeekGeneratedFrameType(const uint8_t* payload, size_t availablePayloadSize, uint32_t& frameType, uint8_t& showFrame) {
    BitReader reader{payload, availablePayloadSize, 0};
    uint8_t showExistingFrame = 0;
    if (!reader.ReadFlag(showExistingFrame) || showExistingFrame || !reader.ReadBits(2, frameType) || !reader.ReadFlag(showFrame))
        return false;

    return true;
}

inline uint32_t TileLog2(uint32_t blockSize, uint32_t target);
inline bool ReadDeltaQ(BitReader& reader, int8_t& value);
inline void BindPointers(VideoAV1EncodeDecodeInfo& info);
inline void FillIdentityGlobalMotion(VideoAV1GlobalMotionDesc& globalMotion);
inline void FillSingleTileLayout(VideoAV1EncodeDecodeInfo& info, uint32_t width, uint32_t height);

inline VideoAV1ReferenceName GetReferenceNameFromReferenceIndex(uint32_t referenceIndex) {
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

inline const VideoAV1ReferenceDesc* FindReferenceByRefFrameIndex(const VideoAV1ReferenceDesc* references, uint32_t referenceNum, uint32_t refFrameIndex) {
    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].refFrameIndex == refFrameIndex)
            return references + i;
    }

    return nullptr;
}

inline bool BuildInterFrameReferences(const VideoAV1EncodeDecodeInfoDesc& desc, const std::array<uint8_t, 7>& refFrameIndices, VideoAV1EncodeDecodeInfo& info) {
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

    info.picture.references = info.references;
    info.picture.referenceNum = referenceNum;
    return true;
}

inline bool ParseGeneratedInterFrameHeader(const uint8_t* payload, size_t availablePayloadSize, size_t fullPayloadSize, bool requireTilePayload,
    const VideoAV1SequenceDesc& sequence,
    std::array<uint8_t, 7>& refFrameIndices, VideoAV1EncodeDecodeInfo& info) {
    BitReader reader{payload, availablePayloadSize, 0};
    VideoAV1PictureBits flags = VideoAV1PictureBits::SHOW_FRAME | VideoAV1PictureBits::SHOWABLE_FRAME;

    uint8_t showExistingFrame = 0;
    uint32_t frameType = 0;
    uint8_t showFrame = 0;
    uint8_t errorResilient = 0;
    uint8_t disableCdfUpdate = 0;
    uint8_t allowScreenContentTools = 0;
    uint8_t forceIntegerMv = 0;
    uint8_t frameSizeOverride = 0;
    uint32_t orderHint = 0;
    uint32_t primaryRefFrame = 0;
    uint32_t refreshFrameFlags = 0;
    uint32_t ignored = 0;
    if (!reader.ReadFlag(showExistingFrame) || showExistingFrame || !reader.ReadBits(2, frameType) || !reader.ReadFlag(showFrame) || frameType != 1 || !showFrame)
        return false;
    if (!reader.ReadFlag(errorResilient) || errorResilient || !reader.ReadFlag(disableCdfUpdate))
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
        if (forceIntegerMv)
            flags |= VideoAV1PictureBits::FORCE_INTEGER_MV;
    }

    if (sequence.flags & VideoAV1SequenceBits::FRAME_ID_NUMBERS_PRESENT) {
        uint32_t currentFrameId = 0;
        const uint32_t frameIdBits = sequence.additionalFrameIdLengthMinus1 + sequence.deltaFrameIdLengthMinus2 + 3;

        if (!reader.ReadBits(frameIdBits, currentFrameId))

            return false;

        info.picture.currentFrameId = currentFrameId;
    }

    if (!reader.ReadFlag(frameSizeOverride))
        return false;

    if (sequence.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT) {
        if (!reader.ReadBits(sequence.orderHintBitsMinus1 + 1, orderHint))
            return false;
    }
    if (!reader.ReadBits(3, primaryRefFrame) || !reader.ReadBits(8, refreshFrameFlags))
        return false;

    if (sequence.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT) {
        uint8_t frameRefsShortSignaling = 0;
        if (!reader.ReadFlag(frameRefsShortSignaling) || frameRefsShortSignaling)
            return false;
    }
    for (uint32_t i = 0; i < 7; i++) {
        if (!reader.ReadBits(3, ignored))
            return false;
        refFrameIndices[i] = (uint8_t)ignored;
    }
    if (frameSizeOverride)
        return false;

    const uint32_t width = sequence.maxFrameWidthMinus1 + 1;
    const uint32_t height = sequence.maxFrameHeightMinus1 + 1;
    if (sequence.flags & VideoAV1SequenceBits::ENABLE_SUPERRES) {
        uint8_t useSuperres = 0;
        if (!reader.ReadFlag(useSuperres))
            return false;
        if (useSuperres) {
            uint32_t codedDenom = 0;
            if (!reader.ReadBits(3, codedDenom))
                return false;
            info.picture.codedDenom = (uint8_t)codedDenom;
            info.picture.superresDenom = (uint8_t)(codedDenom + 9);
            flags |= VideoAV1PictureBits::USE_SUPERRES;
        }
    }
    uint32_t renderWidthMinus1 = width - 1;
    uint32_t renderHeightMinus1 = height - 1;
    uint8_t renderAndFrameSizeDifferent = 0;
    if (!reader.ReadFlag(renderAndFrameSizeDifferent))
        return false;
    if (renderAndFrameSizeDifferent) {
        if (!reader.ReadBits(16, renderWidthMinus1) || !reader.ReadBits(16, renderHeightMinus1))
            return false;
        flags |= VideoAV1PictureBits::RENDER_AND_FRAME_SIZE_DIFFERENT;
    }

    uint8_t allowHighPrecisionMv = 0;
    if (!allowScreenContentTools && !reader.ReadFlag(allowHighPrecisionMv))
        return false;
    if (allowHighPrecisionMv)
        flags |= VideoAV1PictureBits::ALLOW_HIGH_PRECISION_MV;
    uint8_t isFilterSwitchable = 0;
    if (!reader.ReadFlag(isFilterSwitchable))
        return false;
    if (isFilterSwitchable)
        flags |= VideoAV1PictureBits::IS_FILTER_SWITCHABLE;
    uint32_t interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP;
    if (isFilterSwitchable)
        interpolationFilter = INTERPOLATION_FILTER_SWITCHABLE;
    else if (!reader.ReadBits(2, interpolationFilter))
        return false;
    uint8_t isMotionModeSwitchable = 0;
    if (!reader.ReadFlag(isMotionModeSwitchable))
        return false;
    if (isMotionModeSwitchable)
        flags |= VideoAV1PictureBits::IS_MOTION_MODE_SWITCHABLE;
    if (sequence.flags & VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS) {
        uint8_t useRefFrameMvs = 0;
        if (!reader.ReadFlag(useRefFrameMvs))
            return false;
        if (useRefFrameMvs)
            flags |= VideoAV1PictureBits::USE_REF_FRAME_MVS;
    }
    if (!disableCdfUpdate) {
        uint8_t disableFrameEndUpdateCdf = 0;
        if (!reader.ReadFlag(disableFrameEndUpdateCdf))
            return false;
        if (disableFrameEndUpdateCdf)
            flags |= VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF;
    }

    const uint32_t miCols = 2 * ((width + 7) >> 3);
    const uint32_t miRows = 2 * ((height + 7) >> 3);
    const uint32_t sbShift = 4;
    const uint32_t sbSize = sbShift + 2;
    const uint32_t sbCols = (miCols + 15) >> 4;
    const uint32_t sbRows = (miRows + 15) >> 4;
    const uint32_t minLog2TileCols = TileLog2(4096 >> sbSize, sbCols);
    const uint32_t maxLog2TileCols = TileLog2(1, std::min(sbCols, 64u));
    const uint32_t maxLog2TileRows = TileLog2(1, std::min(sbRows, 64u));
    const uint32_t minLog2Tiles = std::max(minLog2TileCols, TileLog2((4096 * 2304) >> (2 * sbSize), sbRows * sbCols));
    uint8_t uniformTileSpacing = 0;
    if (!reader.ReadFlag(uniformTileSpacing) || !uniformTileSpacing)
        return false;
    uint32_t tileColsLog2 = 0;
    uint32_t tileRowsLog2 = 0;
    if (!reader.ReadIncrement(minLog2TileCols, maxLog2TileCols, tileColsLog2))
        return false;
    const uint32_t minLog2TileRows = std::max<int32_t>((int32_t)minLog2Tiles - (int32_t)tileColsLog2, 0);
    if (!reader.ReadIncrement(minLog2TileRows, maxLog2TileRows, tileRowsLog2))
        return false;
    if (tileColsLog2 || tileRowsLog2)
        return false;

    uint32_t baseQIndex = 0;
    uint8_t usingQmatrix = 0;
    int8_t deltaQYDc = 0;
    int8_t deltaQUDc = 0;
    int8_t deltaQUAc = 0;

    if (!reader.ReadBits(8, baseQIndex) || !ReadDeltaQ(reader, deltaQYDc) || !ReadDeltaQ(reader, deltaQUDc) || !ReadDeltaQ(reader, deltaQUAc) || !reader.ReadFlag(usingQmatrix))
        return false;

    uint32_t qmY = 0;
    uint32_t qmU = 0;
    uint32_t qmV = 0;

    if (usingQmatrix) {

        if (!reader.ReadBits(4, qmY) || !reader.ReadBits(4, qmU))

            return false;

        if (sequence.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q) {

            if (!reader.ReadBits(4, qmV))

                return false;
        } else {
            qmV = qmU;
        }
    }

    uint8_t segmentationEnabled = 0;

    if (!reader.ReadFlag(segmentationEnabled) || segmentationEnabled)
        return false;
    uint32_t deltaQRes = 0;
    if (baseQIndex) {
        uint8_t deltaQPresent = 0;
        if (!reader.ReadFlag(deltaQPresent))
            return false;
        if (deltaQPresent) {
            if (!reader.ReadBits(2, deltaQRes))
                return false;
            flags |= VideoAV1PictureBits::DELTA_Q_PRESENT;
            uint8_t deltaLfPresent = 0;
            if (!reader.ReadFlag(deltaLfPresent) || deltaLfPresent)
                return false;
        }
    }

    const bool codedLossless = baseQIndex == 0 && deltaQYDc == 0 && deltaQUDc == 0 && deltaQUAc == 0;
    uint32_t loopFilterLevel0 = 0;
    uint32_t loopFilterLevel1 = 0;
    uint32_t loopFilterLevelU = 0;
    uint32_t loopFilterLevelV = 0;
    uint32_t loopFilterSharpness = 0;
    uint32_t value = 0;
    if (!codedLossless) {
        if (!reader.ReadBits(6, loopFilterLevel0) || !reader.ReadBits(6, loopFilterLevel1))
            return false;
        if (loopFilterLevel0 || loopFilterLevel1) {
            if (!reader.ReadBits(6, loopFilterLevelU) || !reader.ReadBits(6, loopFilterLevelV))
                return false;
        }
        uint8_t loopFilterDeltaEnabled = 0;
        if (!reader.ReadBits(3, loopFilterSharpness) || !reader.ReadFlag(loopFilterDeltaEnabled) || loopFilterDeltaEnabled)
            return false;
    }
    uint32_t cdefDampingMinus3 = 0;
    uint32_t cdefBits = 0;
    std::array<uint8_t, 8> cdefYPrimaryStrength = {};
    std::array<uint8_t, 8> cdefYSecondaryStrength = {};
    std::array<uint8_t, 8> cdefUvPrimaryStrength = {};
    std::array<uint8_t, 8> cdefUvSecondaryStrength = {};
    if ((sequence.flags & VideoAV1SequenceBits::ENABLE_CDEF) && !codedLossless) {
        if (!reader.ReadBits(2, cdefDampingMinus3) || !reader.ReadBits(2, cdefBits))
            return false;
        for (uint32_t i = 0; i < (1u << cdefBits); i++) {
            if (!reader.ReadBits(4, value))
                return false;
            cdefYPrimaryStrength[i] = (uint8_t)value;
            if (!reader.ReadBits(2, value))
                return false;
            cdefYSecondaryStrength[i] = (uint8_t)(value == 3 ? 4 : value);
            if (!reader.ReadBits(4, value))
                return false;
            cdefUvPrimaryStrength[i] = (uint8_t)value;
            if (!reader.ReadBits(2, value))
                return false;
            cdefUvSecondaryStrength[i] = (uint8_t)(value == 3 ? 4 : value);
        }
    }
    std::array<uint8_t, 3> restorationTypes = {};
    if ((sequence.flags & VideoAV1SequenceBits::ENABLE_RESTORATION) && !codedLossless) {
        for (uint32_t i = 0; i < 3; i++) {
            if (!reader.ReadBits(2, value) || value)
                return false;
            restorationTypes[i] = (uint8_t)value;
        }
    }
    uint32_t txMode = TX_MODE_ONLY_4X4;
    if (!codedLossless) {
        if (!reader.ReadIncrement(TX_MODE_LARGEST, TX_MODE_SELECT, txMode))
            return false;
    }
    uint8_t referenceSelect = 0;
    if (!reader.ReadFlag(referenceSelect))
        return false;
    if (referenceSelect)
        flags |= VideoAV1PictureBits::REFERENCE_SELECT;
    for (uint32_t i = 0; i < 7; i++) {
        uint8_t isGlobal = 0;
        if (!reader.ReadFlag(isGlobal) || isGlobal)
            return false;
    }
    uint8_t reducedTxSet = 0;
    if (!reader.ReadFlag(reducedTxSet) || !reader.ByteAlign())
        return false;
    if (reducedTxSet)
        flags |= VideoAV1PictureBits::REDUCED_TX_SET;

    const size_t tileDataOffset = reader.ByteOffset();

    if (requireTilePayload && tileDataOffset >= fullPayloadSize)
        return false;

    if (requireTilePayload) {

        if (fullPayloadSize - tileDataOffset > std::numeric_limits<uint32_t>::max())

            return false;

        info.bitstreamOffset = tileDataOffset;
        info.bitstreamSize = fullPayloadSize - tileDataOffset;
    }

    info.sequence = sequence;
    FillSingleTileLayout(info, width, height);
    info.picture.tileNum = 1;

    if (requireTilePayload)
        info.tiles[0] = {(uint32_t)tileDataOffset, (uint32_t)(fullPayloadSize - tileDataOffset), 0, 0, 0xFF};

    info.picture.frameType = VideoEncodeFrameType::P;
    info.picture.orderHint = (uint8_t)orderHint;
    info.picture.refreshFrameFlags = (uint8_t)refreshFrameFlags;
    info.picture.primaryReferenceName = GetReferenceNameFromReferenceIndex(primaryRefFrame);
    info.picture.flags = flags;
    info.picture.renderWidthMinus1 = (uint16_t)renderWidthMinus1;
    info.picture.renderHeightMinus1 = (uint16_t)renderHeightMinus1;
    info.picture.baseQIndex = (uint8_t)baseQIndex;
    info.picture.interpolationFilter = (uint8_t)interpolationFilter;
    info.picture.txMode = (uint8_t)txMode;
    info.picture.cdefDampingMinus3 = (uint8_t)cdefDampingMinus3;
    info.picture.cdefBits = (uint8_t)cdefBits;
    info.picture.deltaQRes = (uint8_t)deltaQRes;
    info.tileLayout.contextUpdateTileId = 0;
    info.quantization.deltaQYDc = deltaQYDc;
    info.quantization.deltaQUDc = deltaQUDc;
    info.quantization.deltaQUAc = deltaQUAc;
    info.quantization.deltaQVDc = deltaQUDc;
    info.quantization.deltaQVAc = deltaQUAc;
    info.quantization.usingQmatrix = usingQmatrix;
    info.quantization.qmY = (uint8_t)qmY;
    info.quantization.qmU = (uint8_t)qmU;
    info.quantization.qmV = (uint8_t)qmV;
    info.loopFilter.level[0] = (uint8_t)loopFilterLevel0;
    info.loopFilter.level[1] = (uint8_t)loopFilterLevel1;
    info.loopFilter.level[2] = (uint8_t)loopFilterLevelU;
    info.loopFilter.level[3] = (uint8_t)loopFilterLevelV;
    info.loopFilter.sharpness = (uint8_t)loopFilterSharpness;
    info.loopFilter.refDeltas[0] = 1;
    info.loopFilter.refDeltas[4] = -1;
    info.loopFilter.refDeltas[6] = -1;
    info.loopFilter.refDeltas[7] = -1;
    for (uint32_t i = 0; i < 8; i++) {
        info.cdef.yPrimaryStrength[i] = cdefYPrimaryStrength[i];
        info.cdef.ySecondaryStrength[i] = cdefYSecondaryStrength[i];
        info.cdef.uvPrimaryStrength[i] = cdefUvPrimaryStrength[i];
        info.cdef.uvSecondaryStrength[i] = cdefUvSecondaryStrength[i];
    }
    for (uint32_t i = 0; i < 3; i++)
        info.loopRestoration.frameRestorationType[i] = restorationTypes[i];
    FillIdentityGlobalMotion(info.globalMotion);
    BindPointers(info);
    return true;
}

inline uint32_t TileLog2(uint32_t blockSize, uint32_t target) {
    uint32_t value = 0;
    while ((blockSize << value) < target)
        ++value;
    return value;
}

inline bool ReadNs(BitReader& reader, uint32_t n, uint32_t& value) {
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

inline bool ReadDeltaQ(BitReader& reader, int8_t& value) {
    uint8_t deltaCoded = 0;
    if (!reader.ReadFlag(deltaCoded))
        return false;
    if (!deltaCoded) {
        value = 0;
        return true;
    }
    return reader.ReadSigned(7, value);
}

inline void BindPointers(VideoAV1EncodeDecodeInfo& info) {
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
    info.picture.tiles = info.tiles;
    info.picture.references = info.references;
}

inline void FillIdentityGlobalMotion(VideoAV1GlobalMotionDesc& globalMotion) {
    for (auto& params : globalMotion.params) {
        params[2] = 1 << 16;
        params[5] = 1 << 16;
    }
}

inline void FillSingleTileLayout(VideoAV1EncodeDecodeInfo& info, uint32_t width, uint32_t height) {
    const uint32_t miCols = 2 * ((width + 7) >> 3);
    const uint32_t miRows = 2 * ((height + 7) >> 3);
    const uint32_t sbCols = (miCols + 15) >> 4;
    const uint32_t sbRows = (miRows + 15) >> 4;

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

inline bool ParseGeneratedKeyFrameHeader(const uint8_t* payload, size_t availablePayloadSize, const uint8_t* tilePayload, size_t availableTilePayloadSize,
    size_t fullTilePayloadSize, bool combinedFrameObu, const VideoAV1SequenceDesc& sequence, VideoAV1EncodeDecodeInfo& info) {
    constexpr uint32_t MAX_TILE_WIDTH = 4096;
    constexpr uint32_t MAX_TILE_AREA = 4096 * 2304;
    constexpr uint32_t MAX_TILE_COLS = 64;
    constexpr uint32_t MAX_TILE_ROWS = 64;

    BitReader reader{payload, availablePayloadSize, 0};
    VideoAV1PictureBits flags = VideoAV1PictureBits::NONE;

    uint8_t showExistingFrame = 0;
    uint32_t frameType = 0;
    uint8_t showFrame = 0;

    if (!reader.ReadFlag(showExistingFrame) || showExistingFrame || !reader.ReadBits(2, frameType) || !reader.ReadFlag(showFrame))
        return false;

    if (frameType != 0 || !showFrame)
        return false;

    flags |= VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::SHOW_FRAME;

    uint8_t disableCdfUpdate = 0;
    uint8_t allowScreenContentTools = 0;
    uint8_t forceIntegerMv = 0;
    if (!reader.ReadFlag(disableCdfUpdate))
        return false;
    if (sequence.seqForceScreenContentTools == SELECT_SCREEN_CONTENT_TOOLS) {
        if (!reader.ReadFlag(allowScreenContentTools))
            return false;
    } else
        allowScreenContentTools = sequence.seqForceScreenContentTools;
    if (disableCdfUpdate)
        flags |= VideoAV1PictureBits::DISABLE_CDF_UPDATE | VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF;
    if (allowScreenContentTools) {
        flags |= VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS;
        if (sequence.seqForceIntegerMv == SELECT_SCREEN_CONTENT_TOOLS) {
            if (!reader.ReadFlag(forceIntegerMv))
                return false;
        } else
            forceIntegerMv = sequence.seqForceIntegerMv;
        if (forceIntegerMv)
            flags |= VideoAV1PictureBits::FORCE_INTEGER_MV;
    }

    if (sequence.flags & VideoAV1SequenceBits::FRAME_ID_NUMBERS_PRESENT) {
        uint32_t currentFrameId = 0;
        const uint32_t frameIdBits = sequence.additionalFrameIdLengthMinus1 + sequence.deltaFrameIdLengthMinus2 + 3;

        if (!reader.ReadBits(frameIdBits, currentFrameId))

            return false;

        info.picture.currentFrameId = currentFrameId;
    }

    uint8_t frameSizeOverride = 0;
    uint32_t orderHint = 0;
    if (!reader.ReadFlag(frameSizeOverride))
        return false;
    if (sequence.flags & VideoAV1SequenceBits::ENABLE_ORDER_HINT) {
        if (!reader.ReadBits(sequence.orderHintBitsMinus1 + 1, orderHint))
            return false;
    }
    if (frameSizeOverride) {
        flags |= VideoAV1PictureBits::FRAME_SIZE_OVERRIDE;
        uint32_t ignored = 0;
        if (!reader.ReadBits(sequence.frameWidthBitsMinus1 + 1, ignored) || !reader.ReadBits(sequence.frameHeightBitsMinus1 + 1, ignored))
            return false;
    }

    const uint32_t width = sequence.maxFrameWidthMinus1 + 1;
    const uint32_t height = sequence.maxFrameHeightMinus1 + 1;
    if (sequence.flags & VideoAV1SequenceBits::ENABLE_SUPERRES) {
        uint8_t useSuperres = 0;
        if (!reader.ReadFlag(useSuperres))
            return false;
        if (useSuperres) {
            uint32_t codedDenom = 0;
            if (!reader.ReadBits(3, codedDenom))
                return false;
            info.picture.codedDenom = (uint8_t)codedDenom;
            info.picture.superresDenom = (uint8_t)(codedDenom + 9);
            flags |= VideoAV1PictureBits::USE_SUPERRES;
        }
    }

    uint32_t renderWidthMinus1 = width - 1;
    uint32_t renderHeightMinus1 = height - 1;
    uint8_t renderAndFrameSizeDifferent = 0;
    if (!reader.ReadFlag(renderAndFrameSizeDifferent))
        return false;
    if (renderAndFrameSizeDifferent) {
        if (!reader.ReadBits(16, renderWidthMinus1) || !reader.ReadBits(16, renderHeightMinus1))
            return false;
        flags |= VideoAV1PictureBits::RENDER_AND_FRAME_SIZE_DIFFERENT;
    }

    uint8_t allowIntrabc = 0;
    if (allowScreenContentTools && !reader.ReadFlag(allowIntrabc))
        return false;
    if (allowIntrabc)
        flags |= VideoAV1PictureBits::ALLOW_INTRABC;

    if (!disableCdfUpdate) {
        uint8_t disableFrameEndUpdateCdf = 0;
        if (!reader.ReadFlag(disableFrameEndUpdateCdf))
            return false;
        if (disableFrameEndUpdateCdf)
            flags |= VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF;
    }

    const uint32_t miCols = 2 * ((width + 7) >> 3);
    const uint32_t miRows = 2 * ((height + 7) >> 3);
    const uint32_t sbShift = 4;
    const uint32_t sbSize = sbShift + 2;
    const uint32_t sbCols = (miCols + 15) >> 4;
    const uint32_t sbRows = (miRows + 15) >> 4;
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

    uint32_t baseQIndex = 0;
    uint8_t usingQmatrix = 0;

    if (!reader.ReadBits(8, baseQIndex) || !ReadDeltaQ(reader, info.quantization.deltaQYDc) || !ReadDeltaQ(reader, info.quantization.deltaQUDc) || !ReadDeltaQ(reader, info.quantization.deltaQUAc) || !reader.ReadFlag(usingQmatrix))
        return false;

    info.quantization.deltaQVDc = info.quantization.deltaQUDc;
    info.quantization.deltaQVAc = info.quantization.deltaQUAc;
    info.quantization.usingQmatrix = usingQmatrix;

    if (usingQmatrix) {
        uint32_t qmY = 0;
        uint32_t qmU = 0;
        uint32_t qmV = 0;

        if (!reader.ReadBits(4, qmY) || !reader.ReadBits(4, qmU))

            return false;

        if (sequence.flags & VideoAV1SequenceBits::SEPARATE_UV_DELTA_Q) {

            if (!reader.ReadBits(4, qmV))

                return false;
        } else {
            qmV = qmU;
        }

        info.quantization.qmY = (uint8_t)qmY;
        info.quantization.qmU = (uint8_t)qmU;
        info.quantization.qmV = (uint8_t)qmV;
    }

    uint8_t segmentationEnabled = 0;

    if (!reader.ReadFlag(segmentationEnabled))
        return false;

    if (segmentationEnabled)
        return false;

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
        }
    }

    if (flags & VideoAV1PictureBits::DELTA_Q_PRESENT) {
        uint8_t deltaLfPresent = 0;

        if (!allowIntrabc && !reader.ReadFlag(deltaLfPresent))
            return false;

        if (deltaLfPresent)
            return false;
    }

    const bool codedLossless = baseQIndex == 0 && info.quantization.deltaQYDc == 0 && info.quantization.deltaQUDc == 0 && info.quantization.deltaQUAc == 0;
    info.loopFilter.refDeltas[0] = 1;
    info.loopFilter.refDeltas[4] = -1;
    info.loopFilter.refDeltas[6] = -1;
    info.loopFilter.refDeltas[7] = -1;

    if (!codedLossless && !allowIntrabc) {
        uint32_t value = 0;

        if (!reader.ReadBits(6, value))
            return false;
        info.loopFilter.level[0] = (uint8_t)value;

        if (!reader.ReadBits(6, value))
            return false;
        info.loopFilter.level[1] = (uint8_t)value;

        if (info.loopFilter.level[0] || info.loopFilter.level[1]) {
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

                if (updateModeDelta) {
                    int8_t modeDelta = 0;

                    if (!reader.ReadSigned(7, modeDelta))

                        return false;

                    info.loopFilter.modeDeltas[i] = modeDelta;
                }
            }
        }
    }

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
            info.cdef.ySecondaryStrength[i] = (uint8_t)(value == 3 ? 4 : value);
            if (!reader.ReadBits(4, value))
                return false;
            info.cdef.uvPrimaryStrength[i] = (uint8_t)value;
            if (!reader.ReadBits(2, value))
                return false;
            info.cdef.uvSecondaryStrength[i] = (uint8_t)(value == 3 ? 4 : value);
        }
    }

    if ((sequence.flags & VideoAV1SequenceBits::ENABLE_RESTORATION) && !codedLossless && !allowIntrabc) {
        uint32_t restorationType = 0;

        for (uint32_t plane = 0; plane < 3; plane++) {
            if (!reader.ReadBits(2, restorationType))
                return false;
            info.loopRestoration.frameRestorationType[plane] = (uint8_t)restorationType;
        }
    }

    if (!codedLossless) {
        uint32_t txMode = TX_MODE_ONLY_4X4;

        if (!reader.ReadIncrement(TX_MODE_LARGEST, TX_MODE_SELECT, txMode))
            return false;

        info.picture.txMode = (uint8_t)txMode;
    }
    uint8_t reducedTxSet = 0;

    if (!reader.ReadFlag(reducedTxSet))
        return false;

    if (reducedTxSet)
        flags |= VideoAV1PictureBits::REDUCED_TX_SET;

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

    info.sequence = sequence;

    if (fullTilePayloadSize > std::numeric_limits<uint32_t>::max())
        return false;

    info.tileLayout.columnNum = (uint8_t)tileCols;
    info.tileLayout.rowNum = (uint8_t)tileRows;
    info.tileLayout.tileSizeBytesMinus1 = (uint8_t)tileSizeBytesMinus1;
    info.tileLayout.uniformSpacing = uniformTileSpacing;
    info.tileLayout.contextUpdateTileId = (uint16_t)contextUpdateTileId;
    info.picture.frameType = VideoEncodeFrameType::IDR;
    info.picture.orderHint = (uint8_t)orderHint;
    info.picture.refreshFrameFlags = 0xFF;
    info.picture.primaryReferenceName = VideoAV1ReferenceName::NONE;
    info.picture.flags = flags;
    info.picture.renderWidthMinus1 = (uint16_t)renderWidthMinus1;
    info.picture.renderHeightMinus1 = (uint16_t)renderHeightMinus1;
    info.picture.baseQIndex = (uint8_t)baseQIndex;
    info.picture.interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP;
    info.picture.tileNum = tileGroupTileNum;
    FillIdentityGlobalMotion(info.globalMotion);
    BindPointers(info);

    return true;
}

inline Result GetVideoEncodeAV1DecodeInfoFromHeader(const VideoAV1EncodeDecodeInfoDesc& desc, VideoAV1EncodeDecodeInfo& info) {
    if (!desc.feedback || !desc.sequence || !desc.encodedPayloadHeader || !desc.encodedPayloadHeaderSize || !desc.feedback->encodedBitstreamWrittenBytes)
        return Result::INVALID_ARGUMENT;

    FramePayloadSpan frame = {};

    if (!FindFramePayload(desc.encodedPayloadHeader, (size_t)desc.encodedPayloadHeaderSize, frame))
        return Result::FAILURE;

    if (frame.headerPayloadOffset >= desc.encodedPayloadHeaderSize || frame.tilePayloadOffset >= desc.encodedPayloadHeaderSize || frame.tilePayloadOffset >= desc.feedback->encodedBitstreamWrittenBytes || !frame.tilePayloadSize)
        return Result::FAILURE;

    const size_t availablePayload = (size_t)desc.encodedPayloadHeaderSize - frame.headerPayloadOffset;
    const uint8_t* tilePayload = desc.encodedPayloadHeader + (frame.combinedFrameObu ? frame.headerPayloadOffset : frame.tilePayloadOffset);
    const size_t availableTilePayload = frame.combinedFrameObu ? availablePayload : (size_t)desc.encodedPayloadHeaderSize - frame.tilePayloadOffset;
    const size_t fullTilePayloadSize = frame.combinedFrameObu ? frame.headerPayloadSize : frame.tilePayloadSize;
    const size_t fullHeaderPayloadSize = frame.headerPayloadSize;

    if (!ParseGeneratedKeyFrameHeader(desc.encodedPayloadHeader + frame.headerPayloadOffset, availablePayload, tilePayload, availableTilePayload, fullTilePayloadSize, frame.combinedFrameObu, *desc.sequence, info)) {
        uint32_t frameType = 0;
        uint8_t showFrame = 0;

        if (!PeekGeneratedFrameType(desc.encodedPayloadHeader + frame.headerPayloadOffset, availablePayload, frameType, showFrame) || frameType != 1 || !showFrame)
            return Result::FAILURE;

        info = {};
        std::array<uint8_t, 7> refFrameIndices = {};

        if (!ParseGeneratedInterFrameHeader(desc.encodedPayloadHeader + frame.headerPayloadOffset, availablePayload, fullHeaderPayloadSize, frame.combinedFrameObu, *desc.sequence, refFrameIndices, info))
            return Result::FAILURE;

        if (!BuildInterFrameReferences(desc, refFrameIndices, info))
            return Result::FAILURE;

        if (!frame.combinedFrameObu) {

            if (frame.tilePayloadSize > std::numeric_limits<uint32_t>::max())

                return Result::FAILURE;

            info.picture.tileNum = 1;
            info.tiles[0] = {0, (uint32_t)frame.tilePayloadSize, 0, 0, 0xFF};
        }
    }

    if (frame.combinedFrameObu) {
        info.bitstreamOffset = frame.headerPayloadOffset;
        info.bitstreamSize = frame.headerPayloadSize;
        info.picture.frameHeaderOffset = 0;
    } else {
        info.bitstreamOffset = frame.tilePayloadOffset;
        info.bitstreamSize = frame.tilePayloadSize;
        info.picture.frameHeaderOffset = 0;
    }

    if (info.bitstreamOffset > desc.feedback->encodedBitstreamWrittenBytes || info.bitstreamSize > desc.feedback->encodedBitstreamWrittenBytes - info.bitstreamOffset)
        return Result::FAILURE;

    if (info.bitstreamSize > std::numeric_limits<uint32_t>::max())

        return Result::FAILURE;

    return Result::SUCCESS;
}

} // namespace video_av1

} // namespace nri
