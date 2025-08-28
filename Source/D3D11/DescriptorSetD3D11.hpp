// © 2021 NVIDIA Corporation

void DescriptorSetD3D11::Create(const PipelineLayoutD3D11* pipelineLayout, const BindingSet* bindingSet, const DescriptorD3D11** descriptors) {
    m_PipelineLayout = pipelineLayout;
    m_BindingSet = bindingSet;
    m_Descriptors = descriptors;
}

NRI_INLINE void DescriptorSetD3D11::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    rangeOffset += m_BindingSet->startRange;
    CHECK(rangeOffset + rangeNum <= m_BindingSet->endRange, "Out of bounds");

    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeUpdateDesc& range = rangeUpdateDescs[i];
        uint32_t descriptorOffset = range.baseDescriptor;

        const BindingRange& bindingRange = m_PipelineLayout->GetBindingRange(rangeOffset + i);
        descriptorOffset += bindingRange.descriptorOffset;

        const DescriptorD3D11** dstDescriptors = m_Descriptors + descriptorOffset;
        const DescriptorD3D11** srcDescriptors = (const DescriptorD3D11**)range.descriptors;

        memcpy(dstDescriptors, srcDescriptors, range.descriptorNum * sizeof(DescriptorD3D11*));
    }
}

NRI_INLINE void DescriptorSetD3D11::Copy(const CopyDescriptorSetDesc& copyDescriptorSetDesc) {
    DescriptorSetD3D11& srcSet = (DescriptorSetD3D11&)copyDescriptorSetDesc.srcDescriptorSet;

    uint32_t dstBaseRange = m_BindingSet->startRange + copyDescriptorSetDesc.dstBaseRange;
    uint32_t srcBaseRange = srcSet.m_BindingSet->startRange + copyDescriptorSetDesc.srcBaseRange;
    CHECK(dstBaseRange + copyDescriptorSetDesc.rangeNum <= m_BindingSet->endRange, "Out of bounds");
    CHECK(srcBaseRange + copyDescriptorSetDesc.rangeNum <= srcSet.m_BindingSet->endRange, "Out of bounds");

    for (uint32_t i = 0; i < copyDescriptorSetDesc.rangeNum; i++) {
        const BindingRange& dst = m_PipelineLayout->GetBindingRange(dstBaseRange + i);
        const DescriptorD3D11** dstDescriptors = m_Descriptors + dst.descriptorOffset;

        const BindingRange& src = m_PipelineLayout->GetBindingRange(srcBaseRange + i);
        const DescriptorD3D11** srcDescriptors = srcSet.m_Descriptors + src.descriptorOffset;

        memcpy(dstDescriptors, srcDescriptors, dst.descriptorNum * sizeof(DescriptorD3D11*));
    }
}
