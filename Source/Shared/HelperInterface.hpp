// © 2021 NVIDIA Corporation

#include <algorithm>

// Helper data upload
constexpr uint32_t BARRIERS_PER_PASS = 256;
constexpr uint64_t MAX_UPLOAD_BUFFER_SIZE = 64 * 1024 * 1024;

enum class BarrierMode {
    INITIAL,       // transition to COPY_DEST state
    FINAL,         // transition from COPY_DEST to "final" state
    FINAL_NO_DATA, // initial state is not needed, since there is nothing to upload
};

static void DoTransition(const CoreInterface& m_iCore, CommandBuffer* commandBuffer, BarrierMode barrierMode, const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum) {
    TextureBarrierDesc textureBarriers[BARRIERS_PER_PASS];

    constexpr AccessLayoutStage copyDestState = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::ALL}; // we don't know which stages to wait
    constexpr AccessLayoutStage unknownState = {AccessBits::NONE, Layout::UNDEFINED, StageBits::NONE};                    // since the whole resource is updated, don't care about the previous state

    for (uint32_t i = 0; i < textureDataDescNum;) {
        uint32_t passEnd = std::min(i + BARRIERS_PER_PASS, textureDataDescNum);

        uint32_t n = 0;
        for (; i < passEnd; i++) {
            const TextureUploadDesc& textureUploadDesc = textureUploadDescs[i];
            const TextureDesc& textureDesc = m_iCore.GetTextureDesc(*textureUploadDesc.texture);

            TextureBarrierDesc& barrier = textureBarriers[n];
            barrier = {};
            barrier.texture = textureUploadDesc.texture;
            barrier.mipNum = textureDesc.mipNum;
            barrier.layerNum = textureDesc.layerNum;
            barrier.before = barrierMode == BarrierMode::FINAL ? copyDestState : unknownState;
            barrier.after = barrierMode == BarrierMode::INITIAL ? copyDestState : textureUploadDesc.after;

            if (barrierMode != BarrierMode::INITIAL)
                barrier.planes = textureUploadDesc.planes;

            // Filter out redundant barriers
            if (barrier.before.access != barrier.after.access || barrier.before.layout != barrier.after.layout)
                n++;
        }

        BarrierDesc barrierGroup = {};
        barrierGroup.textures = textureBarriers;
        barrierGroup.textureNum = n;

        m_iCore.CmdBarrier(*commandBuffer, barrierGroup);
    }
}

static void DoTransition(const CoreInterface& m_iCore, CommandBuffer* commandBuffer, BarrierMode barrierMode, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    BufferBarrierDesc bufferBarriers[BARRIERS_PER_PASS];

    constexpr AccessStage copyDestState = {AccessBits::COPY_DESTINATION, StageBits::ALL}; // we don't know which stages to wait
    constexpr AccessStage unknownState = {AccessBits::NONE, StageBits::NONE};             // since the whole resource is updated, don't care about the previous state

    for (uint32_t i = 0; i < bufferUploadDescNum;) {
        uint32_t passEnd = std::min(i + BARRIERS_PER_PASS, bufferUploadDescNum);

        uint32_t n = 0;
        for (; i < passEnd; i++) {
            const BufferUploadDesc& bufferUploadDesc = bufferUploadDescs[i];

            BufferBarrierDesc& barrier = bufferBarriers[n];
            barrier = {};
            barrier.buffer = bufferUploadDesc.buffer;
            barrier.before = barrierMode == BarrierMode::FINAL ? copyDestState : unknownState;
            barrier.after = barrierMode == BarrierMode::INITIAL ? copyDestState : bufferUploadDesc.after;

            // Filter out redundant barriers
            if (barrier.before.access != barrier.after.access)
                n++;
        }

        BarrierDesc barrierGroup = {};
        barrierGroup.buffers = bufferBarriers;
        barrierGroup.bufferNum = n;

        m_iCore.CmdBarrier(*commandBuffer, barrierGroup);
    }
}

