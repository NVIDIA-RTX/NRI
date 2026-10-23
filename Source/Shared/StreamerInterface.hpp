// © 2024 NVIDIA Corporation

constexpr uint64_t CHUNK_SIZE = 65536;

static inline StreamerCopyBatch PackStreamerCopyBatch(uint32_t index, uint32_t generation) {
    return (uint64_t(generation) << 32) | uint64_t(index + 1);
}

static inline uint32_t GetStreamerCopyBatchIndex(StreamerCopyBatch copyBatch) {
    return uint32_t(copyBatch) - 1;
}

static inline uint32_t GetStreamerCopyBatchGeneration(StreamerCopyBatch copyBatch) {
    return uint32_t(copyBatch >> 32);
}

StreamerImpl::~StreamerImpl() {
    for (GarbageInFlight& garbageInFlight : m_GarbageInFlight)
        m_iCore.DestroyBuffer(garbageInFlight.buffer);

    m_iCore.DestroyBuffer(m_ConstantBuffer);
    m_iCore.DestroyBuffer(m_DynamicBuffer);
}

bool StreamerImpl::Grow() {
    if (m_DynamicBufferOffset <= m_DynamicBufferSizePerFrame)
        return true;

    uint64_t newSize = m_DynamicBufferOffset;
    m_DynamicBufferSizePerFrame = Align(newSize, CHUNK_SIZE);

    // Add to garbage, keeping it alive for some frames
    if (m_DynamicBuffer)
        m_GarbageInFlight.push_back({m_DynamicBuffer, 0});

    // Create a new dynamic buffer
    BufferDesc bufferDesc = m_Desc.dynamicBufferDesc;
    bufferDesc.size = m_DynamicBufferSizePerFrame * m_Desc.queuedFrameNum;

    Result result = m_iCore.CreateCommittedBuffer(m_Device, m_Desc.dynamicBufferMemoryLocation, 0.0f, bufferDesc, m_DynamicBuffer);

    return result == Result::SUCCESS;
}

Result StreamerImpl::Create(const StreamerDesc& desc) {
    if (desc.constantBufferSize) {
        // Create the constant buffer
        BufferDesc bufferDesc = {};
        bufferDesc.size = desc.constantBufferSize;
        bufferDesc.usage = BufferUsageBits::CONSTANT;

        Result result = m_iCore.CreateCommittedBuffer(m_Device, desc.constantBufferMemoryLocation, 0.0f, bufferDesc, m_ConstantBuffer);
        if (result != Result::SUCCESS)
            return result;
    }

    if (desc.hostDataCapacity)
        m_HostData.resize((size_t)desc.hostDataCapacity);

    m_Desc = desc;

    return Result::SUCCESS;
}

StreamerCopyBatch StreamerImpl::BeginCopyBatch() {
#if NRI_STREAMER_THREAD_SAFE
    ExclusiveScope lock(m_Lock);
#endif

    uint32_t index = 0;
    for (; index < m_CopyBatches.size(); index++) {
        if (!m_CopyBatches[index].active)
            break;
    }

    if (index == m_CopyBatches.size())
        m_CopyBatches.emplace_back(((DeviceBase&)m_Device).GetStdAllocator());

    StreamerCopyBatchState& copyBatchState = m_CopyBatches[index];
    copyBatchState.bufferRequests.clear();
    copyBatchState.textureRequests.clear();
    copyBatchState.generation++;
    copyBatchState.active = true;

    return PackStreamerCopyBatch(index, copyBatchState.generation);
}

uint32_t StreamerImpl::StreamConstantData(const void* data, uint32_t dataSize) {
#if NRI_STREAMER_THREAD_SAFE
    ExclusiveScope lock(m_Lock);
#endif

    const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
    m_ConstantBufferOffset = Align(m_ConstantBufferOffset, deviceDesc.memoryAlignment.constantBufferOffset);

    // Update
    if (m_ConstantBufferOffset + dataSize > m_Desc.constantBufferSize)
        m_ConstantBufferOffset = 0;

    uint32_t offset = m_ConstantBufferOffset;

    // Increment head
    m_ConstantBufferOffset += dataSize;

    // Copy
    if (dataSize) {
        uint8_t* dst = (uint8_t*)m_iCore.MapBuffer(*m_ConstantBuffer, offset, dataSize);

        memcpy(dst, data, dataSize);

        m_iCore.UnmapBuffer(*m_ConstantBuffer);
    }

    return offset;
}

