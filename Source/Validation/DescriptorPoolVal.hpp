// © 2021 NVIDIA Corporation

NRI_INLINE void DescriptorPoolVal::Reset() {
    m_DescriptorSetsNum = 0;
    m_SamplerNum = 0;
    m_ConstantBufferNum = 0;
    m_TextureNum = 0;
    m_StorageTextureNum = 0;
    m_BufferNum = 0;
    m_StorageBufferNum = 0;
    m_StructuredBufferNum = 0;
    m_StorageStructuredBufferNum = 0;
    m_AccelerationStructureNum = 0;

    GetCoreInterfaceImpl().ResetDescriptorPool(*GetImpl());
}

NRI_INLINE Result DescriptorPoolVal::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    RETURN_ON_FAILURE(&m_Device, instanceNum != 0, Result::INVALID_ARGUMENT, "'instanceNum' is 0");
    RETURN_ON_FAILURE(&m_Device, m_DescriptorSetsNum + instanceNum <= m_Desc.descriptorSetMaxNum, Result::INVALID_ARGUMENT, "the maximum number of descriptor sets exceeded");

    const PipelineLayoutVal& pipelineLayoutVal = (PipelineLayoutVal&)pipelineLayout;
    const PipelineLayoutDesc& pipelineLayoutDesc = pipelineLayoutVal.GetPipelineLayoutDesc();
    RETURN_ON_FAILURE(&m_Device, m_SkipValidation || setIndex < pipelineLayoutDesc.descriptorSetNum, Result::INVALID_ARGUMENT, "'setIndex' is invalid");

    const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[setIndex];
    if (!m_SkipValidation) {
        for (uint32_t i = 0; i < instanceNum; i++) {
            for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
                const DescriptorRangeDesc& rangeDesc = descriptorSetDesc.ranges[j];
                RETURN_ON_FAILURE(&m_Device, (uint32_t)rangeDesc.descriptorType < (uint32_t)DescriptorType::MAX_NUM, Result::INVALID_ARGUMENT, "Invalid DescriptorType=%u", (uint32_t)rangeDesc.descriptorType);

                uint32_t descriptorNum = (rangeDesc.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY) ? variableDescriptorNum : rangeDesc.descriptorNum;
                RETURN_ON_FAILURE(&m_Device, descriptorNum <= rangeDesc.descriptorNum, Result::INVALID_ARGUMENT, "'variableDescriptorNum=%u' is greater than 'descriptorNum=%u'", variableDescriptorNum, rangeDesc.descriptorNum);

                bool enoughDescriptors = false;
                switch (rangeDesc.descriptorType) {
                    case DescriptorType::SAMPLER:
                        m_SamplerNum += descriptorNum;
                        enoughDescriptors = m_SamplerNum <= m_Desc.samplerMaxNum;
                        break;
                    case DescriptorType::CONSTANT_BUFFER:
                        m_ConstantBufferNum += descriptorNum;
                        enoughDescriptors = m_ConstantBufferNum <= m_Desc.constantBufferMaxNum;
                        break;
                    case DescriptorType::TEXTURE:
                        m_TextureNum += descriptorNum;
                        enoughDescriptors = m_TextureNum <= m_Desc.textureMaxNum;
                        break;
                    case DescriptorType::STORAGE_TEXTURE:
                        m_StorageTextureNum += descriptorNum;
                        enoughDescriptors = m_StorageTextureNum <= m_Desc.storageTextureMaxNum;
                        break;
                    case DescriptorType::BUFFER:
                        m_BufferNum += descriptorNum;
                        enoughDescriptors = m_BufferNum <= m_Desc.bufferMaxNum;
                        break;
                    case DescriptorType::STORAGE_BUFFER:
                        m_StorageBufferNum += descriptorNum;
                        enoughDescriptors = m_StorageBufferNum <= m_Desc.storageBufferMaxNum;
                        break;
                    case DescriptorType::STRUCTURED_BUFFER:
                        m_StructuredBufferNum += descriptorNum;
                        enoughDescriptors = m_StructuredBufferNum <= m_Desc.structuredBufferMaxNum;
                        break;
                    case DescriptorType::STORAGE_STRUCTURED_BUFFER:
                        m_StorageStructuredBufferNum += descriptorNum;
                        enoughDescriptors = m_StorageStructuredBufferNum <= m_Desc.storageStructuredBufferMaxNum;
                        break;
                    case DescriptorType::ACCELERATION_STRUCTURE:
                        m_AccelerationStructureNum += descriptorNum;
                        enoughDescriptors = m_AccelerationStructureNum <= m_Desc.accelerationStructureMaxNum;
                        break;
                    default:
                        CHECK(false, "Unexpected");
                        break;
                }

                RETURN_ON_FAILURE(&m_Device, enoughDescriptors, Result::INVALID_ARGUMENT, "the maximum number of '%s' descriptors in DescriptorPool exceeded at DescriptorSet instance #%u", GetDescriptorTypeName(rangeDesc.descriptorType), i);
            }
        }
    }

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    Result result = GetCoreInterfaceImpl().AllocateDescriptorSets(*GetImpl(), *pipelineLayoutImpl, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
    if (result != Result::SUCCESS)
        return result;

    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetVal* descriptorSetVal = &m_DescriptorSets[m_DescriptorSetsNum++];
        descriptorSetVal->SetImpl(descriptorSets[i], &descriptorSetDesc);
        descriptorSets[i] = (DescriptorSet*)descriptorSetVal;
    }

    return result;
}
