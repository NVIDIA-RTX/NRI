// © 2021 NVIDIA Corporation

AccelerationStructureD3D12::~AccelerationStructureD3D12() {
    Destroy(m_Buffer);
}

Result AccelerationStructureD3D12::Create(const AccelerationStructureD3D12Desc& accelerationStructureDesc) {
    m_PrebuildInfo.ResultDataMaxSizeInBytes = accelerationStructureDesc.size;
    m_PrebuildInfo.ScratchDataSizeInBytes = accelerationStructureDesc.buildScratchSize;
    m_PrebuildInfo.UpdateScratchDataSizeInBytes = accelerationStructureDesc.updateScratchSize;
    m_Flags = accelerationStructureDesc.flags;

    BufferD3D12Desc bufferDesc = {};
    bufferDesc.d3d12Resource = accelerationStructureDesc.d3d12Resource;

    return m_Device.CreateImplementation<BufferD3D12>(m_Buffer, bufferDesc);
}

Result AccelerationStructureD3D12::Create(const AccelerationStructureDesc& accelerationStructureDesc) {
    m_Device.GetAccelerationStructurePrebuildInfo(accelerationStructureDesc, m_PrebuildInfo);
    m_Flags = accelerationStructureDesc.flags;

    if (accelerationStructureDesc.optimizedSize)
        m_PrebuildInfo.ResultDataMaxSizeInBytes = std::min(m_PrebuildInfo.ResultDataMaxSizeInBytes, accelerationStructureDesc.optimizedSize);

    BufferDesc bufferDesc = {};
    bufferDesc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    return m_Device.CreateImplementation<BufferD3D12>(m_Buffer, bufferDesc);
}

Result AccelerationStructureD3D12::BindMemory(Memory* memory, uint64_t offset) {
    Result result = m_Buffer->BindMemory((MemoryD3D12*)memory, offset);

    return result;
}

Result AccelerationStructureD3D12::CreateDescriptor(Descriptor*& descriptor) const {
    const AccelerationStructure& accelerationStructure = (const AccelerationStructure&)*this;

    return m_Device.CreateImplementation<DescriptorD3D12>(descriptor, accelerationStructure);
}

void AccelerationStructureD3D12::GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    BufferDesc bufferDesc = {};
    bufferDesc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    D3D12_RESOURCE_DESC resourceDesc = {};
    m_Device.GetResourceDesc(bufferDesc, resourceDesc);
    m_Device.GetMemoryDesc(memoryLocation, resourceDesc, memoryDesc);
}

NRI_INLINE void AccelerationStructureD3D12::SetDebugName(const char* name) {
    m_Buffer->SetDebugName(name);
}

NRI_INLINE uint64_t AccelerationStructureD3D12::GetHandle() const {
    return m_Buffer->GetPointerGPU();
}

NRI_INLINE AccelerationStructureD3D12::operator ID3D12Resource*() const {
    return (ID3D12Resource*)(*m_Buffer);
}
