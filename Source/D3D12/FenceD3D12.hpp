// © 2021 NVIDIA Corporation

Result FenceD3D12::Create(uint64_t initialValue) {
    if (initialValue != SWAPCHAIN_SEMAPHORE) {
        HRESULT hr = m_Device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateFence");

        m_Event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    }

    return Result::SUCCESS;
}

Result FenceD3D12::Create(const FenceD3D12Desc& fenceD3D12Desc) {
    m_Fence = fenceD3D12Desc.d3d12Fence;
    m_Event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

    return Result::SUCCESS;
}

NRI_INLINE uint64_t FenceD3D12::GetFenceValue() const {
    return m_Fence ? m_Fence->GetCompletedValue() : 0;
}

NRI_INLINE void FenceD3D12::QueueSignal(QueueD3D12& queue, uint64_t value) {
    if (m_Fence) {
        HRESULT hr = ((ID3D12CommandQueue*)queue)->Signal(m_Fence, value);
        RETURN_VOID_ON_BAD_HRESULT(&m_Device, hr, "ID3D12CommandQueue::Signal");
    }
}

NRI_INLINE void FenceD3D12::QueueWait(QueueD3D12& queue, uint64_t value) {
    if (m_Fence && value) {
        HRESULT hr = ((ID3D12CommandQueue*)queue)->Wait(m_Fence, value);
        RETURN_VOID_ON_BAD_HRESULT(&m_Device, hr, "ID3D12CommandQueue::Wait");
    }
}

NRI_INLINE void FenceD3D12::Wait(uint64_t value) {
    if (!m_Fence)
        return;

    if (m_Event == 0 || m_Event == INVALID_HANDLE_VALUE) {
        // Busy wait
        while (m_Fence->GetCompletedValue() < value)
            ;
    } else if (m_Fence->GetCompletedValue() < value) {
        // Event-based wait
        HRESULT hr = m_Fence->SetEventOnCompletion(value, m_Event);
        RETURN_VOID_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Fence::SetEventOnCompletion");

        uint32_t result = WaitForSingleObjectEx(m_Event, TIMEOUT_FENCE, TRUE);
        RETURN_ON_FAILURE(&m_Device, result == WAIT_OBJECT_0, ReturnVoid(), "WaitForSingleObjectEx() failed!");
    }
}
