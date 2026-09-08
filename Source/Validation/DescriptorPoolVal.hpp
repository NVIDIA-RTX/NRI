// © 2021 NVIDIA Corporation

static inline uint32_t GetDescriptorMaxNum(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorType descriptorType) {
    switch (descriptorType) {
        case DescriptorType::SAMPLER:
            return descriptorPoolDesc.samplerMaxNum;
        case DescriptorType::MUTABLE:
            return descriptorPoolDesc.mutableMaxNum;
        case DescriptorType::TEXTURE:
            return descriptorPoolDesc.textureMaxNum;
        case DescriptorType::STORAGE_TEXTURE:
            return descriptorPoolDesc.storageTextureMaxNum;
        case DescriptorType::INPUT_ATTACHMENT:
            return descriptorPoolDesc.inputAttachmentMaxNum;
        case DescriptorType::BUFFER:
            return descriptorPoolDesc.bufferMaxNum;
        case DescriptorType::STORAGE_BUFFER:
            return descriptorPoolDesc.storageBufferMaxNum;
        case DescriptorType::CONSTANT_BUFFER:
            return descriptorPoolDesc.constantBufferMaxNum;
        case DescriptorType::STRUCTURED_BUFFER:
            return descriptorPoolDesc.structuredBufferMaxNum;
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            return descriptorPoolDesc.storageStructuredBufferMaxNum;
        case DescriptorType::ACCELERATION_STRUCTURE:
            return descriptorPoolDesc.accelerationStructureMaxNum;
        default:
            return 0;
    }
}

NRI_INLINE void DescriptorPoolVal::Reset() {
    ExclusiveScope lock(m_Lock);

    m_DescriptorSetsNum = 0;
    m_DescriptorNums.fill(0);

    GetCoreInterfaceImpl().ResetDescriptorPool(*GetImpl());
}

NRI_INLINE Result DescriptorPoolVal::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    ExclusiveScope lock(m_Lock);

    NRI_RETURN_ON_FAILURE(&m_Device, instanceNum != 0, Result::INVALID_ARGUMENT, "'instanceNum' is 0");
    NRI_RETURN_ON_FAILURE(&m_Device, m_DescriptorSetsNum <= m_Desc.descriptorSetMaxNum && instanceNum <= m_Desc.descriptorSetMaxNum - m_DescriptorSetsNum, Result::INVALID_ARGUMENT, "exceeded the maximum number of descriptor sets (=%u)", m_Desc.descriptorSetMaxNum);

    const PipelineLayoutVal& pipelineLayoutVal = (PipelineLayoutVal&)pipelineLayout;
    const PipelineLayoutDesc& pipelineLayoutDesc = pipelineLayoutVal.GetPipelineLayoutDesc();
    NRI_RETURN_ON_FAILURE(&m_Device, setIndex < pipelineLayoutDesc.descriptorSetNum, Result::INVALID_ARGUMENT, "'setIndex' is invalid");

    const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[setIndex];
    auto descriptorNums = m_DescriptorNums;

    for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
        const DescriptorRangeDesc& rangeDesc = descriptorSetDesc.ranges[j];
        bool isVariableSized = rangeDesc.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY;

        NRI_RETURN_ON_FAILURE(&m_Device, !isVariableSized || variableDescriptorNum != 0, Result::INVALID_ARGUMENT, "'variableDescriptorNum' is 0");
        NRI_RETURN_ON_FAILURE(&m_Device, !isVariableSized || variableDescriptorNum <= rangeDesc.descriptorNum, Result::INVALID_ARGUMENT, "'variableDescriptorNum=%u' is greater than 'descriptorNum=%u'", variableDescriptorNum, rangeDesc.descriptorNum);
    }

    if (!m_SkipValidation) {
        for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
            const DescriptorRangeDesc& rangeDesc = descriptorSetDesc.ranges[j];
            NRI_RETURN_ON_FAILURE(&m_Device, (uint32_t)rangeDesc.descriptorType < (uint32_t)DescriptorType::MAX_NUM, Result::INVALID_ARGUMENT, "Invalid DescriptorType=%u", (uint32_t)rangeDesc.descriptorType);

            bool isVariableSized = rangeDesc.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY;
            uint32_t descriptorNum = isVariableSized ? variableDescriptorNum : rangeDesc.descriptorNum;
            uint64_t requiredDescriptorNum = uint64_t(descriptorNum) * instanceNum;

            uint32_t descriptorType = (uint32_t)rangeDesc.descriptorType;
            uint32_t descriptorMaxNum = GetDescriptorMaxNum(m_Desc, rangeDesc.descriptorType);
            uint32_t& allocatedDescriptorNum = descriptorNums[descriptorType];

            bool enoughDescriptors = allocatedDescriptorNum <= descriptorMaxNum && requiredDescriptorNum <= descriptorMaxNum - allocatedDescriptorNum;
            NRI_RETURN_ON_FAILURE(&m_Device, enoughDescriptors, Result::INVALID_ARGUMENT, "the maximum number of '%s' descriptors in DescriptorPool exceeded", GetDescriptorTypeName(rangeDesc.descriptorType));

            allocatedDescriptorNum += (uint32_t)requiredDescriptorNum;
        }
    }

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    Result result = GetCoreInterfaceImpl().AllocateDescriptorSets(*GetImpl(), *pipelineLayoutImpl, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
    if (result != Result::SUCCESS)
        return result;

    if (!m_SkipValidation)
        m_DescriptorNums = descriptorNums;

    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetVal* descriptorSetVal = &m_DescriptorSets[m_DescriptorSetsNum++];
        descriptorSetVal->SetImpl(descriptorSets[i], &descriptorSetDesc, variableDescriptorNum, IsCopySource());
        descriptorSets[i] = (DescriptorSet*)descriptorSetVal;
    }

    return result;
}
