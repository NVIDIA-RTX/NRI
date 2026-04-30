// © 2021 NVIDIA Corporation

// Goal: backend-neutral hardware video encode/decode command submission
// Video formats: https://learn.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering

#pragma once

#define NRI_VIDEO_H 1

NriNamespaceBegin

NriForwardStruct(VideoSession);
NriForwardStruct(VideoSessionParameters);
NriForwardStruct(VideoPicture);

NriEnum(VideoUsage, uint8_t,
    DECODE,
    ENCODE
);

NriEnum(VideoCodec, uint8_t,
    H264,
    H265,
    AV1
);

NriEnum(VideoDecodeArgumentType, uint8_t,
    PICTURE_PARAMETERS,
    INVERSE_QUANTIZATION_MATRIX,
    SLICE_CONTROL
);

NriBits(VideoH264SequenceParameterSetBits, uint16_t,
    NONE                                = 0,
    CONSTRAINT_SET0                     = NriBit(0),
    CONSTRAINT_SET1                     = NriBit(1),
    CONSTRAINT_SET2                     = NriBit(2),
    CONSTRAINT_SET3                     = NriBit(3),
    CONSTRAINT_SET4                     = NriBit(4),
    CONSTRAINT_SET5                     = NriBit(5),
    DIRECT_8X8_INFERENCE                = NriBit(6),
    MB_ADAPTIVE_FRAME_FIELD             = NriBit(7),
    FRAME_MBS_ONLY                      = NriBit(8),
    DELTA_PIC_ORDER_ALWAYS_ZERO         = NriBit(9),
    SEPARATE_COLOUR_PLANE               = NriBit(10),
    GAPS_IN_FRAME_NUM_ALLOWED           = NriBit(11),
    QPPRIME_Y_ZERO_TRANSFORM_BYPASS     = NriBit(12)
);

NriBits(VideoH264PictureParameterSetBits, uint8_t,
    NONE                                = 0,
    TRANSFORM_8X8_MODE                  = NriBit(0),
    REDUNDANT_PIC_CNT_PRESENT           = NriBit(1),
    CONSTRAINED_INTRA_PRED              = NriBit(2),
    DEBLOCKING_FILTER_CONTROL_PRESENT   = NriBit(3),
    WEIGHTED_PRED                       = NriBit(4),
    BOTTOM_FIELD_PIC_ORDER_IN_FRAME     = NriBit(5),
    ENTROPY_CODING_MODE                 = NriBit(6)
);

NriBits(VideoH264DecodePictureBits, uint8_t,
    NONE                                = 0,
    FIELD_PICTURE                       = NriBit(0),
    INTRA                               = NriBit(1),
    IDR                                 = NriBit(2),
    BOTTOM_FIELD                        = NriBit(3),
    REFERENCE                           = NriBit(4),
    COMPLEMENTARY_FIELD_PAIR            = NriBit(5)
);

NriEnum(VideoEncodeFrameType, uint8_t,
    IDR,
    I,
    P,
    B
);

NriEnum(VideoEncodeRateControlMode, uint8_t,
    CQP
);

NriStruct(VideoSessionDesc) {
    Nri(VideoUsage) usage;
    Nri(VideoCodec) codec;
    Nri(Format) format;
    uint32_t width;
    uint32_t height;
    uint32_t maxReferenceNum;
};

NriStruct(VideoReference) {
    NriPtr(VideoPicture) picture;
    uint32_t slot;
};

NriStruct(VideoPictureDesc) {
    NriPtr(Texture) texture;
    Nri(Format) format;
    uint32_t subresource;
    uint32_t layer;
    uint32_t width;
    uint32_t height;
};

NriStruct(VideoH264SequenceParameterSetDesc) {
    Nri(VideoH264SequenceParameterSetBits) flags;
    uint8_t profileIdc;
    uint8_t levelIdc;
    uint8_t chromaFormatIdc;
    uint8_t sequenceParameterSetId;
    uint8_t bitDepthLumaMinus8;
    uint8_t bitDepthChromaMinus8;
    uint8_t log2MaxFrameNumMinus4;
    uint8_t pictureOrderCountType;
    int32_t offsetForNonReferencePicture;
    int32_t offsetForTopToBottomField;
    uint8_t log2MaxPictureOrderCountLsbMinus4;
    uint8_t referenceFrameNum;
    uint16_t pictureWidthInMbsMinus1;
    uint16_t pictureHeightInMapUnitsMinus1;
};

NriStruct(VideoH264PictureParameterSetDesc) {
    Nri(VideoH264PictureParameterSetBits) flags;
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    uint8_t refIndexL0DefaultActiveMinus1;
    uint8_t refIndexL1DefaultActiveMinus1;
    uint8_t weightedBipredIdc;
    int8_t pictureInitQpMinus26;
    int8_t pictureInitQsMinus26;
    int8_t chromaQpIndexOffset;
    int8_t secondChromaQpIndexOffset;
};

