#include "NRI.h"
#include "Extensions/NRIVideo.h"
#include "VideoAV1.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

struct BitWriter {
    std::vector<uint8_t> bytes;
    uint32_t bitOffset = 0;

    void WriteBit(uint32_t bit) {
        if ((bitOffset % 8) == 0)
            bytes.push_back(0);

        if (bit)
            bytes.back() |= uint8_t(1u << (7u - (bitOffset % 8u)));
        bitOffset++;
    }

    void WriteBits(uint32_t value, uint32_t bitCount) {
        for (uint32_t i = 0; i < bitCount; i++)
            WriteBit((value >> (bitCount - 1u - i)) & 1u);
    }

    void ByteAlign() {
        while (bitOffset % 8)
            WriteBit(0);
    }
};

struct InterFrameOptions {
    uint8_t primaryRefFrame = 0;
    uint8_t refreshFrameFlags = 0x02;
    std::array<uint8_t, 7> refFrameIndices = {0, 1, 2, 3, 4, 5, 6};
    uint8_t orderHint = 9;
    uint8_t baseQIndex = 32;
    bool enableRefFrameMvs = false;
    bool useRefFrameMvs = false;
    bool enableCdef = false;
    bool enableRestoration = false;
};

std::vector<uint8_t> MakeFrameObu(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> obu;
    obu.push_back((uint8_t(nri::video_av1::ObuType::Frame) << 3u) | 0x02u);

    size_t value = payload.size();
    do {
        uint8_t byte = uint8_t(value & 0x7Fu);
        value >>= 7u;
        if (value)
            byte |= 0x80u;
        obu.push_back(byte);
    } while (value);

    obu.insert(obu.end(), payload.begin(), payload.end());
    return obu;
}

std::vector<uint8_t> MakeInterFramePayload(const InterFrameOptions& options) {
    BitWriter w;

    w.WriteBit(0);             // show_existing_frame
    w.WriteBits(1, 2);         // frame_type = INTER
    w.WriteBit(1);             // show_frame
    w.WriteBit(0);             // error_resilient_mode
    w.WriteBit(0);             // disable_cdf_update
    w.WriteBit(0);             // frame_size_override_flag
    w.WriteBits(options.orderHint, 8);
    w.WriteBits(options.primaryRefFrame, 3);
    w.WriteBits(options.refreshFrameFlags, 8);
    w.WriteBit(0);             // frame_refs_short_signaling
    for (uint8_t refFrameIndex : options.refFrameIndices)
        w.WriteBits(refFrameIndex, 3);
    w.WriteBit(0);             // render_and_frame_size_different
    w.WriteBit(0);             // allow_high_precision_mv
    w.WriteBit(1);             // is_filter_switchable
    w.WriteBit(0);             // is_motion_mode_switchable
    if (options.enableRefFrameMvs)
        w.WriteBit(options.useRefFrameMvs ? 1 : 0);
    w.WriteBit(0);             // disable_frame_end_update_cdf
    w.WriteBit(1);             // uniform_tile_spacing_flag

    w.WriteBits(options.baseQIndex, 8);
    w.WriteBit(0);             // DeltaQYDc not coded
    w.WriteBit(0);             // DeltaQUDc not coded
    w.WriteBit(0);             // DeltaQUAc not coded
    w.WriteBit(0);             // using_qmatrix
    w.WriteBit(0);             // segmentation_enabled

    if (options.baseQIndex) {
        w.WriteBit(0);         // delta_q_present
        w.WriteBits(0, 6);     // loop_filter_level[0]
        w.WriteBits(0, 6);     // loop_filter_level[1]
        w.WriteBits(0, 3);     // loop_filter_sharpness
        w.WriteBit(0);         // loop_filter_delta_enabled

        if (options.enableCdef) {
            w.WriteBits(0, 2); // cdef_damping_minus_3
            w.WriteBits(0, 2); // cdef_bits
            w.WriteBits(0, 4); // cdef_y_pri_strength[0]
            w.WriteBits(0, 2); // cdef_y_sec_strength[0]
            w.WriteBits(0, 4); // cdef_uv_pri_strength[0]
            w.WriteBits(0, 2); // cdef_uv_sec_strength[0]
        }

        if (options.enableRestoration) {
            w.WriteBits(0, 2);
            w.WriteBits(0, 2);
            w.WriteBits(0, 2);
        }

        w.WriteBit(1);         // tx_mode = SELECT
    }

    w.WriteBit(0);             // reference_select
    for (uint32_t i = 0; i < 7; i++)
        w.WriteBit(0);         // is_global
    w.WriteBit(0);             // reduced_tx_set
    w.ByteAlign();
    w.bytes.push_back(0x80);   // minimal tile payload byte
    return w.bytes;
}

nri::VideoAV1SequenceDesc MakeSequence(nri::VideoAV1SequenceBits flags) {
    nri::VideoAV1SequenceDesc sequence = {};
    sequence.flags = flags;
    sequence.bitDepth = 8;
    sequence.subsamplingX = 1;
    sequence.subsamplingY = 1;
    sequence.maxFrameWidthMinus1 = 63;
    sequence.maxFrameHeightMinus1 = 63;
    sequence.frameWidthBitsMinus1 = 5;
    sequence.frameHeightBitsMinus1 = 5;
    sequence.orderHintBitsMinus1 = 7;
    sequence.seqForceScreenContentTools = 0;
    sequence.seqForceIntegerMv = 0;
    return sequence;
}

nri::VideoEncodeFeedback MakeFeedback(uint64_t size) {
    nri::VideoEncodeFeedback feedback = {};
    feedback.encodedBitstreamWrittenBytes = size;
    feedback.writtenSubregionNum = 1;
    return feedback;
}

