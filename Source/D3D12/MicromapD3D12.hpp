// © 2025 NVIDIA Corporation

MicromapD3D12::~MicromapD3D12() {
    Destroy(m_Buffer);
}

Result MicromapD3D12::Create(const MicromapDesc& micromapDesc) {
#ifdef NRI_D3D12_HAS_OPACITY_MICROMAP
#else
    MaybeUnused(micromapDesc);

    return Result::UNSUPPORTED;
#endif
}

Result MicromapD3D12::BindMemory(Memory* memory, uint64_t offset) {
    Result result = m_Buffer->BindMemory((MemoryD3D12*)memory, offset);

    return result;
}

void MicromapD3D12::GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    BufferDesc bufferDesc = {};
    bufferDesc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.usage = BufferUsageBits::MICROMAP_STORAGE;

    D3D12_RESOURCE_DESC resourceDesc = {};
    m_Device.GetResourceDesc(bufferDesc, resourceDesc);
    m_Device.GetMemoryDesc(memoryLocation, resourceDesc, memoryDesc);
}

NRI_INLINE void MicromapD3D12::SetDebugName(const char* name) {
    m_Buffer->SetDebugName(name);
}

NRI_INLINE uint64_t MicromapD3D12::GetHandle() const {
    return m_Buffer->GetPointerGPU();
}

NRI_INLINE MicromapD3D12::operator ID3D12Resource*() const {
    return (ID3D12Resource*)(*m_Buffer);
}
