// © 2021 NVIDIA Corporation

Result MemoryD3D12::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    MemoryTypeInfo memoryTypeInfo = Unpack(allocateMemoryDesc.type);
    bool isMsaaAlignmentNeeded = (memoryTypeInfo.heapFlags & HEAP_FLAG_MSAA_ALIGNMENT) != 0;

    D3D12_HEAP_FLAGS heapFlags = (D3D12_HEAP_FLAGS)(memoryTypeInfo.heapFlags & ~HEAP_FLAG_MSAA_ALIGNMENT);
    if (!m_Device.IsMemoryZeroInitializationEnabled())
        heapFlags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

    m_HeapDesc.SizeInBytes = allocateMemoryDesc.size;
    m_HeapDesc.Properties.Type = (D3D12_HEAP_TYPE)memoryTypeInfo.heapType;
    m_HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    m_HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    m_HeapDesc.Properties.CreationNodeMask = NODE_MASK;
    m_HeapDesc.Properties.VisibleNodeMask = NODE_MASK;
    m_HeapDesc.Alignment = isMsaaAlignmentNeeded ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    m_HeapDesc.Flags = allocateMemoryDesc.size ? heapFlags : D3D12_HEAP_FLAG_NONE;

    m_Priority = allocateMemoryDesc.priority;

    // An allocation is not needed, it's implicit for committed resources
    if (memoryTypeInfo.mustBeDedicated)
        return Result::SUCCESS;

    if (allocateMemoryDesc.useVMA) { // TODO: no residency priority, because a custom pool is needed
        uint32_t flags = D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY | D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = m_HeapDesc.Properties.Type;
        allocationDesc.Flags = (D3D12MA::ALLOCATION_FLAGS)flags;
        allocationDesc.ExtraHeapFlags = m_HeapDesc.Flags;

        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = {};
        allocInfo.SizeInBytes = Align(allocateMemoryDesc.size, 64 * 1024); // TODO: alignment is a silly requirement of D3D12MA!
        allocInfo.Alignment = m_HeapDesc.Alignment;

        HRESULT hr = m_Device.GetVma()->AllocateMemory(&allocationDesc, &allocInfo, &m_VmaAllocation);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::AllocateMemory");
    } else {
        HRESULT hr = m_Device->CreateHeap(&m_HeapDesc, IID_PPV_ARGS(&m_Heap));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateHeap");

        D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(allocateMemoryDesc.priority);
        if (residencyPriority != 0) {
            ID3D12Pageable* obj = m_Heap.GetInterface();
            hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority");
        }
    }

    return Result::SUCCESS;
}

Result MemoryD3D12::Create(const MemoryD3D12Desc& memoryD3D12Desc) {
    m_Heap = memoryD3D12Desc.d3d12Heap;
    m_Offset = memoryD3D12Desc.offset;
    m_HeapDesc = m_Heap->GetDesc();

    return Result::SUCCESS;
}
