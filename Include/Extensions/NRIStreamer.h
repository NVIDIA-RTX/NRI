// © 2024 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriForwardStruct(Streamer);

NriStruct(StreamerDesc) {
    // Statically allocated ring-buffer for dynamic constants
    NriOptional Nri(MemoryLocation) constantBufferMemoryLocation; // UPLOAD or DEVICE_UPLOAD
    NriOptional uint64_t constantBufferSize;

    // Dynamically (re)allocated ring-buffer for copying and rendering
    Nri(MemoryLocation) dynamicBufferMemoryLocation; // UPLOAD or DEVICE_UPLOAD
    Nri(BufferUsageBits) dynamicBufferUsageBits;
    uint32_t frameInFlightNum;
};

NriStruct(BufferUpdateRequestDesc) {
    // Data to upload
    const void* data; // pointer must be valid until "CopyStreamerUpdateRequests" call
    uint64_t dataSize;

    // Destination (ignored for constants)
    NriOptional NriPtr(Buffer) dstBuffer;
    NriOptional uint64_t dstBufferOffset;
};

NriStruct(TextureUpdateRequestDesc) {
    // Data to upload
    const void* data; // pointer must be valid until "CopyStreamerUpdateRequests" call
    uint32_t dataRowPitch;
    uint32_t dataSlicePitch;

    // Destination
    NriPtr(Texture) dstTexture;
    Nri(TextureRegionDesc) dstRegionDesc;
};

NriStruct(StreamerInterface) {
    Nri(Result)     (NRI_CALL *CreateStreamer)                  (NriRef(Device) device, const NriRef(StreamerDesc) streamerDesc, NriOut NriRef(Streamer*) streamer);
    void            (NRI_CALL *DestroyStreamer)                 (NriRef(Streamer) streamer);

    // Get internal buffers
    NriPtr(Buffer)  (NRI_CALL *GetStreamerConstantBuffer)       (NriRef(Streamer) streamer); // Never changes (if requested)
    NriPtr(Buffer)  (NRI_CALL *GetStreamerDynamicBuffer)        (NriRef(Streamer) streamer); // Valid only after "CopyStreamerUpdateRequests"

    // Add an update request. Return the offset in the ring buffer and don't invoke any work
    uint64_t        (NRI_CALL *AddStreamerBufferUpdateRequest)  (NriRef(Streamer) streamer, const NriRef(BufferUpdateRequestDesc) bufferUpdateRequestDesc);
    uint64_t        (NRI_CALL *AddStreamerTextureUpdateRequest) (NriRef(Streamer) streamer, const NriRef(TextureUpdateRequestDesc) textureUpdateRequestDesc);

    // (HOST) Copy data and get the offset in the dedicated ring buffer (for dynamic constant buffers)
    uint32_t        (NRI_CALL *UpdateStreamerConstantBuffer)    (NriRef(Streamer) streamer, const void* data, uint32_t dataSize);

    // (HOST) Copy gathered requests to the internal buffer, potentially a new one if the capacity exceeded. Must be called once per frame
    Nri(Result)     (NRI_CALL *CopyStreamerUpdateRequests)      (NriRef(Streamer) streamer);

    // (DEVICE) Copy data to destinations (if any), barriers are externally controlled. Must be called after "CopyStreamerUpdateRequests"
    // WARNING: D3D12 can silently promote a resource state to COPY_DESTINATION!
    void            (NRI_CALL *CmdUploadStreamerUpdateRequests) (NriRef(CommandBuffer) commandBuffer, NriRef(Streamer) streamer);
};

NriNamespaceEnd