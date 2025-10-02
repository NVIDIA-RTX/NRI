// © 2021 NVIDIA Corporation

PipelineLayoutVK::~PipelineLayoutVK() {
    const auto& vk = m_Device.GetDispatchTable();
    const auto allocationCallbacks = m_Device.GetVkAllocationCallbacks();

    if (m_Handle)
        vk.DestroyPipelineLayout(m_Device, m_Handle, allocationCallbacks);

    for (auto handle : m_DescriptorSetLayouts)
        vk.DestroyDescriptorSetLayout(m_Device, handle, allocationCallbacks);

    for (auto handle : m_ImmutableSamplers)
        vk.DestroySampler(m_Device, handle, allocationCallbacks);
}

Result PipelineLayoutVK::Create(const PipelineLayoutDesc& pipelineLayoutDesc) {
    // Binding offsets
    bool ignoreGlobalSPIRVOffsets = (pipelineLayoutDesc.flags & PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS) != 0;

    VKBindingOffsets vkBindingOffsets = {};
    if (!ignoreGlobalSPIRVOffsets)
        vkBindingOffsets = m_Device.GetBindingOffsets();

    std::array<uint32_t, (size_t)DescriptorType::MAX_NUM> bindingOffsets = {};
    bindingOffsets[(size_t)DescriptorType::SAMPLER] = vkBindingOffsets.samplerOffset;
    bindingOffsets[(size_t)DescriptorType::CONSTANT_BUFFER] = vkBindingOffsets.constantBufferOffset;
    bindingOffsets[(size_t)DescriptorType::TEXTURE] = vkBindingOffsets.textureOffset;
    bindingOffsets[(size_t)DescriptorType::STORAGE_TEXTURE] = vkBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(size_t)DescriptorType::BUFFER] = vkBindingOffsets.textureOffset;
    bindingOffsets[(size_t)DescriptorType::STORAGE_BUFFER] = vkBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(size_t)DescriptorType::STRUCTURED_BUFFER] = vkBindingOffsets.textureOffset;
    bindingOffsets[(size_t)DescriptorType::STORAGE_STRUCTURED_BUFFER] = vkBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(size_t)DescriptorType::ACCELERATION_STRUCTURE] = vkBindingOffsets.textureOffset;

    // Binding info
    size_t rangeNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++)
        rangeNum += pipelineLayoutDesc.descriptorSets[i].rangeNum;

    m_BindingInfo.sets.insert(m_BindingInfo.sets.begin(), pipelineLayoutDesc.descriptorSets, pipelineLayoutDesc.descriptorSets + pipelineLayoutDesc.descriptorSetNum);
    m_BindingInfo.ranges.reserve(rangeNum);
    m_BindingInfo.pushConstants.reserve(pipelineLayoutDesc.rootConstantNum);
    m_BindingInfo.pushDescriptors.reserve(pipelineLayoutDesc.rootDescriptorNum + pipelineLayoutDesc.rootSamplerNum);
    m_BindingInfo.rootRegisterSpace = pipelineLayoutDesc.rootRegisterSpace;
    m_BindingInfo.rootSamplerBindingOffset = pipelineLayoutDesc.rootDescriptorNum;

    // Descriptor sets
    uint32_t setNum = 0;

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];

        // Create "non-push" set layout
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        CreateSetLayout(&descriptorSetLayout, descriptorSetDesc, nullptr, 0, ignoreGlobalSPIRVOffsets, false);

        m_DescriptorSetLayouts.push_back(descriptorSetLayout);
        setNum = std::max(setNum, descriptorSetDesc.registerSpace);

        // Binding info
        m_BindingInfo.sets[i].ranges = m_BindingInfo.ranges.data() + m_BindingInfo.ranges.size();
        m_BindingInfo.ranges.insert(m_BindingInfo.ranges.end(), descriptorSetDesc.ranges, descriptorSetDesc.ranges + descriptorSetDesc.rangeNum);

        DescriptorRangeDesc* ranges = (DescriptorRangeDesc*)m_BindingInfo.sets[i].ranges;
        for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++)
            ranges[j].baseRegisterIndex += bindingOffsets[(uint32_t)descriptorSetDesc.ranges[j].descriptorType];
    }

    // Root constants
    Scratch<VkPushConstantRange> pushConstantRanges = AllocateScratch(m_Device, VkPushConstantRange, pipelineLayoutDesc.rootConstantNum);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++) {
        const RootConstantDesc& pushConstantDesc = pipelineLayoutDesc.rootConstants[i];

        VkPushConstantRange& range = pushConstantRanges[i];
        range = {};
        range.stageFlags = GetShaderStageFlags(pushConstantDesc.shaderStages);
        range.offset = offset;
        range.size = pushConstantDesc.size;

        offset += pushConstantDesc.size;

        // Binding info
        m_BindingInfo.pushConstants.push_back({range.stageFlags, range.offset});
    }

    // Root descriptors & samplers
    bool hasRootSet = pipelineLayoutDesc.rootDescriptorNum || pipelineLayoutDesc.rootSamplerNum;
    if (hasRootSet) {
        Scratch<DescriptorRangeDesc> rootRanges = AllocateScratch(m_Device, DescriptorRangeDesc, pipelineLayoutDesc.rootDescriptorNum);

        DescriptorSetDesc rootSet = {};
        rootSet.ranges = rootRanges;
        rootSet.registerSpace = pipelineLayoutDesc.rootRegisterSpace;
        rootSet.rangeNum = pipelineLayoutDesc.rootDescriptorNum;

        // Root descriptors
        for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorNum; i++) {
            const RootDescriptorDesc& rootDescriptorDesc = pipelineLayoutDesc.rootDescriptors[i];
            DescriptorRangeDesc& range = rootRanges[i];

            range = {};
            range.baseRegisterIndex = rootDescriptorDesc.registerIndex;
            range.descriptorNum = 1;
            range.descriptorType = rootDescriptorDesc.descriptorType;
            range.shaderStages = rootDescriptorDesc.shaderStages;

            // Binding info
            uint32_t registerIndex = rootDescriptorDesc.registerIndex + bindingOffsets[(uint32_t)rootDescriptorDesc.descriptorType];
            m_BindingInfo.pushDescriptors.push_back(registerIndex);
        }

        // Root samplers
        for (uint32_t i = 0; i < pipelineLayoutDesc.rootSamplerNum; i++) {
            const RootSamplerDesc& rootSamplerDesc = pipelineLayoutDesc.rootSamplers[i];

            // Binding info
            uint32_t registerIndex = rootSamplerDesc.registerIndex + bindingOffsets[(uint32_t)DescriptorType::SAMPLER];
            m_BindingInfo.pushDescriptors.push_back(registerIndex);
        }

        // Create "push" set layout
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        CreateSetLayout(&descriptorSetLayout, rootSet, pipelineLayoutDesc.rootSamplers, pipelineLayoutDesc.rootSamplerNum, ignoreGlobalSPIRVOffsets, true);

        m_DescriptorSetLayouts.push_back(descriptorSetLayout);
        setNum = std::max(setNum, pipelineLayoutDesc.rootRegisterSpace);
    }

    // Allocate temp memory for ALL "register spaces" making the entire range consecutive (thanks to VK API!)
    setNum++;
    Scratch<VkDescriptorSetLayout> descriptorSetLayouts = AllocateScratch(m_Device, VkDescriptorSetLayout, setNum);

    bool hasGaps = setNum > (pipelineLayoutDesc.descriptorSetNum + (hasRootSet ? 1 : 0));
    if (hasGaps) {
        // Create a "dummy" set layout
        VkDescriptorSetLayout dummyDescriptorSetLayout = VK_NULL_HANDLE;
        CreateSetLayout(&dummyDescriptorSetLayout, {}, nullptr, 0, ignoreGlobalSPIRVOffsets, false);

        m_DescriptorSetLayouts.push_back(dummyDescriptorSetLayout);

        for (uint32_t i = 0; i < setNum; i++)
            descriptorSetLayouts[i] = dummyDescriptorSetLayout;
    }

    // Populate descriptor set layouts in "register space" order
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        uint32_t setIndex = pipelineLayoutDesc.descriptorSets[i].registerSpace;
        descriptorSetLayouts[setIndex] = m_DescriptorSetLayouts[i];
    }

    if (hasRootSet) {
        uint32_t setIndex = pipelineLayoutDesc.rootRegisterSpace;
        descriptorSetLayouts[setIndex] = m_DescriptorSetLayouts[pipelineLayoutDesc.descriptorSetNum];
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.setLayoutCount = setNum;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = pipelineLayoutDesc.rootConstantNum;
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreatePipelineLayout");

    return Result::SUCCESS;
}

