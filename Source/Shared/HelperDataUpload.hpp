// © 2021 NVIDIA Corporation

constexpr uint32_t BARRIERS_PER_PASS = 256;
constexpr uint64_t COPY_ALIGNMENT = 16;

static void DoTransition(const CoreInterface& m_NRI, CommandBuffer* commandBuffer, bool isInitial, const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum) {
    TextureBarrierDesc textureBarriers[BARRIERS_PER_PASS];

    const AccessLayoutStage state = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY};
    const AccessLayoutStage initialState = {AccessBits::UNKNOWN, Layout::UNKNOWN, StageBits::NONE};

    for (uint32_t i = 0; i < textureDataDescNum;) {
        const uint32_t passBegin = i;
        const uint32_t passEnd = std::min(i + BARRIERS_PER_PASS, textureDataDescNum);

        for (; i < passEnd; i++) {
            const TextureUploadDesc& textureUploadDesc = textureUploadDescs[i];
            const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*textureUploadDesc.texture);

            TextureBarrierDesc& barrier = textureBarriers[i - passBegin];
            barrier = {};
            barrier.texture = textureUploadDesc.texture;
            barrier.mipNum = textureDesc.mipNum;
            barrier.layerNum = textureDesc.layerNum;
            barrier.before = isInitial ? initialState : state;
            barrier.after = isInitial ? state : textureUploadDesc.after;
            barrier.planes = isInitial ? PlaneBits::ALL : textureUploadDesc.planes;
        }

        BarrierGroupDesc barrierGroup = {};
        barrierGroup.textures = textureBarriers;
        barrierGroup.textureNum = uint16_t(passEnd - passBegin);

        m_NRI.CmdBarrier(*commandBuffer, barrierGroup);
    }
}

static void DoTransition(const CoreInterface& m_NRI, CommandBuffer* commandBuffer, bool isInitial, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    BufferBarrierDesc bufferBarriers[BARRIERS_PER_PASS];

    const AccessStage state = {AccessBits::COPY_DESTINATION, StageBits::COPY};
    const AccessStage initialState = {AccessBits::UNKNOWN, StageBits::NONE};

    for (uint32_t i = 0; i < bufferUploadDescNum;) {
        const uint32_t passBegin = i;
        const uint32_t passEnd = std::min(i + BARRIERS_PER_PASS, bufferUploadDescNum);

        for (; i < passEnd; i++) {
            const BufferUploadDesc& bufferUploadDesc = bufferUploadDescs[i];

            BufferBarrierDesc& barrier = bufferBarriers[i - passBegin];
            barrier = {};
            barrier.buffer = bufferUploadDesc.buffer;
            barrier.before = isInitial ? initialState : state;
            barrier.after = isInitial ? state : bufferUploadDesc.after;
        }

        BarrierGroupDesc barrierGroup = {};
        barrierGroup.buffers = bufferBarriers;
        barrierGroup.bufferNum = uint16_t(passEnd - passBegin);

        m_NRI.CmdBarrier(*commandBuffer, barrierGroup);
    }
}

Result HelperDataUpload::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);

    for (uint32_t i = 0; i < textureUploadDescNum; i++) {
        if (!textureUploadDescs[i].subresources)
            continue;

        const TextureSubresourceUploadDesc& subresource = textureUploadDescs[i].subresources[0];

        uint32_t sliceRowNum = std::max(subresource.slicePitch / subresource.rowPitch, 1u);
        uint64_t alignedRowPitch = Align(subresource.rowPitch, deviceDesc.uploadBufferTextureRowAlignment);
        uint64_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.uploadBufferTextureSliceAlignment);
        uint64_t contentSize = alignedSlicePitch * std::max(subresource.sliceNum, 1u);

        m_UploadBufferSize = std::max(m_UploadBufferSize, contentSize);
    }

    m_UploadBufferSize = Align(m_UploadBufferSize, COPY_ALIGNMENT);

    Result result = Create();
    if (result == Result::SUCCESS) {
        result = UploadTextures(textureUploadDescs, textureUploadDescNum);
        if (result == Result::SUCCESS)
            result = UploadBuffers(bufferUploadDescs, bufferUploadDescNum);
    }

    m_NRI.DestroyCommandBuffer(*m_CommandBuffer);
    m_NRI.DestroyCommandAllocator(*m_CommandAllocators);
    m_NRI.DestroyFence(*m_Fence);
    m_NRI.DestroyBuffer(*m_UploadBuffer);
    m_NRI.FreeMemory(*m_UploadBufferMemory);

    return result;
}

