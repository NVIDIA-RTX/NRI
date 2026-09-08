// © 2026 NVIDIA Corporation

DescriptorPoolWGPU::~DescriptorPoolWGPU() {
    Reset();
}

Result DescriptorPoolWGPU::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    m_Desc = descriptorPoolDesc;

    return Result::SUCCESS;
}

Result DescriptorPoolWGPU::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    ExclusiveScope lock(m_Lock);

    // TODO: Variable descriptor count is ignored because descriptor heap indexing/bindless is not exposed by this backend.
    MaybeUnused(variableDescriptorNum);

    const DescriptorSetMappingWGPU& mapping = ((PipelineLayoutWGPU&)pipelineLayout).GetDescriptorSetMapping(setIndex);
    const bool isCopySource = (m_Desc.flags & DescriptorPoolBits::COPY_SOURCE) != 0;
    const size_t descriptorSetOffset = m_DescriptorSets.size();

    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetWGPU* descriptorSet = Allocate<DescriptorSetWGPU>(m_Device.GetAllocationCallbacks(), m_Device, mapping, isCopySource);
        if (!descriptorSet) {
            while (m_DescriptorSets.size() > descriptorSetOffset) {
                Destroy(m_Device.GetAllocationCallbacks(), m_DescriptorSets.back());
                m_DescriptorSets.pop_back();
            }

            for (uint32_t j = 0; j < i; j++)
                descriptorSets[j] = nullptr;

            return Result::FAILURE;
        }

        m_DescriptorSets.push_back(descriptorSet);
        descriptorSets[i] = (DescriptorSet*)descriptorSet;
    }

    return Result::SUCCESS;
}

void DescriptorPoolWGPU::Reset() {
    ExclusiveScope lock(m_Lock);

    for (DescriptorSetWGPU* descriptorSet : m_DescriptorSets)
        Destroy(m_Device.GetAllocationCallbacks(), descriptorSet);

    m_DescriptorSets.clear();
}