Result HelperDataUpload::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    Result result = Create(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);

    if (result == Result::SUCCESS)
        result = UploadTextures(textureUploadDescs, textureUploadDescNum);
    if (result == Result::SUCCESS)
        result = UploadBuffers(bufferUploadDescs, bufferUploadDescNum);

    m_iCore.DestroyCommandBuffer(m_CommandBuffer);
    m_iCore.DestroyCommandAllocator(m_CommandAllocator);
    m_iCore.DestroyFence(m_Fence);
    m_iCore.DestroyBuffer(m_UploadBuffer);
    m_iCore.FreeMemory(m_UploadBufferMemory);

    return result;
}

Result HelperDataUpload::Create(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);

    { // Calculate upload buffer size
        uint64_t maxSubresourceSize = 0;
        uint64_t totalSize = 0;

        for (uint32_t i = 0; i < textureUploadDescNum; i++) {
            const TextureUploadDesc& textureUploadDesc = textureUploadDescs[i];
            if (textureUploadDesc.subresources) {
                const TextureSubresourceUploadDesc& subresource0 = textureUploadDesc.subresources[0];
                const TextureDesc& textureDesc = m_iCore.GetTextureDesc(*textureUploadDesc.texture);

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
                const BufferDesc& bufferDesc = m_iCore.GetBufferDesc(*bufferUploadDesc.buffer);

                totalSize += bufferDesc.size;
            }
        }

        // Can use up to "MAX_UPLOAD_BUFFER_SIZE" bytes
        m_UploadBufferSize = std::min(totalSize, MAX_UPLOAD_BUFFER_SIZE);

        // Worst case subresource must fit
        m_UploadBufferSize = std::max(m_UploadBufferSize, maxSubresourceSize);
    }

    // Create upload buffer
    if (m_UploadBufferSize) {
        BufferDesc bufferDesc = {};
        bufferDesc.size = m_UploadBufferSize;

        Result result = m_iCore.CreateBuffer(m_Device, bufferDesc, m_UploadBuffer);
        if (result != Result::SUCCESS)
            return result;

        MemoryDesc memoryDesc = {};
        m_iCore.GetBufferMemoryDesc(*m_UploadBuffer, MemoryLocation::HOST_UPLOAD, memoryDesc);

        AllocateMemoryDesc allocateMemoryDesc = {};
        allocateMemoryDesc.type = memoryDesc.type;
        allocateMemoryDesc.size = memoryDesc.size;

        result = m_iCore.AllocateMemory(m_Device, allocateMemoryDesc, m_UploadBufferMemory);
        if (result != Result::SUCCESS)
            return result;

        BindBufferMemoryDesc bufferMemoryBindingDesc = {m_UploadBuffer, m_UploadBufferMemory, 0};

        result = m_iCore.BindBufferMemory(&bufferMemoryBindingDesc, 1);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Create other resources
        Result result = m_iCore.CreateFence(m_Device, 0, m_Fence);
        if (result != Result::SUCCESS)
            return result;

        result = m_iCore.CreateCommandAllocator(m_Queue, m_CommandAllocator);
        if (result != Result::SUCCESS)
            return result;

        result = m_iCore.CreateCommandBuffer(*m_CommandAllocator, m_CommandBuffer);
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
    Dim_t mipOffset = 0;
    i = 0;

    while (i < textureDataDescNum) {
        if (!isInitial) {
            Result result = EndCommandBuffersAndSubmit();
            if (result != Result::SUCCESS)
                return result;
        }

        Result result = m_iCore.BeginCommandBuffer(*m_CommandBuffer, nullptr);
        if (result != Result::SUCCESS)
            return result;

        if (isInitial) {
            if (barrierMode != BarrierMode::FINAL_NO_DATA)
                DoTransition(m_iCore, m_CommandBuffer, BarrierMode::INITIAL, textureUploadDescs, textureDataDescNum);
            isInitial = false;
        }

        m_UploadBufferOffset = 0;
        for (; i < textureDataDescNum && CopyTextureContent(textureUploadDescs[i], layerOffset, mipOffset); i++)
            ;
    }

    DoTransition(m_iCore, m_CommandBuffer, barrierMode, textureUploadDescs, textureDataDescNum);

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

        Result result = m_iCore.BeginCommandBuffer(*m_CommandBuffer, nullptr);
        if (result != Result::SUCCESS)
            return result;

        if (isInitial) {
            if (barrierMode != BarrierMode::FINAL_NO_DATA)
                DoTransition(m_iCore, m_CommandBuffer, BarrierMode::INITIAL, bufferUploadDescs, bufferUploadDescNum);
            isInitial = false;
        }

        m_UploadBufferOffset = 0;
        m_MappedMemory = (uint8_t*)m_iCore.MapBuffer(*m_UploadBuffer, 0, m_UploadBufferSize);

        for (; i < bufferUploadDescNum && CopyBufferContent(bufferUploadDescs[i], bufferContentOffset); i++)
            ;

        m_iCore.UnmapBuffer(*m_UploadBuffer);
    }

    DoTransition(m_iCore, m_CommandBuffer, barrierMode, bufferUploadDescs, bufferUploadDescNum);

    return EndCommandBuffersAndSubmit();
}

