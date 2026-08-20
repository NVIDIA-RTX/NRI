// © 2021 NVIDIA Corporation

TransferContextD3D12::~TransferContextD3D12() {
    Destroy(m_CommandBuffer);
    Destroy(m_CommandAllocator);
    Destroy(m_Fence);
    Destroy(m_UploadBuffer);
    Destroy(m_ReadbackBuffer);
}

Result TransferContextD3D12::Create(QueueD3D12& queue) {
    m_CommandListType = queue.GetType();

    Result result = m_Device.CreateImplementation<FenceD3D12>(m_Fence, 0);
    if (result != Result::SUCCESS)
        return result;

    result = m_Device.CreateImplementation<CommandAllocatorD3D12>(m_CommandAllocator, (Queue&)queue);
    if (result != Result::SUCCESS)
        return result;

    CommandBuffer* commandBuffer = nullptr;
    result = m_CommandAllocator->CreateCommandBuffer(commandBuffer);
    if (result != Result::SUCCESS)
        return result;

    m_CommandBuffer = (CommandBufferD3D12*)commandBuffer;

    return Result::SUCCESS;
}

Result TransferContextD3D12::EnsureBuffer(MemoryLocation memoryLocation, uint64_t size, BufferD3D12*& buffer) {
    uint64_t capacity = buffer ? buffer->GetDesc().size : 0;
    if (size <= capacity)
        return Result::SUCCESS;

    uint64_t newCapacity = size;
    if (size <= MAX_CACHED_HOST_COPY_BUFFER_SIZE && capacity)
        newCapacity = std::min(std::max(capacity * 2, size), MAX_CACHED_HOST_COPY_BUFFER_SIZE);

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

    return Result::SUCCESS;
}

Result TransferContextD3D12::EnsureUploadBuffer(uint64_t size) {
    return EnsureBuffer(MemoryLocation::HOST_UPLOAD, size, m_UploadBuffer);
}

Result TransferContextD3D12::EnsureReadbackBuffer(uint64_t size) {
    return EnsureBuffer(MemoryLocation::HOST_READBACK, size, m_ReadbackBuffer);
}

void TransferContextD3D12::Reset() {
    m_CommandAllocator->Reset();
    m_FenceValue++;
    m_IsReusable = true;
}

bool TransferContextD3D12::TryRecover() {
    if (!m_IsReusable && m_Fence->GetFenceValue() >= m_FenceValue) {
        Reset();
        Trim();
    }

    return m_IsReusable;
}

void TransferContextD3D12::Trim() {
    if (!m_IsReusable)
        return;

    if (m_UploadBuffer && m_UploadBuffer->GetDesc().size > MAX_CACHED_HOST_COPY_BUFFER_SIZE) {
        Destroy(m_UploadBuffer);
        m_UploadBuffer = nullptr;
    }

    if (m_ReadbackBuffer && m_ReadbackBuffer->GetDesc().size > MAX_CACHED_HOST_COPY_BUFFER_SIZE) {
        Destroy(m_ReadbackBuffer);
        m_ReadbackBuffer = nullptr;
    }
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

    m_IsReusable = false;

    Result result = queue.Submit(queueSubmitDesc);
    if (result == Result::SUCCESS)
        result = m_Fence->Wait(m_FenceValue);

    if (result == Result::SUCCESS)
        Reset();

    return result;
}