Result HelperDataUpload::Create() {
    BufferDesc bufferDesc = {};
    bufferDesc.size = m_UploadBufferSize;

    Result result = m_NRI.CreateBuffer(m_Device, bufferDesc, m_UploadBuffer);
    if (result != Result::SUCCESS)
        return result;

    MemoryDesc memoryDesc = {};
    m_NRI.GetBufferMemoryDesc(*m_UploadBuffer, MemoryLocation::HOST_UPLOAD, memoryDesc);

    AllocateMemoryDesc allocateMemoryDesc = {};
    allocateMemoryDesc.type = memoryDesc.type;
    allocateMemoryDesc.size = memoryDesc.size;

    result = m_NRI.AllocateMemory(m_Device, allocateMemoryDesc, m_UploadBufferMemory);
    if (result != Result::SUCCESS)
        return result;

    const BufferMemoryBindingDesc bufferMemoryBindingDesc = {m_UploadBufferMemory, m_UploadBuffer, 0};
    result = m_NRI.BindBufferMemory(m_Device, &bufferMemoryBindingDesc, 1);
    if (result != Result::SUCCESS)
        return result;

    result = m_NRI.CreateFence(m_Device, 0, m_Fence);
    if (result != Result::SUCCESS)
        return result;

    result = m_NRI.CreateCommandAllocator(m_Queue, m_CommandAllocators);
    if (result != Result::SUCCESS)
        return result;

    result = m_NRI.CreateCommandBuffer(*m_CommandAllocators, m_CommandBuffer);
    if (result != Result::SUCCESS)
        return result;

    return result;
}

Result HelperDataUpload::UploadTextures(const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum) {
    if (!textureDataDescNum)
        return Result::SUCCESS;

    bool isInitial = true;
    uint32_t i = 0;
    Dim_t layerOffset = 0;
    Mip_t mipOffset = 0;

    while (i < textureDataDescNum) {
        if (!isInitial) {
            Result result = EndCommandBuffersAndSubmit();
            if (result != Result::SUCCESS)
                return result;
        }

        Result result = m_NRI.BeginCommandBuffer(*m_CommandBuffer, nullptr);
        if (result != Result::SUCCESS)
            return result;

        if (isInitial) {
            DoTransition(m_NRI, m_CommandBuffer, true, textureUploadDescs, textureDataDescNum);
            isInitial = false;
        }

        m_UploadBufferOffset = 0;
        bool isCapacityInsufficient = false;

        for (; i < textureDataDescNum && CopyTextureContent(textureUploadDescs[i], layerOffset, mipOffset, isCapacityInsufficient); i++)
            ;

        if (isCapacityInsufficient)
            return Result::OUT_OF_MEMORY;
    }

    DoTransition(m_NRI, m_CommandBuffer, false, textureUploadDescs, textureDataDescNum);

    return EndCommandBuffersAndSubmit();
}

Result HelperDataUpload::UploadBuffers(const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    if (!bufferUploadDescNum)
        return Result::SUCCESS;

    bool isInitial = true;
    uint32_t i = 0;
    uint64_t bufferContentOffset = 0;

    while (i < bufferUploadDescNum) {
        if (!isInitial) {
            Result result = EndCommandBuffersAndSubmit();
            if (result != Result::SUCCESS)
                return result;
        }

        Result result = m_NRI.BeginCommandBuffer(*m_CommandBuffer, nullptr);
        if (result != Result::SUCCESS)
            return result;

        if (isInitial) {
            DoTransition(m_NRI, m_CommandBuffer, true, bufferUploadDescs, bufferUploadDescNum);
            isInitial = false;
        }

        m_UploadBufferOffset = 0;
        m_MappedMemory = (uint8_t*)m_NRI.MapBuffer(*m_UploadBuffer, 0, m_UploadBufferSize);

        for (; i < bufferUploadDescNum && CopyBufferContent(bufferUploadDescs[i], bufferContentOffset); i++)
            ;

        m_NRI.UnmapBuffer(*m_UploadBuffer);
    }

    DoTransition(m_NRI, m_CommandBuffer, false, bufferUploadDescs, bufferUploadDescNum);

    return EndCommandBuffersAndSubmit();
}

Result HelperDataUpload::EndCommandBuffersAndSubmit() {
    const Result result = m_NRI.EndCommandBuffer(*m_CommandBuffer);
    if (result != Result::SUCCESS)
        return result;

    FenceSubmitDesc fenceSubmitDesc = {};
    fenceSubmitDesc.fence = m_Fence;
    fenceSubmitDesc.value = m_FenceValue;

    QueueSubmitDesc queueSubmitDesc = {};
    queueSubmitDesc.commandBufferNum = 1;
    queueSubmitDesc.commandBuffers = &m_CommandBuffer;
    queueSubmitDesc.signalFences = &fenceSubmitDesc;
    queueSubmitDesc.signalFenceNum = 1;

    m_NRI.QueueSubmit(m_Queue, queueSubmitDesc);
    m_NRI.Wait(*m_Fence, m_FenceValue);

    m_FenceValue++;

    m_NRI.ResetCommandAllocator(*m_CommandAllocators);

    return Result::SUCCESS;
}

