// © 2024 NVIDIA Corporation

constexpr uint64_t CHUNK_SIZE = 65536;
constexpr bool USE_DEDICATED = true;

StreamerImpl::~StreamerImpl() {
    for (GarbageInFlight& garbageInFlight : m_GarbageInFlight)
        m_iCore.DestroyBuffer(*garbageInFlight.buffer);

    m_iCore.DestroyBuffer(*m_ConstantBuffer);
    m_iCore.DestroyBuffer(*m_DynamicBuffer);
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
    AllocateBufferDesc allocateBufferDesc = {};
    allocateBufferDesc.desc.size = m_DynamicBufferSizePerFrame * m_Desc.queuedFrameNum;
    allocateBufferDesc.desc.usage = m_Desc.dynamicBufferUsageBits;
    allocateBufferDesc.memoryLocation = m_Desc.dynamicBufferMemoryLocation;
    allocateBufferDesc.dedicated = USE_DEDICATED;

    Result result = m_iResourceAllocator.AllocateBuffer(m_Device, allocateBufferDesc, m_DynamicBuffer);

    return result == Result::SUCCESS;
}

Result StreamerImpl::Create(const StreamerDesc& desc) {
    Result result = nriGetInterface(m_Device, NRI_INTERFACE(ResourceAllocatorInterface), &m_iResourceAllocator);
    if (result != Result::SUCCESS)
        return result;

    if (desc.constantBufferSize) {
        // Create the constant buffer
        AllocateBufferDesc allocateBufferDesc = {};
        allocateBufferDesc.desc.size = desc.constantBufferSize;
        allocateBufferDesc.desc.usage = BufferUsageBits::CONSTANT_BUFFER;
        allocateBufferDesc.memoryLocation = desc.constantBufferMemoryLocation;
        allocateBufferDesc.dedicated = USE_DEDICATED;

        result = m_iResourceAllocator.AllocateBuffer(m_Device, allocateBufferDesc, m_ConstantBuffer);
        if (result != Result::SUCCESS)
            return result;
    }

    m_Desc = desc;

    return Result::SUCCESS;
}

uint32_t StreamerImpl::StreamConstantData(const void* data, uint32_t dataSize) {
    ExclusiveScope lock(m_Lock);

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

BufferOffset StreamerImpl::StreamBufferData(const StreamBufferDataDesc& streamBufferDataDesc) {
    ExclusiveScope lock(m_Lock);

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
            BufferUpdateRequest& request = m_BufferRequestsWithDst.emplace_back();
            request = {};
            request.dstBuffer = streamBufferDataDesc.dstBuffer;
            request.dstOffset = streamBufferDataDesc.dstBufferOffset;
            request.srcBuffer = m_DynamicBuffer;
            request.srcOffset = offset;
            request.size = dataSize;
        }
    }

    return {m_DynamicBuffer, offset};
}

BufferOffset StreamerImpl::StreamTextureData(const StreamTextureDataDesc& streamTextureDataDesc) {
    ExclusiveScope lock(m_Lock);

    const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = m_iCore.GetTextureDesc(*streamTextureDataDesc.dstTexture);

    Dim_t h = streamTextureDataDesc.dstRegionDesc.height;
    h = h == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 1, streamTextureDataDesc.dstRegionDesc.mipOffset) : h;

    Dim_t d = streamTextureDataDesc.dstRegionDesc.depth;
    d = d == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 2, streamTextureDataDesc.dstRegionDesc.mipOffset) : d;

    uint32_t alignedRowPitch = Align(streamTextureDataDesc.dataRowPitch, deviceDesc.memoryAlignment.uploadBufferTextureRow);
    uint32_t alignedSlicePitch = Align(alignedRowPitch * h, deviceDesc.memoryAlignment.uploadBufferTextureSlice);
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
            for (uint32_t y = 0; y < h; y++) {
                uint8_t* dstRow = dst + z * alignedSlicePitch + y * alignedRowPitch;
                const uint8_t* srcRow = (uint8_t*)streamTextureDataDesc.data + z * streamTextureDataDesc.dataSlicePitch + y * streamTextureDataDesc.dataRowPitch;
                memcpy(dstRow, srcRow, streamTextureDataDesc.dataRowPitch);
            }
        }

        m_iCore.UnmapBuffer(*m_DynamicBuffer);

        // Gather requests with destinations
        TextureUpdateRequest& request = m_TextureRequestsWithDst.emplace_back();
        request = {};
        request.desc = streamTextureDataDesc;
        request.srcBuffer = m_DynamicBuffer;
        request.srcOffset = offset;
    }

    return {m_DynamicBuffer, offset};
}

void StreamerImpl::CmdCopyStreamedData(CommandBuffer& commandBuffer) {
    ExclusiveScope lock(m_Lock);

    // TODO: dynamic buffer(s) is in the persistent state, including "COPY_SOURCE", so there is no need to do a barrier... right? :)

    // Buffers
    for (const BufferUpdateRequest& request : m_BufferRequestsWithDst)
        m_iCore.CmdCopyBuffer(commandBuffer, *request.dstBuffer, request.dstOffset, *request.srcBuffer, request.srcOffset, request.size);

    // Textures
    for (const TextureUpdateRequest& request : m_TextureRequestsWithDst) {
        TextureDataLayoutDesc dataLayout = {};
        dataLayout.offset = request.srcOffset;
        dataLayout.rowPitch = request.desc.dataRowPitch;
        dataLayout.slicePitch = request.desc.dataSlicePitch;

        m_iCore.CmdUploadBufferToTexture(commandBuffer, *request.desc.dstTexture, request.desc.dstRegionDesc, *request.srcBuffer, dataLayout);
    }
}

void StreamerImpl::Finalize() {
    // Process garbage
    for (size_t i = 0; i < m_GarbageInFlight.size(); i++) {
        GarbageInFlight& garbageInFlight = m_GarbageInFlight[i];
        if (garbageInFlight.frameNum < m_Desc.queuedFrameNum)
            garbageInFlight.frameNum++;
        else {
            m_iCore.DestroyBuffer(*garbageInFlight.buffer);

            m_GarbageInFlight[i--] = m_GarbageInFlight.back();
            m_GarbageInFlight.pop_back();
        }
    }

    // Cleanup
    m_BufferRequestsWithDst.clear();
    m_TextureRequestsWithDst.clear();

    // Next frame
    m_FrameIndex = (m_FrameIndex + 1) % m_Desc.queuedFrameNum;
    m_DynamicBufferOffset = 0;
}
