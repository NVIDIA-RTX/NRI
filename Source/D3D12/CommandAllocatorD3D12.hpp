// © 2021 NVIDIA Corporation

Result CommandAllocatorD3D12::Create(const Queue& queue) {
    const QueueD3D12& queueD3D12 = (QueueD3D12&)queue;
    m_CommandListType = queueD3D12.GetType();
    HRESULT hr = m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&m_CommandAllocator));
    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandAllocator");

    return Result::SUCCESS;
}

NRI_INLINE Result CommandAllocatorD3D12::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    ExclusiveScope lock(m_Lock);

    return m_Device.CreateImplementation<CommandBufferD3D12>(commandBuffer, m_CommandListType, m_CommandAllocator);
}

NRI_INLINE void CommandAllocatorD3D12::Reset() {
    ExclusiveScope lock(m_Lock);

    m_CommandAllocator->Reset();
}