bool HelperDataUpload::CopyTextureContent(const TextureUploadDesc& textureUploadDesc, Dim_t& layerOffset, Mip_t& mipOffset, bool& isCapacityInsufficient) {
    if (!textureUploadDesc.subresources)
        return true;

    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*textureUploadDesc.texture);

    for (; layerOffset < textureDesc.layerNum; layerOffset++) {
        for (; mipOffset < textureDesc.mipNum; mipOffset++) {
            const auto& subresource = textureUploadDesc.subresources[layerOffset * textureDesc.mipNum + mipOffset];

            uint32_t sliceRowNum = subresource.slicePitch / subresource.rowPitch;
            uint32_t alignedRowPitch = Align(subresource.rowPitch, deviceDesc.uploadBufferTextureRowAlignment);
            uint32_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.uploadBufferTextureSliceAlignment);
            uint64_t contentSize = uint64_t(alignedSlicePitch) * subresource.sliceNum;
            uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;

            if (contentSize > freeSpace) {
                isCapacityInsufficient = contentSize > m_UploadBufferSize;
                return false;
            }

            CopyTextureSubresourceContent(subresource, alignedRowPitch, alignedSlicePitch);

            TextureDataLayoutDesc srcDataLayout = {};
            srcDataLayout.offset = m_UploadBufferOffset;
            srcDataLayout.rowPitch = alignedRowPitch;
            srcDataLayout.slicePitch = alignedSlicePitch;

            TextureRegionDesc dstRegion = {};
            dstRegion.layerOffset = layerOffset;
            dstRegion.mipOffset = mipOffset;

            m_NRI.CmdUploadBufferToTexture(*m_CommandBuffer, *textureUploadDesc.texture, dstRegion, *m_UploadBuffer, srcDataLayout);

            m_UploadBufferOffset = Align(m_UploadBufferOffset + contentSize, COPY_ALIGNMENT);
        }
        mipOffset = 0;
    }
    layerOffset = 0;

    m_UploadBufferOffset = Align(m_UploadBufferOffset, COPY_ALIGNMENT);

    return true;
}

void HelperDataUpload::CopyTextureSubresourceContent(const TextureSubresourceUploadDesc& subresource, uint64_t alignedRowPitch, uint64_t alignedSlicePitch) {
    const uint32_t sliceRowNum = subresource.slicePitch / subresource.rowPitch;

    // TODO: D3D11 does not allow to call CmdUploadBufferToTexture() while the upload buffer is mapped
    m_MappedMemory = (uint8_t*)m_NRI.MapBuffer(*m_UploadBuffer, m_UploadBufferOffset, subresource.sliceNum * alignedSlicePitch);

    uint8_t* slices = m_MappedMemory;
    for (uint32_t k = 0; k < subresource.sliceNum; k++) {
        for (uint32_t l = 0; l < sliceRowNum; l++) {
            uint8_t* dstRow = slices + k * alignedSlicePitch + l * alignedRowPitch;
            uint8_t* srcRow = (uint8_t*)subresource.slices + k * subresource.slicePitch + l * subresource.rowPitch;
            memcpy(dstRow, srcRow, subresource.rowPitch);
        }
    }

    m_NRI.UnmapBuffer(*m_UploadBuffer);
}

bool HelperDataUpload::CopyBufferContent(const BufferUploadDesc& bufferUploadDesc, uint64_t& bufferContentOffset) {
    if (!bufferUploadDesc.dataSize)
        return true;

    const uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;
    const uint64_t copySize = std::min(bufferUploadDesc.dataSize - bufferContentOffset, freeSpace);

    if (freeSpace == 0)
        return false;

    memcpy(m_MappedMemory + m_UploadBufferOffset, (uint8_t*)bufferUploadDesc.data + bufferContentOffset, (size_t)copySize);

    m_NRI.CmdCopyBuffer(*m_CommandBuffer, *bufferUploadDesc.buffer, bufferUploadDesc.bufferOffset + bufferContentOffset, *m_UploadBuffer, m_UploadBufferOffset, copySize);

    bufferContentOffset += copySize;
    m_UploadBufferOffset += copySize;

    if (bufferContentOffset != bufferUploadDesc.dataSize)
        return false;

    bufferContentOffset = 0;
    m_UploadBufferOffset = Align(m_UploadBufferOffset, COPY_ALIGNMENT);

    return true;
}