Result HelperDataUpload::EndCommandBuffersAndSubmit() {
    Result result = m_iCore.EndCommandBuffer(*m_CommandBuffer);

    if (result == Result::SUCCESS) {
        FenceSubmitDesc fenceSubmitDesc = {};
        fenceSubmitDesc.fence = m_Fence;
        fenceSubmitDesc.value = m_FenceValue;

        QueueSubmitDesc queueSubmitDesc = {};
        queueSubmitDesc.commandBufferNum = 1;
        queueSubmitDesc.commandBuffers = &m_CommandBuffer;
        queueSubmitDesc.signalFences = &fenceSubmitDesc;
        queueSubmitDesc.signalFenceNum = 1;

        result = m_iCore.QueueSubmit(m_Queue, queueSubmitDesc);
        if (result == Result::SUCCESS) {
            m_iCore.Wait(*m_Fence, m_FenceValue);
            m_iCore.ResetCommandAllocator(*m_CommandAllocator);

            m_FenceValue++;
        }
    }

    return result;
}

bool HelperDataUpload::CopyTextureContent(const TextureUploadDesc& textureUploadDesc, Dim_t& layerOffset, Dim_t& mipOffset) {
    if (!textureUploadDesc.subresources)
        return true;

    const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = m_iCore.GetTextureDesc(*textureUploadDesc.texture);

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
            uint8_t* slices = (uint8_t*)m_iCore.MapBuffer(*m_UploadBuffer, m_UploadBufferOffset, subresource.sliceNum * alignedSlicePitch);
            {
                for (uint32_t k = 0; k < subresource.sliceNum; k++) {
                    for (uint32_t l = 0; l < sliceRowNum; l++) {
                        uint8_t* dstRow = slices + k * alignedSlicePitch + l * alignedRowPitch;
                        uint8_t* srcRow = (uint8_t*)subresource.slices + k * subresource.slicePitch + l * subresource.rowPitch;
                        memcpy(dstRow, srcRow, subresource.rowPitch);
                    }
                }
            }
            m_iCore.UnmapBuffer(*m_UploadBuffer);

            { // Copy
                TextureDataLayoutDesc srcDataLayout = {};
                srcDataLayout.offset = m_UploadBufferOffset;
                srcDataLayout.rowPitch = alignedRowPitch;
                srcDataLayout.slicePitch = alignedSlicePitch;

                TextureRegionDesc dstRegion = {};
                dstRegion.layerOffset = layerOffset;
                dstRegion.mipOffset = mipOffset;

                m_iCore.CmdUploadBufferToTexture(*m_CommandBuffer, *textureUploadDesc.texture, dstRegion, *m_UploadBuffer, srcDataLayout);
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

    const BufferDesc& bufferDesc = m_iCore.GetBufferDesc(*bufferUploadDesc.buffer);

    uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;
    uint64_t copySize = std::min(bufferDesc.size - bufferContentOffset, freeSpace);

    if (freeSpace == 0)
        return false;

    memcpy(m_MappedMemory + m_UploadBufferOffset, (uint8_t*)bufferUploadDesc.data + bufferContentOffset, copySize);

    m_iCore.CmdCopyBuffer(*m_CommandBuffer, *bufferUploadDesc.buffer, bufferContentOffset, *m_UploadBuffer, m_UploadBufferOffset, copySize);

    bufferContentOffset += copySize;
    m_UploadBufferOffset += copySize;

    if (bufferContentOffset != bufferDesc.size)
        return false;

    bufferContentOffset = 0;

    return true;
}

// HelperDeviceMemoryAllocator
HelperDeviceMemoryAllocator::MemoryHeap::MemoryHeap(MemoryType memoryType, const StdAllocator<uint8_t>& stdAllocator)
    : buffers(stdAllocator)
    , bufferOffsets(stdAllocator)
    , textures(stdAllocator)
    , textureOffsets(stdAllocator)
    , size(0)
    , type(memoryType) {
}

HelperDeviceMemoryAllocator::HelperDeviceMemoryAllocator(const CoreInterface& NRI, Device& device)
    : m_iCore(NRI)
    , m_Device(device)
    , m_Heaps(((DeviceBase&)device).GetStdAllocator())
    , m_DedicatedBuffers(((DeviceBase&)device).GetStdAllocator())
    , m_DedicatedTextures(((DeviceBase&)device).GetStdAllocator())
    , m_BufferBindingDescs(((DeviceBase&)device).GetStdAllocator())
    , m_TextureBindingDescs(((DeviceBase&)device).GetStdAllocator()) {
}

uint32_t HelperDeviceMemoryAllocator::CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) {
    GroupByMemoryType(resourceGroupDesc.memoryLocation, resourceGroupDesc);

    size_t allocationNum = m_Heaps.size() + m_DedicatedBuffers.size() + m_DedicatedTextures.size();

    return (uint32_t)allocationNum;
}

