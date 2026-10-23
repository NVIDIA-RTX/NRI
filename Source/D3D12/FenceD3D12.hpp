// © 2021 NVIDIA Corporation

Result FenceD3D12::Create(const FenceDesc& fenceDesc) {
    if (fenceDesc.initialValue != SWAPCHAIN_SEMAPHORE) {
        D3D12_FENCE_FLAGS flags = fenceDesc.shared ? D3D12_FENCE_FLAG_SHARED : D3D12_FENCE_FLAG_NONE;
        HRESULT hr = m_Device->CreateFence(fenceDesc.initialValue, flags, IID_PPV_ARGS(&m_Fence));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateFence");

        m_Event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
        m_IsShared = fenceDesc.shared;
    } else if (fenceDesc.shared)
        return Result::UNSUPPORTED; // no binary fences

    return Result::SUCCESS;
}

Result FenceD3D12::Create(const FenceD3D12Desc& fenceD3D12Desc) {
    m_Fence = fenceD3D12Desc.d3d12Fence;
    m_Event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

    return Result::SUCCESS;
}

Result FenceD3D12::GetSharedHandle(SharedHandle& sharedHandle) const {
    sharedHandle = {};

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsShared && m_Fence, Result::INVALID_ARGUMENT, "'shared' is false");

    HANDLE handle = nullptr;
    HRESULT hr = m_Device->CreateSharedHandle(m_Fence.GetInterface(), nullptr, GENERIC_ALL, nullptr, &handle);
    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateSharedHandle");

    sharedHandle.handle = handle;
    sharedHandle.type = SharedHandleType::D3D12_FENCE;

    return Result::SUCCESS;
}

NRI_INLINE uint64_t FenceD3D12::GetFenceValue() const {
    return m_Fence ? m_Fence->GetCompletedValue() : 0;
}

NRI_INLINE void FenceD3D12::QueueSignal(QueueD3D12& queue, uint64_t value) {
    if (m_Fence) {
        HRESULT hr = ((ID3D12CommandQueue*)queue)->Signal(m_Fence, value);
        NRI_RETURN_VOID_ON_BAD_HRESULT(&m_Device, hr, "ID3D12CommandQueue::Signal");
    }
}

NRI_INLINE void FenceD3D12::QueueWait(QueueD3D12& queue, uint64_t value) {
    if (m_Fence && value) {
        HRESULT hr = ((ID3D12CommandQueue*)queue)->Wait(m_Fence, value);
        NRI_RETURN_VOID_ON_BAD_HRESULT(&m_Device, hr, "ID3D12CommandQueue::Wait");
    }
}

NRI_INLINE Result FenceD3D12::Wait(uint64_t value) {
    if (!m_Fence)
        return Result::SUCCESS;

    if (m_Event == 0 || m_Event == INVALID_HANDLE_VALUE) {
        // Busy wait
        while (m_Fence->GetCompletedValue() < value)
            ;
    } else if (m_Fence->GetCompletedValue() < value) {
        // Event-based wait
        ResetEvent(m_Event);

        HRESULT hr = m_Fence->SetEventOnCompletion(value, m_Event);
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Fence::SetEventOnCompletion");

        uint32_t result = WaitForSingleObjectEx(m_Event, NRI_TIMEOUT_FENCE, FALSE);
        NRI_RETURN_ON_FAILURE(&m_Device, result == WAIT_OBJECT_0, Result::FAILURE, "WaitForSingleObjectEx() failed, result = 0x%08X!", result);
    }

    return Result::SUCCESS;
}
