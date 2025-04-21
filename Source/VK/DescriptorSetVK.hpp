// © 2021 NVIDIA Corporation

struct SlabAllocator {
    inline SlabAllocator(void* memory, size_t size)
        : m_CurrentOffset((uint8_t*)memory)
        , m_End((size_t)memory + size)
        , m_Memory((uint8_t*)memory) {
    }

    template <typename T>
    inline T* Allocate(uint32_t& number) {
        T* items = (T*)Align(m_CurrentOffset, alignof(T));
        const size_t itemsLeft = (m_End - (size_t)items) / sizeof(T);
        number = std::min<uint32_t>((uint32_t)itemsLeft, number);
        m_CurrentOffset = (uint8_t*)(items + number);
        return items;
    }

    inline void Reset() {
        m_CurrentOffset = m_Memory;
    }

private:
    uint8_t* m_CurrentOffset;
    size_t m_End;
    uint8_t* m_Memory;
};

static bool WriteTextures(const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update, uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab) {
    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkDescriptorImageInfo* infoArray = slab.Allocate<VkDescriptorImageInfo>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++) {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];

        infoArray[i].imageView = descriptorImpl.GetImageView();
        infoArray[i].imageLayout = descriptorImpl.GetTexDesc().layout;
        infoArray[i].sampler = VK_NULL_HANDLE;
    }

    write.descriptorType = GetDescriptorType(rangeDesc.descriptorType);
    write.pImageInfo = infoArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;

    return itemNumForWriting == totalItemNum;
}

static bool WriteTypedBuffers(const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update, uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab) {
    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkBufferView* viewArray = slab.Allocate<VkBufferView>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++) {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        viewArray[i] = descriptorImpl.GetBufferView();
    }

    write.descriptorType = GetDescriptorType(rangeDesc.descriptorType);
    write.pTexelBufferView = viewArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;

    return itemNumForWriting == totalItemNum;
}

static bool WriteBuffers(const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update, uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab) {
    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkDescriptorBufferInfo* infoArray = slab.Allocate<VkDescriptorBufferInfo>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++) {
        const DescriptorVK& descriptor = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        infoArray[i] = descriptor.GetBufferInfo();
    }

    write.descriptorType = GetDescriptorType(rangeDesc.descriptorType);
    write.pBufferInfo = infoArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;

    return itemNumForWriting == totalItemNum;
}

static bool WriteSamplers(const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update, uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab) {
    MaybeUnused(rangeDesc);

    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;
    VkDescriptorImageInfo* infoArray = slab.Allocate<VkDescriptorImageInfo>(itemNumForWriting);

    for (uint32_t i = 0; i < itemNumForWriting; i++) {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        infoArray[i].imageView = VK_NULL_HANDLE;
        infoArray[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        infoArray[i].sampler = descriptorImpl.GetSampler();
    }

    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo = infoArray;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;

    return itemNumForWriting == totalItemNum;
}

static bool WriteAccelerationStructures(const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update, uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab) {
    MaybeUnused(rangeDesc);

    const uint32_t totalItemNum = update.descriptorNum - descriptorOffset;
    uint32_t itemNumForWriting = totalItemNum;

    uint32_t infoNum = 1;
    VkWriteDescriptorSetAccelerationStructureKHR* info = slab.Allocate<VkWriteDescriptorSetAccelerationStructureKHR>(infoNum);

    if (infoNum != 1)
        return false;

    VkAccelerationStructureKHR* handles = slab.Allocate<VkAccelerationStructureKHR>(itemNumForWriting);

    info->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    info->pNext = nullptr;
    info->accelerationStructureCount = itemNumForWriting;
    info->pAccelerationStructures = handles;

    for (uint32_t i = 0; i < itemNumForWriting; i++) {
        const DescriptorVK& descriptorImpl = *(DescriptorVK*)update.descriptors[descriptorOffset + i];
        handles[i] = descriptorImpl.GetAccelerationStructure();
    }

    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.pNext = info;
    write.descriptorCount = itemNumForWriting;

    descriptorOffset += itemNumForWriting;

    return itemNumForWriting == totalItemNum;
}

typedef bool (*WriteDescriptorsFunc)(const DescriptorRangeDesc& rangeDesc, const DescriptorRangeUpdateDesc& update, uint32_t& descriptorOffset, VkWriteDescriptorSet& write, SlabAllocator& slab);

constexpr std::array<WriteDescriptorsFunc, (size_t)DescriptorType::MAX_NUM> g_WriteFuncs = {
    (WriteDescriptorsFunc)&WriteSamplers,               // SAMPLER
    (WriteDescriptorsFunc)&WriteBuffers,                // CONSTANT_BUFFER
    (WriteDescriptorsFunc)&WriteTextures,               // TEXTURE
    (WriteDescriptorsFunc)&WriteTextures,               // STORAGE_TEXTURE
    (WriteDescriptorsFunc)&WriteTypedBuffers,           // BUFFER
    (WriteDescriptorsFunc)&WriteTypedBuffers,           // STORAGE_BUFFER
    (WriteDescriptorsFunc)&WriteBuffers,                // STRUCTURED_BUFFER
    (WriteDescriptorsFunc)&WriteBuffers,                // STORAGE_STRUCTURED_BUFFER
    (WriteDescriptorsFunc)&WriteAccelerationStructures, // ACCELERATION_STRUCTURE
};
VALIDATE_ARRAY_BY_PTR(g_WriteFuncs);

NRI_INLINE void DescriptorSetVK::SetDebugName(const char* name) {
    m_Device->SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)m_Handle, name);
}

