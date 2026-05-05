// © 2026 NVIDIA Corporation

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace nri {

namespace video_av1 {

constexpr uint32_t SELECT_SCREEN_CONTENT_TOOLS = 2;
constexpr uint32_t PROFILE_HIGH = 1;
constexpr uint8_t INTERPOLATION_FILTER_EIGHTTAP = 0;
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

inline bool ParseGeneratedKeyFrameHeader(const uint8_t* payload, size_t availablePayloadSize, size_t fullPayloadSize, const VideoAV1SequenceDesc& sequence,
    VideoAV1EncodeDecodeInfo& info) {
    constexpr uint32_t MAX_TILE_WIDTH = 4096;
    constexpr uint32_t MAX_TILE_AREA = 4096 * 2304;
    constexpr uint32_t MAX_TILE_COLS = 64;
    constexpr uint32_t MAX_TILE_ROWS = 64;

    BitReader reader {payload, availablePayloadSize, 0};
    VideoAV1PictureBits flags = VideoAV1PictureBits::NONE;

    uint8_t showExistingFrame = 0;
    uint32_t frameType = 0;
    uint8_t showFrame = 0;
    if (!reader.ReadFlag(showExistingFrame) || showExistingFrame || !reader.ReadBits(2, frameType) || !reader.ReadFlag(showFrame))
        return false;
    if (frameType != 0 || !showFrame)
        return false;

    flags |= VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::FORCE_INTEGER_MV | VideoAV1PictureBits::SHOW_FRAME;

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
    } else
        return false;

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
    if (!reader.ReadBits(8, baseQIndex) || !ReadDeltaQ(reader, info.quantization.deltaQYDc) || !ReadDeltaQ(reader, info.quantization.deltaQUDc) ||
        !ReadDeltaQ(reader, info.quantization.deltaQUAc) || !reader.ReadFlag(usingQmatrix))
        return false;
    info.quantization.deltaQVDc = info.quantization.deltaQUDc;
    info.quantization.deltaQVAc = info.quantization.deltaQUAc;
    info.quantization.usingQmatrix = usingQmatrix;
    if (usingQmatrix)
        return false;

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
        if (info.loopFilter.deltaEnabled)
            return false;
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
        uint8_t txModeSelect = 0;
        if (!reader.ReadFlag(txModeSelect))
            return false;
        info.picture.txMode = txModeSelect ? TX_MODE_SELECT : TX_MODE_LARGEST;
    }
    uint8_t reducedTxSet = 0;
    if (!reader.ReadFlag(reducedTxSet))
        return false;
    if (reducedTxSet)
        flags |= VideoAV1PictureBits::REDUCED_TX_SET;
    if (!reader.ByteAlign())
        return false;

    if (tileCols != 1 || tileRows != 1)
        return false;

    const size_t tileDataOffset = reader.ByteOffset();
    if (tileDataOffset >= fullPayloadSize)
        return false;

    info.sequence = sequence;
    info.bitstreamOffset = tileDataOffset;
    info.bitstreamSize = fullPayloadSize - tileDataOffset;
    if (info.bitstreamSize > std::numeric_limits<uint32_t>::max())
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
    info.picture.tileNum = 1;
    info.tiles[0] = {0, (uint32_t)info.bitstreamSize, 0, 0, 0xFF, {}};
    FillIdentityGlobalMotion(info.globalMotion);
    BindPointers(info);
    return true;
}

inline Result GetVideoEncodeAV1DecodeInfoFromHeader(const VideoAV1EncodeDecodeInfoDesc& desc, VideoAV1EncodeDecodeInfo& info) {
    if (!desc.feedback || !desc.sequence || !desc.encodedPayloadHeader || !desc.encodedPayloadHeaderSize || !desc.feedback->encodedBitstreamWrittenBytes)
        return Result::INVALID_ARGUMENT;

    ObuSpan frame = {};
    if (!FindFrameObu(desc.encodedPayloadHeader, (size_t)desc.encodedPayloadHeaderSize, frame))
        return Result::FAILURE;
    if (frame.payloadOffset >= desc.encodedPayloadHeaderSize)
        return Result::FAILURE;

    const size_t availablePayload = (size_t)desc.encodedPayloadHeaderSize - frame.payloadOffset;
    if (!ParseGeneratedKeyFrameHeader(desc.encodedPayloadHeader + frame.payloadOffset, availablePayload, frame.payloadSize, *desc.sequence, info))
        return Result::FAILURE;

    info.bitstreamOffset += frame.payloadOffset;
    if (info.bitstreamOffset > desc.feedback->encodedBitstreamWrittenBytes ||
        info.bitstreamSize > desc.feedback->encodedBitstreamWrittenBytes - info.bitstreamOffset)
        return Result::FAILURE;

    return Result::SUCCESS;
}

} // namespace video_av1

} // namespace nri
