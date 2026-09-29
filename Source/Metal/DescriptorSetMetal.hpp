// © 2026 NVIDIA Corporation

void DescriptorSetMetal::Create(const DescriptorPoolMetal* pool, const DescriptorSetMappingMetal* mapping, uint32_t resourceOffset, uint32_t samplerOffset) {
    m_Pool = pool;
    m_Mapping = mapping;
    m_ResourceOffset = resourceOffset;
    m_SamplerOffset = samplerOffset;
}

DeviceMetal& DescriptorSetMetal::GetDevice() const {
    return m_Pool->GetDevice();
}

void DescriptorSetMetal::GetOffsets(uint32_t& resourceOffset, uint32_t& samplerOffset) const {
    resourceOffset = m_ResourceOffset;
    samplerOffset = m_SamplerOffset;
}

uint64_t DescriptorSetMetal::GetResourceAddress() const {
    const uint64_t heap = m_Pool->GetResourceHeapAddress();

    return heap ? heap + m_ResourceOffset * DESCRIPTOR_ENTRY_SIZE : 0;
}

uint64_t DescriptorSetMetal::GetSamplerAddress() const {
    const uint64_t heap = m_Pool->GetSamplerHeapAddress();

    return heap ? heap + m_SamplerOffset * DESCRIPTOR_ENTRY_SIZE : 0;
}

uint8_t* DescriptorSetMetal::GetEntry(const DescriptorRangeMappingMetal& range, uint32_t index) const {
    const uint32_t base = range.sampler ? m_SamplerOffset : m_ResourceOffset;

    return m_Pool->GetEntry(range.sampler, base + range.offset + index);
}

void DescriptorSetMetal::Update(uint32_t rangeIndex, uint32_t baseDescriptor, const Descriptor* const* descriptors, uint32_t num) {
    const DescriptorRangeMappingMetal& range = m_Mapping->ranges[rangeIndex];

    for (uint32_t i = 0; i < num; i++)
        ((const DescriptorMetal*)descriptors[i])->WriteEntry(GetEntry(range, baseDescriptor + i));
}

void DescriptorSetMetal::Copy(uint32_t dstRange, uint32_t dstBase, const DescriptorSetMetal& source, uint32_t srcRange, uint32_t srcBase, uint32_t num) {
    const DescriptorRangeMappingMetal& d = m_Mapping->ranges[dstRange];
    const DescriptorRangeMappingMetal& s = source.m_Mapping->ranges[srcRange];

    memmove(GetEntry(d, dstBase), source.GetEntry(s, srcBase), num * DESCRIPTOR_ENTRY_SIZE);
}

void DescriptorSetMetal::UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* descs, uint32_t num) {
    for (uint32_t i = 0; i < num; i++) {
        const UpdateDescriptorRangeDesc& d = descs[i];
        ((DescriptorSetMetal*)d.descriptorSet)->Update(d.rangeIndex, d.baseDescriptor, d.descriptors, d.descriptorNum);
    }
}

void DescriptorSetMetal::Copy(const CopyDescriptorRangeDesc* descs, uint32_t num) {
    for (uint32_t i = 0; i < num; i++) {
        const CopyDescriptorRangeDesc& d = descs[i];
        ((DescriptorSetMetal*)d.dstDescriptorSet)->Copy(d.dstRangeIndex, d.dstBaseDescriptor, *(DescriptorSetMetal*)d.srcDescriptorSet, d.srcRangeIndex, d.srcBaseDescriptor, d.descriptorNum);
    }
}
