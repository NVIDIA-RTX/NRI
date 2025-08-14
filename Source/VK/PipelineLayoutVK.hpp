// © 2021 NVIDIA Corporation

BindingInfo::BindingInfo(StdAllocator<uint8_t>& allocator)
    : hasVariableDescriptorNum(allocator)
    , descriptorSetRangeDescs(allocator)
    , dynamicConstantBufferDescs(allocator)
    , descriptorSetDescs(allocator)
    , pushConstantBindings(allocator)
    , pushDescriptorBindings(allocator) {
}

PipelineLayoutVK::~PipelineLayoutVK() {
    const auto& vk = m_Device.GetDispatchTable();
    const auto allocationCallbacks = m_Device.GetVkAllocationCallbacks();

    if (m_Handle)
        vk.DestroyPipelineLayout(m_Device, m_Handle, allocationCallbacks);

    for (auto& handle : m_DescriptorSetLayouts)
        vk.DestroyDescriptorSetLayout(m_Device, handle, allocationCallbacks);
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
    size_t dynamicConstantBufferNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        rangeNum += pipelineLayoutDesc.descriptorSets[i].rangeNum;
        dynamicConstantBufferNum += pipelineLayoutDesc.descriptorSets[i].dynamicConstantBufferNum;
    }

    m_BindingInfo.descriptorSetDescs.insert(m_BindingInfo.descriptorSetDescs.begin(), pipelineLayoutDesc.descriptorSets, pipelineLayoutDesc.descriptorSets + pipelineLayoutDesc.descriptorSetNum);
    m_BindingInfo.hasVariableDescriptorNum.resize(pipelineLayoutDesc.descriptorSetNum);
    m_BindingInfo.descriptorSetRangeDescs.reserve(rangeNum);
    m_BindingInfo.dynamicConstantBufferDescs.reserve(dynamicConstantBufferNum);

    // Descriptor sets
    uint32_t setNum = 0;

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];

        setNum = std::max(setNum, descriptorSetDesc.registerSpace);

        // Create set layout
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        CreateSetLayout(&descriptorSetLayout, descriptorSetDesc, ignoreGlobalSPIRVOffsets, false); // non-push

        m_DescriptorSetLayouts.push_back(descriptorSetLayout);

        // Binding info
        m_BindingInfo.hasVariableDescriptorNum[i] = false;
        m_BindingInfo.descriptorSetDescs[i].ranges = m_BindingInfo.descriptorSetRangeDescs.data() + m_BindingInfo.descriptorSetRangeDescs.size();
        m_BindingInfo.descriptorSetDescs[i].dynamicConstantBuffers = m_BindingInfo.dynamicConstantBufferDescs.data() + m_BindingInfo.dynamicConstantBufferDescs.size();
        m_BindingInfo.descriptorSetRangeDescs.insert(m_BindingInfo.descriptorSetRangeDescs.end(), descriptorSetDesc.ranges, descriptorSetDesc.ranges + descriptorSetDesc.rangeNum);
        m_BindingInfo.dynamicConstantBufferDescs.insert(m_BindingInfo.dynamicConstantBufferDescs.end(), descriptorSetDesc.dynamicConstantBuffers, descriptorSetDesc.dynamicConstantBuffers + descriptorSetDesc.dynamicConstantBufferNum);

        DescriptorRangeDesc* ranges = (DescriptorRangeDesc*)m_BindingInfo.descriptorSetDescs[i].ranges;
        for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
            ranges[j].baseRegisterIndex += bindingOffsets[(uint32_t)descriptorSetDesc.ranges[j].descriptorType];

            if (m_Device.m_IsSupported.descriptorIndexing && (descriptorSetDesc.ranges[j].flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY))
                m_BindingInfo.hasVariableDescriptorNum[i] = true;
        }

        DynamicConstantBufferDesc* dynamicConstantBuffers = (DynamicConstantBufferDesc*)m_BindingInfo.descriptorSetDescs[i].dynamicConstantBuffers;
        for (uint32_t j = 0; j < descriptorSetDesc.dynamicConstantBufferNum; j++)
            dynamicConstantBuffers[j].registerIndex += bindingOffsets[(uint32_t)DescriptorType::CONSTANT_BUFFER];
    }

    // Root descriptors
    m_BindingInfo.pushDescriptorBindings.resize(pipelineLayoutDesc.rootDescriptorNum);

    if (pipelineLayoutDesc.rootDescriptorNum) {
        Scratch<DescriptorRangeDesc> rootRanges = AllocateScratch(m_Device, DescriptorRangeDesc, pipelineLayoutDesc.rootDescriptorNum);

        DescriptorSetDesc rootSet = {};
        rootSet.ranges = rootRanges;
        rootSet.registerSpace = pipelineLayoutDesc.rootRegisterSpace;
        rootSet.rangeNum = pipelineLayoutDesc.rootDescriptorNum;

        setNum = std::max(setNum, rootSet.registerSpace);

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
            m_BindingInfo.pushDescriptorBindings[i] = {rootSet.registerSpace, registerIndex};
        }

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        CreateSetLayout(&descriptorSetLayout, rootSet, ignoreGlobalSPIRVOffsets, true); // push

        m_DescriptorSetLayouts.push_back(descriptorSetLayout);
    }

    // Allocate temp memory for ALL "register spaces" making the entire range consecutive (thanks VK API!)
    setNum++;
    Scratch<VkDescriptorSetLayout> descriptorSetLayouts = AllocateScratch(m_Device, VkDescriptorSetLayout, setNum);

    bool hasGaps = setNum > pipelineLayoutDesc.descriptorSetNum + (pipelineLayoutDesc.rootDescriptorNum ? 1 : 0);
    if (hasGaps) {
        // Create a "dummy" set layout (needed only if "register space" indices are not consecutive)
        VkDescriptorSetLayout dummyDescriptorSetLayout = VK_NULL_HANDLE;
        CreateSetLayout(&dummyDescriptorSetLayout, {}, ignoreGlobalSPIRVOffsets, false); // non-push

        m_DescriptorSetLayouts.push_back(dummyDescriptorSetLayout);

        for (uint32_t i = 0; i < setNum; i++)
            descriptorSetLayouts[i] = dummyDescriptorSetLayout;
    }

    // Populate descriptor set layouts in "register space" order
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        uint32_t setIndex = pipelineLayoutDesc.descriptorSets[i].registerSpace;
        descriptorSetLayouts[setIndex] = m_DescriptorSetLayouts[i];
    }

    if (pipelineLayoutDesc.rootDescriptorNum) {
        uint32_t setIndex = pipelineLayoutDesc.rootRegisterSpace;
        descriptorSetLayouts[setIndex] = m_DescriptorSetLayouts[pipelineLayoutDesc.descriptorSetNum];
    }

    // Root constants
    m_BindingInfo.pushConstantBindings.resize(pipelineLayoutDesc.rootConstantNum);
    Scratch<VkPushConstantRange> pushConstantRanges = AllocateScratch(m_Device, VkPushConstantRange, pipelineLayoutDesc.rootConstantNum);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++) {
        const RootConstantDesc& pushConstantDesc = pipelineLayoutDesc.rootConstants[i];

        VkPushConstantRange& range = pushConstantRanges[i];
        range = {};
        range.stageFlags = GetShaderStageFlags(pushConstantDesc.shaderStages);
        range.offset = offset;
        range.size = pushConstantDesc.size;

        // Binding info
        m_BindingInfo.pushConstantBindings[i] = {range.stageFlags, range.offset};

        offset += pushConstantDesc.size;
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

void PipelineLayoutVK::CreateSetLayout(VkDescriptorSetLayout* setLayout, const DescriptorSetDesc& descriptorSetDesc, bool ignoreGlobalSPIRVOffsets, bool isPush) {
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
    uint32_t bindingMaxNum = descriptorSetDesc.dynamicConstantBufferNum;
    for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
        const DescriptorRangeDesc& range = descriptorSetDesc.ranges[i];
        bool isArray = range.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        bindingMaxNum += isArray ? 1 : range.descriptorNum;
    }

    // Allocate scratch
    Scratch<VkDescriptorSetLayoutBinding> bindingsScratch = AllocateScratch(m_Device, VkDescriptorSetLayoutBinding, bindingMaxNum);
    Scratch<VkDescriptorBindingFlags> bindingFlagsScratch = AllocateScratch(m_Device, VkDescriptorBindingFlags, bindingMaxNum);
    VkDescriptorSetLayoutBinding* bindingsBegin = bindingsScratch;
    VkDescriptorBindingFlags* bindingFlagsBegin = bindingFlagsScratch;

    VkDescriptorSetLayoutBinding* bindings = bindingsScratch;
    VkDescriptorBindingFlags* bindingFlags = bindingFlagsScratch;

    // Descriptor bindings
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
            *bindingFlags++ = flags;

            VkDescriptorSetLayoutBinding& descriptorBinding = *bindings++;
            descriptorBinding = {};
            descriptorBinding.descriptorType = GetDescriptorType(range.descriptorType);
            descriptorBinding.stageFlags = GetShaderStageFlags(range.shaderStages);
            descriptorBinding.binding = baseBindingIndex + j;
            descriptorBinding.descriptorCount = isArray ? range.descriptorNum : 1;
        }
    }

    // Dynamic constant buffer bindings
    for (uint32_t i = 0; i < descriptorSetDesc.dynamicConstantBufferNum; i++) {
        const DynamicConstantBufferDesc& buffer = descriptorSetDesc.dynamicConstantBuffers[i];

        *bindingFlags++ = 0;

        VkDescriptorSetLayoutBinding& descriptorBinding = *bindings++;
        descriptorBinding = {};
        descriptorBinding.binding = buffer.registerIndex + bindingOffsets[(uint32_t)DescriptorType::CONSTANT_BUFFER];
        descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorBinding.descriptorCount = 1;
        descriptorBinding.stageFlags = GetShaderStageFlags(buffer.shaderStages);
    }

    uint32_t bindingNum = uint32_t(bindings - bindingsBegin);

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    bindingFlagsInfo.bindingCount = bindingNum;
    bindingFlagsInfo.pBindingFlags = bindingFlagsBegin;

    VkDescriptorSetLayoutCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = m_Device.m_IsSupported.descriptorIndexing ? &bindingFlagsInfo : nullptr;
    info.bindingCount = bindingNum;
    info.pBindings = bindingsBegin;
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
