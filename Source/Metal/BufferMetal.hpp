// © 2026 NVIDIA Corporation

static MTL::ResourceOptions GetBufferOptionsMetal(MemoryLocation location) {
    MTL::ResourceOptions options = location == MemoryLocation::DEVICE ? MTL::ResourceStorageModePrivate : MTL::ResourceStorageModeShared;
    options |= MTL::ResourceHazardTrackingModeUntracked;

    if (location != MemoryLocation::DEVICE && location != MemoryLocation::HOST_READBACK)
        options |= MTL::ResourceCPUCacheModeWriteCombined;

    return options;
}

BufferMetal::~BufferMetal() {
    Release();
}

void BufferMetal::Release() {
    if (m_Buffer) {
        if (m_IsResident)
            m_Device.RemoveResidency(m_Buffer);
        m_Buffer->release();
    }
    m_Buffer = nullptr;
    m_IsResident = false;
}

Result BufferMetal::Create(const BufferDesc& desc) {
    Release();
    m_Desc = desc;

    return Result::SUCCESS;
}

Result BufferMetal::Create(const BufferDesc& desc, MemoryLocation location) {
    Create(desc);
    m_Location = location;
    m_Buffer = m_Device.GetNativeObject()->newBuffer((NS::UInteger)desc.size, GetBufferOptionsMetal(location));

    if (!m_Buffer)
        return Result::OUT_OF_MEMORY;

    m_Device.AddResidency(m_Buffer);
    m_IsResident = true;

    return Result::SUCCESS;
}

Result BufferMetal::Create(const BufferMetalDesc& desc) {
    Release();
    m_Desc = desc.desc;
    m_Buffer = (MTL::Buffer*)desc.mtlBuffer;
    m_Buffer->retain();
    m_Device.AddResidency(m_Buffer);
    m_IsResident = true;
    m_Location = m_Buffer->storageMode() == MTL::StorageModePrivate ? MemoryLocation::DEVICE : MemoryLocation::HOST_UPLOAD;

    return Result::SUCCESS;
}

Result BufferMetal::Bind(MemoryMetal& memory, uint64_t offset) {
    Release();
    m_Location = memory.GetLocation();
    m_Buffer = memory.GetNativeObject()->newBuffer((NS::UInteger)m_Desc.size, GetBufferOptionsMetal(m_Location), (NS::UInteger)offset);

    return m_Buffer ? Result::SUCCESS : Result::FAILURE;
}

void BufferMetal::GetMemoryDesc(MemoryLocation location, MemoryDesc& memoryDesc) const {
    MTL::SizeAndAlign requirements = m_Device.GetNativeObject()->heapBufferSizeAndAlign((NS::UInteger)m_Desc.size, GetBufferOptionsMetal(location));
    memoryDesc.size = requirements.size;
    memoryDesc.alignment = (uint32_t)requirements.align;
    memoryDesc.type = (MemoryType)location;
    memoryDesc.mustBeDedicated = false;
}

void* BufferMetal::Map(uint64_t offset, uint64_t size) {
    if (!m_Buffer || m_Location == MemoryLocation::DEVICE)
        return nullptr;

    m_MapOffset = offset;
    m_MapSize = size == WHOLE_SIZE ? m_Desc.size - offset : size;

    return (uint8_t*)m_Buffer->contents() + offset;
}

void BufferMetal::Unmap() {
    if (m_Buffer && m_MapSize && m_Buffer->storageMode() == MTL::StorageModeManaged)
        m_Buffer->didModifyRange(NS::Range::Make((NS::UInteger)m_MapOffset, (NS::UInteger)m_MapSize));

    m_MapOffset = 0;
    m_MapSize = 0;
}

void BufferMetal::SetDebugName(const char* name) {
    if (!m_Buffer)
        return;

    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);
    m_Buffer->setLabel(label);
    label->release();
}
