// © 2021 NVIDIA Corporation

constexpr uint32_t BARRIERS_PER_PASS = 256;
constexpr uint64_t MAX_UPLOAD_BUFFER_SIZE = 64 * 1024 * 1024;

enum class BarrierMode {
    INITIAL,       // transition to COPY_DEST state
    FINAL,         // transition from COPY_DEST to "final" state
    FINAL_NO_DATA, // initial state is not needed, since there is nothing to upload
};

static void DoTransition(const CoreInterface& m_NRI, CommandBuffer* commandBuffer, BarrierMode barrierMode, const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum) {
    TextureBarrierDesc textureBarriers[BARRIERS_PER_PASS];

    constexpr AccessLayoutStage copyDestState = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::ALL}; // we don't know which stages to wait
    constexpr AccessLayoutStage unknownState = {AccessBits::UNKNOWN, Layout::UNKNOWN, StageBits::NONE}; // since the whole resource is updated, don't care about the previous state

    for (uint32_t i = 0; i < textureDataDescNum;) {
        uint32_t passBegin = i;
        uint32_t passEnd = std::min(i + BARRIERS_PER_PASS, textureDataDescNum);

        for (; i < passEnd; i++) {
            const TextureUploadDesc& textureUploadDesc = textureUploadDescs[i];
            const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*textureUploadDesc.texture);

            TextureBarrierDesc& barrier = textureBarriers[i - passBegin];
            barrier = {};
            barrier.texture = textureUploadDesc.texture;
            barrier.mipNum = textureDesc.mipNum;
            barrier.layerNum = textureDesc.layerNum;
            barrier.before = barrierMode == BarrierMode::FINAL ? copyDestState : unknownState;
            barrier.after = barrierMode == BarrierMode::INITIAL ? copyDestState : textureUploadDesc.after;

            if (barrierMode != BarrierMode::INITIAL)
                barrier.planes = textureUploadDesc.planes;
        }

        BarrierGroupDesc barrierGroup = {};
        barrierGroup.textures = textureBarriers;
        barrierGroup.textureNum = uint16_t(passEnd - passBegin);

        m_NRI.CmdBarrier(*commandBuffer, barrierGroup);
    }
}

static void DoTransition(const CoreInterface& m_NRI, CommandBuffer* commandBuffer, BarrierMode barrierMode, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    BufferBarrierDesc bufferBarriers[BARRIERS_PER_PASS];

    constexpr AccessStage copyDestState = {AccessBits::COPY_DESTINATION, StageBits::ALL}; // we don't know which stages to wait
    constexpr AccessStage unknownState = {AccessBits::UNKNOWN, StageBits::NONE}; // since the whole resource is updated, don't care about the previous state

    for (uint32_t i = 0; i < bufferUploadDescNum;) {
        uint32_t passBegin = i;
        uint32_t passEnd = std::min(i + BARRIERS_PER_PASS, bufferUploadDescNum);

        for (; i < passEnd; i++) {
            const BufferUploadDesc& bufferUploadDesc = bufferUploadDescs[i];

            BufferBarrierDesc& barrier = bufferBarriers[i - passBegin];
            barrier = {};
            barrier.buffer = bufferUploadDesc.buffer;
            barrier.before = barrierMode == BarrierMode::FINAL ? copyDestState : unknownState;
            barrier.after = barrierMode == BarrierMode::INITIAL ? copyDestState : bufferUploadDesc.after;
        }

        BarrierGroupDesc barrierGroup = {};
        barrierGroup.buffers = bufferBarriers;
        barrierGroup.bufferNum = uint16_t(passEnd - passBegin);

        m_NRI.CmdBarrier(*commandBuffer, barrierGroup);
    }
}

Result HelperDataUpload::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    Result result = Create(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);

    if (result == Result::SUCCESS)
        result = UploadTextures(textureUploadDescs, textureUploadDescNum);
    if (result == Result::SUCCESS)
        result = UploadBuffers(bufferUploadDescs, bufferUploadDescNum);

    m_NRI.DestroyCommandBuffer(*m_CommandBuffer);
    m_NRI.DestroyCommandAllocator(*m_CommandAllocators);
    m_NRI.DestroyFence(*m_Fence);
    m_NRI.DestroyBuffer(*m_UploadBuffer);
    m_NRI.FreeMemory(*m_UploadBufferMemory);

    return result;
}

