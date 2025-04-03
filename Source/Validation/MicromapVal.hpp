// © 2021 NVIDIA Corporation

MicromapVal::~MicromapVal() {
    if (m_Memory)
        m_Memory->Unbind(*this);

    Destroy(m_Buffer);
}

NRI_INLINE uint64_t MicromapVal::GetBuildScratchBufferSize() const {
    return GetRayTracingInterface().GetMicromapBuildScratchBufferSize(*GetImpl());
}

NRI_INLINE uint64_t MicromapVal::GetNativeObject() const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "Micromap is not bound to memory");

    return GetRayTracingInterface().GetMicromapNativeObject(*GetImpl());
}

NRI_INLINE Buffer* MicromapVal::GetBuffer() {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "Micromap is not bound to memory");

    if (!m_Buffer) {
        Buffer* buffer = GetRayTracingInterface().GetMicromapBuffer(*GetImpl());
        m_Buffer = Allocate<BufferVal>(m_Device.GetAllocationCallbacks(), m_Device, buffer, false);
    }

    return (Buffer*)m_Buffer;
}
