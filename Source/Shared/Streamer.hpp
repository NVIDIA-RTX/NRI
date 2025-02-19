// © 2024 NVIDIA Corporation

constexpr uint64_t CHUNK_SIZE = 65536;

StreamerImpl::~StreamerImpl() {
    for (GarbageInFlight& garbageInFlight : m_GarbageInFlight) {
        m_NRI.DestroyBuffer(*garbageInFlight.buffer);
        m_NRI.FreeMemory(*garbageInFlight.memory);
    }

    m_NRI.DestroyBuffer(*m_ConstantBuffer);
    m_NRI.DestroyBuffer(*m_DynamicBuffer);

    m_NRI.FreeMemory(*m_ConstantBufferMemory);
    m_NRI.FreeMemory(*m_DynamicBufferMemory);
}

Result StreamerImpl::Create(const StreamerDesc& desc) {
    if (desc.constantBufferSize) {
        // Create constant buffer
        BufferDesc bufferDesc = {};
        bufferDesc.size = desc.constantBufferSize;
        bufferDesc.usage = BufferUsageBits::CONSTANT_BUFFER;

        Result result = m_NRI.CreateBuffer(m_Device, bufferDesc, m_ConstantBuffer);
        if (result != Result::SUCCESS)
            return result;

        // Allocate memory
        MemoryDesc memoryDesc = {};
        m_NRI.GetBufferMemoryDesc(*m_ConstantBuffer, desc.constantBufferMemoryLocation, memoryDesc);

        AllocateMemoryDesc allocateMemoryDesc = {};
        allocateMemoryDesc.type = memoryDesc.type;
        allocateMemoryDesc.size = memoryDesc.size;

        result = m_NRI.AllocateMemory(m_Device, allocateMemoryDesc, m_ConstantBufferMemory);
        if (result != Result::SUCCESS)
            return result;

        // Bind to memory
        BufferMemoryBindingDesc memoryBindingDesc = {};
        memoryBindingDesc.buffer = m_ConstantBuffer;
        memoryBindingDesc.memory = m_ConstantBufferMemory;

        result = m_NRI.BindBufferMemory(m_Device, &memoryBindingDesc, 1);
        if (result != Result::SUCCESS)
            return result;
    }

    m_Desc = desc;

    return Result::SUCCESS;
}

uint32_t StreamerImpl::UpdateConstantBuffer(const void* data, uint32_t dataSize) {
    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
    uint32_t alignedSize = Align(dataSize, deviceDesc.constantBufferOffsetAlignment);

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
    uint64_t alignedSize = Align(bufferUpdateRequestDesc.dataSize, 16);

    uint64_t offset = m_DynamicDataOffsetBase + m_DynamicDataOffset;
    m_BufferRequests.push_back({bufferUpdateRequestDesc, m_DynamicDataOffset}); // store local offset
    m_DynamicDataOffset += alignedSize;

    return offset;
}

uint64_t StreamerImpl::AddTextureUpdateRequest(const TextureUpdateRequestDesc& textureUpdateRequestDesc) {
    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*textureUpdateRequestDesc.dstTexture);

    Dim_t h = textureUpdateRequestDesc.dstRegionDesc.height;
    h = h == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 1, textureUpdateRequestDesc.dstRegionDesc.mipOffset) : h;

    Dim_t d = textureUpdateRequestDesc.dstRegionDesc.depth;
    d = d == WHOLE_SIZE ? GetDimension(deviceDesc.graphicsAPI, textureDesc, 2, textureUpdateRequestDesc.dstRegionDesc.mipOffset) : d;

    uint32_t alignedRowPitch = Align(textureUpdateRequestDesc.dataRowPitch, deviceDesc.uploadBufferTextureRowAlignment);
    uint32_t alignedSlicePitch = Align(alignedRowPitch * h, deviceDesc.uploadBufferTextureSliceAlignment);
    uint64_t alignedSize = alignedSlicePitch * d;

    uint64_t offset = m_DynamicDataOffsetBase + m_DynamicDataOffset;
    m_TextureRequests.push_back({textureUpdateRequestDesc, m_DynamicDataOffset}); // store local offset
    m_DynamicDataOffset += alignedSize;

    return offset;
}

Result StreamerImpl::CopyUpdateRequests() {
    if (!m_DynamicDataOffset)
        return Result::SUCCESS;

    // Process garbage
    for (size_t i = 0; i < m_GarbageInFlight.size(); i++) {
        GarbageInFlight& garbageInFlight = m_GarbageInFlight[i];
        if (garbageInFlight.frameNum < m_Desc.frameInFlightNum)
            garbageInFlight.frameNum++;
        else {
            m_NRI.DestroyBuffer(*garbageInFlight.buffer);
            m_NRI.FreeMemory(*garbageInFlight.memory);

            m_GarbageInFlight[i--] = m_GarbageInFlight.back();
            m_GarbageInFlight.pop_back();
        }
    }

    // Grow
    if (m_DynamicDataOffsetBase + m_DynamicDataOffset > m_DynamicBufferSize) {
        m_DynamicBufferSize = Align(m_DynamicDataOffset, CHUNK_SIZE) * (m_Desc.frameInFlightNum + 1);

        // Add the current buffer to the garbage collector immediately, but keep it alive for some frames
        if (m_DynamicBuffer)
            m_GarbageInFlight.push_back({m_DynamicBuffer, m_DynamicBufferMemory, 0});

        { // Create new dynamic buffer & allocate memory
            BufferDesc bufferDesc = {};
            bufferDesc.size = m_DynamicBufferSize;
            bufferDesc.usage = m_Desc.dynamicBufferUsageBits;

            Result result = m_NRI.CreateBuffer(m_Device, bufferDesc, m_DynamicBuffer);
            if (result != Result::SUCCESS)
                return result;

            MemoryDesc memoryDesc = {};
            m_NRI.GetBufferMemoryDesc(*m_DynamicBuffer, m_Desc.dynamicBufferMemoryLocation, memoryDesc);

            AllocateMemoryDesc allocateMemoryDesc = {};
            allocateMemoryDesc.type = memoryDesc.type;
            allocateMemoryDesc.size = memoryDesc.size;

            result = m_NRI.AllocateMemory(m_Device, allocateMemoryDesc, m_DynamicBufferMemory);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Bind to memory
            BufferMemoryBindingDesc memoryBindingDesc = {};
            memoryBindingDesc.buffer = m_DynamicBuffer;
            memoryBindingDesc.memory = m_DynamicBufferMemory;

            Result result = m_NRI.BindBufferMemory(m_Device, &memoryBindingDesc, 1);
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

            uint32_t alignedRowPitch = Align(request.desc.dataRowPitch, deviceDesc.uploadBufferTextureRowAlignment);
            uint32_t alignedSlicePitch = Align(alignedRowPitch * h, deviceDesc.uploadBufferTextureSliceAlignment);

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
