// © 2021 NVIDIA Corporation

// Goal: backend-neutral hardware video encode/decode command submission
// Video formats: https://learn.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering

#pragma once

#define NRI_VIDEO_H 1

NriNamespaceBegin

NriForwardStruct(VideoSession);

NriEnum(VideoUsage, uint8_t,
    DECODE,
    ENCODE
);

NriEnum(VideoCodec, uint8_t,
    H264,
    H265,
    AV1
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
    Nri(Texture)* picture;
    uint32_t subresource;
    uint32_t slot;
};

NriStruct(VideoDecodeDesc) {
    Nri(VideoSession)* session;
    Nri(Buffer)* bitstream;
    uint64_t bitstreamOffset;
    uint64_t bitstreamSize;
    Nri(Texture)* dstPicture;
    uint32_t dstSubresource;
    const NriPtr(VideoReference) references;
    uint32_t referenceNum;
    NriOptional const void* codecDesc;
};

NriStruct(VideoEncodeDesc) {
    Nri(VideoSession)* session;
    Nri(Texture)* srcPicture;
    uint32_t srcSubresource;
    Nri(Buffer)* dstBitstream;
    uint64_t dstBitstreamOffset;
    const NriPtr(VideoReference) references;
    uint32_t referenceNum;
    NriOptional const void* codecDesc;
};

// Threadsafe: no
NriStruct(VideoInterface) {
    // Session
    // {
        Nri(Result) (NRI_CALL *CreateVideoSession)  (NriRef(Device) device, const NriRef(VideoSessionDesc) videoSessionDesc, NriOut NriRef(VideoSession*) videoSession);
        void        (NRI_CALL *DestroyVideoSession) (NriRef(VideoSession) videoSession);
    // }

    // Command buffer
    // {
        // Video decode/encode command buffers must be created from "QueueType::VIDEO_DECODE" or "QueueType::VIDEO_ENCODE" queues.
        void (NRI_CALL *CmdDecodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoDecodeDesc) videoDecodeDesc);
        void (NRI_CALL *CmdEncodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoEncodeDesc) videoEncodeDesc);
    // }
};

NriNamespaceEnd
