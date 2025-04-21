// © 2021 NVIDIA Corporation

DescriptorSetD3D12::~DescriptorSetD3D12() {
    Destroy(m_DescriptorPoolD3D12.GetDevice().GetAllocationCallbacks(), m_DynamicConstantBuffers);
}

DeviceD3D12& DescriptorSetD3D12::GetDevice() const {
    return m_DescriptorPoolD3D12.GetDevice();
}

void DescriptorSetD3D12::Create(const DescriptorSetMapping* descriptorSetMapping, uint16_t dynamicConstantBufferNum) {
    m_DescriptorSetMapping = descriptorSetMapping;

    if (dynamicConstantBufferNum) {
        DeviceD3D12& deviceD3D12 = m_DescriptorPoolD3D12.GetDevice();
        m_DynamicConstantBuffers = Allocate<Vector<DescriptorPointerGPU>>(deviceD3D12.GetAllocationCallbacks(), deviceD3D12.GetStdAllocator());
        m_DynamicConstantBuffers->resize(dynamicConstantBufferNum, 0);
    }

    for (size_t i = 0; i < m_HeapOffset.size(); i++) {
        uint32_t descriptorNum = m_DescriptorSetMapping->descriptorNum[i];

        if (descriptorNum)
            m_HeapOffset[i] = m_DescriptorPoolD3D12.AllocateDescriptors((DescriptorHeapType)i, descriptorNum);
        else
            m_HeapOffset[i] = 0;
    }
}

DescriptorPointerCPU DescriptorSetD3D12::GetPointerCPU(uint32_t rangeIndex, uint32_t rangeOffset) const {
    const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex];

    DescriptorHeapType descriptorHeapType = rangeMapping.descriptorHeapType;
    uint32_t heapOffset = m_HeapOffset[descriptorHeapType];
    uint32_t offset = rangeMapping.heapOffset + heapOffset + rangeOffset;
    DescriptorPointerCPU descriptorPointerCPU = m_DescriptorPoolD3D12.GetDescriptorPointerCPU(descriptorHeapType, offset);

    return descriptorPointerCPU;
}

DescriptorPointerGPU DescriptorSetD3D12::GetPointerGPU(uint32_t rangeIndex, uint32_t rangeOffset) const {
    const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex];

    DescriptorHeapType descriptorHeapType = rangeMapping.descriptorHeapType;
    uint32_t heapOffset = m_HeapOffset[descriptorHeapType];
    uint32_t offset = rangeMapping.heapOffset + heapOffset + rangeOffset;
    DescriptorPointerGPU descriptorPointerGPU = m_DescriptorPoolD3D12.GetDescriptorPointerGPU(descriptorHeapType, offset);

    return descriptorPointerGPU;
}

DescriptorPointerGPU DescriptorSetD3D12::GetDynamicPointerGPU(uint32_t dynamicConstantBufferIndex) const {
    return (*m_DynamicConstantBuffers)[dynamicConstantBufferIndex];
}

NRI_INLINE void DescriptorSetD3D12::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeOffset + i];

        uint32_t heapOffset = m_HeapOffset[rangeMapping.descriptorHeapType];
        uint32_t baseOffset = rangeMapping.heapOffset + heapOffset + rangeUpdateDescs[i].baseDescriptor;

        for (uint32_t j = 0; j < rangeUpdateDescs[i].descriptorNum; j++) {
            DescriptorPointerCPU dstPointer = m_DescriptorPoolD3D12.GetDescriptorPointerCPU(rangeMapping.descriptorHeapType, baseOffset + j);
            DescriptorPointerCPU srcPointer = ((DescriptorD3D12*)rangeUpdateDescs[i].descriptors[j])->GetPointerCPU();

            m_DescriptorPoolD3D12.GetDevice()->CopyDescriptorsSimple(1, {dstPointer}, {srcPointer}, (D3D12_DESCRIPTOR_HEAP_TYPE)rangeMapping.descriptorHeapType);
        }
    }
}

NRI_INLINE void DescriptorSetD3D12::UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    for (uint32_t i = 0; i < dynamicConstantBufferNum; i++)
        (*m_DynamicConstantBuffers)[baseDynamicConstantBuffer + i] = ((DescriptorD3D12*)descriptors[i])->GetPointerGPU();
}

NRI_INLINE void DescriptorSetD3D12::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    const DescriptorSetD3D12* srcDescriptorSet = (DescriptorSetD3D12*)descriptorSetCopyDesc.srcDescriptorSet;

    for (uint32_t i = 0; i < descriptorSetCopyDesc.rangeNum; i++) {
        const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[i];

        DescriptorPointerCPU dstPointer = GetPointerCPU(descriptorSetCopyDesc.dstBaseRange + i, 0);
        DescriptorPointerCPU srcPointer = srcDescriptorSet->GetPointerCPU(descriptorSetCopyDesc.srcBaseRange + i, 0);

        m_DescriptorPoolD3D12.GetDevice()->CopyDescriptorsSimple(rangeMapping.descriptorNum, {dstPointer}, {srcPointer}, (D3D12_DESCRIPTOR_HEAP_TYPE)rangeMapping.descriptorHeapType);
    }

    for (uint32_t i = 0; i < descriptorSetCopyDesc.dynamicConstantBufferNum; i++) {
        DescriptorPointerGPU descriptorPointerGPU = srcDescriptorSet->GetDynamicPointerGPU(descriptorSetCopyDesc.srcBaseDynamicConstantBuffer + i);
        (*m_DynamicConstantBuffers)[descriptorSetCopyDesc.dstBaseDynamicConstantBuffer + i] = descriptorPointerGPU;
    }
}
