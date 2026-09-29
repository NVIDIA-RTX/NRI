// © 2026 NVIDIA Corporation

DescriptorPoolMetal::DescriptorPoolMetal(DeviceMetal& device)
    : m_Device(device), m_Sets(device.GetStdAllocator()) {
}

DescriptorPoolMetal::~DescriptorPoolMetal() {
    if (m_ResourceHeap) {
        m_Device.RemoveResidency(m_ResourceHeap);
        m_ResourceHeap->release();
    }

    if (m_SamplerHeap) {
        m_Device.RemoveResidency(m_SamplerHeap);
        m_SamplerHeap->release();
    }
}

Result DescriptorPoolMetal::Create(const DescriptorPoolDesc& desc) {
    m_SamplerCapacity = desc.samplerMaxNum;
    m_ResourceCapacity = desc.mutableMaxNum + desc.constantBufferMaxNum + desc.textureMaxNum + desc.storageTextureMaxNum + desc.bufferMaxNum + desc.storageBufferMaxNum + desc.structuredBufferMaxNum + desc.storageStructuredBufferMaxNum + desc.accelerationStructureMaxNum + desc.inputAttachmentMaxNum;

    if (m_ResourceCapacity) {
        m_ResourceHeap = m_Device.GetNativeObject()->newBuffer(m_ResourceCapacity * DESCRIPTOR_ENTRY_SIZE, MTL::ResourceStorageModeShared);

        if (!m_ResourceHeap)
            return Result::OUT_OF_MEMORY;

        m_Device.AddResidency(m_ResourceHeap);
        m_ResourceHeapAddress = m_ResourceHeap->gpuAddress();
        m_ResourceEntries = (uint8_t*)m_ResourceHeap->contents();
    }

    if (m_SamplerCapacity) {
        m_SamplerHeap = m_Device.GetNativeObject()->newBuffer(m_SamplerCapacity * DESCRIPTOR_ENTRY_SIZE, MTL::ResourceStorageModeShared);

        if (!m_SamplerHeap)
            return Result::OUT_OF_MEMORY;

        m_Device.AddResidency(m_SamplerHeap);
        m_SamplerHeapAddress = m_SamplerHeap->gpuAddress();
        m_SamplerEntries = (uint8_t*)m_SamplerHeap->contents();
    }

    m_Sets.resize(desc.descriptorSetMaxNum);

    return Result::SUCCESS;
}

Result DescriptorPoolMetal::Create(const DescriptorHeapDesc& desc) {
    DescriptorPoolDesc pool = {};
    pool.mutableMaxNum = desc.resourceDescriptorNum;
    pool.samplerMaxNum = desc.samplerDescriptorNum;

    return Create(pool);
}

Result DescriptorPoolMetal::WriteResourceDescriptors(const WriteResourceDescriptorsDesc* descs, uint32_t num) {
    for (uint32_t i = 0; i < num; i++)
        ((const DescriptorMetal*)descs[i].resource)->WriteEntry(GetEntry(false, descs[i].descriptorIndex));

    return Result::SUCCESS;
}

Result DescriptorPoolMetal::WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* descs, uint32_t num) {
    for (uint32_t i = 0; i < num; i++)
        ((const DescriptorMetal*)descs[i].sampler)->WriteEntry(GetEntry(true, descs[i].descriptorIndex));

    return Result::SUCCESS;
}

Result DescriptorPoolMetal::AllocateDescriptorSets(const PipelineLayout& layout, uint32_t setIndex, DescriptorSet** sets, uint32_t instanceNum, uint32_t variableNum) {
    ExclusiveScope lock(m_Lock);

    const DescriptorSetMappingMetal& mapping = ((const PipelineLayoutMetal&)layout).GetDescriptorSetMapping(setIndex);

    // The variable-sized range reserves exactly "variableNum" descriptors (0 is valid)
    uint32_t resourceNum = mapping.resourceNum;
    uint32_t samplerNum = mapping.samplerNum;

    if (mapping.variableRange != UINT32_MAX) {
        const DescriptorRangeMappingMetal& variable = mapping.ranges[mapping.variableRange];

        if (variable.sampler)
            samplerNum = samplerNum - variable.descriptorNum + variableNum;
        else
            resourceNum = resourceNum - variable.descriptorNum + variableNum;
    }

    if (m_SetNum + instanceNum > m_Sets.size() || m_ResourceUsed + resourceNum * instanceNum > m_ResourceCapacity || m_SamplerUsed + samplerNum * instanceNum > m_SamplerCapacity)
        return Result::OUT_OF_MEMORY;

    // Since there is no "free" functionality allocation strategy is "linear grow"
    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetMetal* set = &m_Sets[m_SetNum++];
        set->Create(this, &mapping, m_ResourceUsed, m_SamplerUsed);
        sets[i] = (DescriptorSet*)set;

        m_ResourceUsed += resourceNum;
        m_SamplerUsed += samplerNum;
    }

    return Result::SUCCESS;
}

void DescriptorPoolMetal::Reset() {
    ExclusiveScope lock(m_Lock);

    m_ResourceUsed = 0;
    m_SamplerUsed = 0;
    m_SetNum = 0;
}

DeviceMetal& DescriptorPoolMetal::GetDevice() const {
    return m_Device;
}

uint64_t DescriptorPoolMetal::GetResourceHeapAddress() const {
    return m_ResourceHeapAddress;
}

uint64_t DescriptorPoolMetal::GetSamplerHeapAddress() const {
    return m_SamplerHeapAddress;
}

uint8_t* DescriptorPoolMetal::GetEntry(bool sampler, uint32_t index) const {
    return (sampler ? m_SamplerEntries : m_ResourceEntries) + index * DESCRIPTOR_ENTRY_SIZE;
}

void DescriptorPoolMetal::SetDebugName(const char* name) {
    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);

    if (m_ResourceHeap)
        m_ResourceHeap->setLabel(label);

    if (m_SamplerHeap)
        m_SamplerHeap->setLabel(label);

    label->release();
}
