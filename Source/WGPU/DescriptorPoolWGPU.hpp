// © 2026 NVIDIA Corporation

DescriptorPoolWGPU::~DescriptorPoolWGPU() {
    Reset();
}

Result DescriptorPoolWGPU::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    m_Desc = descriptorPoolDesc;

    return Result::SUCCESS;
}

Result DescriptorPoolWGPU::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    MaybeUnused(variableDescriptorNum);

    const DescriptorSetMappingWGPU& mapping = ((PipelineLayoutWGPU&)pipelineLayout).GetDescriptorSetMapping(setIndex);

    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetWGPU* descriptorSet = Allocate<DescriptorSetWGPU>(m_Device.GetAllocationCallbacks(), m_Device, mapping);
        if (!descriptorSet)
            return Result::FAILURE;

        m_DescriptorSets.push_back(descriptorSet);
        descriptorSets[i] = (DescriptorSet*)descriptorSet;
    }

    return Result::SUCCESS;
}

void DescriptorPoolWGPU::Reset() {
    for (DescriptorSetWGPU* descriptorSet : m_DescriptorSets)
        Destroy(m_Device.GetAllocationCallbacks(), descriptorSet);

    m_DescriptorSets.clear();
}
