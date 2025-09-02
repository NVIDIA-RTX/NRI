// © 2021 NVIDIA Corporation

DeviceD3D12& DescriptorSetD3D12::GetDevice() const {
    return m_DescriptorPoolD3D12->GetDevice();
}

void DescriptorSetD3D12::Create(DescriptorPoolD3D12* desriptorPoolD3D12, const DescriptorSetMapping* descriptorSetMapping, std::array<uint32_t, DescriptorHeapType::MAX_NUM>& heapOffsets) {
    m_DescriptorPoolD3D12 = desriptorPoolD3D12;
    m_DescriptorSetMapping = descriptorSetMapping;
    m_HeapOffsets = heapOffsets;
}

DescriptorHandleGPU DescriptorSetD3D12::GetDescriptorHandleGPU(uint32_t rangeIndex, uint32_t baseDescriptor) const {
    const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex];

    uint32_t offset = m_HeapOffsets[rangeMapping.descriptorHeapType];
    offset += rangeMapping.heapOffset;
    offset += baseDescriptor;

    DescriptorHandleGPU descriptorHandleGPU = m_DescriptorPoolD3D12->GetDescriptorHandleGPU(rangeMapping.descriptorHeapType, offset);

    return descriptorHandleGPU;
}

NRI_INLINE void DescriptorSetD3D12::UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum) {
    for (uint32_t i = 0; i < updateDescriptorRangeDescNum; i++) {
        const UpdateDescriptorRangeDesc& updateDescriptorRangeDesc = updateDescriptorRangeDescs[i];

        DescriptorSetD3D12& dst = *(DescriptorSetD3D12*)updateDescriptorRangeDesc.descriptorSet;
        
        const DescriptorRangeMapping& dstRangeMapping = dst.m_DescriptorSetMapping->descriptorRangeMappings[updateDescriptorRangeDesc.rangeIndex];

        uint32_t dstOffset = dst.m_HeapOffsets[dstRangeMapping.descriptorHeapType];
        dstOffset += dstRangeMapping.heapOffset;
        dstOffset += updateDescriptorRangeDesc.baseDescriptor;

        for (uint32_t j = 0; j < updateDescriptorRangeDesc.descriptorNum; j++) {
            DescriptorHandleCPU dstHandle = dst.m_DescriptorPoolD3D12->GetDescriptorHandleCPU(dstRangeMapping.descriptorHeapType, dstOffset + j);
            DescriptorHandleCPU srcHandle = ((DescriptorD3D12*)updateDescriptorRangeDesc.descriptors[j])->GetDescriptorHandleCPU();

            dst.GetDevice()->CopyDescriptorsSimple(1, {dstHandle}, {srcHandle}, (D3D12_DESCRIPTOR_HEAP_TYPE)dstRangeMapping.descriptorHeapType);
        }
    }
}

NRI_INLINE void DescriptorSetD3D12::Copy(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum) {
    for (uint32_t i = 0; i < copyDescriptorRangeDescNum; i++) {
        const CopyDescriptorRangeDesc& copyDescriptorSetDesc = copyDescriptorRangeDescs[i];

        DescriptorSetD3D12& dst = *(DescriptorSetD3D12*)copyDescriptorSetDesc.dstDescriptorSet;
        const DescriptorSetD3D12& src = *(DescriptorSetD3D12*)copyDescriptorSetDesc.srcDescriptorSet;

        const DescriptorRangeMapping& dstRangeMapping = dst.m_DescriptorSetMapping->descriptorRangeMappings[copyDescriptorSetDesc.dstRangeIndex];
        const DescriptorRangeMapping& srcRangeMapping = src.m_DescriptorSetMapping->descriptorRangeMappings[copyDescriptorSetDesc.srcRangeIndex];

        uint32_t dstOffset = dst.m_HeapOffsets[dstRangeMapping.descriptorHeapType];
        dstOffset += dstRangeMapping.heapOffset;
        dstOffset += copyDescriptorSetDesc.dstBaseDescriptor;

        uint32_t srcOffset = src.m_HeapOffsets[srcRangeMapping.descriptorHeapType];
        srcOffset += srcRangeMapping.heapOffset;
        srcOffset += copyDescriptorSetDesc.srcBaseDescriptor;

        DescriptorHandleCPU dstHandle = dst.m_DescriptorPoolD3D12->GetDescriptorHandleCPU(dstRangeMapping.descriptorHeapType, dstOffset);
        DescriptorHandleCPU srcHandle = src.m_DescriptorPoolD3D12->GetDescriptorHandleCPU(srcRangeMapping.descriptorHeapType, srcOffset);

        uint32_t descriptorNum = copyDescriptorSetDesc.descriptorNum;
        if (!descriptorNum)
            descriptorNum = srcRangeMapping.descriptorNum;

        dst.GetDevice()->CopyDescriptorsSimple(descriptorNum, {dstHandle}, {srcHandle}, (D3D12_DESCRIPTOR_HEAP_TYPE)dstRangeMapping.descriptorHeapType);
    }
}