void* StreamerImpl::StreamHostData(const void* data, uint64_t dataSize, uint32_t placementAlignment) {
#if NRI_STREAMER_THREAD_SAFE
    ExclusiveScope lock(m_Lock);
#endif

    if (m_HostData.empty())
        return nullptr;

    uint64_t capacity = m_HostData.size();
    uint64_t alignment = std::max(placementAlignment, 1u);
    uint64_t baseAddress = (uint64_t)m_HostData.data();
    uint64_t firstOffset = Align(baseAddress, alignment) - baseAddress;
    m_HostDataOffset = Align(baseAddress + m_HostDataOffset, alignment) - baseAddress;

    // Update
    if (m_HostDataOffset > capacity || dataSize > capacity - m_HostDataOffset)
        m_HostDataOffset = firstOffset;

    if (m_HostDataOffset > capacity || dataSize > capacity - m_HostDataOffset)
        return nullptr;

    uint64_t offset = m_HostDataOffset;

    // Increment head
    m_HostDataOffset += dataSize;

    // Copy
    uint8_t* dst = &m_HostData[(size_t)offset];

    if (dataSize)
        memcpy(dst, data, dataSize);

    return dst;
}

BufferOffset StreamerImpl::StreamBufferData(const StreamBufferDataDesc& streamBufferDataDesc) {
#if NRI_STREAMER_THREAD_SAFE
    ExclusiveScope lock(m_Lock);
#endif

    uint64_t dataSize = 0;
    for (uint32_t i = 0; i < streamBufferDataDesc.dataChunkNum; i++)
        dataSize += streamBufferDataDesc.dataChunks[i].size;

    uint32_t alignment = std::max(streamBufferDataDesc.placementAlignment, 1u);
    m_DynamicBufferOffset = Align(m_DynamicBufferOffset, alignment);

    uint64_t offset = m_FrameIndex * m_DynamicBufferSizePerFrame + m_DynamicBufferOffset;

    // Increment head
    m_DynamicBufferOffset += dataSize;

    // Grow
    if (!Grow())
        return {};

    // Copy
    if (dataSize) {
        uint8_t* dst = (uint8_t*)m_iCore.MapBuffer(*m_DynamicBuffer, offset, dataSize);

        for (uint32_t i = 0; i < streamBufferDataDesc.dataChunkNum; i++) {
            const DataSize& dataChunk = streamBufferDataDesc.dataChunks[i];
            memcpy(dst, dataChunk.data, dataChunk.size);
            dst += dataChunk.size;
        }

        m_iCore.UnmapBuffer(*m_DynamicBuffer);

        // Gather requests with destinations
        if (streamBufferDataDesc.dstBuffer) {
            StreamerCopyBatchState& copyBatchState = m_CopyBatches[GetStreamerCopyBatchIndex(streamBufferDataDesc.copyBatch)];
            NRI_CHECK(copyBatchState.active && copyBatchState.generation == GetStreamerCopyBatchGeneration(streamBufferDataDesc.copyBatch), "Invalid 'copyBatch'");

            BufferUpdateRequest& request = copyBatchState.bufferRequests.emplace_back();
            request = {};
            request.dstBuffer = streamBufferDataDesc.dstBuffer;
            request.dstOffset = streamBufferDataDesc.dstOffset;
            request.srcBuffer = m_DynamicBuffer;
            request.srcOffset = offset;
            request.size = dataSize;
        }
    }

    return {m_DynamicBuffer, offset};
}

