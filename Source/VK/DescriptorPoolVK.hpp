// © 2021 NVIDIA Corporation

DescriptorPoolVK::~DescriptorPoolVK() {
    const auto& allocator = m_Device.GetAllocationCallbacks();
    for (size_t i = 0; i < m_AllocatedSets.size(); i++) {
        m_AllocatedSets[i]->~DescriptorSetVK();
        allocator.Free(allocator.userArg, m_AllocatedSets[i]);
    }

    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyDescriptorPool(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

static inline void AddDescriptorPoolSize(std::array<VkDescriptorPoolSize, 16>& poolSizes, uint32_t& poolSizeNum, VkDescriptorType type, uint32_t descriptorCount) {
    if (descriptorCount) {
        VkDescriptorPoolSize& poolSize = poolSizes[poolSizeNum++];
        poolSize.type = type;
        poolSize.descriptorCount = descriptorCount;
    }
}

Result DescriptorPoolVK::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    std::array<VkDescriptorPoolSize, 16> poolSizes = {};
    uint32_t poolSizeNum = 0;

    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_SAMPLER, descriptorPoolDesc.samplerMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorPoolDesc.constantBufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, descriptorPoolDesc.dynamicConstantBufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorPoolDesc.textureMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorPoolDesc.storageTextureMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, descriptorPoolDesc.bufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, descriptorPoolDesc.storageBufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorPoolDesc.structuredBufferMaxNum + descriptorPoolDesc.storageStructuredBufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, descriptorPoolDesc.accelerationStructureMaxNum);

    VkDescriptorPoolCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.flags = (descriptorPoolDesc.flags & DescriptorPoolBits::ALLOW_UPDATE_AFTER_SET) ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
    info.maxSets = descriptorPoolDesc.descriptorSetMaxNum;
    info.poolSizeCount = poolSizeNum;
    info.pPoolSizes = poolSizes.data();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateDescriptorPool(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateDescriptorPool returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result DescriptorPoolVK::Create(const DescriptorPoolVKDesc& descriptorPoolVKDesc) {
    if (!descriptorPoolVKDesc.vkDescriptorPool)
        return Result::INVALID_ARGUMENT;

    m_OwnsNativeObjects = false;
    m_Handle = (VkDescriptorPool)descriptorPoolVKDesc.vkDescriptorPool;

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorPoolVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)m_Handle, name);
}

NRI_INLINE Result DescriptorPoolVK::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    ExclusiveScope lock(m_Lock);

    const PipelineLayoutVK& pipelineLayoutVK = (const PipelineLayoutVK&)pipelineLayout;
    VkDescriptorSetLayout setLayout = pipelineLayoutVK.GetDescriptorSetLayout(setIndex);

    uint32_t freeSetNum = (uint32_t)m_AllocatedSets.size() - m_UsedSets;
    if (freeSetNum < instanceNum) {
        uint32_t newSetNum = instanceNum - freeSetNum;
        uint32_t prevSetNum = (uint32_t)m_AllocatedSets.size();
        m_AllocatedSets.resize(prevSetNum + newSetNum);

        const auto& allocationCallbacks = m_Device.GetAllocationCallbacks();
        for (size_t i = 0; i < newSetNum; i++) {
            m_AllocatedSets[prevSetNum + i] = (DescriptorSetVK*)allocationCallbacks.Allocate(allocationCallbacks.userArg, sizeof(DescriptorSetVK), alignof(DescriptorSetVK));
            Construct(m_AllocatedSets[prevSetNum + i], 1, m_Device);
        }
    }

    const auto& bindingInfo = pipelineLayoutVK.GetBindingInfo();
    const DescriptorSetDesc& setDesc = bindingInfo.descriptorSetDescs[setIndex];
    bool hasVariableDescriptorNum = bindingInfo.hasVariableDescriptorNum[setIndex];

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    variableDescriptorCountInfo.descriptorSetCount = 1;
    variableDescriptorCountInfo.pDescriptorCounts = &variableDescriptorNum;

    VkDescriptorSetAllocateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.pNext = hasVariableDescriptorNum ? &variableDescriptorCountInfo : nullptr;
    info.descriptorPool = m_Handle;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &setLayout;

    const auto& vk = m_Device.GetDispatchTable();
    for (uint32_t i = 0; i < instanceNum; i++) {
        VkDescriptorSet handle = VK_NULL_HANDLE;
        VkResult result = vk.AllocateDescriptorSets(m_Device, &info, &handle);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateDescriptorSets returned %d", (int32_t)result);

        descriptorSets[i] = (DescriptorSet*)m_AllocatedSets[m_UsedSets++];
        ((DescriptorSetVK*)descriptorSets[i])->Create(handle, setDesc);
    }

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorPoolVK::Reset() {
    ExclusiveScope lock(m_Lock);

    m_UsedSets = 0;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.ResetDescriptorPool(m_Device, m_Handle, (VkDescriptorPoolResetFlags)0);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, ReturnVoid(), "vkResetDescriptorPool returned %d", (int32_t)result);
}
