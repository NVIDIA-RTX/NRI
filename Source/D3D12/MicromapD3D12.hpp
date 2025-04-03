// © 2025 NVIDIA Corporation

MicromapD3D12::~MicromapD3D12() {
    Destroy(m_Buffer);
}

Result MicromapD3D12::Create(const MicromapDesc& micromapDesc) {
    if (!m_Device.GetDesc().isMicromapSupported)
        return Result::UNSUPPORTED;

    for (uint32_t i = 0; i < micromapDesc.usageNum; i++) {
        const MicromapUsageDesc& in = micromapDesc.usages[i];
        // TODO: convert
        m_Usages.push_back(in);
    }

    BufferDesc bufferDesc = {};
    bufferDesc.size = 0; // TODO
    bufferDesc.usage = BufferUsageBits::MICROMAP_STORAGE;

    return m_Device.CreateImplementation<BufferD3D12>(m_Buffer, bufferDesc);
}

NRI_INLINE void MicromapD3D12::SetDebugName(const char* name) {
    m_Buffer->SetDebugName(name);
}
