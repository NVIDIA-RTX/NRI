// © 2021 NVIDIA Corporation

static inline void AddDescriptorPoolSize(std::array<VkDescriptorPoolSize, 16>& poolSizes, uint32_t& poolSizeNum, VkDescriptorType type, uint32_t descriptorCount) {
    if (descriptorCount) {
        VkDescriptorPoolSize& poolSize = poolSizes[poolSizeNum++];
        poolSize.type = type;
        poolSize.descriptorCount = descriptorCount;
    }
}

DescriptorPoolVK::~DescriptorPoolVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyDescriptorPool(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result DescriptorPoolVK::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    std::array<VkDescriptorPoolSize, 16> poolSizes = {};
    uint32_t poolSizeNum = 0;

    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_SAMPLER, descriptorPoolDesc.samplerMaxNum);

    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_MUTABLE_EXT, descriptorPoolDesc.mutableMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorPoolDesc.constantBufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorPoolDesc.textureMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorPoolDesc.storageTextureMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, descriptorPoolDesc.bufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, descriptorPoolDesc.storageBufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorPoolDesc.structuredBufferMaxNum + descriptorPoolDesc.storageStructuredBufferMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, descriptorPoolDesc.accelerationStructureMaxNum);
    AddDescriptorPoolSize(poolSizes, poolSizeNum, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, descriptorPoolDesc.inputAttachmentMaxNum);

    VkDescriptorPoolCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.flags = (descriptorPoolDesc.flags & DescriptorPoolBits::ALLOW_UPDATE_AFTER_SET) ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
    info.maxSets = descriptorPoolDesc.descriptorSetMaxNum;
    info.poolSizeCount = poolSizeNum;
    info.pPoolSizes = poolSizes.data();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateDescriptorPool(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateDescriptorPool");

    m_DescriptorSets.resize(descriptorPoolDesc.descriptorSetMaxNum);

    return Result::SUCCESS;
}

Result DescriptorPoolVK::Create(const DescriptorPoolVKDesc& descriptorPoolVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkDescriptorPool)descriptorPoolVKDesc.vkDescriptorPool;
    m_DescriptorSets.resize(descriptorPoolVKDesc.descriptorSetMaxNum);

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorPoolVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)m_Handle, name);
}

NRI_INLINE Result DescriptorPoolVK::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    ExclusiveScope lock(m_Lock);

    const PipelineLayoutVK& pipelineLayoutVK = (PipelineLayoutVK&)pipelineLayout;
    VkDescriptorSetLayout setLayout = pipelineLayoutVK.GetDescriptorSetLayout(setIndex);

    const auto& bindingInfo = pipelineLayoutVK.GetBindingInfo();
    const DescriptorSetDesc* descriptorSetDesc = &bindingInfo.sets[setIndex];

    bool hasVariableDescriptorNum = false;
    for (uint32_t i = 0; i < descriptorSetDesc->rangeNum; i++) {
        if (descriptorSetDesc->ranges[i].flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY) {
            hasVariableDescriptorNum = true;
            break;
        }
    }

    Scratch<VkDescriptorSetLayout> setLayouts = NRI_ALLOCATE_SCRATCH(m_Device, VkDescriptorSetLayout, instanceNum);
    Scratch<uint32_t> variableDescriptorNums = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, hasVariableDescriptorNum ? instanceNum : 0);
    Scratch<VkDescriptorSet> handles = NRI_ALLOCATE_SCRATCH(m_Device, VkDescriptorSet, instanceNum);

    for (uint32_t i = 0; i < instanceNum; i++) {
        setLayouts[i] = setLayout;
        handles[i] = VK_NULL_HANDLE;

        if (hasVariableDescriptorNum)
            variableDescriptorNums[i] = variableDescriptorNum;
    }

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    variableDescriptorCountInfo.descriptorSetCount = instanceNum;
    variableDescriptorCountInfo.pDescriptorCounts = variableDescriptorNums;

    VkDescriptorSetAllocateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.pNext = hasVariableDescriptorNum ? &variableDescriptorCountInfo : nullptr;
    info.descriptorPool = m_Handle;
    info.descriptorSetCount = instanceNum;
    info.pSetLayouts = setLayouts;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.AllocateDescriptorSets(m_Device, &info, handles);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateDescriptorSets");

    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetVK* descriptorSet = &m_DescriptorSets[m_DescriptorSetNum++];
        descriptorSet->Create(&m_Device, handles[i], descriptorSetDesc);

        descriptorSets[i] = (DescriptorSet*)descriptorSet;
    }

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorPoolVK::Reset() {
    ExclusiveScope lock(m_Lock);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.ResetDescriptorPool(m_Device, m_Handle, (VkDescriptorPoolResetFlags)0);
    NRI_RETURN_VOID_ON_BAD_VKRESULT(&m_Device, vkResult, "vkResetDescriptorPool");

    m_DescriptorSetNum = 0;
}