Result HelperDeviceMemoryAllocator::AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    size_t allocationNum = 0;
    Result result = TryToAllocateAndBindMemory(resourceGroupDesc, allocations, allocationNum);

    if (result != Result::SUCCESS) {
        for (size_t i = 0; i < allocationNum; i++) {
            m_iCore.FreeMemory(allocations[i]);
            allocations[i] = nullptr;
        }
    }

    return result;
}

Result HelperDeviceMemoryAllocator::TryToAllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations, size_t& allocationNum) {
    GroupByMemoryType(resourceGroupDesc.memoryLocation, resourceGroupDesc);

    for (MemoryHeap& heap : m_Heaps) {
        Memory*& memory = allocations[allocationNum];

        AllocateMemoryDesc allocateMemoryDesc = {};
        allocateMemoryDesc.type = heap.type;
        allocateMemoryDesc.size = heap.size;

        Result result = m_iCore.AllocateMemory(m_Device, allocateMemoryDesc, memory);
        if (result != Result::SUCCESS)
            return result;

        FillMemoryBindingDescs(heap.buffers.data(), heap.bufferOffsets.data(), (uint32_t)heap.buffers.size(), *memory);
        FillMemoryBindingDescs(heap.textures.data(), heap.textureOffsets.data(), (uint32_t)heap.textures.size(), *memory);

        allocationNum++;
    }

    Result result = ProcessDedicatedResources(resourceGroupDesc.memoryLocation, allocations, allocationNum);
    if (result != Result::SUCCESS)
        return result;

    result = m_iCore.BindBufferMemory(m_BufferBindingDescs.data(), (uint32_t)m_BufferBindingDescs.size());
    if (result != Result::SUCCESS)
        return result;

    result = m_iCore.BindTextureMemory(m_TextureBindingDescs.data(), (uint32_t)m_TextureBindingDescs.size());

    return result;
}

