// © 2024 NVIDIA Corporation

constexpr uint32_t COPY_BUFFER_ALIGNMENT = 1;
constexpr uint64_t CHUNK_SIZE = 65536;
constexpr bool USE_DEDICATED = true;

StreamerImpl::~StreamerImpl() {
    for (GarbageInFlight& garbageInFlight : m_GarbageInFlight)
        m_NRI.DestroyBuffer(*garbageInFlight.buffer);

    m_NRI.DestroyBuffer(*m_ConstantBuffer);
    m_NRI.DestroyBuffer(*m_DynamicBuffer);
}

Result StreamerImpl::Create(const StreamerDesc& desc) {
    ResourceAllocatorInterface iResourceAllocator = {};
    Result result = nriGetInterface(m_Device, NRI_INTERFACE(ResourceAllocatorInterface), &iResourceAllocator);
    if (result != Result::SUCCESS)
        return result;

    if (desc.constantBufferSize) {
        // Create the constant buffer
        AllocateBufferDesc allocateBufferDesc = {};
        allocateBufferDesc.desc.size = desc.constantBufferSize;
        allocateBufferDesc.desc.usage = BufferUsageBits::CONSTANT_BUFFER;
        allocateBufferDesc.memoryLocation = desc.constantBufferMemoryLocation;
        allocateBufferDesc.dedicated = USE_DEDICATED;

        result = iResourceAllocator.AllocateBuffer(m_Device, allocateBufferDesc, m_ConstantBuffer);
        if (result != Result::SUCCESS)
            return result;
    }

    if (desc.dynamicBufferSize) {
        // Create the dynamic buffer
        AllocateBufferDesc allocateBufferDesc = {};
        allocateBufferDesc.desc.size = desc.dynamicBufferSize;
        allocateBufferDesc.desc.usage = desc.dynamicBufferUsageBits;
        allocateBufferDesc.memoryLocation = desc.dynamicBufferMemoryLocation;
        allocateBufferDesc.dedicated = USE_DEDICATED;

        result = iResourceAllocator.AllocateBuffer(m_Device, allocateBufferDesc, m_DynamicBuffer);
        if (result != Result::SUCCESS)
            return result;

        m_DynamicBufferSize = desc.dynamicBufferSize;
    }

    m_Desc = desc;

    return Result::SUCCESS;
}

uint32_t StreamerImpl::UpdateConstantBuffer(const void* data, uint32_t dataSize) {
    ExclusiveScope lock(m_Lock);

    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
    uint32_t alignedSize = Align(dataSize, deviceDesc.memoryAlignment.constantBufferOffset);

    // Update
    if (m_ConstantDataOffset + alignedSize > m_Desc.constantBufferSize)
        m_ConstantDataOffset = 0;

    uint32_t offset = m_ConstantDataOffset;
    m_ConstantDataOffset += alignedSize;

    // Copy
    uint8_t* dest = (uint8_t*)m_NRI.MapBuffer(*m_ConstantBuffer, offset, alignedSize);
    if (dest) {
        memcpy(dest, data, dataSize);
        m_NRI.UnmapBuffer(*m_ConstantBuffer);
    }

    return offset;
}

uint64_t StreamerImpl::AddBufferUpdateRequest(const BufferUpdateRequestDesc& bufferUpdateRequestDesc) {
    ExclusiveScope lock(m_Lock);

    m_DynamicDataOffset = Align(m_DynamicDataOffset, COPY_BUFFER_ALIGNMENT);

    uint64_t alignedSize = Align(bufferUpdateRequestDesc.dataSize, COPY_BUFFER_ALIGNMENT);
    uint64_t offset = m_DynamicDataOffsetBase + m_DynamicDataOffset;

    BufferUpdateRequest& request = m_BufferRequests.emplace_back();
    request = {};
    request.desc = bufferUpdateRequestDesc;
    request.offset = m_DynamicDataOffset; // store local offset

    m_DynamicDataOffset += alignedSize;

    return offset;
}

uint64_t StreamerImpl::AddTextureUpdateRequest(const TextureUpdateRequestDesc& textureUpdateRequestDesc) {
    ExclusiveScope lock(m_Lock);

    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*textureUpdateRequestDesc.dstTexture);

    Dim_t h = textureUpdateRequestDesc.dstRegionDesc.height;
    h = h == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 1, textureUpdateRequestDesc.dstRegionDesc.mipOffset) : h;

    Dim_t d = textureUpdateRequestDesc.dstRegionDesc.depth;
    d = d == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 2, textureUpdateRequestDesc.dstRegionDesc.mipOffset) : d;

    uint32_t alignedRowPitch = Align(textureUpdateRequestDesc.dataRowPitch, deviceDesc.memoryAlignment.uploadBufferTextureRow);
    uint32_t alignedSlicePitch = Align(alignedRowPitch * h, deviceDesc.memoryAlignment.uploadBufferTextureSlice);

    m_DynamicDataOffset = Align(m_DynamicDataOffset, deviceDesc.memoryAlignment.uploadBufferTextureSlice);

    uint64_t alignedSize = alignedSlicePitch * d;
    uint64_t offset = m_DynamicDataOffsetBase + m_DynamicDataOffset;

    TextureUpdateRequest& request = m_TextureRequests.emplace_back();
    request = {};
    request.desc = textureUpdateRequestDesc;
    request.offset = m_DynamicDataOffset; // store local offset

    m_DynamicDataOffset += alignedSize;

    return offset;
}

