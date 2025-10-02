// © 2021 NVIDIA Corporation

Result DescriptorPoolD3D11::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    uint32_t descriptorNum = descriptorPoolDesc.samplerMaxNum;
    descriptorNum += descriptorPoolDesc.samplerMaxNum;
    descriptorNum += descriptorPoolDesc.constantBufferMaxNum;
    descriptorNum += descriptorPoolDesc.textureMaxNum;
    descriptorNum += descriptorPoolDesc.storageTextureMaxNum;
    descriptorNum += descriptorPoolDesc.bufferMaxNum;
    descriptorNum += descriptorPoolDesc.storageBufferMaxNum;
    descriptorNum += descriptorPoolDesc.structuredBufferMaxNum;
    descriptorNum += descriptorPoolDesc.storageStructuredBufferMaxNum;
    descriptorNum += descriptorPoolDesc.mutableMaxNum;

    m_DescriptorPool.resize(descriptorNum, nullptr);
    m_DescriptorSets.resize(descriptorPoolDesc.descriptorSetMaxNum);

    return Result::SUCCESS;
}

NRI_INLINE Result DescriptorPoolD3D11::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    ExclusiveScope lock(m_Lock);

    if (variableDescriptorNum)
        return Result::UNSUPPORTED;

    if (m_DescriptorNum + instanceNum > m_DescriptorPool.size())
        return Result::OUT_OF_MEMORY;

    const PipelineLayoutD3D11& pipelineLayoutD3D11 = (PipelineLayoutD3D11&)pipelineLayout;
    const BindingSet& bindingSet = pipelineLayoutD3D11.GetBindingSet(setIndex);

    for (uint32_t i = 0; i < instanceNum; i++) {
        const DescriptorD3D11** descriptors = &m_DescriptorPool[m_DescriptorNum];
        m_DescriptorNum += bindingSet.descriptorNum;

        DescriptorSetD3D11* descriptorSet = &m_DescriptorSets[m_DescriptorSetNum++];
        descriptorSet->Create(&pipelineLayoutD3D11, &bindingSet, descriptors);

        descriptorSets[i] = (DescriptorSet*)descriptorSet;
    }

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorPoolD3D11::Reset() {
    ExclusiveScope lock(m_Lock);

    m_DescriptorNum = 0;
    m_DescriptorSetNum = 0;
}