Result HelperDeviceMemoryAllocator::ProcessDedicatedResources(MemoryLocation memoryLocation, Memory** allocations, size_t& allocationNum) {
    constexpr uint64_t zeroOffset = 0;
    MemoryDesc memoryDesc = {};

    for (size_t i = 0; i < m_DedicatedBuffers.size(); i++) {
        m_iCore.GetBufferMemoryDesc(*m_DedicatedBuffers[i], memoryLocation, memoryDesc);

        Memory*& memory = allocations[allocationNum];

        AllocateMemoryDesc allocateMemoryDesc = {};
        allocateMemoryDesc.type = memoryDesc.type;
        allocateMemoryDesc.size = memoryDesc.size;

        Result result = m_iCore.AllocateMemory(m_Device, allocateMemoryDesc, memory);
        if (result != Result::SUCCESS)
            return result;

        FillMemoryBindingDescs(m_DedicatedBuffers.data() + i, &zeroOffset, 1, *memory);

        allocationNum++;
    }

    for (size_t i = 0; i < m_DedicatedTextures.size(); i++) {
        m_iCore.GetTextureMemoryDesc(*m_DedicatedTextures[i], memoryLocation, memoryDesc);

        Memory*& memory = allocations[allocationNum];

        AllocateMemoryDesc allocateMemoryDesc = {};
        allocateMemoryDesc.type = memoryDesc.type;
        allocateMemoryDesc.size = memoryDesc.size;

        Result result = m_iCore.AllocateMemory(m_Device, allocateMemoryDesc, memory);
        if (result != Result::SUCCESS)
            return result;

        FillMemoryBindingDescs(m_DedicatedTextures.data() + i, &zeroOffset, 1, *memory);

        allocationNum++;
    }

    return Result::SUCCESS;
}

HelperDeviceMemoryAllocator::MemoryHeap& HelperDeviceMemoryAllocator::FindOrCreateHeap(const MemoryDesc& memoryDesc, uint64_t preferredMemorySize) {
    if (preferredMemorySize == 0)
        preferredMemorySize = 256 * 1024 * 1024;

    size_t j = 0;
    for (; j < m_Heaps.size(); j++) {
        const MemoryHeap& heap = m_Heaps[j];

        uint64_t offset = Align(heap.size, memoryDesc.alignment);
        uint64_t newSize = offset + memoryDesc.size;

        if (heap.type == memoryDesc.type && newSize <= preferredMemorySize)
            break;
    }

    if (j == m_Heaps.size())
        m_Heaps.push_back(MemoryHeap(memoryDesc.type, ((DeviceBase&)m_Device).GetStdAllocator()));

    return m_Heaps[j];
}

