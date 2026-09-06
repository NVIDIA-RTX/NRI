// © 2024 NVIDIA Corporation

// Goal: data streaming

#pragma once

#define NRI_STREAMER_H 1

/*
Single-threaded usage:
- stream constants, host data or dynamic-buffer data as needed
- for destination copies, begin a batch, set it in each "StreamBufferDataDesc" or "StreamTextureDataDesc" and record "CmdCopyStreamedData" once
- call "EndStreamerFrame" after all command recording that uses streamed data is complete

Multi-threaded usage:
- one "Streamer" can serve multiple threads if "NRI_STREAMER_THREAD_SAFE" is enabled; otherwise synchronize all access externally
- use a different copy batch for each independently populated command buffer and finish populating it before "CmdCopyStreamedData"
- synchronize all users before "EndStreamerFrame" and size the constant and host-data rings to avoid overwriting live data
*/

NriNamespaceBegin

NriForwardStruct(Streamer);

// Identifier for a batch of destination copy requests scoped to a "Streamer" (0 is invalid)
typedef uint64_t Nri(StreamerCopyBatch);

NriStruct(StreamerDesc) {
    // Statically allocated ring-buffer for dynamic constants
    NriOptional Nri(MemoryLocation) constantBufferMemoryLocation;   // UPLOAD or DEVICE_UPLOAD
    NriOptional uint64_t constantBufferSize;                        // should be large enough to avoid overwriting data for enqueued frames

    // Dynamically (re)allocated ring-buffer for copying and rendering
    Nri(MemoryLocation) dynamicBufferMemoryLocation;                // UPLOAD or DEVICE_UPLOAD
    Nri(BufferDesc) dynamicBufferDesc;                              // "size" is ignored
    uint32_t queuedFrameNum;                                        // number of frames "in-flight" (usually 1-3), adds 1 under the hood for the current "not-yet-committed" frame

    // Statically allocated read/write ring-buffer for generic CPU data
    NriOptional uint64_t hostDataCapacity;                          // capacity for all simultaneously live data and alignment padding
};

NriStruct(StreamBufferDataDesc) {
    // Data to upload
    const NriPtr(DataSize) dataChunks;              // will be concatenated in dynamic buffer memory
    uint32_t dataChunkNum;
    uint32_t placementAlignment;                    // desired alignment for "BufferOffset::offset"

    // Destination
    NriOptional Nri(StreamerCopyBatch) copyBatch;   // required if "dstBuffer" is not NULL
    NriOptional NriPtr(Buffer) dstBuffer;
    NriOptional uint64_t dstOffset;
};

NriStruct(StreamTextureDataDesc) {
    // Data to upload
    const void* data;
    uint32_t dataRowPitch;
    uint32_t dataSlicePitch;

    // Destination
    Nri(StreamerCopyBatch) copyBatch;
    NriPtr(Texture) dstTexture;
    NriOptional Nri(TextureRegionDesc) dstRegion;
};

// Threadsafe: yes by default (see NRI_STREAMER_THREAD_SAFE CMake option)
// Different batches can be populated concurrently; using the same batch concurrently requires external synchronization
NriStruct(StreamerInterface) {
    Nri(Result)             (NRI_CALL *CreateStreamer)              (NriRef(Device) device, const NriRef(StreamerDesc) streamerDesc, NriOut NriRef(Streamer*) streamer);
    void                    (NRI_CALL *DestroyStreamer)             (NriPtr(Streamer) streamer);

    // Statically allocated (never changes)
    NriPtr(Buffer)          (NRI_CALL *GetStreamerConstantBuffer)   (NriRef(Streamer) streamer);

    // (HOST) Streams data to a constant buffer. Returns "offset" in "GetStreamerConstantBuffer" for direct usage in the current frame
    uint32_t                (NRI_CALL *StreamConstantData)          (NriRef(Streamer) streamer, const void* data, uint32_t dataSize);

    // (HOST) Streams generic CPU data. Returns a read/write pointer that remains valid until the ring buffer wraps. "placementAlignment" must be a power of 2 (0 is treated as 1)
    void*                   (NRI_CALL *StreamHostData)              (NriRef(Streamer) streamer, const void* data, uint64_t dataSize, uint32_t placementAlignment);

    // Copy batch identifier (unconsumed batches are discarded by "EndStreamerFrame")
    Nri(StreamerCopyBatch)  (NRI_CALL *BeginStreamerCopyBatch)      (NriRef(Streamer) streamer);

    // (HOST) Stream data to a dynamic buffer. Return "buffer & offset" for direct usage in the current frame
    Nri(BufferOffset)       (NRI_CALL *StreamBufferData)            (NriRef(Streamer) streamer, const NriRef(StreamBufferDataDesc) streamBufferDataDesc);
    Nri(BufferOffset)       (NRI_CALL *StreamTextureData)           (NriRef(Streamer) streamer, const NriRef(StreamTextureDataDesc) streamTextureDataDesc);

    // Command buffer
    // {
        // (DEVICE) Copies batched data to destinations, which must be in "COPY_DESTINATION" state. Consumes "copyBatch", making it invalid
        void                (NRI_CALL *CmdCopyStreamedData)         (NriRef(CommandBuffer) commandBuffer, NriRef(Streamer) streamer, Nri(StreamerCopyBatch) copyBatch);
    // }

    // (HOST) Must be called once at the very end of the frame (also discards unconsumed copy batches)
    void                    (NRI_CALL *EndStreamerFrame)            (NriRef(Streamer) streamer);
};

NriNamespaceEnd