std::array<nri::VideoAV1ReferenceDesc, 8> MakeReferences() {
    std::array<nri::VideoAV1ReferenceDesc, 8> references = {};
    for (uint32_t i = 0; i < references.size(); i++) {
        references[i].slot = i + 10;
        references[i].refFrameIndex = (uint8_t)i;
        references[i].frameType = i == 0 ? nri::VideoEncodeFrameType::IDR : nri::VideoEncodeFrameType::P;
        references[i].orderHint = (uint8_t)i;
        references[i].frameId = 100 + i;
    }
    return references;
}

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAILED: %s\n", message);
        std::exit(1);
    }
}

void TestInterFrameReferences() {
    InterFrameOptions options = {};
    options.primaryRefFrame = 3;
    options.refreshFrameFlags = 0x24;

    std::vector<uint8_t> payload = MakeFrameObu(MakeInterFramePayload(options));
    nri::VideoEncodeFeedback feedback = MakeFeedback(payload.size());
    nri::VideoAV1SequenceDesc sequence = MakeSequence(nri::VideoAV1SequenceBits::ENABLE_ORDER_HINT);
    std::array<nri::VideoAV1ReferenceDesc, 8> references = MakeReferences();

    nri::VideoAV1EncodeDecodeInfoDesc desc = {};
    desc.feedback = &feedback;
    desc.sequence = &sequence;
    desc.encodedPayloadHeader = payload.data();
    desc.encodedPayloadHeaderSize = payload.size();
    desc.references = references.data();
    desc.referenceNum = (uint32_t)references.size();

    nri::VideoAV1EncodeDecodeInfo info = {};
    Require(nri::video_av1::GetVideoEncodeAV1DecodeInfoFromHeader(desc, info) == nri::Result::SUCCESS, "inter frame parse succeeds");
    Require(info.picture.frameType == nri::VideoEncodeFrameType::P, "inter frame type is P");
    Require(info.picture.refreshFrameFlags == options.refreshFrameFlags, "refresh flags are parsed");
    Require(info.picture.primaryReferenceName == nri::VideoAV1ReferenceName::GOLDEN, "primary reference name is parsed");
    Require(info.picture.referenceNum == 7, "all named references are populated");
    Require(info.picture.references[0].slot == 10, "first reference is copied from DPB snapshot");
    Require(info.bitstreamSize == 1, "tile payload range is bounded to tile data");
}

void TestLosslessInterFrame() {
    InterFrameOptions options = {};
    options.baseQIndex = 0;
    options.enableCdef = true;
    options.enableRestoration = true;

    std::vector<uint8_t> payload = MakeFrameObu(MakeInterFramePayload(options));
    nri::VideoEncodeFeedback feedback = MakeFeedback(payload.size());
    nri::VideoAV1SequenceDesc sequence = MakeSequence(nri::VideoAV1SequenceBits::ENABLE_ORDER_HINT | nri::VideoAV1SequenceBits::ENABLE_CDEF | nri::VideoAV1SequenceBits::ENABLE_RESTORATION);
    std::array<nri::VideoAV1ReferenceDesc, 8> references = MakeReferences();

    nri::VideoAV1EncodeDecodeInfoDesc desc = {};
    desc.feedback = &feedback;
    desc.sequence = &sequence;
    desc.encodedPayloadHeader = payload.data();
    desc.encodedPayloadHeaderSize = payload.size();
    desc.references = references.data();
    desc.referenceNum = (uint32_t)references.size();

    nri::VideoAV1EncodeDecodeInfo info = {};
    Require(nri::video_av1::GetVideoEncodeAV1DecodeInfoFromHeader(desc, info) == nri::Result::SUCCESS, "lossless inter frame parse succeeds");
    Require(info.picture.baseQIndex == 0, "lossless base q index is parsed");
    Require(info.picture.txMode == 0, "lossless tx mode remains ONLY_4X4");
    Require(info.bitstreamSize == 1, "lossless tile payload range is bounded to tile data");
}

void TestUseRefFrameMvs() {
    InterFrameOptions options = {};
    options.enableRefFrameMvs = true;
    options.useRefFrameMvs = true;

    std::vector<uint8_t> payload = MakeFrameObu(MakeInterFramePayload(options));
    nri::VideoEncodeFeedback feedback = MakeFeedback(payload.size());
    nri::VideoAV1SequenceDesc sequence = MakeSequence(nri::VideoAV1SequenceBits::ENABLE_ORDER_HINT | nri::VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS);
    std::array<nri::VideoAV1ReferenceDesc, 8> references = MakeReferences();

    nri::VideoAV1EncodeDecodeInfoDesc desc = {};
    desc.feedback = &feedback;
    desc.sequence = &sequence;
    desc.encodedPayloadHeader = payload.data();
    desc.encodedPayloadHeaderSize = payload.size();
    desc.references = references.data();
    desc.referenceNum = (uint32_t)references.size();

    nri::VideoAV1EncodeDecodeInfo info = {};
    Require(nri::video_av1::GetVideoEncodeAV1DecodeInfoFromHeader(desc, info) == nri::Result::SUCCESS, "ref-frame-mvs inter frame parse succeeds");
    Require((uint32_t)(info.picture.flags & nri::VideoAV1PictureBits::USE_REF_FRAME_MVS) != 0, "use_ref_frame_mvs flag is parsed");
}

} // namespace

int main() {
    TestInterFrameReferences();
    TestLosslessInterFrame();
    TestUseRefFrameMvs();
    return 0;
}
