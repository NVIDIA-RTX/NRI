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
    size_t scratchOffset = rangeNum * sizeof(VkWriteDescriptorSet);
    size_t scratchSize = scratchOffset;
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
    }

    Scratch<uint8_t> scratch = AllocateScratch(*m_Device, uint8_t, scratchSize);

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
