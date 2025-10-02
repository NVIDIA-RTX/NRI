// © 2021 NVIDIA Corporation

Result DescriptorPoolD3D12::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> descriptorHeapSize = {};

    descriptorHeapSize[DescriptorHeapType::SAMPLER] += descriptorPoolDesc.samplerMaxNum;

    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.constantBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.textureMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.storageTextureMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.bufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.storageBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.structuredBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.storageStructuredBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.accelerationStructureMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.mutableMaxNum;

    for (uint32_t i = 0; i < DescriptorHeapType::MAX_NUM; i++) {
        DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[i];

        descriptorHeapDesc = {};
        if (descriptorHeapSize[i]) {
            ComPtr<ID3D12DescriptorHeap> descriptorHeap;
            D3D12_DESCRIPTOR_HEAP_DESC desc = {(D3D12_DESCRIPTOR_HEAP_TYPE)i, descriptorHeapSize[i], D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, NODE_MASK};
            HRESULT hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateDescriptorHeap");

            descriptorHeapDesc.heap = descriptorHeap;
            descriptorHeapDesc.baseHandleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.baseHandleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.descriptorSize = m_Device->GetDescriptorHandleIncrementSize((D3D12_DESCRIPTOR_HEAP_TYPE)i);

            m_DescriptorHeaps[m_DescriptorHeapNum++] = descriptorHeap;
        }
    }

    m_DescriptorSets.resize(descriptorPoolDesc.descriptorSetMaxNum);

    return Result::SUCCESS;
}

Result DescriptorPoolD3D12::Create(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc) {
    static_assert(static_cast<size_t>(DescriptorHeapType::MAX_NUM) == 2, "DescriptorHeapType::MAX_NUM != 2");
    static_assert(static_cast<uint32_t>(DescriptorHeapType::RESOURCE) == 0, "DescriptorHeapType::RESOURCE != 0");
    static_assert(static_cast<uint32_t>(DescriptorHeapType::SAMPLER) == 1, "DescriptorHeapType::SAMPLER != 1");

    const std::array<ID3D12DescriptorHeap*, DescriptorHeapType::MAX_NUM> descriptorHeaps = {
        descriptorPoolD3D12Desc.d3d12ResourceDescriptorHeap,
        descriptorPoolD3D12Desc.d3d12SamplerDescriptorHeap,
    };

    for (uint32_t i = 0; i < DescriptorHeapType::MAX_NUM; i++) {
        DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[i];

        descriptorHeapDesc = {};
        if (descriptorHeaps[i]) {
            D3D12_DESCRIPTOR_HEAP_DESC desc = descriptorHeaps[i]->GetDesc();
            descriptorHeapDesc.heap = descriptorHeaps[i];
            descriptorHeapDesc.baseHandleCPU = descriptorHeaps[i]->GetCPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.baseHandleGPU = descriptorHeaps[i]->GetGPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.descriptorSize = m_Device->GetDescriptorHandleIncrementSize(desc.Type);

            m_DescriptorHeaps[m_DescriptorHeapNum++] = descriptorHeaps[i];
        }
    }

    m_DescriptorSets.resize(descriptorPoolD3D12Desc.descriptorSetMaxNum);

    return Result::SUCCESS;
}

void DescriptorPoolD3D12::Bind(ID3D12GraphicsCommandList* graphicsCommandList) const {
    graphicsCommandList->SetDescriptorHeaps(m_DescriptorHeapNum, m_DescriptorHeaps.data());
}

DescriptorHandleCPU DescriptorPoolD3D12::GetDescriptorHandleCPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const {
    const DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[descriptorHeapType];
    DescriptorHandleCPU descriptorHandleCPU = descriptorHeapDesc.baseHandleCPU + offset * descriptorHeapDesc.descriptorSize;

    return descriptorHandleCPU;
}

DescriptorHandleGPU DescriptorPoolD3D12::GetDescriptorHandleGPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const {
    const DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[descriptorHeapType];
    DescriptorHandleGPU descriptorHandleGPU = descriptorHeapDesc.baseHandleGPU + offset * descriptorHeapDesc.descriptorSize;

    return descriptorHandleGPU;
}

NRI_INLINE void DescriptorPoolD3D12::SetDebugName(const char* name) {
    for (ID3D12DescriptorHeap* descriptorHeap : m_DescriptorHeaps)
        SET_D3D_DEBUG_OBJECT_NAME(descriptorHeap, name);
}

NRI_INLINE Result DescriptorPoolD3D12::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t) {
    ExclusiveScope lock(m_Lock);

    if (m_DescriptorSetNum + instanceNum > m_DescriptorSets.size())
        return Result::OUT_OF_MEMORY;

    const PipelineLayoutD3D12& pipelineLayoutD3D12 = (PipelineLayoutD3D12&)pipelineLayout;
    const DescriptorSetMapping& descriptorSetMapping = pipelineLayoutD3D12.GetDescriptorSetMapping(setIndex);

    // Since there is no "free" functionality allocation strategy is "linear grow"
    for (uint32_t i = 0; i < instanceNum; i++) {
        // Heap offsets
        std::array<uint32_t, DescriptorHeapType::MAX_NUM> heapOffsets = {};
        for (uint32_t h = 0; h < heapOffsets.size(); h++) {
            uint32_t descriptorNum = descriptorSetMapping.descriptorNum[h];

            if (descriptorNum) {
                DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[(DescriptorHeapType)h];
                heapOffsets[h] = descriptorHeapDesc.num;
                descriptorHeapDesc.num += descriptorNum;
            }
        }

        // Create descriptor set
        DescriptorSetD3D12* descriptorSet = &m_DescriptorSets[m_DescriptorSetNum++];
        descriptorSet->Create(this, &descriptorSetMapping, heapOffsets);

        descriptorSets[i] = (DescriptorSet*)descriptorSet;
    }

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorPoolD3D12::Reset() {
    ExclusiveScope lock(m_Lock);

    for (DescriptorHeapDesc& descriptorHeapDesc : m_DescriptorHeapDescs)
        descriptorHeapDesc.num = 0;

    m_DescriptorSetNum = 0;
}
