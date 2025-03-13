// © 2021 NVIDIA Corporation

BufferVal::~BufferVal() {
    if (m_Memory != nullptr)
        m_Memory->UnbindBuffer(*this);
}

NRI_INLINE void* BufferVal::Map(uint64_t offset, uint64_t size) {
    if (size == WHOLE_SIZE)
        size = m_Desc.size;

    RETURN_ON_FAILURE(&m_Device, m_IsBoundToMemory, nullptr, "the buffer is not bound to memory");
    RETURN_ON_FAILURE(&m_Device, !m_IsMapped, nullptr, "the buffer is already mapped (D3D11 doesn't support nested calls)");
    RETURN_ON_FAILURE(&m_Device, offset + size <= m_Desc.size, nullptr, "out of bounds");

    m_IsMapped = true;

    return GetCoreInterface().MapBuffer(*GetImpl(), offset, size);
}

NRI_INLINE void BufferVal::Unmap() {
    RETURN_ON_FAILURE(&m_Device, m_IsMapped, ReturnVoid(), "the buffer is not mapped");

    m_IsMapped = false;

    GetCoreInterface().UnmapBuffer(*GetImpl());
}
