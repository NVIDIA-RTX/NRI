// © 2021 NVIDIA Corporation

AccelerationStructureD3D12::~AccelerationStructureD3D12() {
    Destroy(m_Device.GetAllocationCallbacks(), m_Buffer);
}

Result AccelerationStructureD3D12::Create(const AccelerationStructureD3D12Desc& accelerationStructureDesc) {
    m_PrebuildInfo.ScratchDataSizeInBytes = accelerationStructureDesc.scratchDataSize;
    m_PrebuildInfo.UpdateScratchDataSizeInBytes = accelerationStructureDesc.updateScratchDataSize;

    BufferD3D12Desc bufferDesc = {};
    bufferDesc.d3d12Resource = accelerationStructureDesc.d3d12Resource;

    return m_Device.CreateImplementation<BufferD3D12>((Buffer*&)m_Buffer, bufferDesc);
}

Result AccelerationStructureD3D12::Create(const AccelerationStructureDesc& accelerationStructureDesc) {
    if (m_Device.GetVersion() < 5)
        return Result::UNSUPPORTED;

    m_Device.GetAccelerationStructurePrebuildInfo(accelerationStructureDesc, m_PrebuildInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    return m_Device.CreateImplementation<BufferD3D12>((Buffer*&)m_Buffer, bufferDesc);
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
    D3D12_HEAP_TYPE heapType = GetHeapType(memoryLocation);
    D3D12_HEAP_FLAGS heapFlags = m_Device.GetDesc().isMemoryTier2Supported ? D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES : D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    MemoryTypeInfo memoryTypeInfo = {};
    memoryTypeInfo.heapFlags = (uint16_t)heapFlags;
    memoryTypeInfo.heapType = (uint8_t)heapType;

    memoryDesc = {};
    memoryDesc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    memoryDesc.alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
    memoryDesc.type = Pack(memoryTypeInfo);
}

uint64_t AccelerationStructureD3D12::GetHandle() const {
    return m_Buffer->GetPointerGPU();
}

AccelerationStructureD3D12::operator ID3D12Resource*() const {
    return (ID3D12Resource*)(*m_Buffer);
}

NRI_INLINE void AccelerationStructureD3D12::SetDebugName(const char* name) {
    m_Buffer->SetDebugName(name);
}
