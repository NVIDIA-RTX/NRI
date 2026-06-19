// © 2026 NVIDIA Corporation

DescriptorSetWGPU::DescriptorSetWGPU(DeviceWGPU& device, const DescriptorSetMappingWGPU& mapping)
    : m_Device(device)
    , m_Mapping(mapping)
    , m_Descriptors(device.GetStdAllocator()) {
    uint32_t descriptorNum = 0;
    for (const DescriptorRangeMappingWGPU& range : mapping.ranges)
        descriptorNum = std::max(descriptorNum, range.descriptorOffset + range.descriptorNum);

    m_Descriptors.resize(descriptorNum);
}

DescriptorSetWGPU::~DescriptorSetWGPU() {
    if (m_BindGroup)
        wgpuBindGroupRelease(m_BindGroup);
}

void DescriptorSetWGPU::UpdateRange(uint32_t rangeIndex, uint32_t baseDescriptor, const Descriptor* const* descriptors, uint32_t descriptorNum) {
    const DescriptorRangeMappingWGPU& range = m_Mapping.ranges[rangeIndex];

    for (uint32_t i = 0; i < descriptorNum; i++)
        m_Descriptors[range.descriptorOffset + baseDescriptor + i] = (DescriptorWGPU*)descriptors[i];

    m_IsDirty = true;
    RecreateBindGroup();
}

void DescriptorSetWGPU::CopyRangeFrom(uint32_t dstRangeIndex, uint32_t dstBaseDescriptor, const DescriptorSetWGPU& srcDescriptorSet, uint32_t srcRangeIndex, uint32_t srcBaseDescriptor, uint32_t descriptorNum) {
    const DescriptorRangeMappingWGPU& dstRange = m_Mapping.ranges[dstRangeIndex];
    const DescriptorRangeMappingWGPU& srcRange = srcDescriptorSet.m_Mapping.ranges[srcRangeIndex];
    uint32_t copyNum = descriptorNum == ALL ? srcRange.descriptorNum - srcBaseDescriptor : descriptorNum;

    for (uint32_t i = 0; i < copyNum; i++)
        m_Descriptors[dstRange.descriptorOffset + dstBaseDescriptor + i] = srcDescriptorSet.m_Descriptors[srcRange.descriptorOffset + srcBaseDescriptor + i];

    m_IsDirty = true;
    RecreateBindGroup();
}

void DescriptorSetWGPU::RecreateBindGroup() const {
    for (DescriptorWGPU* descriptor : m_Descriptors) {
        if (!descriptor)
            return;
    }

    uint32_t entryMaxNum = 0;
    for (const DescriptorRangeMappingWGPU& range : m_Mapping.ranges)
        entryMaxNum += range.isArray ? 1 : range.descriptorNum;

    Scratch<WGPUBindGroupEntry> entries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupEntry, entryMaxNum);
    Scratch<WGPUBindGroupEntryExtras> entryExtras = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupEntryExtras, entryMaxNum);
    Scratch<WGPUSampler> samplers = NRI_ALLOCATE_SCRATCH(m_Device, WGPUSampler, m_Descriptors.size());
    Scratch<WGPUTextureView> textureViews = NRI_ALLOCATE_SCRATCH(m_Device, WGPUTextureView, m_Descriptors.size());
    Scratch<WGPUBuffer> buffers = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBuffer, m_Descriptors.size());

    uint32_t entryNum = 0;
    uint32_t resourceOffset = 0;

    for (const DescriptorRangeMappingWGPU& range : m_Mapping.ranges) {
        if (range.isArray) {
            WGPUBindGroupEntry& entry = entries[entryNum];
            WGPUBindGroupEntryExtras& extras = entryExtras[entryNum++];

            entry = WGPU_BIND_GROUP_ENTRY_INIT;
            entry.binding = range.bindingBase;
            entry.nextInChain = &extras.chain;

            extras = {};
            extras.chain.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras;

            switch (range.type) {
                case DescriptorType::SAMPLER:
                    extras.samplers = samplers + resourceOffset;
                    extras.samplerCount = range.descriptorNum;
                    for (uint32_t i = 0; i < range.descriptorNum; i++)
                        samplers[resourceOffset + i] = m_Descriptors[range.descriptorOffset + i]->GetSampler();
                    break;
                case DescriptorType::TEXTURE:
                case DescriptorType::STORAGE_TEXTURE:
                case DescriptorType::INPUT_ATTACHMENT:
                    extras.textureViews = textureViews + resourceOffset;
                    extras.textureViewCount = range.descriptorNum;
                    for (uint32_t i = 0; i < range.descriptorNum; i++)
                        textureViews[resourceOffset + i] = m_Descriptors[range.descriptorOffset + i]->GetTextureView();
                    break;
                default:
                    extras.buffers = buffers + resourceOffset;
                    extras.bufferCount = range.descriptorNum;
                    for (uint32_t i = 0; i < range.descriptorNum; i++)
                        buffers[resourceOffset + i] = m_Descriptors[range.descriptorOffset + i]->GetBuffer();
                    break;
            }

            resourceOffset += range.descriptorNum;
            continue;
        }

        for (uint32_t i = 0; i < range.descriptorNum; i++) {
            DescriptorWGPU* descriptor = m_Descriptors[range.descriptorOffset + i];
            WGPUBindGroupEntry& entry = entries[entryNum++];
            entry = WGPU_BIND_GROUP_ENTRY_INIT;
            entry.binding = range.bindingBase + i;

            switch (descriptor->GetDescriptorType()) {
                case DescriptorType::SAMPLER:
                    entry.sampler = descriptor->GetSampler();
                    break;
                case DescriptorType::TEXTURE:
                case DescriptorType::STORAGE_TEXTURE:
                    entry.textureView = descriptor->GetTextureView();
                    break;
                default:
                    entry.buffer = descriptor->GetBuffer();
                    entry.offset = descriptor->GetOffset();
                    entry.size = descriptor->GetSize();
                    break;
            }
        }
    }

    if (m_BindGroup)
        wgpuBindGroupRelease(m_BindGroup);

    WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    desc.layout = m_Mapping.layout;
    desc.entryCount = entryNum;
    desc.entries = entries;

    m_BindGroup = wgpuDeviceCreateBindGroup(m_Device, &desc);
    m_IsDirty = false;
}

void DescriptorSetWGPU::GetOffsets(uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) const {
    resourceHeapOffset = 0;
    samplerHeapOffset = 0;
}
