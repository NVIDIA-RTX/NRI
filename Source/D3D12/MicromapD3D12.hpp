// © 2025 NVIDIA Corporation

MicromapD3D12::~MicromapD3D12() {
    Destroy(m_Buffer);
}

Result MicromapD3D12::Create(const MicromapDesc& micromapDesc) {
#ifdef NRI_D3D12_HAS_OPACITY_MICROMAP
    static_assert((uint32_t)MicromapFormat::OPACITY_2_STATE == D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_2_STATE, "Type mismatch");
    static_assert((uint32_t)MicromapFormat::OPACITY_4_STATE == D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_4_STATE, "Type mismatch");

    if (!m_Device.GetDesc().features.micromap)
        return Result::UNSUPPORTED;

    for (uint32_t i = 0; i < micromapDesc.usageNum; i++) {
        const MicromapUsageDesc& in = micromapDesc.usages[i];

        D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY out = {};
        out.Count = in.triangleNum;
        out.SubdivisionLevel = in.subdivisionLevel;
        out.Format = (D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT)in.format;

        m_Usages.push_back(out);
    }

    m_Device.GetMicromapPrebuildInfo(micromapDesc, m_PrebuildInfo);
    m_Flags = micromapDesc.flags;

    if (micromapDesc.optimizedSize)
        m_PrebuildInfo.ResultDataMaxSizeInBytes = std::min(m_PrebuildInfo.ResultDataMaxSizeInBytes, micromapDesc.optimizedSize);

    BufferDesc bufferDesc = {};
    bufferDesc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.usage = BufferUsageBits::MICROMAP_STORAGE;

    return m_Device.CreateImplementation<BufferD3D12>(m_Buffer, bufferDesc);
#else
    MaybeUnused(micromapDesc);

    return Result::UNSUPPORTED;
#endif
}

Result MicromapD3D12::Allocate(MemoryLocation memoryLocation, float priority, bool committed) {
    return m_Buffer->Allocate(memoryLocation, priority, committed);
}

Result MicromapD3D12::BindMemory(const MemoryD3D12& memory, uint64_t offset) {
    return m_Buffer->BindMemory(memory, offset);
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
    return m_Buffer->GetGPUVA();
}

NRI_INLINE MicromapD3D12::operator ID3D12Resource*() const {
    return (ID3D12Resource*)(*m_Buffer);
}