Result HelperDataUpload::Create(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);

    { // Calculate upload buffer size
        uint64_t maxSubresourceSize = 0;
        uint64_t totalSize = 0;

        for (uint32_t i = 0; i < textureUploadDescNum; i++) {
            const TextureUploadDesc& textureUploadDesc = textureUploadDescs[i];
            if (textureUploadDesc.subresources) {
                const TextureSubresourceUploadDesc& subresource0 = textureUploadDesc.subresources[0];
                const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*textureUploadDesc.texture);

                uint32_t sliceRowNum = subresource0.slicePitch / subresource0.rowPitch;
                uint64_t alignedRowPitch = Align(subresource0.rowPitch, deviceDesc.memoryAlignment.uploadBufferTextureRow);
                uint64_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.memoryAlignment.uploadBufferTextureSlice);
                uint64_t alignedSize = alignedSlicePitch * subresource0.sliceNum;

                CHECK(alignedSize != 0, "Unexpected");

                maxSubresourceSize = std::max(maxSubresourceSize, alignedSize);

                alignedSize *= textureDesc.layerNum;
                if (textureDesc.mipNum > 1)
                    totalSize += (alignedSize * 4) / 3; // assume full mip chain
                else
                    totalSize += alignedSize;
            }
        }

        for (uint32_t i = 0; i < bufferUploadDescNum; i++) {
            // Doesn't contribute to "maxSubresourceSize" because buffer copies can work with any non-0 upload buffer size
            const BufferUploadDesc& bufferUploadDesc = bufferUploadDescs[i];
            if (bufferUploadDesc.data) {
                const BufferDesc& bufferDesc = m_NRI.GetBufferDesc(*bufferUploadDesc.buffer);

                totalSize += bufferDesc.size;
            }
        }

        // Can use up to "MAX_UPLOAD_BUFFER_SIZE" bytes
        m_UploadBufferSize = std::min(totalSize, MAX_UPLOAD_BUFFER_SIZE);

        // Worst case subresource must fit
        m_UploadBufferSize = std::max(m_UploadBufferSize, maxSubresourceSize);
    }

    // Create upload buffer
    if (m_UploadBufferSize)
    {
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

        BufferMemoryBindingDesc bufferMemoryBindingDesc = {m_UploadBuffer, m_UploadBufferMemory, 0};

        result = m_NRI.BindBufferMemory(m_Device, &bufferMemoryBindingDesc, 1);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Create other resources
        Result result = m_NRI.CreateFence(m_Device, 0, m_Fence);
        if (result != Result::SUCCESS)
            return result;

        result = m_NRI.CreateCommandAllocator(m_Queue, m_CommandAllocators);
        if (result != Result::SUCCESS)
            return result;

        result = m_NRI.CreateCommandBuffer(*m_CommandAllocators, m_CommandBuffer);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

Result HelperDataUpload::UploadTextures(const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum) {
    if (!textureDataDescNum)
        return Result::SUCCESS;

    uint32_t i = 0;
    for (; i < textureDataDescNum; i++) {
        const TextureUploadDesc& textureUploadDesc = textureUploadDescs[i];
        if (textureUploadDesc.subresources)
            break;
    }

    BarrierMode barrierMode = i == textureDataDescNum ? BarrierMode::FINAL_NO_DATA : BarrierMode::FINAL;

    bool isInitial = true;
    Dim_t layerOffset = 0;
    Mip_t mipOffset = 0;
    i = 0;

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
            if (barrierMode != BarrierMode::FINAL_NO_DATA)
                DoTransition(m_NRI, m_CommandBuffer, BarrierMode::INITIAL, textureUploadDescs, textureDataDescNum);
            isInitial = false;
        }

        m_UploadBufferOffset = 0;
        for (; i < textureDataDescNum && CopyTextureContent(textureUploadDescs[i], layerOffset, mipOffset); i++)
            ;
    }

    DoTransition(m_NRI, m_CommandBuffer, barrierMode, textureUploadDescs, textureDataDescNum);

    return EndCommandBuffersAndSubmit();
}

Result HelperDataUpload::UploadBuffers(const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    if (!bufferUploadDescNum)
        return Result::SUCCESS;

    uint32_t i = 0;
    for (; i < bufferUploadDescNum; i++) {
        const BufferUploadDesc& bufferUploadDesc = bufferUploadDescs[i];
        if (bufferUploadDesc.data)
            break;
    }

    BarrierMode barrierMode = i == bufferUploadDescNum ? BarrierMode::FINAL_NO_DATA : BarrierMode::FINAL;

    bool isInitial = true;
    uint64_t bufferContentOffset = 0;
    i = 0;

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
            if (barrierMode != BarrierMode::FINAL_NO_DATA)
                DoTransition(m_NRI, m_CommandBuffer, BarrierMode::INITIAL, bufferUploadDescs, bufferUploadDescNum);
            isInitial = false;
        }

        m_UploadBufferOffset = 0;
        m_MappedMemory = (uint8_t*)m_NRI.MapBuffer(*m_UploadBuffer, 0, m_UploadBufferSize);

        for (; i < bufferUploadDescNum && CopyBufferContent(bufferUploadDescs[i], bufferContentOffset); i++)
            ;

        m_NRI.UnmapBuffer(*m_UploadBuffer);
    }

    DoTransition(m_NRI, m_CommandBuffer, barrierMode, bufferUploadDescs, bufferUploadDescNum);

    return EndCommandBuffersAndSubmit();
}

