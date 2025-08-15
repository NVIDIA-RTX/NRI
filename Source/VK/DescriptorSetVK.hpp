// © 2021 NVIDIA Corporation

static void WriteSamplers(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const DescriptorRangeUpdateDesc& rangeUpdateDesc) {
    VkDescriptorImageInfo* imageInfos = (VkDescriptorImageInfo*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkDescriptorImageInfo);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        imageInfos[i].imageView = VK_NULL_HANDLE;
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfos[i].sampler = descriptorVK.GetSampler();
    }

    writeDescriptorSet.pImageInfo = imageInfos;
}

static void WriteTextures(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const DescriptorRangeUpdateDesc& rangeUpdateDesc) {
    VkDescriptorImageInfo* imageInfos = (VkDescriptorImageInfo*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkDescriptorImageInfo);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];

        imageInfos[i].imageView = descriptorVK.GetImageView();
        imageInfos[i].imageLayout = descriptorVK.GetTexDesc().layout;
        imageInfos[i].sampler = VK_NULL_HANDLE;
    }

    writeDescriptorSet.pImageInfo = imageInfos;
}

static void WriteBuffers(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const DescriptorRangeUpdateDesc& rangeUpdateDesc) {
    VkDescriptorBufferInfo* bufferInfos = (VkDescriptorBufferInfo*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkDescriptorBufferInfo);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptor = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        bufferInfos[i] = descriptor.GetBufferInfo();
    }

    writeDescriptorSet.pBufferInfo = bufferInfos;
}

static void WriteTypedBuffers(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const DescriptorRangeUpdateDesc& rangeUpdateDesc) {
    VkBufferView* bufferViews = (VkBufferView*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkBufferView);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        bufferViews[i] = descriptorVK.GetBufferView();
    }

    writeDescriptorSet.pTexelBufferView = bufferViews;
}

static void WriteAccelerationStructures(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const DescriptorRangeUpdateDesc& rangeUpdateDesc) {
    VkAccelerationStructureKHR* accelerationStructures = (VkAccelerationStructureKHR*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkAccelerationStructureKHR);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        accelerationStructures[i] = descriptorVK.GetAccelerationStructure();
    }

    VkWriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo = (VkWriteDescriptorSetAccelerationStructureKHR*)(scratch + scratchOffset);
    scratchOffset += sizeof(VkWriteDescriptorSetAccelerationStructureKHR);

    accelerationStructureInfo->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureInfo->pNext = nullptr;
    accelerationStructureInfo->accelerationStructureCount = rangeUpdateDesc.descriptorNum;
    accelerationStructureInfo->pAccelerationStructures = accelerationStructures;

    writeDescriptorSet.pNext = accelerationStructureInfo;
}

typedef void (*WriteDescriptorsFunc)(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const DescriptorRangeUpdateDesc& rangeUpdateDesc);

constexpr std::array<WriteDescriptorsFunc, (size_t)DescriptorType::MAX_NUM> g_WriteFuncs = {
    WriteSamplers,               // SAMPLER
    WriteBuffers,                // CONSTANT_BUFFER
    WriteTextures,               // TEXTURE
    WriteTextures,               // STORAGE_TEXTURE
    WriteTypedBuffers,           // BUFFER
    WriteTypedBuffers,           // STORAGE_BUFFER
    WriteBuffers,                // STRUCTURED_BUFFER
    WriteBuffers,                // STORAGE_STRUCTURED_BUFFER
    WriteAccelerationStructures, // ACCELERATION_STRUCTURE
};
VALIDATE_ARRAY_BY_PTR(g_WriteFuncs);

NRI_INLINE void DescriptorSetVK::SetDebugName(const char* name) {
    m_Device->SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)m_Handle, name);
}

NRI_INLINE void DescriptorSetVK::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    // Count and allocate scratch memory
    uint32_t scratchSize = 0;
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeUpdateDesc& rangeUpdateDesc = rangeUpdateDescs[i];
        const DescriptorRangeDesc& rangeDesc = m_Desc->ranges[rangeOffset + i];

        switch (rangeDesc.descriptorType) {
            case DescriptorType::SAMPLER:
            case DescriptorType::TEXTURE:
            case DescriptorType::STORAGE_TEXTURE:
                scratchSize += sizeof(VkDescriptorImageInfo) * rangeUpdateDesc.descriptorNum;
                break;
            case DescriptorType::CONSTANT_BUFFER:
            case DescriptorType::STRUCTURED_BUFFER:
            case DescriptorType::STORAGE_STRUCTURED_BUFFER:
                scratchSize += sizeof(VkDescriptorBufferInfo) * rangeUpdateDesc.descriptorNum;
                break;
            case DescriptorType::BUFFER:
            case DescriptorType::STORAGE_BUFFER:
                scratchSize += sizeof(VkBufferView) * rangeUpdateDesc.descriptorNum;
                break;

            case DescriptorType::ACCELERATION_STRUCTURE:
                scratchSize += sizeof(VkAccelerationStructureKHR) * rangeUpdateDesc.descriptorNum + sizeof(VkWriteDescriptorSetAccelerationStructureKHR);
                break;
        }

        scratchSize += sizeof(VkWriteDescriptorSet);
    }

    Scratch<uint8_t> scratch = AllocateScratch(*m_Device, uint8_t, scratchSize);
    size_t scratchOffset = rangeNum * sizeof(VkWriteDescriptorSet);

    // Update ranges
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeUpdateDesc& rangeUpdateDesc = rangeUpdateDescs[i];
        const DescriptorRangeDesc& rangeDesc = m_Desc->ranges[rangeOffset + i];

        VkWriteDescriptorSet& writeDescriptorSet = *(VkWriteDescriptorSet*)(scratch + i * sizeof(VkWriteDescriptorSet)); // must be first and consecutive in "scratch"
        writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeDescriptorSet.dstSet = m_Handle;
        writeDescriptorSet.descriptorCount = rangeUpdateDesc.descriptorNum;
        writeDescriptorSet.descriptorType = GetDescriptorType(rangeDesc.descriptorType);

        bool isArray = rangeDesc.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        if (isArray) {
            writeDescriptorSet.dstBinding = rangeDesc.baseRegisterIndex;
            writeDescriptorSet.dstArrayElement = rangeUpdateDesc.baseDescriptor;
        } else
            writeDescriptorSet.dstBinding = rangeDesc.baseRegisterIndex + rangeUpdateDesc.baseDescriptor;

        g_WriteFuncs[(uint32_t)rangeDesc.descriptorType](writeDescriptorSet, scratchOffset, scratch, rangeUpdateDesc);
    }

    const auto& vk = m_Device->GetDispatchTable();
    vk.UpdateDescriptorSets(*m_Device, rangeNum, (VkWriteDescriptorSet*)(scratch + 0), 0, nullptr);
}

