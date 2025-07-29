// © 2021 NVIDIA Corporation

Result MemoryD3D12::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    MemoryTypeInfo memoryTypeInfo = Unpack(allocateMemoryDesc.type);
    D3D12_HEAP_FLAGS heapFlags = (D3D12_HEAP_FLAGS)(memoryTypeInfo.heapFlags & ~HEAP_FLAG_MSAA_ALIGNMENT);
    bool isMsaaAlignmentNeeded = (memoryTypeInfo.heapFlags & HEAP_FLAG_MSAA_ALIGNMENT) != 0;

    D3D12_HEAP_DESC heapDesc = {};
    heapDesc.SizeInBytes = allocateMemoryDesc.size;
    heapDesc.Properties.Type = (D3D12_HEAP_TYPE)memoryTypeInfo.heapType;
    heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapDesc.Properties.CreationNodeMask = NODE_MASK;
    heapDesc.Properties.VisibleNodeMask = NODE_MASK;
    heapDesc.Alignment = isMsaaAlignmentNeeded ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heapDesc.Flags = allocateMemoryDesc.size ? heapFlags : D3D12_HEAP_FLAG_NONE;

    if (!m_Device.IsMemoryZeroInitializationEnabled())
        heapDesc.Flags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

    if (!memoryTypeInfo.mustBeDedicated) {
        HRESULT hr = m_Device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateHeap");

        D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(allocateMemoryDesc.priority);
        if (residencyPriority != 0) {
            ID3D12Pageable* obj = m_Heap.GetInterface();
            hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority");
        }
    }

    m_HeapDesc = heapDesc;
    m_Priority = allocateMemoryDesc.priority;

    return Result::SUCCESS;
}

Result MemoryD3D12::Create(const MemoryD3D12Desc& memoryDesc) {
    m_Heap = memoryDesc.d3d12Heap;
    m_HeapDesc = m_Heap->GetDesc();

    return Result::SUCCESS;
}
