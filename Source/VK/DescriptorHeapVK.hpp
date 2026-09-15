// © 2026 NVIDIA Corporation

static inline bool IsCustomBorderColor(VkBorderColor borderColor) {
    return borderColor == VK_BORDER_COLOR_FLOAT_CUSTOM_EXT || borderColor == VK_BORDER_COLOR_INT_CUSTOM_EXT;
}

struct ImageDescriptorWriteDataVK {
    VkImageViewCreateInfo view;
    VkImageViewUsageCreateInfo usage;
    VkImageViewSlicedCreateInfoEXT slice;
    VkImageDescriptorInfoEXT descriptor;
};

union ResourceDescriptorWriteDataVK {
    ImageDescriptorWriteDataVK image;
    VkTexelBufferDescriptorInfoEXT texelBuffer;
    VkDeviceAddressRangeEXT addressRange;
};

struct SamplerDescriptorWriteDataVK {
    VkSamplerReductionModeCreateInfo reduction;
    VkSamplerCustomBorderColorCreateInfoEXT borderColor;
    VkSamplerCustomBorderColorIndexCreateInfoEXT borderColorIndex;
    uint32_t registeredIndex;
};

static inline Result FlushDescriptorWrites(BufferVK& heap, VkHostAddressRangeEXT* writes, uint32_t writeNum) {
    // Non-coherent memory requires explicit flushing. Writes can target arbitrary descriptor indices, so sort them by address before merging ranges that share a non-coherent atom.
    std::sort(writes, writes + writeNum, [](const VkHostAddressRangeEXT& a, const VkHostAddressRangeEXT& b) { return (uintptr_t)a.address < (uintptr_t)b.address; });

    const uint64_t atomSize = heap.GetDevice().GetNonCoherentAtomSize();
    const uintptr_t heapAddress = (uintptr_t)heap.GetMappedMemory();
    uint64_t flushBegin = (uintptr_t)writes[0].address - heapAddress;
    uint64_t flushEnd = flushBegin + writes[0].size;
    uint64_t alignedFlushEnd = Align(flushEnd, atomSize);

    for (uint32_t i = 1; i < writeNum; i++) {
        const uint64_t writeBegin = (uintptr_t)writes[i].address - heapAddress;
        const uint64_t writeEnd = writeBegin + writes[i].size;
        const uint64_t alignedWriteBegin = writeBegin & ~(atomSize - 1);
        if (alignedWriteBegin <= alignedFlushEnd) {
            flushEnd = std::max(flushEnd, writeEnd);
            alignedFlushEnd = Align(flushEnd, atomSize);
            continue;
        }

        Result result = heap.FlushMappedRange(flushBegin, flushEnd - flushBegin);
        if (result != Result::SUCCESS)
            return result;

        flushBegin = writeBegin;
        flushEnd = writeEnd;
        alignedFlushEnd = Align(flushEnd, atomSize);
    }

    return heap.FlushMappedRange(flushBegin, flushEnd - flushBegin);
}

DescriptorHeapVK::~DescriptorHeapVK() {
    const auto& vk = m_Device.GetDispatchTable();
    for (uint32_t registeredIndex : m_CustomBorderColorIndices) {
        if (registeredIndex != UINT32_MAX)
            vk.UnregisterCustomBorderColorEXT(m_Device, registeredIndex);
    }

    Destroy(m_Device.GetAllocationCallbacks(), m_ResourceHeap);
    Destroy(m_Device.GetAllocationCallbacks(), m_SamplerHeap);
}

Result DescriptorHeapVK::Create(const DescriptorHeapDesc& descriptorHeapDesc) {
    const VkPhysicalDeviceDescriptorHeapPropertiesEXT& props = m_Device.GetDescriptorHeapProperties();
    m_ResourceDescriptorSize = std::max(props.imageDescriptorSize, props.bufferDescriptorSize);
    m_SamplerDescriptorSize = props.samplerDescriptorSize;

    const uint64_t resourceReservedSize = descriptorHeapDesc.resourceDescriptorNum ? props.minResourceHeapReservedRange : 0;
    Result result = CreateHeap(m_ResourceHeap, m_ResourceBindInfo, descriptorHeapDesc.resourceDescriptorNum, m_ResourceDescriptorSize, resourceReservedSize, props.resourceHeapAlignment);
    if (result != Result::SUCCESS)
        return result;

    result = CreateHeap(m_SamplerHeap, m_SamplerBindInfo, descriptorHeapDesc.samplerDescriptorNum, m_SamplerDescriptorSize, props.minSamplerHeapReservedRangeWithEmbedded, props.samplerHeapAlignment);
    if (result != Result::SUCCESS)
        return result;

    m_CustomBorderColorIndices.resize(descriptorHeapDesc.samplerDescriptorNum, UINT32_MAX);

    return Result::SUCCESS;
}

