// © 2026 NVIDIA Corporation

DescriptorSetWGPU::DescriptorSetWGPU(DeviceWGPU& device, const DescriptorSetMappingWGPU& mapping)
    : m_Device(device)
    , m_Mapping(mapping)
    , m_Descriptors(device.GetStdAllocator())
    , m_BindGroups(device.GetStdAllocator()) {
    uint32_t descriptorNum = 0;
    for (const DescriptorRangeMappingWGPU& range : mapping.ranges)
        descriptorNum = std::max(descriptorNum, range.descriptorOffset + range.descriptorNum);

    m_Descriptors.resize(descriptorNum);
}

DescriptorSetWGPU::~DescriptorSetWGPU() {
    for (DescriptorSetBindGroupWGPU& cache : m_BindGroups) {
        if (cache.bindGroup)
            wgpuBindGroupRelease(cache.bindGroup);
    }
}

static bool IsDescriptorCompatibleWithRange(const DescriptorRangeMappingWGPU& range, const DescriptorWGPU& descriptor) {
    const TextureDesc* textureDesc = descriptor.GetTextureDesc();
    if (!textureDesc) {
        return range.type != DescriptorType::TEXTURE
            && range.type != DescriptorType::INPUT_ATTACHMENT
            && range.type != DescriptorType::STORAGE_TEXTURE;
    }

    WGPUTextureViewDimension viewDimension = GetTextureViewDimension(descriptor.GetTextureViewDesc().type, *textureDesc);
    if (range.type == DescriptorType::TEXTURE || range.type == DescriptorType::INPUT_ATTACHMENT) {
        WGPUTextureSampleType sampleType = GetTextureSampleType(descriptor.GetFormat());
        bool isSampleTypeCompatible = sampleType == range.textureSampleType || (sampleType == WGPUTextureSampleType_Float && range.textureSampleType == WGPUTextureSampleType_UnfilterableFloat);

        return isSampleTypeCompatible
            && viewDimension == range.textureViewDimension
            && (textureDesc->sampleNum > 1 ? WGPU_TRUE : WGPU_FALSE) == range.textureMultisampled;
    }

    if (range.type != DescriptorType::STORAGE_TEXTURE)
        return true;

    if (textureDesc->sampleNum > 1)
        return false;

    return GetTextureFormat(descriptor.GetFormat()) == range.storageTextureFormat && viewDimension == range.storageTextureViewDimension;
}

void DescriptorSetWGPU::UpdateRange(uint32_t rangeIndex, uint32_t baseDescriptor, const Descriptor* const* descriptors, uint32_t descriptorNum) {
    ExclusiveScope lock(m_BindGroupLock);

    const DescriptorRangeMappingWGPU& range = m_Mapping.ranges[rangeIndex];

    for (uint32_t i = 0; i < descriptorNum; i++)
        m_Descriptors[range.descriptorOffset + baseDescriptor + i] = (DescriptorWGPU*)descriptors[i];

    m_UpdateVersion++;
}

void DescriptorSetWGPU::CopyRangeFrom(uint32_t dstRangeIndex, uint32_t dstBaseDescriptor, const DescriptorSetWGPU& srcDescriptorSet, uint32_t srcRangeIndex, uint32_t srcBaseDescriptor, uint32_t descriptorNum) {
    ExclusiveScope lock(m_BindGroupLock);

    const DescriptorRangeMappingWGPU& dstRange = m_Mapping.ranges[dstRangeIndex];
    const DescriptorRangeMappingWGPU& srcRange = srcDescriptorSet.m_Mapping.ranges[srcRangeIndex];
    uint32_t copyNum = descriptorNum == ALL ? srcRange.descriptorNum - srcBaseDescriptor : descriptorNum;

    for (uint32_t i = 0; i < copyNum; i++)
        m_Descriptors[dstRange.descriptorOffset + dstBaseDescriptor + i] = srcDescriptorSet.m_Descriptors[srcRange.descriptorOffset + srcBaseDescriptor + i];

    m_UpdateVersion++;
}

void DescriptorSetWGPU::FinalizeUpdate() const {
    GetBindGroup();
}

WGPUBindGroup DescriptorSetWGPU::GetBindGroup(const DescriptorSetMappingWGPU& mapping) const {
    if (!mapping.layout)
        return nullptr;

    ExclusiveScope lock(m_BindGroupLock);

    for (DescriptorSetBindGroupWGPU& cache : m_BindGroups) {
        if (cache.layout == mapping.layout) {
            if (cache.updateVersion != m_UpdateVersion && !RecreateBindGroup(mapping, cache))
                return nullptr;

            return cache.bindGroup;
        }
    }

    m_BindGroups.push_back({mapping.layout});
    DescriptorSetBindGroupWGPU& cache = m_BindGroups.back();
    if (!RecreateBindGroup(mapping, cache))
        return nullptr;

    return cache.bindGroup;
}

bool DescriptorSetWGPU::RecreateBindGroup(const DescriptorSetMappingWGPU& mapping, DescriptorSetBindGroupWGPU& cache) const {
    // TODO: Bind groups are recreated on descriptor updates/copies. This is correct but can be expensive for update-heavy workloads.
    auto resetBindGroup = [&]() {
        if (cache.bindGroup) {
            wgpuBindGroupRelease(cache.bindGroup);
            cache.bindGroup = nullptr;
        }
    };

    for (const DescriptorRangeMappingWGPU& range : mapping.ranges) {
        for (uint32_t i = 0; i < range.descriptorNum; i++) {
            DescriptorWGPU* descriptor = m_Descriptors[range.descriptorOffset + i];
            if (!descriptor || !IsDescriptorCompatibleWithRange(range, *descriptor)) {
                resetBindGroup();
                cache.updateVersion = m_UpdateVersion;
                return false;
            }
        }
    }

    uint32_t entryMaxNum = 0;
    for (const DescriptorRangeMappingWGPU& range : mapping.ranges)
        entryMaxNum += range.isArray ? 1 : range.descriptorNum;

    Scratch<WGPUBindGroupEntry> entries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupEntry, entryMaxNum);
    Scratch<WGPUBindGroupEntryExtras> entryExtras = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupEntryExtras, entryMaxNum);
    Scratch<WGPUSampler> samplers = NRI_ALLOCATE_SCRATCH(m_Device, WGPUSampler, m_Descriptors.size());
    Scratch<WGPUTextureView> textureViews = NRI_ALLOCATE_SCRATCH(m_Device, WGPUTextureView, m_Descriptors.size());
    Scratch<WGPUBuffer> buffers = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBuffer, m_Descriptors.size());

    uint32_t entryNum = 0;
    uint32_t resourceOffset = 0;

    for (const DescriptorRangeMappingWGPU& range : mapping.ranges) {
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

    WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    desc.layout = mapping.layout;
    desc.entryCount = entryNum;
    desc.entries = entries;

    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_Device, &desc);
    if (!bindGroup) {
        resetBindGroup();
        cache.updateVersion = m_UpdateVersion;
        return false;
    }

    if (cache.bindGroup)
        wgpuBindGroupRelease(cache.bindGroup);

    cache.bindGroup = bindGroup;
    cache.updateVersion = m_UpdateVersion;

    return true;
}

void DescriptorSetWGPU::GetOffsets(uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) const {
    // TODO: Descriptor heap indexing/bindless is unsupported, so heap offsets are always zero.
    resourceHeapOffset = 0;
    samplerHeapOffset = 0;
}
