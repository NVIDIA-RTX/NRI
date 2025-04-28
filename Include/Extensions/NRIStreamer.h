// © 2024 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriForwardStruct(Streamer);

NriStruct(DataSize) {
    const void* data;
    uint64_t size;
};

NriStruct(BufferOffset) {
    NriPtr(Buffer) buffer;
    uint64_t offset;
};

NriStruct(StreamerDesc) {
    // Statically allocated ring-buffer for dynamic constants
    NriOptional Nri(MemoryLocation) constantBufferMemoryLocation; // UPLOAD or DEVICE_UPLOAD
    NriOptional uint64_t constantBufferSize;

    // Dynamically (re)allocated ring-buffer for copying and rendering
    Nri(MemoryLocation) dynamicBufferMemoryLocation;    // UPLOAD or DEVICE_UPLOAD
    Nri(BufferUsageBits) dynamicBufferUsageBits;
    uint32_t frameInFlightNum;                          // number of frames "in-flight" (usually 1-3)
};

NriStruct(StreamBufferDataDesc) {
    // Data to upload
    const NriPtr(DataSize) dataChunks;                  // will be concatenated in dynamic buffer memory
    uint32_t dataChunkNum;
    uint32_t placementAlignment;                        // desired alignment for "BufferOffset::offset"

    // Destination
    NriOptional NriPtr(Buffer) dstBuffer;
    NriOptional uint64_t dstBufferOffset;
};

NriStruct(StreamTextureDataDesc) {
    // Data to upload
    const void* data;
    uint32_t dataRowPitch;
    uint32_t dataSlicePitch;

    // Destination
    NriPtr(Texture) dstTexture;
    Nri(TextureRegionDesc) dstRegionDesc;
};

// Threadsafe: yes
NriStruct(StreamerInterface) {
    Nri(Result)         (NRI_CALL *CreateStreamer)              (NriRef(Device) device, const NriRef(StreamerDesc) streamerDesc, NriOut NriRef(Streamer*) streamer);
    void                (NRI_CALL *DestroyStreamer)             (NriRef(Streamer) streamer);

    // Statically allocated (never changes)
    NriPtr(Buffer)      (NRI_CALL *GetStreamerConstantBuffer)   (NriRef(Streamer) streamer);

    // (HOST) Copy data to a dynamic buffer. Return "buffer & offset" for direct usage in the current frame
    Nri(BufferOffset)   (NRI_CALL *StreamBufferData)            (NriRef(Streamer) streamer, const NriRef(StreamBufferDataDesc) streamBufferDataDesc);
    Nri(BufferOffset)   (NRI_CALL *StreamTextureData)           (NriRef(Streamer) streamer, const NriRef(StreamTextureDataDesc) streamTextureDataDesc);

    // (HOST) Copy data to a constant buffer. Return "offset" in "GetStreamerConstantBuffer" for direct usage in the current frame
    uint32_t            (NRI_CALL *StreamConstantData)          (NriRef(Streamer) streamer, const void* data, uint32_t dataSize);

    // Command buffer
    // {
            // (DEVICE) Copy data to destinations (if any), barriers are externally controlled
            void        (NRI_CALL *CmdCopyStreamedData)         (NriRef(CommandBuffer) commandBuffer, NriRef(Streamer) streamer);
    // }

    // (HOST) Must be called once at the very end of the frame
    void                (NRI_CALL *StreamerFinalize)            (NriRef(Streamer) streamer);
};

NriNamespaceEnd