void HelperDeviceMemoryAllocator::GroupByMemoryType(MemoryLocation memoryLocation, const ResourceGroupDesc& resourceGroupDesc) {
    struct BufferAndMemoryDesc {
        Buffer* buffer;
        MemoryDesc memoryDesc;
    };

    struct TextureAndMemoryDesc {
        Texture* texture;
        MemoryDesc memoryDesc;
    };

    // Copy to temp memory
    Scratch<BufferAndMemoryDesc> buffers = AllocateScratch((DeviceBase&)m_Device, BufferAndMemoryDesc, resourceGroupDesc.bufferNum);
    for (uint32_t i = 0; i < resourceGroupDesc.bufferNum; i++) {
        Buffer* buffer = resourceGroupDesc.buffers[i];

        buffers[i].buffer = buffer;
        m_iCore.GetBufferMemoryDesc(*buffer, memoryLocation, buffers[i].memoryDesc);
    }

    Scratch<TextureAndMemoryDesc> textures = AllocateScratch((DeviceBase&)m_Device, TextureAndMemoryDesc, resourceGroupDesc.textureNum);
    for (uint32_t i = 0; i < resourceGroupDesc.textureNum; i++) {
        Texture* texture = resourceGroupDesc.textures[i];

        textures[i].texture = texture;
        m_iCore.GetTextureMemoryDesc(*texture, memoryLocation, textures[i].memoryDesc);
    }

    // Sort by "alignment"
    if (resourceGroupDesc.bufferNum > 1) {
        std::sort(&buffers[0], &buffers[resourceGroupDesc.bufferNum - 1],
            [](const BufferAndMemoryDesc& a, const BufferAndMemoryDesc& b) -> bool {
                // Primary key: group by type
                if (a.memoryDesc.type != b.memoryDesc.type)
                    return a.memoryDesc.type < b.memoryDesc.type;

                // Secondary key: smallest to largest alignment
                return a.memoryDesc.alignment < b.memoryDesc.alignment;
            });
    }

    if (resourceGroupDesc.textureNum > 1) {
        std::sort(&textures[0], &textures[resourceGroupDesc.textureNum - 1],
            [](const TextureAndMemoryDesc& a, const TextureAndMemoryDesc& b) -> bool {
                // Primary key: group by type
                if (a.memoryDesc.type != b.memoryDesc.type)
                    return a.memoryDesc.type < b.memoryDesc.type;

                // Secondary key: smallest to largest alignment
                return a.memoryDesc.alignment < b.memoryDesc.alignment;
            });
    }

    // Linearly assign memory
    for (uint32_t i = 0; i < resourceGroupDesc.bufferNum; i++) {
        Buffer* buffer = buffers[i].buffer;
        const MemoryDesc& memoryDesc = buffers[i].memoryDesc;

        if (memoryDesc.mustBeDedicated)
            m_DedicatedBuffers.push_back(buffer);
        else {
            MemoryHeap& heap = FindOrCreateHeap(memoryDesc, resourceGroupDesc.preferredMemorySize);

            uint64_t offset = Align(heap.size, memoryDesc.alignment);

            heap.buffers.push_back(buffer);
            heap.bufferOffsets.push_back(offset);
            heap.size = offset + memoryDesc.size;
        }
    }

    for (uint32_t i = 0; i < resourceGroupDesc.textureNum; i++) {
        Texture* texture = textures[i].texture;
        const MemoryDesc& memoryDesc = textures[i].memoryDesc;

        if (memoryDesc.mustBeDedicated)
            m_DedicatedTextures.push_back(texture);
        else {
            MemoryHeap& heap = FindOrCreateHeap(memoryDesc, resourceGroupDesc.preferredMemorySize);

            if (heap.textures.empty()) {
                const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
                heap.size = Align(heap.size, deviceDesc.memory.bufferTextureGranularity);
            }

            uint64_t offset = Align(heap.size, memoryDesc.alignment);

            heap.textures.push_back(texture);
            heap.textureOffsets.push_back(offset);
            heap.size = offset + memoryDesc.size;
        }
    }
}

void HelperDeviceMemoryAllocator::FillMemoryBindingDescs(Buffer* const* buffers, const uint64_t* bufferOffsets, uint32_t bufferNum, Memory& memory) {
    for (uint32_t i = 0; i < bufferNum; i++) {
        BindBufferMemoryDesc desc = {};
        desc.memory = &memory;
        desc.buffer = buffers[i];
        desc.offset = bufferOffsets[i];

        m_BufferBindingDescs.push_back(desc);
    }
}

void HelperDeviceMemoryAllocator::FillMemoryBindingDescs(Texture* const* textures, const uint64_t* textureOffsets, uint32_t textureNum, Memory& memory) {
    for (uint32_t i = 0; i < textureNum; i++) {
        BindTextureMemoryDesc desc = {};
        desc.memory = &memory;
        desc.texture = textures[i];
        desc.offset = textureOffsets[i];

        m_TextureBindingDescs.push_back(desc);
    }
}
