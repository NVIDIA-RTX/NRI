// © 2021 NVIDIA Corporation

Result BufferD3D12::Create(const BufferDesc& bufferDesc) {
    m_Desc = bufferDesc;

    return Result::SUCCESS;
}

Result BufferD3D12::Create(const BufferD3D12Desc& bufferD3D12Desc) {
    if (bufferD3D12Desc.desc)
        m_Desc = *bufferD3D12Desc.desc;
    else if (!GetBufferDesc(bufferD3D12Desc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Buffer = (ID3D12ResourceBest*)bufferD3D12Desc.d3d12Resource;

    return Result::SUCCESS;
}

Result BufferD3D12::Allocate(MemoryLocation memoryLocation, float priority, bool committed) {
    CHECK(!m_Buffer, "Unexpected");

    uint32_t flags = D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY;
    flags |= committed ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    if (deviceDesc.tiers.memory == 0)
        heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = m_Device.GetHeapType(memoryLocation);
    allocationDesc.Flags = (D3D12MA::ALLOCATION_FLAGS)flags;
    allocationDesc.ExtraHeapFlags = heapFlags;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_RESOURCE_DESC1 desc1 = {};
    m_Device.GetResourceDesc(m_Desc, (D3D12_RESOURCE_DESC&)desc1);

    const D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

    HRESULT hr = m_Device.GetVma()->CreateResource3(&allocationDesc, &desc1, initialLayout, nullptr, NO_CASTABLE_FORMATS, &m_VmaAllocation, IID_PPV_ARGS(&m_Buffer));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3");
#else
    D3D12_RESOURCE_DESC desc = {};
    m_Device.GetResourceDesc(m_Desc, desc);

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    if (memoryLocation == MemoryLocation::HOST_UPLOAD || memoryLocation == MemoryLocation::DEVICE_UPLOAD)
        initialState |= D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (memoryLocation == MemoryLocation::HOST_READBACK)
        initialState |= D3D12_RESOURCE_STATE_COPY_DEST;

    if (m_Desc.usage & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE)
        initialState |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    HRESULT hr = m_Device.GetVma()->CreateResource(&allocationDesc, &desc, initialState, nullptr, &m_VmaAllocation, IID_PPV_ARGS(&m_Buffer));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource");
#endif

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = allocationDesc.HeapType;

    return SetPriorityAndPersistentlyMap(priority, heapProps);
}

Result BufferD3D12::BindMemory(const MemoryD3D12& memory, uint64_t offset) {
    CHECK(!m_Buffer, "Unexpected");

    const D3D12_HEAP_DESC& heapDesc = memory.GetHeapDesc();

    // STATE_CREATION ERROR #640: CREATERESOURCEANDHEAP_INVALIDHEAPMISCFLAGS
    D3D12_HEAP_FLAGS heapFlagsFixed = heapDesc.Flags & ~(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS);
    if (!m_Device.IsMemoryZeroInitializationEnabled())
        heapFlagsFixed |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

    offset += memory.GetOffset();

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_RESOURCE_DESC1 desc1 = {};
    m_Device.GetResourceDesc(m_Desc, (D3D12_RESOURCE_DESC&)desc1);

    const D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

    if (memory.IsDummy()) {
        HRESULT hr = m_Device->CreateCommittedResource3(&heapDesc.Properties, heapFlagsFixed, &desc1, initialLayout, nullptr, nullptr, NO_CASTABLE_FORMATS, IID_PPV_ARGS(&m_Buffer));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreateCommittedResource3");
    } else {
        HRESULT hr = m_Device->CreatePlacedResource2(memory, offset, &desc1, initialLayout, nullptr, NO_CASTABLE_FORMATS, IID_PPV_ARGS(&m_Buffer));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreatePlacedResource2");
    }
#else
    bool isUpload = heapDesc.Properties.Type == D3D12_HEAP_TYPE_UPLOAD
#    if NRI_ENABLE_AGILITY_SDK_SUPPORT
        || heapDesc.Properties.Type == D3D12_HEAP_TYPE_GPU_UPLOAD
#    endif
        || (heapDesc.Properties.Type == D3D12_HEAP_TYPE_CUSTOM && heapDesc.Properties.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE);

    bool isReadback = heapDesc.Properties.Type == D3D12_HEAP_TYPE_READBACK
        || (heapDesc.Properties.Type == D3D12_HEAP_TYPE_CUSTOM && heapDesc.Properties.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK);

    D3D12_RESOURCE_DESC desc = {};
    m_Device.GetResourceDesc(m_Desc, desc);

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    if (isUpload)
        initialState |= D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (isReadback)
        initialState |= D3D12_RESOURCE_STATE_COPY_DEST;

    if (m_Desc.usage & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE)
        initialState |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    if (memory.IsDummy()) {
        HRESULT hr = m_Device->CreateCommittedResource(&heapDesc.Properties, heapFlagsFixed, &desc, initialState, nullptr, IID_PPV_ARGS(&m_Buffer));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommittedResource");
    } else {
        HRESULT hr = m_Device->CreatePlacedResource(memory, offset, &desc, initialState, nullptr, IID_PPV_ARGS(&m_Buffer));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreatePlacedResource");
    }
#endif
    return SetPriorityAndPersistentlyMap(memory.GetPriority(), heapDesc.Properties);
}

NRI_INLINE Result BufferD3D12::SetPriorityAndPersistentlyMap(float priority, const D3D12_HEAP_PROPERTIES& heapProps) {
    // Priority
    D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(priority);
    if (residencyPriority != 0) {
        ID3D12Pageable* obj = m_Buffer.GetInterface();
        HRESULT hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority");
    }

    // Mapping
    bool isUpload = heapProps.Type == D3D12_HEAP_TYPE_UPLOAD
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        || heapProps.Type == D3D12_HEAP_TYPE_GPU_UPLOAD
#endif
        || (heapProps.Type == D3D12_HEAP_TYPE_CUSTOM && heapProps.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE);

    bool isReadback = heapProps.Type == D3D12_HEAP_TYPE_READBACK
        || (heapProps.Type == D3D12_HEAP_TYPE_CUSTOM && heapProps.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK);

    if (isUpload || isReadback) {
        D3D12_RANGE readRange = {};
        if (isReadback)
            readRange.End = m_Desc.size;

        HRESULT hr = m_Buffer->Map(0, &readRange, (void**)&m_MappedMemory);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Resource::Map");
    }

    return Result::SUCCESS;
}

NRI_INLINE void* BufferD3D12::Map(uint64_t offset) {
    CHECK(m_MappedMemory, "No CPU access");

    return m_MappedMemory + offset;
}