Result DescriptorHeapVK::CreateHeap(BufferVK*& heap, VkBindHeapInfoEXT& bindInfo, uint64_t descriptorNum, uint64_t descriptorSize, uint64_t reservedSize, uint64_t alignment) {
    if (!descriptorNum && !reservedSize)
        return Result::SUCCESS;

    const uint64_t applicationSize = descriptorNum * descriptorSize;
    const uint64_t reservedOffset = applicationSize;
    const uint64_t heapSize = reservedOffset + reservedSize;

    heap = Allocate<BufferVK>(m_Device.GetAllocationCallbacks(), m_Device);
    if (!heap)
        return Result::OUT_OF_MEMORY;

    Result result = heap->CreateDescriptorHeap(heapSize, alignment);
    if (result != Result::SUCCESS)
        return result;

    bindInfo.heapRange.address = heap->GetDeviceAddress();
    bindInfo.heapRange.size = heapSize;
    bindInfo.reservedRangeOffset = reservedOffset;
    bindInfo.reservedRangeSize = reservedSize;

    return Result::SUCCESS;
}

Result DescriptorHeapVK::WriteResourceDescriptors(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    if (!writeDescNum)
        return Result::SUCCESS;

    if (m_ResourceHeap->RequiresMappedMemoryFlush()) {
        ExclusiveScope lock(m_ResourceLock);

        return WriteResourceDescriptorsInternal(writeDescs, writeDescNum);
    }

    return WriteResourceDescriptorsInternal(writeDescs, writeDescNum);
}