Result HelperDataUpload::EndCommandBuffersAndSubmit() {
    Result result = m_NRI.EndCommandBuffer(*m_CommandBuffer);

    if (result == Result::SUCCESS) {
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
        m_NRI.ResetCommandAllocator(*m_CommandAllocators);

        m_FenceValue++;
    }

    return result;
}

bool HelperDataUpload::CopyTextureContent(const TextureUploadDesc& textureUploadDesc, Dim_t& layerOffset, Mip_t& mipOffset) {
    if (!textureUploadDesc.subresources)
        return true;

    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*textureUploadDesc.texture);

    for (; layerOffset < textureDesc.layerNum; layerOffset++) {
        for (; mipOffset < textureDesc.mipNum; mipOffset++) {
            const auto& subresource = textureUploadDesc.subresources[layerOffset * textureDesc.mipNum + mipOffset];

            uint32_t sliceRowNum = subresource.slicePitch / subresource.rowPitch;
            uint32_t alignedRowPitch = Align(subresource.rowPitch, deviceDesc.memoryAlignment.uploadBufferTextureRow);
            uint32_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.memoryAlignment.uploadBufferTextureSlice);
            uint64_t alignedSize = uint64_t(alignedSlicePitch) * subresource.sliceNum;
            uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;

            if (alignedSize > freeSpace) {
                CHECK(alignedSize <= m_UploadBufferSize, "Unexpected");
                return false;
            }

            // Upload data (D3D11 does not allow to use upload buffer while it's mapped)
            uint8_t* slices = (uint8_t*)m_NRI.MapBuffer(*m_UploadBuffer, m_UploadBufferOffset, subresource.sliceNum * alignedSlicePitch);
            {
                for (uint32_t k = 0; k < subresource.sliceNum; k++) {
                    for (uint32_t l = 0; l < sliceRowNum; l++) {
                        uint8_t* dstRow = slices + k * alignedSlicePitch + l * alignedRowPitch;
                        uint8_t* srcRow = (uint8_t*)subresource.slices + k * subresource.slicePitch + l * subresource.rowPitch;
                        memcpy(dstRow, srcRow, subresource.rowPitch);
                    }
                }
            }
            m_NRI.UnmapBuffer(*m_UploadBuffer);

            { // Copy
                TextureDataLayoutDesc srcDataLayout = {};
                srcDataLayout.offset = m_UploadBufferOffset;
                srcDataLayout.rowPitch = alignedRowPitch;
                srcDataLayout.slicePitch = alignedSlicePitch;

                TextureRegionDesc dstRegion = {};
                dstRegion.layerOffset = layerOffset;
                dstRegion.mipOffset = mipOffset;

                m_NRI.CmdUploadBufferToTexture(*m_CommandBuffer, *textureUploadDesc.texture, dstRegion, *m_UploadBuffer, srcDataLayout);
            }

            // Increment buffer offset
            m_UploadBufferOffset += alignedSize;
        }
        mipOffset = 0;
    }
    layerOffset = 0;

    return true;
}

bool HelperDataUpload::CopyBufferContent(const BufferUploadDesc& bufferUploadDesc, uint64_t& bufferContentOffset) {
    if (!bufferUploadDesc.data)
        return true;

    const BufferDesc& bufferDesc = m_NRI.GetBufferDesc(*bufferUploadDesc.buffer);

    uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;
    uint64_t copySize = std::min(bufferDesc.size - bufferContentOffset, freeSpace);

    if (freeSpace == 0)
        return false;

    memcpy(m_MappedMemory + m_UploadBufferOffset, (uint8_t*)bufferUploadDesc.data + bufferContentOffset, bufferDesc.size);

    m_NRI.CmdCopyBuffer(*m_CommandBuffer, *bufferUploadDesc.buffer, bufferContentOffset, *m_UploadBuffer, m_UploadBufferOffset, copySize);

    bufferContentOffset += copySize;
    m_UploadBufferOffset += copySize;

    if (bufferContentOffset != bufferDesc.size)
        return false;

    bufferContentOffset = 0;

    return true;
}
