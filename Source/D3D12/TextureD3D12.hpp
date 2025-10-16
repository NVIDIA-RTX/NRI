// © 2021 NVIDIA Corporation

Result TextureD3D12::Create(const TextureDesc& textureDesc) {
    m_Desc = FixTextureDesc(textureDesc);

    return Result::SUCCESS;
}

Result TextureD3D12::Create(const TextureD3D12Desc& textureD3D12Desc) {
    if (!GetTextureDesc(textureD3D12Desc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Texture = (ID3D12ResourceBest*)textureD3D12Desc.d3d12Resource;

    return Result::SUCCESS;
}

Result TextureD3D12::Allocate(MemoryLocation memoryLocation, float priority, bool committed) {
    CHECK(!m_Texture, "Unexpected");

    D3D12_CLEAR_VALUE clearValue = {GetDxgiFormat(m_Desc.format).typed};

    const FormatProps& formatProps = GetFormatProps(m_Desc.format);
    if (formatProps.isDepth || formatProps.isStencil) {
        clearValue.DepthStencil.Depth = m_Desc.optimizedClearValue.depthStencil.depth;
        clearValue.DepthStencil.Stencil = m_Desc.optimizedClearValue.depthStencil.stencil;
    } else {
        clearValue.Color[0] = m_Desc.optimizedClearValue.color.f.x;
        clearValue.Color[1] = m_Desc.optimizedClearValue.color.f.y;
        clearValue.Color[2] = m_Desc.optimizedClearValue.color.f.z;
        clearValue.Color[3] = m_Desc.optimizedClearValue.color.f.w;
    }

    uint32_t flags = D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY;
    flags |= committed ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    if (deviceDesc.tiers.memory == 0) {
        if (m_Desc.usage & (TextureUsageBits::COLOR_ATTACHMENT | TextureUsageBits::DEPTH_STENCIL_ATTACHMENT))
            heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        else
            heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
    }

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = m_Device.GetHeapType(memoryLocation);
    allocationDesc.Flags = (D3D12MA::ALLOCATION_FLAGS)flags;
    allocationDesc.ExtraHeapFlags = heapFlags;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_RESOURCE_DESC1 desc1 = {};
    m_Device.GetResourceDesc(m_Desc, (D3D12_RESOURCE_DESC&)desc1);
    desc1.Alignment = 0; // TODO: D3D12MA naively adds "D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT" even if "Alignment" is already set (it's an error)

    bool isRenderableSurface = desc1.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    HRESULT hr = m_Device.GetVma()->CreateResource3(&allocationDesc, &desc1, D3D12_BARRIER_LAYOUT_COMMON, isRenderableSurface ? &clearValue : nullptr, NO_CASTABLE_FORMATS, &m_VmaAllocation, IID_PPV_ARGS(&m_Texture));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3");
#else
    D3D12_RESOURCE_DESC desc = {};
    m_Device.GetResourceDesc(m_Desc, desc);

    bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    HRESULT hr = m_Device.GetVma()->CreateResource(&allocationDesc, &desc, D3D12_RESOURCE_STATE_COMMON, isRenderableSurface ? &clearValue : nullptr, &m_VmaAllocation, IID_PPV_ARGS(&m_Texture));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource");
#endif

    // Priority
    D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(priority);
    if (residencyPriority != 0) {
        ID3D12Pageable* obj = m_Texture.GetInterface();
        hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority");
    }

    return Result::SUCCESS;
}

Result TextureD3D12::BindMemory(const MemoryD3D12& memory, uint64_t offset) {
    CHECK(!m_Texture, "Unexpected");

    D3D12_CLEAR_VALUE clearValue = {GetDxgiFormat(m_Desc.format).typed};

    const FormatProps& formatProps = GetFormatProps(m_Desc.format);
    if (formatProps.isDepth || formatProps.isStencil) {
        clearValue.DepthStencil.Depth = m_Desc.optimizedClearValue.depthStencil.depth;
        clearValue.DepthStencil.Stencil = m_Desc.optimizedClearValue.depthStencil.stencil;
    } else {
        clearValue.Color[0] = m_Desc.optimizedClearValue.color.f.x;
        clearValue.Color[1] = m_Desc.optimizedClearValue.color.f.y;
        clearValue.Color[2] = m_Desc.optimizedClearValue.color.f.z;
        clearValue.Color[3] = m_Desc.optimizedClearValue.color.f.w;
    }

    const D3D12_HEAP_DESC& heapDesc = memory.GetHeapDesc();
    // STATE_CREATION ERROR #640: CREATERESOURCEANDHEAP_INVALIDHEAPMISCFLAGS
    D3D12_HEAP_FLAGS heapFlagsFixed = heapDesc.Flags & ~(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS);
    if (!m_Device.IsMemoryZeroInitializationEnabled())
        heapFlagsFixed |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

    offset += memory.GetOffset();

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_RESOURCE_DESC1 desc1 = {};
    m_Device.GetResourceDesc(m_Desc, (D3D12_RESOURCE_DESC&)desc1);

    const D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_COMMON;
    bool isRenderableSurface = desc1.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    if (memory.IsDummy()) {
        HRESULT hr = m_Device->CreateCommittedResource3(&heapDesc.Properties, heapFlagsFixed, &desc1, initialLayout, isRenderableSurface ? &clearValue : nullptr, nullptr, NO_CASTABLE_FORMATS, IID_PPV_ARGS(&m_Texture));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreateCommittedResource3");
    } else {
        HRESULT hr = m_Device->CreatePlacedResource2(memory, offset, &desc1, initialLayout, isRenderableSurface ? &clearValue : nullptr, NO_CASTABLE_FORMATS, IID_PPV_ARGS(&m_Texture));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreatePlacedResource2");
    }
#else
    // TODO: by design textures should not be created in UPLOAD/READBACK heaps, since they can't be mapped. But what about a wrapped texture?
    D3D12_RESOURCE_DESC desc = {};
    m_Device.GetResourceDesc(m_Desc, desc);

    const D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    if (memory.IsDummy()) {
        HRESULT hr = m_Device->CreateCommittedResource(&heapDesc.Properties, heapFlagsFixed, &desc, initialState, isRenderableSurface ? &clearValue : nullptr, IID_PPV_ARGS(&m_Texture));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommittedResource");
    } else {
        HRESULT hr = m_Device->CreatePlacedResource(memory, offset, &desc, initialState, isRenderableSurface ? &clearValue : nullptr, IID_PPV_ARGS(&m_Texture));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreatePlacedResource");
    }
#endif

    // Priority
    D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(memory.GetPriority());
    if (residencyPriority != 0) {
        ID3D12Pageable* obj = m_Texture.GetInterface();
        HRESULT hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority");
    }

    return Result::SUCCESS;
}
