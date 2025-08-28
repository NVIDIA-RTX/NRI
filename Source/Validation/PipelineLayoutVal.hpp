// © 2021 NVIDIA Corporation

PipelineLayoutVal::PipelineLayoutVal(DeviceVal& device, PipelineLayout* pipelineLayout, const PipelineLayoutDesc& pipelineLayoutDesc)
    : ObjectVal(device, pipelineLayout)
    , m_DescriptorSetDescs(device.GetStdAllocator())
    , m_RootConstantDescs(device.GetStdAllocator())
    , m_DescriptorRangeDescs(device.GetStdAllocator()) {
    uint32_t descriptorRangeDescNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++)
        descriptorRangeDescNum += pipelineLayoutDesc.descriptorSets[i].rangeNum;

    m_DescriptorSetDescs.insert(m_DescriptorSetDescs.begin(), pipelineLayoutDesc.descriptorSets, pipelineLayoutDesc.descriptorSets + pipelineLayoutDesc.descriptorSetNum);
    m_RootConstantDescs.insert(m_RootConstantDescs.begin(), pipelineLayoutDesc.rootConstants, pipelineLayoutDesc.rootConstants + pipelineLayoutDesc.rootConstantNum);

    m_DescriptorRangeDescs.reserve(descriptorRangeDescNum);
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        m_DescriptorSetDescs[i].ranges = m_DescriptorRangeDescs.data() + m_DescriptorRangeDescs.size();

        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];
        m_DescriptorRangeDescs.insert(m_DescriptorRangeDescs.end(), descriptorSetDesc.ranges, descriptorSetDesc.ranges + descriptorSetDesc.rangeNum);
    }

    m_PipelineLayoutDesc = pipelineLayoutDesc;
    m_PipelineLayoutDesc.descriptorSets = m_DescriptorSetDescs.data();
    m_PipelineLayoutDesc.rootConstants = m_RootConstantDescs.data();
}
