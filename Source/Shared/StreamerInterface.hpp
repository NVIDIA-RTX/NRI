// © 2024 NVIDIA Corporation

constexpr uint64_t CHUNK_SIZE = 65536;

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
        bufferDesc.usage = BufferUsageBits::CONSTANT_BUFFER;

        Result result = m_iCore.CreateCommittedBuffer(m_Device, desc.constantBufferMemoryLocation, 0.0f, bufferDesc, m_ConstantBuffer);
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
            request.dstOffset = streamBufferDataDesc.dstOffset;
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

    Dim_t w = streamTextureDataDesc.dstRegion.width;
    w = w == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 0, streamTextureDataDesc.dstRegion.mipOffset) : w;

    Dim_t h = streamTextureDataDesc.dstRegion.height;
    h = h == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 1, streamTextureDataDesc.dstRegion.mipOffset) : h;

    Dim_t d = streamTextureDataDesc.dstRegion.depth;
    d = d == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 2, streamTextureDataDesc.dstRegion.mipOffset) : d;

    // Allocate a minimum continous region in a buffer encompassing the destination texture region
    const FormatProps& formatProps = GetFormatProps(textureDesc.format);
    uint32_t rowPitch = w * formatProps.stride;
    uint32_t alignedRowPitch = Align(rowPitch, deviceDesc.memoryAlignment.uploadBufferTextureRow);
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
                memcpy(dstRow, srcRow, rowPitch);
            }
        }

        m_iCore.UnmapBuffer(*m_DynamicBuffer);

        // Gather requests with destinations
        if (streamTextureDataDesc.dstTexture) {
            TextureUpdateRequest& request = m_TextureRequestsWithDst.emplace_back();
            request = {};
            request.dstTexture = streamTextureDataDesc.dstTexture;
            request.dstRegion = streamTextureDataDesc.dstRegion;
            request.srcBuffer = m_DynamicBuffer;
            request.srcDataLayout = {offset, alignedRowPitch, alignedSlicePitch};
        }
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
    for (const TextureUpdateRequest& request : m_TextureRequestsWithDst)
        m_iCore.CmdUploadBufferToTexture(commandBuffer, *request.dstTexture, request.dstRegion, *request.srcBuffer, request.srcDataLayout);

    // Cleanup
    m_BufferRequestsWithDst.clear();
    m_TextureRequestsWithDst.clear();
}

void StreamerImpl::EndFrame() {
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
    m_BufferRequestsWithDst.clear();
    m_TextureRequestsWithDst.clear();

    // Next frame
    m_FrameIndex = (m_FrameIndex + 1) % m_Desc.queuedFrameNum;
    m_DynamicBufferOffset = 0;
}
