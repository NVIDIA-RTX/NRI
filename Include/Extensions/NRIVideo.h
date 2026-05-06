// © 2021 NVIDIA Corporation

// Goal: backend-neutral hardware video encode/decode command submission
// Video formats: https://learn.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering

#pragma once

#define NRI_VIDEO_H 1
#define NRI_VIDEO_VERSION 1

NriNamespaceBegin

NriForwardStruct(VideoSession);
NriForwardStruct(VideoSessionParameters);
NriForwardStruct(VideoPicture);

NriEnum(VideoUsage, uint8_t,
    DECODE,
    ENCODE
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

NriBits(VideoH264DecodeReferenceBits, uint8_t,
    NONE                                = 0,
    TOP_FIELD                           = NriBit(0),
    BOTTOM_FIELD                        = NriBit(1),
    LONG_TERM                           = NriBit(2),
    NON_EXISTING                        = NriBit(3)
);

NriBits(VideoH265DecodePictureBits, uint8_t,
    NONE                                = 0,
    IRAP                                = NriBit(0),
    IDR                                 = NriBit(1),
    REFERENCE                           = NriBit(2),
    SHORT_TERM_REF_PIC_SET_SPS          = NriBit(3)
);

NriBits(VideoH265ProfileTierLevelBits, uint8_t,
    NONE                                = 0,
    TIER                                = NriBit(0),
    PROGRESSIVE_SOURCE                  = NriBit(1),
    INTERLACED_SOURCE                   = NriBit(2),
    NON_PACKED_CONSTRAINT               = NriBit(3),
    FRAME_ONLY_CONSTRAINT               = NriBit(4)
);

NriBits(VideoH265VideoParameterSetBits, uint8_t,
    NONE                                = 0,
    TEMPORAL_ID_NESTING                 = NriBit(0),
    SUB_LAYER_ORDERING_INFO_PRESENT     = NriBit(1),
    TIMING_INFO_PRESENT                 = NriBit(2),
    POC_PROPORTIONAL_TO_TIMING          = NriBit(3)
);

NriBits(VideoH265SequenceParameterSetBits, uint32_t,
    NONE                                = 0,
    TEMPORAL_ID_NESTING                 = NriBit(0),
    SEPARATE_COLOUR_PLANE               = NriBit(1),
    CONFORMANCE_WINDOW                  = NriBit(2),
    SUB_LAYER_ORDERING_INFO_PRESENT     = NriBit(3),
    SCALING_LIST_ENABLED                = NriBit(4),
    SCALING_LIST_DATA_PRESENT           = NriBit(5),
    AMP_ENABLED                         = NriBit(6),
    SAMPLE_ADAPTIVE_OFFSET_ENABLED      = NriBit(7),
    PCM_ENABLED                         = NriBit(8),
    PCM_LOOP_FILTER_DISABLED            = NriBit(9),
    LONG_TERM_REF_PICS_PRESENT          = NriBit(10),
    TEMPORAL_MVP_ENABLED                = NriBit(11),
    STRONG_INTRA_SMOOTHING_ENABLED      = NriBit(12),
    VUI_PARAMETERS_PRESENT              = NriBit(13)
);

NriBits(VideoH265PictureParameterSetBits, uint32_t,
    NONE                                = 0,
    DEPENDENT_SLICE_SEGMENTS_ENABLED    = NriBit(0),
    OUTPUT_FLAG_PRESENT                 = NriBit(1),
    SIGN_DATA_HIDING_ENABLED            = NriBit(2),
    CABAC_INIT_PRESENT                  = NriBit(3),
    CONSTRAINED_INTRA_PRED              = NriBit(4),
    TRANSFORM_SKIP_ENABLED              = NriBit(5),
    CU_QP_DELTA_ENABLED                 = NriBit(6),
    SLICE_CHROMA_QP_OFFSETS_PRESENT     = NriBit(7),
    WEIGHTED_PRED                       = NriBit(8),
    WEIGHTED_BIPRED                     = NriBit(9),
    TRANSQUANT_BYPASS_ENABLED           = NriBit(10),
    TILES_ENABLED                       = NriBit(11),
    ENTROPY_CODING_SYNC_ENABLED         = NriBit(12),
    UNIFORM_SPACING                     = NriBit(13),
    LOOP_FILTER_ACROSS_TILES_ENABLED    = NriBit(14),
    LOOP_FILTER_ACROSS_SLICES_ENABLED   = NriBit(15),
    DEBLOCKING_FILTER_CONTROL_PRESENT   = NriBit(16),
    DEBLOCKING_FILTER_OVERRIDE_ENABLED  = NriBit(17),
    DEBLOCKING_FILTER_DISABLED          = NriBit(18),
    SCALING_LIST_DATA_PRESENT           = NriBit(19),
    LISTS_MODIFICATION_PRESENT          = NriBit(20),
    SLICE_SEGMENT_HEADER_EXTENSION_PRESENT = NriBit(21)
);

NriBits(VideoH265ShortTermRefPicSetBits, uint8_t,
    NONE                                = 0,
    INTER_REF_PIC_SET_PREDICTION        = NriBit(0),
    DELTA_RPS_SIGN                      = NriBit(1)
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

NriEnum(VideoAV1ReferenceName, uint8_t,
    NONE,
    LAST,
    LAST2,
    LAST3,
    GOLDEN,
    BWDREF,
    ALTREF2,
    ALTREF
);

NriBits(VideoAV1SequenceBits, uint32_t,
    NONE                                = 0,
    STILL_PICTURE                       = NriBit(0),
    REDUCED_STILL_PICTURE_HEADER        = NriBit(1),
    USE_128X128_SUPERBLOCK              = NriBit(2),
    ENABLE_FILTER_INTRA                 = NriBit(3),
    ENABLE_INTRA_EDGE_FILTER            = NriBit(4),
    ENABLE_INTERINTRA_COMPOUND          = NriBit(5),
    ENABLE_MASKED_COMPOUND              = NriBit(6),
    ENABLE_WARPED_MOTION                = NriBit(7),
    ENABLE_DUAL_FILTER                  = NriBit(8),
    ENABLE_ORDER_HINT                   = NriBit(9),
    ENABLE_JNT_COMP                     = NriBit(10),
    ENABLE_REF_FRAME_MVS                = NriBit(11),
    FRAME_ID_NUMBERS_PRESENT            = NriBit(12),
    ENABLE_SUPERRES                     = NriBit(13),
    ENABLE_CDEF                         = NriBit(14),
    ENABLE_RESTORATION                  = NriBit(15),
    FILM_GRAIN_PARAMS_PRESENT           = NriBit(16),
    TIMING_INFO_PRESENT                 = NriBit(17),
    INITIAL_DISPLAY_DELAY_PRESENT       = NriBit(18),
    MONO_CHROME                         = NriBit(19),
    COLOR_RANGE                         = NriBit(20),
    SEPARATE_UV_DELTA_Q                 = NriBit(21),
    COLOR_DESCRIPTION_PRESENT           = NriBit(22)
);

NriBits(VideoAV1PictureBits, uint32_t,
    NONE                                = 0,
    ERROR_RESILIENT_MODE                = NriBit(0),
    DISABLE_CDF_UPDATE                  = NriBit(1),
    USE_SUPERRES                        = NriBit(2),
    RENDER_AND_FRAME_SIZE_DIFFERENT     = NriBit(3),
    ALLOW_SCREEN_CONTENT_TOOLS          = NriBit(4),
    IS_FILTER_SWITCHABLE                = NriBit(5),
    FORCE_INTEGER_MV                    = NriBit(6),
    FRAME_SIZE_OVERRIDE                 = NriBit(7),
    BUFFER_REMOVAL_TIME_PRESENT         = NriBit(8),
    ALLOW_INTRABC                       = NriBit(9),
    FRAME_REFS_SHORT_SIGNALING          = NriBit(10),
    ALLOW_HIGH_PRECISION_MV             = NriBit(11),
    IS_MOTION_MODE_SWITCHABLE           = NriBit(12),
    USE_REF_FRAME_MVS                   = NriBit(13),
    DISABLE_FRAME_END_UPDATE_CDF        = NriBit(14),
    ALLOW_WARPED_MOTION                 = NriBit(15),
    REDUCED_TX_SET                      = NriBit(16),
    REFERENCE_SELECT                    = NriBit(17),
    SKIP_MODE_PRESENT                   = NriBit(18),
    DELTA_Q_PRESENT                     = NriBit(19),
    DELTA_LF_PRESENT                    = NriBit(20),
    DELTA_LF_MULTI                      = NriBit(21),
    SEGMENTATION_ENABLED                = NriBit(22),
    SEGMENTATION_UPDATE_MAP             = NriBit(23),
    SEGMENTATION_TEMPORAL_UPDATE        = NriBit(24),
    SEGMENTATION_UPDATE_DATA            = NriBit(25),
    USES_LR                             = NriBit(26),
    USES_CHROMA_LR                      = NriBit(27),
    SHOW_FRAME                          = NriBit(28),
    SHOWABLE_FRAME                      = NriBit(29),
    APPLY_GRAIN                         = NriBit(30)
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

NriEnum(VideoPictureUsage, uint8_t,
    DECODE_OUTPUT,
    DECODE_REFERENCE,
    ENCODE_INPUT,
    ENCODE_REFERENCE
);

NriStruct(VideoBitstreamRange) {
    NriPtr(Buffer) buffer;
    uint64_t offset; // decode slice/tile offsets are relative to this offset
    uint64_t size; // decode bitstream byte range; H.264/H.265 neutral decode expects Annex-B byte stream data
};

NriStruct(VideoPictureDesc) {
    NriPtr(Texture) texture;
    Nri(VideoPictureUsage) usage;
    Nri(Format) format;
    uint32_t subresource;
    uint32_t layer;
    uint32_t width;
    uint32_t height;
};

NriStruct(VideoDecodePictureStates) {
    Nri(AccessLayoutStage) decodeWrite; // state required before CmdDecodeVideo writes the destination picture
    Nri(AccessLayoutStage) afterDecode; // optional state to transition to on the video decode queue after CmdDecodeVideo
    Nri(AccessLayoutStage) graphicsBefore; // state to use as "before" when the graphics queue consumes the decoded picture
    bool releaseAfterDecode; // if true, caller should record decodeWrite -> afterDecode on the video decode queue
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

NriStruct(VideoH265ProfileTierLevelDesc) {
    Nri(VideoH265ProfileTierLevelBits) flags;
    uint8_t generalProfileIdc;
    uint8_t generalLevelIdc;
    uint16_t reserved;
};

NriStruct(VideoH265DecPicBufMgrDesc) {
    uint8_t maxDecPicBufferingMinus1[8];
    uint8_t maxNumReorderPics[8];
    uint32_t maxLatencyIncreasePlus1[8];
};

NriStruct(VideoH265ScalingListsDesc) {
    uint8_t scalingList4x4[6][16];
    uint8_t scalingList8x8[6][64];
    uint8_t scalingList16x16[6][64];
    uint8_t scalingList32x32[2][64];
    uint8_t scalingListDCCoef16x16[6];
    uint8_t scalingListDCCoef32x32[2];
};

NriStruct(VideoH265ShortTermRefPicSetDesc) {
    Nri(VideoH265ShortTermRefPicSetBits) flags;
    uint8_t numNegativePics;
    uint8_t numPositivePics;
    uint8_t reserved;
    uint32_t deltaIdxMinus1;
    uint16_t useDeltaFlag;
    uint16_t absDeltaRpsMinus1;
    uint16_t usedByCurrPicFlag;
    uint16_t usedByCurrPicS0Flag;
    uint16_t usedByCurrPicS1Flag;
    uint16_t deltaPocS0Minus1[16];
    uint16_t deltaPocS1Minus1[16];
};

NriStruct(VideoH265LongTermRefPicsSpsDesc) {
    uint32_t usedByCurrPicLtSpsFlag;
    uint32_t ltRefPicPocLsbSps[32];
};

NriStruct(VideoH265VideoParameterSetDesc) {
    Nri(VideoH265VideoParameterSetBits) flags;
    uint8_t videoParameterSetId;
    uint8_t maxSubLayersMinus1;
    uint16_t reserved;
    uint32_t numUnitsInTick;
    uint32_t timeScale;
    uint32_t numTicksPocDiffOneMinus1;
    Nri(VideoH265ProfileTierLevelDesc) profileTierLevel;
    Nri(VideoH265DecPicBufMgrDesc) decPicBufMgr;
};

NriStruct(VideoH265SequenceParameterSetDesc) {
    Nri(VideoH265SequenceParameterSetBits) flags;
    uint8_t videoParameterSetId;
    uint8_t maxSubLayersMinus1;
    uint8_t sequenceParameterSetId;
    uint8_t chromaFormatIdc;
    uint32_t pictureWidthInLumaSamples;
    uint32_t pictureHeightInLumaSamples;
    uint8_t bitDepthLumaMinus8;
    uint8_t bitDepthChromaMinus8;
    uint8_t log2MaxPictureOrderCountLsbMinus4;
    uint8_t log2MinLumaCodingBlockSizeMinus3;
    uint8_t log2DiffMaxMinLumaCodingBlockSize;
    uint8_t log2MinLumaTransformBlockSizeMinus2;
    uint8_t log2DiffMaxMinLumaTransformBlockSize;
    uint8_t maxTransformHierarchyDepthInter;
    uint8_t maxTransformHierarchyDepthIntra;
    uint8_t numShortTermRefPicSets;
    uint8_t numLongTermRefPicsSps;
    uint8_t pcmSampleBitDepthLumaMinus1;
    uint8_t pcmSampleBitDepthChromaMinus1;
    uint8_t log2MinPcmLumaCodingBlockSizeMinus3;
    uint8_t log2DiffMaxMinPcmLumaCodingBlockSize;
    uint32_t confWinLeftOffset;
    uint32_t confWinRightOffset;
    uint32_t confWinTopOffset;
    uint32_t confWinBottomOffset;
    Nri(VideoH265ProfileTierLevelDesc) profileTierLevel;
    Nri(VideoH265DecPicBufMgrDesc) decPicBufMgr;
    NriOptional const NriPtr(VideoH265ScalingListsDesc) scalingLists;
    NriOptional const NriPtr(VideoH265ShortTermRefPicSetDesc) shortTermRefPicSets; // if provided, must include "numShortTermRefPicSets" entries
    NriOptional const NriPtr(VideoH265LongTermRefPicsSpsDesc) longTermRefPicsSps;
};

NriStruct(VideoH265PictureParameterSetDesc) {
    Nri(VideoH265PictureParameterSetBits) flags;
    uint8_t pictureParameterSetId;
    uint8_t sequenceParameterSetId;
    uint8_t videoParameterSetId;
    uint8_t numExtraSliceHeaderBits;
    uint8_t refIndexL0DefaultActiveMinus1;
    uint8_t refIndexL1DefaultActiveMinus1;
    int8_t initQpMinus26;
    uint8_t diffCuQpDeltaDepth;
    int8_t cbQpOffset;
    int8_t crQpOffset;
    int8_t betaOffsetDiv2;
    int8_t tcOffsetDiv2;
    uint8_t log2ParallelMergeLevelMinus2;
    uint8_t tileColumnNumMinus1;
    uint8_t tileRowNumMinus1;
    uint8_t reserved;
    uint16_t columnWidthMinus1[19];
    uint16_t rowHeightMinus1[21];
    NriOptional const NriPtr(VideoH265ScalingListsDesc) scalingLists;
};

NriStruct(VideoH265SessionParametersDesc) {
    NriOptional const NriPtr(VideoH265VideoParameterSetDesc) videoParameterSets; // if provided, must include "videoParameterSetNum" entries
    uint32_t videoParameterSetNum;
    NriOptional const NriPtr(VideoH265SequenceParameterSetDesc) sequenceParameterSets; // if provided, must include "sequenceParameterSetNum" entries
    uint32_t sequenceParameterSetNum;
    NriOptional const NriPtr(VideoH265PictureParameterSetDesc) pictureParameterSets; // if provided, must include "pictureParameterSetNum" entries
    uint32_t pictureParameterSetNum;
    NriOptional uint32_t maxVideoParameterSetNum; // defaults to "videoParameterSetNum"
    NriOptional uint32_t maxSequenceParameterSetNum; // defaults to "sequenceParameterSetNum"
    NriOptional uint32_t maxPictureParameterSetNum; // defaults to "pictureParameterSetNum"
};

NriStruct(VideoAnnexBParameterSetsDesc) {
    Nri(VideoCodec) codec;
    NriOptional const NriPtr(VideoH264SequenceParameterSetDesc) h264Sps;
    NriOptional const NriPtr(VideoH264PictureParameterSetDesc) h264Pps;
    NriOptional const NriPtr(VideoH265VideoParameterSetDesc) h265Vps;
    NriOptional const NriPtr(VideoH265SequenceParameterSetDesc) h265Sps;
    NriOptional const NriPtr(VideoH265PictureParameterSetDesc) h265Pps;
    NriOptional NriPtr(uint8_t) dst; // if null, only "writtenSize" is returned
    uint64_t dstSize;
    uint64_t writtenSize;
};

NriStruct(VideoAV1SequenceDesc) {
    Nri(VideoAV1SequenceBits) flags;
    uint8_t seqProfile;
    uint8_t bitDepth;
    uint8_t subsamplingX;
    uint8_t subsamplingY;
    uint16_t maxFrameWidthMinus1;
    uint16_t maxFrameHeightMinus1;
    uint8_t frameWidthBitsMinus1;
    uint8_t frameHeightBitsMinus1;
    uint8_t deltaFrameIdLengthMinus2;
    uint8_t additionalFrameIdLengthMinus1;
    uint8_t orderHintBitsMinus1;
    uint8_t seqForceIntegerMv;
    uint8_t seqForceScreenContentTools;
    uint8_t level;
    uint8_t colorPrimaries;
    uint8_t transferCharacteristics;
    uint8_t matrixCoefficients;
    uint8_t chromaSamplePosition;
    uint32_t numUnitsInDisplayTick;
    uint32_t timeScale;
    uint32_t numTicksPerPictureMinus1;
};

NriStruct(VideoAV1SessionParametersDesc) {
    Nri(VideoAV1SequenceDesc) sequence;
};

NriStruct(VideoSessionParametersDesc) {
    NriPtr(VideoSession) session;
    NriOptional const NriPtr(VideoH264SessionParametersDesc) h264Parameters;
    NriOptional const NriPtr(VideoH265SessionParametersDesc) h265Parameters;
    NriOptional const NriPtr(VideoAV1SessionParametersDesc) av1Parameters;
};

NriStruct(VideoDecodeArgument) {
    Nri(VideoDecodeArgumentType) type;
    uint32_t size;
    const void* data;
};

NriStruct(VideoH264ReferenceDesc) {
    Nri(VideoEncodeFrameType) frameType;
    uint8_t temporalLayer;
    uint8_t listIndex;
    uint8_t longTermReference;
    uint32_t frameNum;
    int32_t pictureOrderCount;
    uint32_t slot;
    uint16_t longTermPictureIndex;
    uint16_t longTermFrameIndex;
};

NriStruct(VideoH264DecodeReferenceDesc) {
    Nri(VideoH264DecodeReferenceBits) flags;
    uint8_t reserved;
    uint16_t frameNum;
    uint32_t slot;
    int32_t topFieldOrderCount;
    int32_t bottomFieldOrderCount;
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
    uint32_t referenceSlot; // used when "flags" includes REFERENCE; falls back to VideoDecodeDesc::dstSlot when zero
    NriOptional const NriPtr(VideoH264DecodeReferenceDesc) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
};

NriStruct(VideoH265ReferenceDesc) {
    uint32_t slot;
    int32_t pictureOrderCount;
    uint8_t temporalLayer;
    Nri(VideoEncodeFrameType) frameType;
    uint8_t longTerm;
    uint8_t listIndex;
};

NriStruct(VideoH265DecodePictureDesc) {
    Nri(VideoH265DecodePictureBits) flags;
    uint8_t videoParameterSetId;
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    int32_t pictureOrderCount;
    uint8_t numDeltaPocsOfRefRpsIdx;
    uint8_t reserved;
    uint16_t numBitsForShortTermRefPicSetInSlice;
    NriOptional const uint32_t* sliceSegmentOffsets; // if provided, must include "sliceSegmentOffsetNum" entries
    uint32_t sliceSegmentOffsetNum;
    NriOptional const NriPtr(VideoH265ReferenceDesc) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
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

NriStruct(VideoH264PictureDesc) {
    uint8_t sequenceParameterSetId;
    uint8_t pictureParameterSetId;
    uint16_t reserved;
    NriOptional const NriPtr(VideoH264ReferenceDesc) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
};

NriStruct(VideoAV1ReferenceDesc) {
    Nri(VideoAV1ReferenceName) name; // NONE describes an AV1 DPB slot that is not mapped to a current-frame reference name
    uint8_t refFrameIndex;
    Nri(VideoEncodeFrameType) frameType;
    uint8_t orderHint;
    uint32_t frameId;
    uint32_t slot;
    NriOptional const uint8_t* savedOrderHints; // if provided, must include 8 entries
};

NriStruct(VideoAV1TileLayoutDesc) {
    uint8_t columnNum;
    uint8_t rowNum;
    uint8_t tileSizeBytesMinus1;
    uint8_t uniformSpacing;
    uint16_t contextUpdateTileId;
    uint16_t reserved;
    NriOptional const uint16_t* miColumnStarts; // if provided, must include "columnNum + 1" entries
    NriOptional const uint16_t* miRowStarts; // if provided, must include "rowNum + 1" entries
    NriOptional const uint16_t* widthInSuperblocksMinus1; // if provided, must include "columnNum" entries
    NriOptional const uint16_t* heightInSuperblocksMinus1; // if provided, must include "rowNum" entries
};

NriStruct(VideoAV1QuantizationDesc) {
    int8_t deltaQYDc;
    int8_t deltaQUDc;
    int8_t deltaQUAc;
    int8_t deltaQVDc;
    int8_t deltaQVAc;
    uint8_t qmY;
    uint8_t qmU;
    uint8_t qmV;
    uint8_t usingQmatrix;
    uint8_t diffUvDelta;
    uint8_t reserved[2];
};

NriStruct(VideoAV1LoopFilterDesc) {
    uint8_t level[4];
    uint8_t sharpness;
    uint8_t deltaEnabled;
    uint8_t deltaUpdate;
    uint8_t updateModeDelta;
    int8_t refDeltas[8];
    int8_t modeDeltas[2];
    uint8_t reserved[2];
};

NriStruct(VideoAV1CdefDesc) {
    uint8_t yPrimaryStrength[8];
    uint8_t ySecondaryStrength[8];
    uint8_t uvPrimaryStrength[8];
    uint8_t uvSecondaryStrength[8];
};

NriStruct(VideoAV1SegmentationDesc) {
    uint8_t featureEnabled[8];
    uint8_t reserved[8];
    int16_t featureData[8][8];
};

NriStruct(VideoAV1LoopRestorationDesc) {
    uint8_t frameRestorationType[3];
    uint8_t lrUnitShift;
    uint8_t lrUvShift;
    uint8_t reserved[3];
};

NriStruct(VideoAV1GlobalMotionDesc) {
    uint8_t type[8];
    uint8_t invalid[8];
    int32_t params[8][6];
};

NriStruct(VideoAV1FilmGrainDesc) {
    uint8_t chromaScalingFromLuma;
    uint8_t overlapFlag;
    uint8_t clipToRestrictedRange;
    uint8_t updateGrain;
    uint8_t matrixCoeffIsIdentity;
    uint8_t grainScalingMinus8;
    uint8_t arCoeffLag;
    uint8_t arCoeffShiftMinus6;
    uint8_t grainScaleShift;
    uint8_t filmGrainParamsRefIdx;
    uint16_t grainSeed;
    uint8_t numYPoints;
    uint8_t pointYValue[14];
    uint8_t pointYScaling[14];
    uint8_t numCbPoints;
    uint8_t pointCbValue[10];
    uint8_t pointCbScaling[10];
    uint8_t numCrPoints;
    uint8_t pointCrValue[10];
    uint8_t pointCrScaling[10];
    uint8_t arCoeffsYPlus128[24];
    uint8_t arCoeffsCbPlus128[25];
    uint8_t arCoeffsCrPlus128[25];
    uint8_t cbMult;
    uint8_t cbLumaMult;
    int16_t cbOffset;
    uint8_t crMult;
    uint8_t crLumaMult;
    int16_t crOffset;
};

NriStruct(VideoAV1PictureDesc) {
    uint32_t currentFrameId;
    uint8_t orderHint;
    uint8_t refreshFrameFlags;
    Nri(VideoAV1ReferenceName) primaryReferenceName;
    Nri(VideoAV1PictureBits) flags;
    uint16_t renderWidthMinus1;
    uint16_t renderHeightMinus1;
    uint8_t codedDenom;
    uint8_t interpolationFilter;
    uint8_t txMode;
    uint8_t baseQIndex;
    uint8_t cdefDampingMinus3;
    uint8_t cdefBits;
    uint8_t deltaQRes;
    uint8_t deltaLfRes;
    NriOptional const NriPtr(VideoAV1TileLayoutDesc) tileLayout;
    NriOptional const NriPtr(VideoAV1QuantizationDesc) quantization;
    NriOptional const NriPtr(VideoAV1LoopFilterDesc) loopFilter;
    NriOptional const NriPtr(VideoAV1CdefDesc) cdef;
    NriOptional const NriPtr(VideoAV1SegmentationDesc) segmentation;
    NriOptional const NriPtr(VideoAV1LoopRestorationDesc) loopRestoration;
    NriOptional const NriPtr(VideoAV1GlobalMotionDesc) globalMotion;
    NriOptional const uint8_t* orderHints; // if provided, must include 8 entries
    NriOptional const NriPtr(VideoAV1ReferenceDesc) references; // if provided, must include "referenceNum" DPB snapshot entries
    uint32_t referenceNum;
};

NriStruct(VideoAV1DecodeTileDesc) {
    uint32_t offset;
    uint32_t size;
    uint16_t row;
    uint16_t column;
    uint8_t anchorFrame;
    uint8_t reserved[3];
};

NriStruct(VideoAV1DecodePictureDesc) {
    Nri(VideoEncodeFrameType) frameType;
    uint8_t orderHint;
    uint8_t refreshFrameFlags;
    Nri(VideoAV1ReferenceName) primaryReferenceName;
    uint32_t currentFrameId;
    uint32_t frameHeaderOffset;
    Nri(VideoAV1PictureBits) flags;
    uint16_t renderWidthMinus1;
    uint16_t renderHeightMinus1;
    uint8_t baseQIndex;
    uint8_t superresDenom;
    uint8_t codedDenom;
    uint8_t interpolationFilter;
    uint8_t txMode;
    uint8_t cdefDampingMinus3;
    uint8_t cdefBits;
    uint8_t deltaQRes;
    uint8_t deltaLfRes;
    NriOptional const NriPtr(VideoAV1TileLayoutDesc) tileLayout;
    NriOptional const NriPtr(VideoAV1QuantizationDesc) quantization;
    NriOptional const NriPtr(VideoAV1LoopFilterDesc) loopFilter;
    NriOptional const NriPtr(VideoAV1CdefDesc) cdef;
    NriOptional const NriPtr(VideoAV1SegmentationDesc) segmentation;
    NriOptional const NriPtr(VideoAV1LoopRestorationDesc) loopRestoration;
    NriOptional const NriPtr(VideoAV1GlobalMotionDesc) globalMotion;
    NriOptional const NriPtr(VideoAV1FilmGrainDesc) filmGrain;
    NriOptional const uint8_t* orderHints; // if provided, must include 8 entries
    NriOptional const NriPtr(VideoAV1DecodeTileDesc) tiles; // if provided, must include "tileNum" entries
    uint32_t tileNum;
    NriOptional const NriPtr(VideoAV1ReferenceDesc) references; // if provided, must include "referenceNum" DPB snapshot entries
    uint32_t referenceNum;
};

NriStruct(VideoDecodeDesc) {
    NriPtr(VideoSession) session;
    NriPtr(VideoSessionParameters) parameters;
    Nri(VideoBitstreamRange) bitstream;
    NriPtr(VideoPicture) dstPicture;
    NriOptional NriPtr(VideoPicture) setupPicture; // if provided, used as the reconstructed/DPB setup reference instead of "dstPicture"
    NriOptional const NriPtr(VideoReference) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
    uint32_t dstSlot;
    NriOptional const NriPtr(VideoDecodeArgument) arguments; // if provided, must include "argumentNum" entries
    uint32_t argumentNum;
    NriOptional const NriPtr(VideoH264DecodePictureDesc) h264PictureDesc;
    NriOptional const NriPtr(VideoH265DecodePictureDesc) h265PictureDesc;
    NriOptional const NriPtr(VideoAV1DecodePictureDesc) av1PictureDesc;
};

NriStruct(VideoEncodeFeedback) {
    uint64_t errorFlags;
    uint64_t averageQP;
    uint64_t intraCodingUnitNum;
    uint64_t interCodingUnitNum;
    uint64_t skipCodingUnitNum;
    uint64_t averageMotionEstimationX;
    uint64_t averageMotionEstimationY;
    uint64_t encodedBitstreamWrittenBytes;
    uint64_t writtenSubregionNum;
    uint64_t encodedBitstreamOffset;
};

NriStruct(VideoAV1EncodeDecodeInfoDesc) {
    const NriPtr(VideoEncodeFeedback) feedback;
    const NriPtr(VideoAV1SequenceDesc) sequence;
    NriOptional const NriPtr(uint8_t) encodedPayloadHeader; // first bytes of the encoded payload; returned bitstream ranges are bounded by "feedback"
    uint64_t encodedPayloadHeaderSize;
};

NriStruct(VideoAV1EncodeDecodeInfo) {
    uint64_t bitstreamOffset; // offset relative to VideoEncodeFeedback::encodedBitstreamOffset
    uint64_t bitstreamSize;
    Nri(VideoAV1SequenceDesc) sequence;
    Nri(VideoAV1DecodePictureDesc) picture;
    Nri(VideoAV1DecodeTileDesc) tiles[64];
    Nri(VideoAV1TileLayoutDesc) tileLayout;
    uint16_t miColumnStarts[65];
    uint16_t miRowStarts[65];
    uint16_t widthInSuperblocksMinus1[64];
    uint16_t heightInSuperblocksMinus1[64];
    Nri(VideoAV1QuantizationDesc) quantization;
    Nri(VideoAV1LoopFilterDesc) loopFilter;
    Nri(VideoAV1CdefDesc) cdef;
    Nri(VideoAV1SegmentationDesc) segmentation;
    Nri(VideoAV1LoopRestorationDesc) loopRestoration;
    Nri(VideoAV1GlobalMotionDesc) globalMotion;
};

NriStruct(VideoEncodeDesc) {
    NriPtr(VideoSession) session;
    NriPtr(VideoSessionParameters) parameters;
    NriPtr(VideoPicture) srcPicture;
    Nri(VideoBitstreamRange) dstBitstream;
    uint64_t bitstreamMetadataSize; // D3D12: bytes of codec metadata already written before the current frame payload
    NriOptional const NriPtr(VideoEncodePictureDesc) pictureDesc;
    NriOptional const NriPtr(VideoEncodeRateControlDesc) rateControlDesc;
    NriOptional NriPtr(VideoPicture) reconstructedPicture;
    NriOptional NriPtr(Buffer) metadata;
    uint64_t metadataOffset;
    NriOptional NriPtr(Buffer) resolvedMetadata; // If provided, contains "VideoEncodeFeedback" after execution.
    uint64_t resolvedMetadataOffset;
    NriOptional const NriPtr(VideoReference) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
    uint32_t reconstructedSlot;
    NriOptional const NriPtr(VideoH264PictureDesc) h264PictureDesc;
    NriOptional const NriPtr(VideoAV1PictureDesc) av1PictureDesc;
    NriOptional const NriPtr(VideoH265ReferenceDesc) h265ReferenceDescs; // if provided, must include "referenceNum" entries
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
        // Returns backend-specific states for explicit caller-recorded decode picture barriers.
        Nri(Result) (NRI_CALL *GetVideoDecodePictureStates) (const NriRef(VideoPicture) videoPicture, NriOut NriRef(VideoDecodePictureStates) states);
        // Serializes H.264 SPS/PPS or H.265 VPS/SPS/PPS parameter sets to Annex-B bytes.
        // Pass "dst = nullptr" to query the required byte size in "writtenSize".
        Nri(Result) (NRI_CALL *WriteVideoAnnexBParameterSets) (NriRef(VideoAnnexBParameterSetsDesc) annexBParameterSetsDesc);
    // }

    // Command buffer
    // {
        // Video decode/encode command buffers must be created from "QueueType::VIDEO_DECODE" or "QueueType::VIDEO_ENCODE" queues.
        void (NRI_CALL *CmdDecodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoDecodeDesc) videoDecodeDesc);
        void (NRI_CALL *CmdEncodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoEncodeDesc) videoEncodeDesc);
        // Vulkan: consumes encode feedback in encode submission order; the query must be host-available before this command is recorded.
        // D3D12 resolves feedback during "CmdEncodeVideo".
        void (NRI_CALL *CmdResolveVideoEncodeFeedback) (NriRef(CommandBuffer) commandBuffer, NriRef(VideoSession) videoSession, NriRef(Buffer) resolvedMetadata, uint64_t resolvedMetadataOffset);
        Nri(Result) (NRI_CALL *GetVideoEncodeFeedback) (NriRef(VideoSession) videoSession, NriRef(Buffer) resolvedMetadataReadback, uint64_t resolvedMetadataOffset, NriOut NriRef(VideoEncodeFeedback) feedback);
        Nri(Result) (NRI_CALL *GetVideoEncodeAV1DecodeInfo) (NriRef(VideoSession) videoSession, NriRef(Buffer) resolvedMetadataReadback, uint64_t resolvedMetadataOffset, const NriRef(VideoAV1EncodeDecodeInfoDesc) desc, NriOut NriRef(VideoAV1EncodeDecodeInfo) info);
    // }
};

NriNamespaceEnd