NRI_INLINE void DescriptorSetVK::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    constexpr uint32_t writesPerIteration = 256;
    constexpr size_t slabSize = 32 * writesPerIteration; // max item size = 32
    static_assert(slabSize <= MAX_STACK_ALLOC_SIZE, "prefer stack alloc");

    uint32_t writeMaxNum = std::min<uint32_t>(writesPerIteration, rangeNum);
    Scratch<VkWriteDescriptorSet> writes = AllocateScratch(*m_Device, VkWriteDescriptorSet, writeMaxNum);

    Scratch<uint8_t> slabScratch = AllocateScratch(*m_Device, uint8_t, slabSize);
    SlabAllocator slab(slabScratch, slabSize);

    uint32_t j = 0;
    uint32_t descriptorOffset = 0;

    do {
        slab.Reset();
        bool slabHasFreeSpace = true;
        uint32_t writeNum = 0;

        while (slabHasFreeSpace && j < rangeNum && writeNum < writeMaxNum) {
            const DescriptorRangeUpdateDesc& update = rangeUpdateDescs[j];
            const DescriptorRangeDesc& rangeDesc = m_Desc->ranges[rangeOffset + j];

            uint32_t offset = update.baseDescriptor + descriptorOffset;

            VkWriteDescriptorSet& write = writes[writeNum++];
            write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = m_Handle;

            bool isArray = rangeDesc.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
            if (isArray) {
                write.dstBinding = rangeDesc.baseRegisterIndex;
                write.dstArrayElement = offset;
            } else {
                write.dstBinding = rangeDesc.baseRegisterIndex + offset;
                write.dstArrayElement = 0;
            }

            const uint32_t index = (uint32_t)rangeDesc.descriptorType;
            slabHasFreeSpace = g_WriteFuncs[index](rangeDesc, update, descriptorOffset, write, slab);

            j += (descriptorOffset == update.descriptorNum) ? 1 : 0;
            descriptorOffset = (descriptorOffset == update.descriptorNum) ? 0 : descriptorOffset;
        }

        const auto& vk = m_Device->GetDispatchTable();
        vk.UpdateDescriptorSets(*m_Device, writeNum, writes, 0, nullptr);
    } while (j < rangeNum);
}

NRI_INLINE void DescriptorSetVK::UpdateDynamicConstantBuffers(uint32_t bufferOffset, uint32_t descriptorNum, const Descriptor* const* descriptors) {
    Scratch<VkWriteDescriptorSet> writes = AllocateScratch(*m_Device, VkWriteDescriptorSet, descriptorNum);
    Scratch<VkDescriptorBufferInfo> infos = AllocateScratch(*m_Device, VkDescriptorBufferInfo, descriptorNum);

    for (uint32_t j = 0; j < descriptorNum; j++) {
        const DynamicConstantBufferDesc& bufferDesc = m_Desc->dynamicConstantBuffers[bufferOffset + j];
        const DescriptorVK& descriptorImpl = *(const DescriptorVK*)descriptors[j];

        VkDescriptorBufferInfo& bufferInfo = infos[j];
        bufferInfo = descriptorImpl.GetBufferInfo();

        VkWriteDescriptorSet& write = writes[j];
        write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_Handle;
        write.dstBinding = bufferDesc.registerIndex;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.pBufferInfo = &bufferInfo;
    }

    const auto& vk = m_Device->GetDispatchTable();
    vk.UpdateDescriptorSets(*m_Device, descriptorNum, writes, 0, nullptr);
}

NRI_INLINE void DescriptorSetVK::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    const uint32_t rangeNum = descriptorSetCopyDesc.rangeNum + descriptorSetCopyDesc.dynamicConstantBufferNum;

    Scratch<VkCopyDescriptorSet> copies = AllocateScratch(*m_Device, VkCopyDescriptorSet, rangeNum);
    uint32_t copyNum = 0;

    const DescriptorSetVK& srcSetImpl = *(const DescriptorSetVK*)descriptorSetCopyDesc.srcDescriptorSet;

    for (uint32_t j = 0; j < descriptorSetCopyDesc.rangeNum; j++) {
        const DescriptorRangeDesc& srcRangeDesc = srcSetImpl.m_Desc->ranges[descriptorSetCopyDesc.srcBaseRange + j];
        const DescriptorRangeDesc& dstRangeDesc = m_Desc->ranges[descriptorSetCopyDesc.dstBaseRange + j];

        VkCopyDescriptorSet& copy = copies[copyNum++];
        copy = {VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
        copy.srcSet = srcSetImpl.GetHandle();
        copy.srcBinding = srcRangeDesc.baseRegisterIndex;
        copy.srcArrayElement = 0; // TODO: special support needed?
        copy.dstSet = m_Handle;
        copy.dstBinding = dstRangeDesc.baseRegisterIndex;
        copy.dstArrayElement = 0; // TODO: special support needed?
        copy.descriptorCount = dstRangeDesc.descriptorNum;
    }

    for (uint32_t j = 0; j < descriptorSetCopyDesc.dynamicConstantBufferNum; j++) {
        const uint32_t srcBufferIndex = descriptorSetCopyDesc.srcBaseDynamicConstantBuffer + j;
        const DynamicConstantBufferDesc& srcBuffer = srcSetImpl.m_Desc->dynamicConstantBuffers[srcBufferIndex];
        const DynamicConstantBufferDesc& dstBuffer = m_Desc->dynamicConstantBuffers[srcBufferIndex];

        VkCopyDescriptorSet& copy = copies[copyNum++];
        copy = {VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
        copy.srcSet = srcSetImpl.GetHandle();
        copy.srcBinding = srcBuffer.registerIndex;
        copy.dstSet = m_Handle;
        copy.dstBinding = dstBuffer.registerIndex;
        copy.descriptorCount = 1;
    }

    const auto& vk = m_Device->GetDispatchTable();
    vk.UpdateDescriptorSets(*m_Device, 0, nullptr, copyNum, copies);
}
