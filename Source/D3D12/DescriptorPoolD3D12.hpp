// © 2021 NVIDIA Corporation

Result DescriptorPoolD3D12::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    uint32_t descriptorHeapSize[DescriptorHeapType::MAX_NUM] = {};
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.constantBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.textureMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.storageTextureMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.bufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.storageBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.structuredBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.storageStructuredBufferMaxNum;
    descriptorHeapSize[DescriptorHeapType::RESOURCE] += descriptorPoolDesc.accelerationStructureMaxNum;
    descriptorHeapSize[DescriptorHeapType::SAMPLER] += descriptorPoolDesc.samplerMaxNum;

    for (uint32_t i = 0; i < DescriptorHeapType::MAX_NUM; i++) {
        DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[i];
        descriptorHeapDesc.num = 0;

        if (descriptorHeapSize[i]) {
            ComPtr<ID3D12DescriptorHeap> descriptorHeap;
            D3D12_DESCRIPTOR_HEAP_DESC desc = {(D3D12_DESCRIPTOR_HEAP_TYPE)i, descriptorHeapSize[i], D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, NODE_MASK};
            HRESULT hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateDescriptorHeap()");

            descriptorHeapDesc.heap = descriptorHeap;
            descriptorHeapDesc.basePointerCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.basePointerGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.descriptorSize = m_Device->GetDescriptorHandleIncrementSize((D3D12_DESCRIPTOR_HEAP_TYPE)i);

            m_DescriptorHeaps[m_DescriptorHeapNum] = descriptorHeap;
            m_DescriptorHeapNum++;
        }
    }

    m_DescriptorSets.resize(descriptorPoolDesc.descriptorSetMaxNum, DescriptorSetD3D12(*this));

    return Result::SUCCESS;
}

Result DescriptorPoolD3D12::Create(const DescriptorPoolD3D12Desc& descriptorPoolDesc) {
    static_assert(static_cast<size_t>(DescriptorHeapType::MAX_NUM) == 2, "DescriptorHeapType::MAX_NUM != 2");
    static_assert(static_cast<uint32_t>(DescriptorHeapType::RESOURCE) == 0, "DescriptorHeapType::RESOURCE != 0");
    static_assert(static_cast<uint32_t>(DescriptorHeapType::SAMPLER) == 1, "DescriptorHeapType::SAMPLER != 1");

    const std::array<ID3D12DescriptorHeap*, DescriptorHeapType::MAX_NUM> descriptorHeaps = {
        descriptorPoolDesc.d3d12ResourceDescriptorHeap,
        descriptorPoolDesc.d3d12SamplerDescriptorHeap,
    };

    for (uint32_t i = 0; i < DescriptorHeapType::MAX_NUM; i++) {
        DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[i];
        descriptorHeapDesc.num = 0;

        if (descriptorHeaps[i]) {
            D3D12_DESCRIPTOR_HEAP_DESC desc = descriptorHeaps[i]->GetDesc();
            descriptorHeapDesc.heap = descriptorHeaps[i];
            descriptorHeapDesc.basePointerCPU = descriptorHeaps[i]->GetCPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.basePointerGPU = descriptorHeaps[i]->GetGPUDescriptorHandleForHeapStart().ptr;
            descriptorHeapDesc.descriptorSize = m_Device->GetDescriptorHandleIncrementSize(desc.Type);

            m_DescriptorHeaps[m_DescriptorHeapNum] = descriptorHeaps[i];
            m_DescriptorHeapNum++;
        }
    }

    m_DescriptorSets.resize(descriptorPoolDesc.descriptorSetMaxNum, DescriptorSetD3D12(*this));

    return Result::SUCCESS;
}

void DescriptorPoolD3D12::Bind(ID3D12GraphicsCommandList* graphicsCommandList) const {
    graphicsCommandList->SetDescriptorHeaps(m_DescriptorHeapNum, m_DescriptorHeaps.data());
}

uint32_t DescriptorPoolD3D12::AllocateDescriptors(DescriptorHeapType descriptorHeapType, uint32_t descriptorNum) {
    DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[descriptorHeapType];
    uint32_t descriptorOffset = descriptorHeapDesc.num;
    descriptorHeapDesc.num += descriptorNum;

    return descriptorOffset;
}

DescriptorPointerCPU DescriptorPoolD3D12::GetDescriptorPointerCPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const {
    const DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[descriptorHeapType];
    DescriptorPointerCPU descriptorPointerCPU = descriptorHeapDesc.basePointerCPU + offset * descriptorHeapDesc.descriptorSize;

    return descriptorPointerCPU;
}

DescriptorPointerGPU DescriptorPoolD3D12::GetDescriptorPointerGPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const {
    const DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeapDescs[descriptorHeapType];
    DescriptorPointerGPU descriptorPointerGPU = descriptorHeapDesc.basePointerGPU + offset * descriptorHeapDesc.descriptorSize;

    return descriptorPointerGPU;
}

NRI_INLINE void DescriptorPoolD3D12::SetDebugName(const char* name) {
    for (ID3D12DescriptorHeap* descriptorHeap : m_DescriptorHeaps)
        SET_D3D_DEBUG_OBJECT_NAME(descriptorHeap, name);
}

NRI_INLINE Result DescriptorPoolD3D12::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t) {
    ExclusiveScope lock(m_Lock);

    if (m_DescriptorSetNum + instanceNum > m_DescriptorSets.size())
        return Result::FAILURE;

    const PipelineLayoutD3D12& pipelineLayoutD3D12 = (PipelineLayoutD3D12&)pipelineLayout;
    const DescriptorSetMapping& descriptorSetMapping = pipelineLayoutD3D12.GetDescriptorSetMapping(setIndex);
    const DynamicConstantBufferMapping& dynamicConstantBufferMapping = pipelineLayoutD3D12.GetDynamicConstantBufferMapping(setIndex);

    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetD3D12* descriptorSet = &m_DescriptorSets[m_DescriptorSetNum++];
        descriptorSet->Initialize(&descriptorSetMapping, dynamicConstantBufferMapping.rootConstantNum);
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
