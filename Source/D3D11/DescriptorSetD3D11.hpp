// © 2021 NVIDIA Corporation

void DescriptorSetD3D11::Create(const PipelineLayoutD3D11* pipelineLayout, const BindingSet* bindingSet, const DescriptorD3D11** descriptors) {
    m_PipelineLayout = pipelineLayout;
    m_BindingSet = bindingSet;
    m_Descriptors = descriptors;
}

NRI_INLINE void DescriptorSetD3D11::UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum) {
    for (uint32_t i = 0; i < updateDescriptorRangeDescNum; i++) {
        const UpdateDescriptorRangeDesc& updateDescriptorRangeDesc = updateDescriptorRangeDescs[i];
        const DescriptorSetD3D11& dst = *(DescriptorSetD3D11*)updateDescriptorRangeDesc.descriptorSet;

        uint32_t rangeIndex = dst.m_BindingSet->startRange + updateDescriptorRangeDesc.rangeIndex;
        
        const BindingRange& dstRange = dst.m_PipelineLayout->GetBindingRange(rangeIndex);
        uint32_t descriptorOffset = dstRange.descriptorOffset + updateDescriptorRangeDesc.baseDescriptor;

        const DescriptorD3D11** dstDescriptors = dst.m_Descriptors + descriptorOffset;
        const DescriptorD3D11** srcDescriptors = (const DescriptorD3D11**)updateDescriptorRangeDesc.descriptors;

        memcpy(dstDescriptors, srcDescriptors, updateDescriptorRangeDesc.descriptorNum * sizeof(DescriptorD3D11*));
    }
}

NRI_INLINE void DescriptorSetD3D11::Copy(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum) {
    for (uint32_t i = 0; i < copyDescriptorRangeDescNum; i++) {
        const CopyDescriptorRangeDesc& copyDescriptorSetDesc = copyDescriptorRangeDescs[i];

        DescriptorSetD3D11& dst = *(DescriptorSetD3D11*)copyDescriptorSetDesc.dstDescriptorSet;
        DescriptorSetD3D11& src = *(DescriptorSetD3D11*)copyDescriptorSetDesc.srcDescriptorSet;

        uint32_t dstRangeIndex = dst.m_BindingSet->startRange + copyDescriptorSetDesc.dstRangeIndex;
        uint32_t srcRangeIndex = src.m_BindingSet->startRange + copyDescriptorSetDesc.srcRangeIndex;

        const BindingRange& dstRange = dst.m_PipelineLayout->GetBindingRange(dstRangeIndex);
        const BindingRange& srcRange = src.m_PipelineLayout->GetBindingRange(srcRangeIndex);

        const DescriptorD3D11** dstDescriptors = dst.m_Descriptors + dstRange.descriptorOffset + copyDescriptorSetDesc.dstBaseDescriptor;
        const DescriptorD3D11** srcDescriptors = src.m_Descriptors + srcRange.descriptorOffset + copyDescriptorSetDesc.srcBaseDescriptor;

        uint32_t descriptorNum = copyDescriptorSetDesc.descriptorNum;
        if (!descriptorNum)
            descriptorNum = srcRange.descriptorNum;

        memcpy(dstDescriptors, srcDescriptors, descriptorNum * sizeof(DescriptorD3D11*));
    }
}