BufferOffset StreamerImpl::StreamTextureData(const StreamTextureDataDesc& streamTextureDataDesc) {
#if NRI_STREAMER_THREAD_SAFE
    ExclusiveScope lock(m_Lock);
#endif

    const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = m_iCore.GetTextureDesc(*streamTextureDataDesc.dstTexture);

    Dim_t w = streamTextureDataDesc.dstRegion.width;
    w = w == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 0, streamTextureDataDesc.dstRegion.mipOffset) : w;

    Dim_t h = streamTextureDataDesc.dstRegion.height;
    h = h == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 1, streamTextureDataDesc.dstRegion.mipOffset) : h;

    Dim_t d = streamTextureDataDesc.dstRegion.depth;
    d = d == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 2, streamTextureDataDesc.dstRegion.mipOffset) : d;

    // Allocate a minimum continous region in a buffer encompassing the destination texture region
    const FormatProps& formatProps = GetFormatProps(textureDesc.format);
    uint32_t rowPitch = w * formatProps.stride;
    uint32_t rowNum = h;

    if (formatProps.isCompressed) {
        rowPitch = ((w + formatProps.blockWidth - 1) / formatProps.blockWidth) * formatProps.stride;
        rowNum = (h + formatProps.blockHeight - 1) / formatProps.blockHeight;
    }

    const uint32_t dataRowPitch = streamTextureDataDesc.dataRowPitch ? streamTextureDataDesc.dataRowPitch : rowPitch;
    const uint64_t dataSlicePitch = streamTextureDataDesc.dataSlicePitch ? streamTextureDataDesc.dataSlicePitch : uint64_t(dataRowPitch) * rowNum;

    uint32_t alignedRowPitch = Align(rowPitch, deviceDesc.memoryAlignment.uploadBufferTextureRow);
    uint32_t alignedSlicePitch = Align(alignedRowPitch * rowNum, deviceDesc.memoryAlignment.uploadBufferTextureSlice);
    uint64_t dataSize = alignedSlicePitch * d;

    m_DynamicBufferOffset = Align(m_DynamicBufferOffset, deviceDesc.memoryAlignment.uploadBufferTextureSlice);

    uint64_t offset = m_FrameIndex * m_DynamicBufferSizePerFrame + m_DynamicBufferOffset;

    // Increment head
    m_DynamicBufferOffset += dataSize;

    // Grow
    if (!Grow())
        return {};

    // Copy
    if (dataSize) {
        uint8_t* dst = (uint8_t*)m_iCore.MapBuffer(*m_DynamicBuffer, offset, dataSize);

        for (uint32_t z = 0; z < d; z++) {
            for (uint32_t y = 0; y < rowNum; y++) {
                uint8_t* dstRow = dst + z * alignedSlicePitch + y * alignedRowPitch;
                const uint8_t* srcRow = (uint8_t*)streamTextureDataDesc.data + z * dataSlicePitch + y * dataRowPitch;
                memcpy(dstRow, srcRow, rowPitch);
            }
        }

        m_iCore.UnmapBuffer(*m_DynamicBuffer);

        // Gather request
        StreamerCopyBatchState& copyBatchState = m_CopyBatches[GetStreamerCopyBatchIndex(streamTextureDataDesc.copyBatch)];
        NRI_CHECK(copyBatchState.active && copyBatchState.generation == GetStreamerCopyBatchGeneration(streamTextureDataDesc.copyBatch), "Invalid 'copyBatch'");

        TextureUpdateRequest& request = copyBatchState.textureRequests.emplace_back();
        request = {};
        request.dstTexture = streamTextureDataDesc.dstTexture;
        request.dstRegion = streamTextureDataDesc.dstRegion;
        request.srcBuffer = m_DynamicBuffer;
        request.srcDataLayout = {offset, alignedRowPitch, alignedSlicePitch};
    }

    return {m_DynamicBuffer, offset};
}

void StreamerImpl::CmdCopyStreamedData(CommandBuffer& commandBuffer, StreamerCopyBatch copyBatch) {
#if NRI_STREAMER_THREAD_SAFE
    ExclusiveScope lock(m_Lock);
#endif

    StreamerCopyBatchState& copyBatchState = m_CopyBatches[GetStreamerCopyBatchIndex(copyBatch)];
    NRI_CHECK(copyBatchState.active && copyBatchState.generation == GetStreamerCopyBatchGeneration(copyBatch), "Invalid 'copyBatch'");

    // TODO: dynamic buffer(s) is in the persistent state, including "COPY_SOURCE", so there is no need to do a barrier... right? :)

    // Buffers
    for (const BufferUpdateRequest& request : copyBatchState.bufferRequests)
        m_iCore.CmdCopyBuffer(commandBuffer, *request.dstBuffer, request.dstOffset, *request.srcBuffer, request.srcOffset, request.size);

    // Textures
    for (const TextureUpdateRequest& request : copyBatchState.textureRequests)
        m_iCore.CmdUploadBufferToTexture(commandBuffer, *request.dstTexture, request.dstRegion, *request.srcBuffer, request.srcDataLayout);

    // Cleanup
    copyBatchState.bufferRequests.clear();
    copyBatchState.textureRequests.clear();
    copyBatchState.active = false;
}

void StreamerImpl::EndFrame() {
#if NRI_STREAMER_THREAD_SAFE
    ExclusiveScope lock(m_Lock);
#endif

    // Process garbage
    for (size_t i = 0; i < m_GarbageInFlight.size(); i++) {
        GarbageInFlight& garbageInFlight = m_GarbageInFlight[i];
        if (garbageInFlight.frameNum < m_Desc.queuedFrameNum)
            garbageInFlight.frameNum++;
        else {
            m_iCore.DestroyBuffer(garbageInFlight.buffer);

            m_GarbageInFlight[i--] = m_GarbageInFlight.back();
            m_GarbageInFlight.pop_back();
        }
    }

    // Ignore unprocessed requests, they become invalid on the next frame
    for (StreamerCopyBatchState& copyBatchState : m_CopyBatches) {
        copyBatchState.bufferRequests.clear();
        copyBatchState.textureRequests.clear();
        copyBatchState.active = false;
    }

    // Next frame
    m_FrameIndex = (m_FrameIndex + 1) % m_Desc.queuedFrameNum;
    m_DynamicBufferOffset = 0;
}
