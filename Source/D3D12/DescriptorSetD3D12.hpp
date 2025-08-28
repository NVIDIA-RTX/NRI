// © 2021 NVIDIA Corporation

DeviceD3D12& DescriptorSetD3D12::GetDevice() const {
    return m_DescriptorPoolD3D12->GetDevice();
}

void DescriptorSetD3D12::Create(DescriptorPoolD3D12* desriptorPoolD3D12, const DescriptorSetMapping* descriptorSetMapping, std::array<uint32_t, DescriptorHeapType::MAX_NUM>& heapOffsets) {
    m_DescriptorPoolD3D12 = desriptorPoolD3D12;
    m_DescriptorSetMapping = descriptorSetMapping;
    m_HeapOffsets = heapOffsets;
}

DescriptorPointerCPU DescriptorSetD3D12::GetDescriptorPointerCPU(uint32_t rangeIndex, uint32_t rangeOffset) const {
    const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex];

    uint32_t heapOffset = m_HeapOffsets[rangeMapping.descriptorHeapType];
    uint32_t offset = rangeMapping.heapOffset + heapOffset + rangeOffset;
    
    DescriptorPointerCPU descriptorPointerCPU = m_DescriptorPoolD3D12->GetDescriptorPointerCPU(rangeMapping.descriptorHeapType, offset);

    return descriptorPointerCPU;
}

DescriptorPointerGPU DescriptorSetD3D12::GetDescriptorPointerGPU(uint32_t rangeIndex, uint32_t rangeOffset) const {
    const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex];

    uint32_t heapOffset = m_HeapOffsets[rangeMapping.descriptorHeapType];
    uint32_t offset = rangeMapping.heapOffset + heapOffset + rangeOffset;

    DescriptorPointerGPU descriptorPointerGPU = m_DescriptorPoolD3D12->GetDescriptorPointerGPU(rangeMapping.descriptorHeapType, offset);

    return descriptorPointerGPU;
}

NRI_INLINE void DescriptorSetD3D12::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeOffset + i];

        uint32_t heapOffset = m_HeapOffsets[rangeMapping.descriptorHeapType];
        uint32_t baseOffset = rangeMapping.heapOffset + heapOffset + rangeUpdateDescs[i].baseDescriptor;

        for (uint32_t j = 0; j < rangeUpdateDescs[i].descriptorNum; j++) {
            DescriptorPointerCPU dstPointer = m_DescriptorPoolD3D12->GetDescriptorPointerCPU(rangeMapping.descriptorHeapType, baseOffset + j);
            DescriptorPointerCPU srcPointer = ((DescriptorD3D12*)rangeUpdateDescs[i].descriptors[j])->GetDescriptorPointerCPU();

            GetDevice()->CopyDescriptorsSimple(1, {dstPointer}, {srcPointer}, (D3D12_DESCRIPTOR_HEAP_TYPE)rangeMapping.descriptorHeapType);
        }
    }
}

NRI_INLINE void DescriptorSetD3D12::Copy(const CopyDescriptorSetDesc& copyDescriptorSetDesc) {
    const DescriptorSetD3D12* srcDescriptorSet = (DescriptorSetD3D12*)copyDescriptorSetDesc.srcDescriptorSet;

    for (uint32_t i = 0; i < copyDescriptorSetDesc.rangeNum; i++) {
        const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[i];

        DescriptorPointerCPU dstPointer = GetDescriptorPointerCPU(copyDescriptorSetDesc.dstBaseRange + i, 0);
        DescriptorPointerCPU srcPointer = srcDescriptorSet->GetDescriptorPointerCPU(copyDescriptorSetDesc.srcBaseRange + i, 0);

        GetDevice()->CopyDescriptorsSimple(rangeMapping.descriptorNum, {dstPointer}, {srcPointer}, (D3D12_DESCRIPTOR_HEAP_TYPE)rangeMapping.descriptorHeapType);
    }
}