Result StreamerImpl::CopyUpdateRequests() {
    ExclusiveScope lock(m_Lock);

    if (!m_DynamicDataOffset)
        return Result::SUCCESS;

    // Process garbage
    for (size_t i = 0; i < m_GarbageInFlight.size(); i++) {
        GarbageInFlight& garbageInFlight = m_GarbageInFlight[i];
        if (garbageInFlight.frameNum < m_Desc.frameInFlightNum)
            garbageInFlight.frameNum++;
        else {
            m_NRI.DestroyBuffer(*garbageInFlight.buffer);

            m_GarbageInFlight[i--] = m_GarbageInFlight.back();
            m_GarbageInFlight.pop_back();
        }
    }

    // Grow
    if (m_DynamicDataOffsetBase + m_DynamicDataOffset > m_DynamicBufferSize) {
        if (m_Desc.dynamicBufferSize)
            return Result::OUT_OF_MEMORY;

        m_DynamicBufferSize = Align(m_DynamicDataOffset, CHUNK_SIZE) * (m_Desc.frameInFlightNum + 1);

        // Add the current buffer to the garbage collector immediately, but keep it alive for some frames
        if (m_DynamicBuffer)
            m_GarbageInFlight.push_back({m_DynamicBuffer, 0});

        { // Create new dynamic buffer
            ResourceAllocatorInterface iResourceAllocator = {};
            Result result = nriGetInterface(m_Device, NRI_INTERFACE(ResourceAllocatorInterface), &iResourceAllocator);
            if (result != Result::SUCCESS)
                return result;

            AllocateBufferDesc allocateBufferDesc = {};
            allocateBufferDesc.desc.size = m_DynamicBufferSize;
            allocateBufferDesc.desc.usage = m_Desc.dynamicBufferUsageBits;
            allocateBufferDesc.memoryLocation = m_Desc.dynamicBufferMemoryLocation;
            allocateBufferDesc.dedicated = USE_DEDICATED;

            result = iResourceAllocator.AllocateBuffer(m_Device, allocateBufferDesc, m_DynamicBuffer);
            if (result != Result::SUCCESS)
                return result;
        }
    }

    // Concatenate & copy to the internal buffer, gather requests with destinations
    uint8_t* data = (uint8_t*)m_NRI.MapBuffer(*m_DynamicBuffer, m_DynamicDataOffsetBase, m_DynamicDataOffset);
    if (data) {
        const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);

        // Buffers
        for (BufferUpdateRequest& request : m_BufferRequests) {
            uint8_t* dst = data + request.offset;
            memcpy(dst, request.desc.data, request.desc.dataSize);

            if (request.desc.dstBuffer) {
                request.offset += m_DynamicDataOffsetBase; // convert to global offset
                m_BufferRequestsWithDst.push_back(request);
            }
        }

        // Textures
        for (TextureUpdateRequest& request : m_TextureRequests) {
            uint8_t* dst = data + request.offset;
            const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*request.desc.dstTexture);

            Dim_t h = request.desc.dstRegionDesc.height;
            h = h == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 1, request.desc.dstRegionDesc.mipOffset) : h;

            Dim_t d = request.desc.dstRegionDesc.depth;
            d = d == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 2, request.desc.dstRegionDesc.mipOffset) : d;

            uint32_t alignedRowPitch = Align(request.desc.dataRowPitch, deviceDesc.memoryAlignment.uploadBufferTextureRow);
            uint32_t alignedSlicePitch = Align(alignedRowPitch * h, deviceDesc.memoryAlignment.uploadBufferTextureSlice);

            for (uint32_t z = 0; z < d; z++) {
                for (uint32_t y = 0; y < h; y++) {
                    uint8_t* dstRow = dst + z * alignedSlicePitch + y * alignedRowPitch;
                    const uint8_t* srcRow = (uint8_t*)request.desc.data + z * request.desc.dataSlicePitch + y * request.desc.dataRowPitch;
                    memcpy(dstRow, srcRow, request.desc.dataRowPitch);
                }
            }

            if (request.desc.dstTexture) {
                request.offset += m_DynamicDataOffsetBase; // convert to global offset
                m_TextureRequestsWithDst.push_back(request);
            }
        }

        m_NRI.UnmapBuffer(*m_DynamicBuffer);
    } else
        return Result::FAILURE;

    // Cleanup
    m_BufferRequests.clear();
    m_TextureRequests.clear();

    m_FrameIndex = (m_FrameIndex + 1) % (m_Desc.frameInFlightNum + 1);

    if (m_FrameIndex == 0)
        m_DynamicDataOffsetBase = 0;
    else
        m_DynamicDataOffsetBase += m_DynamicDataOffset;

    m_DynamicDataOffset = 0;

    return Result::SUCCESS;
}

void StreamerImpl::CmdUploadUpdateRequests(CommandBuffer& commandBuffer) {
    ExclusiveScope lock(m_Lock);

    // Buffers
    for (const BufferUpdateRequest& request : m_BufferRequestsWithDst)
        m_NRI.CmdCopyBuffer(commandBuffer, *request.desc.dstBuffer, request.desc.dstBufferOffset, *m_DynamicBuffer, request.offset, request.desc.dataSize);

    // Textures
    for (const TextureUpdateRequest& request : m_TextureRequestsWithDst) {
        TextureDataLayoutDesc dataLayout = {};
        dataLayout.offset = request.offset;
        dataLayout.rowPitch = request.desc.dataRowPitch;
        dataLayout.slicePitch = request.desc.dataSlicePitch;

        m_NRI.CmdUploadBufferToTexture(commandBuffer, *request.desc.dstTexture, request.desc.dstRegionDesc, *m_DynamicBuffer, dataLayout);
    }

    // Cleanup
    m_BufferRequestsWithDst.clear();
    m_TextureRequestsWithDst.clear();
}
