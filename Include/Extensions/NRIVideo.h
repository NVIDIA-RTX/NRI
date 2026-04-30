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

NriStruct(VideoSessionParametersDesc) {
    NriPtr(VideoSession) session;
    NriOptional const void* codecDesc;
};

NriStruct(VideoDecodeArgument) {
    Nri(VideoDecodeArgumentType) type;
    uint32_t size;
    const void* data;
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
    NriOptional const void* codecDesc;
};

NriStruct(VideoEncodeDesc) {
    NriPtr(VideoSession) session;
    NriOptional NriPtr(VideoSessionParameters) parameters;
    NriPtr(VideoPicture) srcPicture;
    NriPtr(Buffer) dstBitstream;
    uint64_t dstBitstreamOffset;
    NriOptional NriPtr(VideoPicture) reconstructedPicture;
    NriOptional NriPtr(Buffer) metadata;
    uint64_t metadataOffset;
    NriOptional const NriPtr(VideoReference) references; // if provided, must include "referenceNum" entries
    uint32_t referenceNum;
    NriOptional const void* codecDesc;
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