Result DescriptorHeapVK::WriteResourceDescriptorsInternal(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    Scratch<VkResourceDescriptorInfoEXT> resources = NRI_ALLOCATE_SCRATCH(m_Device, VkResourceDescriptorInfoEXT, writeDescNum);
    Scratch<VkHostAddressRangeEXT> outputs = NRI_ALLOCATE_SCRATCH(m_Device, VkHostAddressRangeEXT, writeDescNum);
    Scratch<ResourceDescriptorWriteDataVK> writeData = NRI_ALLOCATE_SCRATCH(m_Device, ResourceDescriptorWriteDataVK, writeDescNum);

    for (uint32_t i = 0; i < writeDescNum; i++) {
        const DescriptorVK& descriptor = *(DescriptorVK*)writeDescs[i].resource;
        const DescriptorType type = descriptor.GetType();
        const uint64_t offset = uint64_t(writeDescs[i].descriptorIndex) * m_ResourceDescriptorSize;
        ResourceDescriptorWriteDataVK& data = writeData[i];

        resources[i] = {VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT};
        resources[i].type = GetDescriptorType(type);
        outputs[i] = {};
        outputs[i].address = m_ResourceHeap->GetMappedMemory() + offset;
        outputs[i].size = (size_t)m_ResourceDescriptorSize;

        if (type == DescriptorType::TEXTURE || type == DescriptorType::STORAGE_TEXTURE) {
            data.image = {};
            data.image.descriptor.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT;
            descriptor.FillImageViewCreateInfo(data.image.view, data.image.usage, data.image.slice);
            data.image.descriptor.pView = &data.image.view;
            data.image.descriptor.layout = descriptor.GetTexViewDesc().expectedLayout;

            resources[i].data.pImage = &data.image.descriptor;
        } else if (descriptor.GetFormat() != Format::UNKNOWN) {
            data.texelBuffer = {VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT};
            const VkDescriptorBufferInfo bufferInfo = descriptor.GetBufferInfo();
            data.texelBuffer.format = GetVkFormat(descriptor.GetFormat());
            data.texelBuffer.addressRange.address = descriptor.GetDeviceAddress();
            data.texelBuffer.addressRange.size = bufferInfo.range;

            resources[i].data.pTexelBuffer = &data.texelBuffer;
        } else {
            data.addressRange = {};
            data.addressRange.address = descriptor.GetDeviceAddress();
            data.addressRange.size = type == DescriptorType::ACCELERATION_STRUCTURE ? 0 : descriptor.GetBufferInfo().range;

            resources[i].data.pAddressRange = &data.addressRange;
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.WriteResourceDescriptorsEXT(m_Device, writeDescNum, resources, outputs);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkWriteResourceDescriptorsEXT");

    if (m_ResourceHeap->RequiresMappedMemoryFlush())
        return FlushDescriptorWrites(*m_ResourceHeap, outputs, writeDescNum);

    return Result::SUCCESS;
}

Result DescriptorHeapVK::WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    if (!writeDescNum)
        return Result::SUCCESS;

    if (m_SamplerHeap->RequiresMappedMemoryFlush()) {
        ExclusiveScope lock(m_SamplerLock);

        return WriteSamplerDescriptorsInternal(writeDescs, writeDescNum);
    }

    return WriteSamplerDescriptorsInternal(writeDescs, writeDescNum);
}

Result DescriptorHeapVK::WriteSamplerDescriptorsInternal(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    Scratch<VkSamplerCreateInfo> samplers = NRI_ALLOCATE_SCRATCH(m_Device, VkSamplerCreateInfo, writeDescNum);
    Scratch<VkHostAddressRangeEXT> outputs = NRI_ALLOCATE_SCRATCH(m_Device, VkHostAddressRangeEXT, writeDescNum);
    Scratch<SamplerDescriptorWriteDataVK> writeData = NRI_ALLOCATE_SCRATCH(m_Device, SamplerDescriptorWriteDataVK, writeDescNum);

    const auto& vk = m_Device.GetDispatchTable();
    bool hasNewCustomBorderColors = false;
    bool hasCustomBorderColorUpdates = false;
    for (uint32_t i = 0; i < writeDescNum; i++) {
        const DescriptorVK& descriptor = *(DescriptorVK*)writeDescs[i].sampler;
        const uint64_t offset = uint64_t(writeDescs[i].descriptorIndex) * m_SamplerDescriptorSize;
        SamplerDescriptorWriteDataVK& data = writeData[i];

        samplers[i] = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        outputs[i] = {};
        data = {};
        data.reduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        data.borderColor.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
        data.borderColorIndex.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_INDEX_CREATE_INFO_EXT;
        data.registeredIndex = UINT32_MAX;

        SamplerDesc samplerDesc = {};
        descriptor.FillSamplerDesc(samplerDesc);
        m_Device.FillCreateInfo(samplerDesc, samplers[i], data.reduction, data.borderColor);
        if (IsCustomBorderColor(samplers[i].borderColor)) {
            VkResult vkResult = vk.RegisterCustomBorderColorEXT(m_Device, &data.borderColor, VK_FALSE, &data.registeredIndex);
            if (vkResult != VK_SUCCESS) {
                if (hasNewCustomBorderColors) {
                    for (uint32_t j = 0; j < i; j++) {
                        if (writeData[j].registeredIndex != UINT32_MAX)
                            vk.UnregisterCustomBorderColorEXT(m_Device, writeData[j].registeredIndex);
                    }
                }

                return GetResultFromVkResult(vkResult);
            }

            hasNewCustomBorderColors = true;
            data.borderColorIndex.index = data.registeredIndex;
            data.borderColorIndex.pNext = samplers[i].pNext;
            samplers[i].pNext = &data.borderColorIndex;
        }

        if (data.registeredIndex != UINT32_MAX || m_CustomBorderColorIndices[writeDescs[i].descriptorIndex] != UINT32_MAX)
            hasCustomBorderColorUpdates = true;

        outputs[i].address = m_SamplerHeap->GetMappedMemory() + offset;
        outputs[i].size = (size_t)m_SamplerDescriptorSize;
    }

    VkResult vkResult = vk.WriteSamplerDescriptorsEXT(m_Device, writeDescNum, samplers, outputs);
    if (vkResult != VK_SUCCESS) {
        if (hasNewCustomBorderColors) {
            for (uint32_t i = 0; i < writeDescNum; i++) {
                if (writeData[i].registeredIndex != UINT32_MAX)
                    vk.UnregisterCustomBorderColorEXT(m_Device, writeData[i].registeredIndex);
            }
        }

        return GetResultFromVkResult(vkResult);
    }

    if (hasCustomBorderColorUpdates) {
        // Commit registration ownership only after the descriptor write succeeds, so a failure leaves existing registrations intact.
        for (uint32_t i = 0; i < writeDescNum; i++) {
            uint32_t& registeredIndex = m_CustomBorderColorIndices[writeDescs[i].descriptorIndex];
            if (registeredIndex != UINT32_MAX)
                vk.UnregisterCustomBorderColorEXT(m_Device, registeredIndex);

            registeredIndex = writeData[i].registeredIndex;
        }
    }

    if (m_SamplerHeap->RequiresMappedMemoryFlush())
        return FlushDescriptorWrites(*m_SamplerHeap, outputs, writeDescNum);

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorHeapVK::Bind(VkCommandBuffer commandBuffer) const {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_ResourceHeap)
        vk.CmdBindResourceHeapEXT(commandBuffer, &m_ResourceBindInfo);
    if (m_SamplerHeap)
        vk.CmdBindSamplerHeapEXT(commandBuffer, &m_SamplerBindInfo);
}

NRI_INLINE void DescriptorHeapVK::SetDebugName(const char* name) {
    if (m_ResourceHeap)
        m_ResourceHeap->SetDebugName(name);
    if (m_SamplerHeap)
        m_SamplerHeap->SetDebugName(name);
}