void PipelineLayoutVK::CreateSetLayout(VkDescriptorSetLayout* setLayout, const DescriptorSetDesc& descriptorSetDesc, const RootSamplerDesc* rootSamplers, uint32_t rootSamplerNum, bool ignoreGlobalSPIRVOffsets, bool isPush) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    // Binding offsets
    VKBindingOffsets vkBindingOffsets = {};
    if (!ignoreGlobalSPIRVOffsets)
        vkBindingOffsets = m_Device.GetBindingOffsets();

    std::array<uint32_t, (size_t)DescriptorType::MAX_NUM> bindingOffsets = {};
    bindingOffsets[(size_t)DescriptorType::SAMPLER] = vkBindingOffsets.samplerOffset;
    bindingOffsets[(size_t)DescriptorType::CONSTANT_BUFFER] = vkBindingOffsets.constantBufferOffset;
    bindingOffsets[(size_t)DescriptorType::TEXTURE] = vkBindingOffsets.textureOffset;
    bindingOffsets[(size_t)DescriptorType::STORAGE_TEXTURE] = vkBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(size_t)DescriptorType::BUFFER] = vkBindingOffsets.textureOffset;
    bindingOffsets[(size_t)DescriptorType::STORAGE_BUFFER] = vkBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(size_t)DescriptorType::STRUCTURED_BUFFER] = vkBindingOffsets.textureOffset;
    bindingOffsets[(size_t)DescriptorType::STORAGE_STRUCTURED_BUFFER] = vkBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(size_t)DescriptorType::ACCELERATION_STRUCTURE] = vkBindingOffsets.textureOffset;

    // Count
    uint32_t bindingMaxNum = rootSamplerNum;
    for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
        const DescriptorRangeDesc& range = descriptorSetDesc.ranges[i];
        bool isArray = range.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        bindingMaxNum += isArray ? 1 : range.descriptorNum;
    }

    // Allocate scratch
    Scratch<VkDescriptorSetLayoutBinding> bindings = AllocateScratch(m_Device, VkDescriptorSetLayoutBinding, bindingMaxNum);
    Scratch<VkDescriptorBindingFlags> bindingFlags = AllocateScratch(m_Device, VkDescriptorBindingFlags, bindingMaxNum);
    Scratch<VkMutableDescriptorTypeListEXT> mutableTypeLists = AllocateScratch(m_Device, VkMutableDescriptorTypeListEXT, bindingMaxNum);
    Scratch<VkSampler> immutableSamplers = AllocateScratch(m_Device, VkSampler, rootSamplerNum);
    uint32_t bindingNum = 0;

    // Add ranges
    const VkDescriptorType mutableTypes[] = {
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    };
    const VkMutableDescriptorTypeListEXT mutableTypeList = {deviceDesc.features.rayTracing ? 7u : 6u, mutableTypes};

    for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
        const DescriptorRangeDesc& range = descriptorSetDesc.ranges[i];
        uint32_t baseBindingIndex = range.baseRegisterIndex + bindingOffsets[(uint32_t)range.descriptorType];

        VkDescriptorBindingFlags flags = 0;
        if (range.flags & DescriptorRangeBits::PARTIALLY_BOUND)
            flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        if (range.flags & DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET)
            flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        uint32_t descriptorNum = 1;
        bool isArray = range.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);

        if (isArray) {
            if (range.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY)
                flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
        } else
            descriptorNum = range.descriptorNum;

        for (uint32_t j = 0; j < descriptorNum; j++) {
            VkDescriptorSetLayoutBinding& binding = bindings[bindingNum];
            binding = {};
            binding.binding = baseBindingIndex + j;
            binding.descriptorCount = isArray ? range.descriptorNum : 1;
            binding.stageFlags = GetShaderStageFlags(range.shaderStages);

            bindingFlags[bindingNum] = flags;

            if (range.flags & DescriptorRangeBits::MUTABLE) {
                binding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
                mutableTypeLists[bindingNum] = mutableTypeList;
            } else {
                binding.descriptorType = GetDescriptorType(range.descriptorType);
                mutableTypeLists[bindingNum] = {};
            }

            bindingNum++;
        }
    }

    // Add root samplers
    for (uint32_t i = 0; i < rootSamplerNum; i++) {
        const RootSamplerDesc& rootSamplerDesc = rootSamplers[i];

        VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        VkSamplerReductionModeCreateInfo reductionModeInfo = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO};
        VkSamplerCustomBorderColorCreateInfoEXT borderColorInfo = {VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT};
        m_Device.FillCreateInfo(rootSamplerDesc.desc, info, reductionModeInfo, borderColorInfo);

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.CreateSampler(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &immutableSamplers[i]);
        RETURN_VOID_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateSampler");

        m_ImmutableSamplers.push_back(immutableSamplers[i]);

        VkDescriptorSetLayoutBinding& binding = bindings[bindingNum];
        binding = {};
        binding.binding = rootSamplerDesc.registerIndex + bindingOffsets[(uint32_t)DescriptorType::SAMPLER];
        binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = GetShaderStageFlags(rootSamplerDesc.shaderStages);
        binding.pImmutableSamplers = &immutableSamplers[i];

        bindingFlags[bindingNum] = 0;
        mutableTypeLists[bindingNum] = {};

        bindingNum++;
    }

    // Create layout
    VkMutableDescriptorTypeCreateInfoEXT mutableTypeInfo = {VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT};
    mutableTypeInfo.mutableDescriptorTypeListCount = bindingNum;
    mutableTypeInfo.pMutableDescriptorTypeLists = mutableTypeLists;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    bindingFlagsInfo.pNext = deviceDesc.features.mutableDescriptorType ? &mutableTypeInfo : nullptr;
    bindingFlagsInfo.bindingCount = bindingNum;
    bindingFlagsInfo.pBindingFlags = bindingFlags;

    VkDescriptorSetLayoutCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = deviceDesc.tiers.bindless != 0 ? &bindingFlagsInfo : nullptr;
    info.bindingCount = bindingNum;
    info.pBindings = bindings;
    info.flags = (descriptorSetDesc.flags & DescriptorSetBits::ALLOW_UPDATE_AFTER_SET) ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;

    if (isPush)
        info.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateDescriptorSetLayout(m_Device, &info, m_Device.GetVkAllocationCallbacks(), setLayout);
    RETURN_VOID_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateDescriptorSetLayout");
}

NRI_INLINE void PipelineLayoutVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)m_Handle, name);
}