NRI_INLINE void DescriptorSetVK::UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    Scratch<VkWriteDescriptorSet> writes = AllocateScratch(*m_Device, VkWriteDescriptorSet, dynamicConstantBufferNum);
    Scratch<VkDescriptorBufferInfo> infos = AllocateScratch(*m_Device, VkDescriptorBufferInfo, dynamicConstantBufferNum);

    for (uint32_t j = 0; j < dynamicConstantBufferNum; j++) {
        const DynamicConstantBufferDesc& bufferDesc = m_Desc->dynamicConstantBuffers[baseDynamicConstantBuffer + j];
        const DescriptorVK& descriptorVK = *(const DescriptorVK*)descriptors[j];

        VkDescriptorBufferInfo& bufferInfo = infos[j];
        bufferInfo = descriptorVK.GetBufferInfo();

        VkWriteDescriptorSet& writeDescriptorSet = writes[j];
        writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeDescriptorSet.dstSet = m_Handle;
        writeDescriptorSet.dstBinding = bufferDesc.registerIndex;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writeDescriptorSet.pBufferInfo = &bufferInfo;
    }

    const auto& vk = m_Device->GetDispatchTable();
    vk.UpdateDescriptorSets(*m_Device, dynamicConstantBufferNum, writes, 0, nullptr);
}

NRI_INLINE void DescriptorSetVK::Copy(const CopyDescriptorSetDesc& copyDescriptorSetDesc) {
    uint32_t totalRangeNum = copyDescriptorSetDesc.rangeNum + copyDescriptorSetDesc.dynamicConstantBufferNum;

    Scratch<VkCopyDescriptorSet> copies = AllocateScratch(*m_Device, VkCopyDescriptorSet, totalRangeNum);
    uint32_t copyNum = 0;

    const DescriptorSetVK& srcDescriptorSetVK = *(DescriptorSetVK*)copyDescriptorSetDesc.srcDescriptorSet;

    for (uint32_t j = 0; j < copyDescriptorSetDesc.rangeNum; j++) {
        const DescriptorRangeDesc& srcRangeDesc = srcDescriptorSetVK.m_Desc->ranges[copyDescriptorSetDesc.srcBaseRange + j];
        const DescriptorRangeDesc& dstRangeDesc = m_Desc->ranges[copyDescriptorSetDesc.dstBaseRange + j];

        VkCopyDescriptorSet& copy = copies[copyNum++];
        copy = {VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
        copy.srcSet = srcDescriptorSetVK.GetHandle();
        copy.srcBinding = srcRangeDesc.baseRegisterIndex;
        copy.srcArrayElement = 0; // TODO: special support needed?
        copy.dstSet = m_Handle;
        copy.dstBinding = dstRangeDesc.baseRegisterIndex;
        copy.dstArrayElement = 0; // TODO: special support needed?
        copy.descriptorCount = dstRangeDesc.descriptorNum;
    }

    for (uint32_t j = 0; j < copyDescriptorSetDesc.dynamicConstantBufferNum; j++) {
        const uint32_t srcBufferIndex = copyDescriptorSetDesc.srcBaseDynamicConstantBuffer + j;
        const DynamicConstantBufferDesc& srcBuffer = srcDescriptorSetVK.m_Desc->dynamicConstantBuffers[srcBufferIndex];
        const DynamicConstantBufferDesc& dstBuffer = m_Desc->dynamicConstantBuffers[srcBufferIndex];

        VkCopyDescriptorSet& copy = copies[copyNum++];
        copy = {VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
        copy.srcSet = srcDescriptorSetVK.GetHandle();
        copy.srcBinding = srcBuffer.registerIndex;
        copy.dstSet = m_Handle;
        copy.dstBinding = dstBuffer.registerIndex;
        copy.descriptorCount = 1;
    }

    const auto& vk = m_Device->GetDispatchTable();
    vk.UpdateDescriptorSets(*m_Device, 0, nullptr, copyNum, copies);
}
