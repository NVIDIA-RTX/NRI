// © 2026 NVIDIA Corporation

Result DescriptorHeapD3D12::Create(const DescriptorHeapDesc& descriptorHeapDesc) {
    Result result = CreateHeap(DescriptorHeapType::RESOURCE, descriptorHeapDesc.resourceDescriptorNum);
    if (result != Result::SUCCESS)
        return result;

    return CreateHeap(DescriptorHeapType::SAMPLER, descriptorHeapDesc.samplerDescriptorNum);
}

Result DescriptorHeapD3D12::CreateHeap(DescriptorHeapType type, uint32_t descriptorNum) {
    if (!descriptorNum)
        return Result::SUCCESS;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = (D3D12_DESCRIPTOR_HEAP_TYPE)type;
    desc.NumDescriptors = descriptorNum;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = NODE_MASK;

    HRESULT hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heaps[type]));
    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateDescriptorHeap");

    m_HeapHandles[m_HeapNum++] = m_Heaps[type];
    m_BaseHandles[type] = m_Heaps[type]->GetCPUDescriptorHandleForHeapStart().ptr;
    m_DescriptorSizes[type] = m_Device->GetDescriptorHandleIncrementSize(desc.Type);

    return Result::SUCCESS;
}

Result DescriptorHeapD3D12::WriteResourceDescriptors(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    if (!writeDescNum)
        return Result::SUCCESS;

    Scratch<D3D12_CPU_DESCRIPTOR_HANDLE> handles = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_CPU_DESCRIPTOR_HANDLE, size_t(writeDescNum) * 2);
    D3D12_CPU_DESCRIPTOR_HANDLE* dstHandles = handles;
    D3D12_CPU_DESCRIPTOR_HANDLE* srcHandles = handles + writeDescNum;

    for (uint32_t i = 0; i < writeDescNum; i++) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)writeDescs[i].resource;
        dstHandles[i] = {m_BaseHandles[DescriptorHeapType::RESOURCE] + writeDescs[i].descriptorIndex * m_DescriptorSizes[DescriptorHeapType::RESOURCE]};
        srcHandles[i] = {descriptor.GetDescriptorHandleCPU()};
    }

    m_Device->CopyDescriptors(writeDescNum, dstHandles, nullptr, writeDescNum, srcHandles, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return Result::SUCCESS;
}

Result DescriptorHeapD3D12::WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    if (!writeDescNum)
        return Result::SUCCESS;

    Scratch<D3D12_CPU_DESCRIPTOR_HANDLE> handles = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_CPU_DESCRIPTOR_HANDLE, size_t(writeDescNum) * 2);
    D3D12_CPU_DESCRIPTOR_HANDLE* dstHandles = handles;
    D3D12_CPU_DESCRIPTOR_HANDLE* srcHandles = handles + writeDescNum;

    for (uint32_t i = 0; i < writeDescNum; i++) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)writeDescs[i].sampler;
        dstHandles[i] = {m_BaseHandles[DescriptorHeapType::SAMPLER] + writeDescs[i].descriptorIndex * m_DescriptorSizes[DescriptorHeapType::SAMPLER]};
        srcHandles[i] = {descriptor.GetDescriptorHandleCPU()};
    }

    m_Device->CopyDescriptors(writeDescNum, dstHandles, nullptr, writeDescNum, srcHandles, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    return Result::SUCCESS;
}

NRI_INLINE void DescriptorHeapD3D12::Bind(ID3D12GraphicsCommandList* commandList) const {
    commandList->SetDescriptorHeaps(m_HeapNum, m_HeapHandles.data());
}

NRI_INLINE void DescriptorHeapD3D12::SetDebugName(const char* name) {
    for (ID3D12DescriptorHeap* heap : m_HeapHandles)
        NRI_SET_D3D_DEBUG_OBJECT_NAME(heap, name);
}
