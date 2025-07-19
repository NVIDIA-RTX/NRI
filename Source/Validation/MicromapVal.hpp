// © 2021 NVIDIA Corporation

MicromapVal::~MicromapVal() {
    if (m_Memory)
        m_Memory->Unbind(*this);

    Destroy(m_Buffer);
}

NRI_INLINE uint64_t MicromapVal::GetBuildScratchBufferSize() const {
    return GetRayTracingInterfaceImpl().GetMicromapBuildScratchBufferSize(*GetImpl());
}

NRI_INLINE uint64_t MicromapVal::GetNativeObject() const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "Micromap is not bound to memory");

    return GetRayTracingInterfaceImpl().GetMicromapNativeObject(GetImpl());
}

NRI_INLINE Buffer* MicromapVal::GetBuffer() {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "Micromap is not bound to memory");

    if (!m_Buffer) {
        Buffer* buffer = GetRayTracingInterfaceImpl().GetMicromapBuffer(*GetImpl());
        m_Buffer = Allocate<BufferVal>(m_Device.GetAllocationCallbacks(), m_Device, buffer, false);
    }

    return (Buffer*)m_Buffer;
}