NriStruct(VideoH264SessionParametersDesc) {
    NriOptional const NriPtr(VideoH264SequenceParameterSetDesc) sequenceParameterSets; // if provided, must include "sequenceParameterSetNum" entries
    uint32_t sequenceParameterSetNum;
    NriOptional const NriPtr(VideoH264PictureParameterSetDesc) pictureParameterSets; // if provided, must include "pictureParameterSetNum" entries
    uint32_t pictureParameterSetNum;
    NriOptional uint32_t maxSequenceParameterSetNum; // defaults to "sequenceParameterSetNum"
    NriOptional uint32_t maxPictureParameterSetNum; // defaults to "pictureParameterSetNum"
};

NriStruct(VideoSessionParametersDesc) {
    NriPtr(VideoSession) session;
    NriOptional const NriPtr(VideoH264SessionParametersDesc) h264Parameters;
};

NriStruct(VideoDecodeArgument) {
    Nri(VideoDecodeArgumentType) type;
    uint32_t size;
    const void* data;
};

NriStruct(VideoH264DecodePictureDesc) {
    Nri(VideoH264DecodePictureBits) flags;
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    uint16_t frameNum;
    uint16_t idrPictureId;
    int32_t topFieldOrderCount;
    int32_t bottomFieldOrderCount;
    NriOptional const uint32_t* sliceOffsets; // if provided, must include "sliceOffsetNum" entries
    uint32_t sliceOffsetNum;
};

NriStruct(VideoEncodeRateControlDesc) {
    Nri(VideoEncodeRateControlMode) mode;
    uint8_t qpI;
    uint8_t qpP;
    uint8_t qpB;
    uint32_t frameRateNumerator;
    uint32_t frameRateDenominator;
};

NriStruct(VideoEncodePictureDesc) {
    Nri(VideoEncodeFrameType) frameType;
    uint8_t temporalLayer;
    uint16_t idrPictureId;
    uint32_t frameIndex;
    int32_t pictureOrderCount;
};

NriStruct(VideoDecodeDesc) {
    NriPtr(VideoSession) session;
    NriOptional NriPtr(VideoSessionParameters) parameters;
    NriPtr(Buffer) bitstream;
    uint64_t bitstreamOffset;
    uint64_t bitstreamSize;
    NriPtr(VideoPicture) dstPicture;
    NriOptional const NriPtr(VideoReference) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
    NriOptional const NriPtr(VideoDecodeArgument) arguments; // if provided, must include "argumentNum" entries
    uint32_t argumentNum;
    NriOptional const NriPtr(VideoH264DecodePictureDesc) h264PictureDesc;
};

NriStruct(VideoEncodeDesc) {
    NriPtr(VideoSession) session;
    NriOptional NriPtr(VideoSessionParameters) parameters;
    NriPtr(VideoPicture) srcPicture;
    NriPtr(Buffer) dstBitstream;
    uint64_t dstBitstreamOffset;
    NriOptional const NriPtr(VideoEncodePictureDesc) pictureDesc;
    NriOptional const NriPtr(VideoEncodeRateControlDesc) rateControlDesc;
    NriOptional NriPtr(VideoPicture) reconstructedPicture;
    NriOptional NriPtr(Buffer) metadata;
    uint64_t metadataOffset;
    NriOptional const NriPtr(VideoReference) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
};

// Threadsafe: no
NriStruct(VideoInterface) {
    // Session
    // {
        Nri(Result) (NRI_CALL *CreateVideoSession)  (NriRef(Device) device, const NriRef(VideoSessionDesc) videoSessionDesc, NriOut NriRef(VideoSession*) videoSession);
        void        (NRI_CALL *DestroyVideoSession) (NriRef(VideoSession) videoSession);
        Nri(Result) (NRI_CALL *CreateVideoSessionParameters)  (NriRef(Device) device, const NriRef(VideoSessionParametersDesc) videoSessionParametersDesc, NriOut NriRef(VideoSessionParameters*) videoSessionParameters);
        void        (NRI_CALL *DestroyVideoSessionParameters) (NriRef(VideoSessionParameters) videoSessionParameters);
        Nri(Result) (NRI_CALL *CreateVideoPicture)  (NriRef(Device) device, const NriRef(VideoPictureDesc) videoPictureDesc, NriOut NriRef(VideoPicture*) videoPicture);
        void        (NRI_CALL *DestroyVideoPicture) (NriRef(VideoPicture) videoPicture);
    // }

    // Command buffer
    // {
        // Video decode/encode command buffers must be created from "QueueType::VIDEO_DECODE" or "QueueType::VIDEO_ENCODE" queues.
        void (NRI_CALL *CmdDecodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoDecodeDesc) videoDecodeDesc);
        void (NRI_CALL *CmdEncodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoEncodeDesc) videoEncodeDesc);
    // }
};

NriNamespaceEnd
