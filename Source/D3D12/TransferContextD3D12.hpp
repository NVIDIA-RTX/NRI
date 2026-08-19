// © 2021 NVIDIA Corporation

TransferContextD3D12::~TransferContextD3D12() {
    Destroy(m_CommandBuffer);
    Destroy(m_CommandAllocator);
    Destroy(m_Fence);
    Destroy(m_UploadBuffer);
    Destroy(m_ReadbackBuffer);
}

Result TransferContextD3D12::Prepare(QueueD3D12& queue) {
    if (m_CommandListType == D3D12_COMMAND_LIST_TYPE(-1))
        m_CommandListType = queue.GetType();

    if (!m_Fence) {
        Result result = m_Device.CreateImplementation<FenceD3D12>(m_Fence, 0);
        if (result != Result::SUCCESS)
            return result;
    }

    if (!m_CommandAllocator) {
        Result result = m_Device.CreateImplementation<CommandAllocatorD3D12>(m_CommandAllocator, (Queue&)queue);
        if (result != Result::SUCCESS)
            return result;
    }

    if (!m_CommandBuffer) {
        CommandBuffer* commandBuffer = nullptr;
        Result result = m_CommandAllocator->CreateCommandBuffer(commandBuffer);
        if (result != Result::SUCCESS)
            return result;

        m_CommandBuffer = (CommandBufferD3D12*)commandBuffer;
    }

    return Result::SUCCESS;
}

Result TransferContextD3D12::EnsureBuffer(MemoryLocation memoryLocation, uint64_t size, BufferD3D12*& buffer, uint64_t& capacity) {
    if (size <= capacity)
        return Result::SUCCESS;

    uint64_t newCapacity = capacity && capacity <= uint64_t(-1) / 2 ? capacity * 2 : size;
    newCapacity = std::max(newCapacity, size);

    BufferDesc bufferDesc = {};
    bufferDesc.size = newCapacity;

    BufferD3D12* newBuffer = nullptr;
    Result result = m_Device.CreateImplementation<BufferD3D12>(newBuffer, bufferDesc);
    if (result == Result::SUCCESS)
        result = newBuffer->Allocate(memoryLocation, 0.0f, true);

    if (result != Result::SUCCESS) {
        Destroy(newBuffer);
        return result;
    }

    Destroy(buffer);
    buffer = newBuffer;
    capacity = newCapacity;

    return Result::SUCCESS;
}

Result TransferContextD3D12::EnsureUploadBuffer(uint64_t size) {
    return EnsureBuffer(MemoryLocation::HOST_UPLOAD, size, m_UploadBuffer, m_UploadBufferSize);
}

Result TransferContextD3D12::EnsureReadbackBuffer(uint64_t size) {
    return EnsureBuffer(MemoryLocation::HOST_READBACK, size, m_ReadbackBuffer, m_ReadbackBufferSize);
}

Result TransferContextD3D12::SubmitAndWait(QueueD3D12& queue) {
    FenceSubmitDesc fenceSubmitDesc = {};
    fenceSubmitDesc.fence = (Fence*)m_Fence;
    fenceSubmitDesc.value = m_FenceValue;

    CommandBuffer* commandBuffer = (CommandBuffer*)m_CommandBuffer;
    QueueSubmitDesc queueSubmitDesc = {};
    queueSubmitDesc.commandBufferNum = 1;
    queueSubmitDesc.commandBuffers = &commandBuffer;
    queueSubmitDesc.signalFences = &fenceSubmitDesc;
    queueSubmitDesc.signalFenceNum = 1;

    Result result = queue.Submit(queueSubmitDesc);
    if (result == Result::SUCCESS) {
        m_Fence->Wait(m_FenceValue);
        m_CommandAllocator->Reset();
        m_FenceValue++;
    }

    return result;
}
