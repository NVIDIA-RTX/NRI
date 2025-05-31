// © 2021 NVIDIA Corporation

Result TextureD3D12::Create(const TextureDesc& textureDesc) {
    m_Desc = FixTextureDesc(textureDesc);

    return Result::SUCCESS;
}

Result TextureD3D12::Create(const TextureD3D12Desc& textureDesc) {
    if (!GetTextureDesc(textureDesc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Texture = (ID3D12ResourceBest*)textureDesc.d3d12Resource;

    return Result::SUCCESS;
}

Result TextureD3D12::BindMemory(const MemoryD3D12* memory, uint64_t offset) {
    // Texture was already created externally
    if (m_Texture)
        return Result::SUCCESS;

    const FormatProps& formatProps = GetFormatProps(m_Desc.format);
    D3D12_CLEAR_VALUE clearValue = {GetDxgiFormat(m_Desc.format).typed};
    if (formatProps.isDepth || formatProps.isStencil)
    {
        clearValue.DepthStencil.Depth = m_Desc.optimizedClearValue.depthStencil.depth;
        clearValue.DepthStencil.Stencil = m_Desc.optimizedClearValue.depthStencil.stencil;
    }
    else {
        clearValue.Color[0] = m_Desc.optimizedClearValue.color.f.x;
        clearValue.Color[1] = m_Desc.optimizedClearValue.color.f.y;
        clearValue.Color[2] = m_Desc.optimizedClearValue.color.f.z;
        clearValue.Color[3] = m_Desc.optimizedClearValue.color.f.w;
    }
    const D3D12_HEAP_DESC& heapDesc = memory->GetHeapDesc();
    // STATE_CREATION ERROR #640: CREATERESOURCEANDHEAP_INVALIDHEAPMISCFLAGS
    D3D12_HEAP_FLAGS heapFlagsFixed = heapDesc.Flags & ~(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS);

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_Device.GetVersion() >= 10) {
        D3D12_RESOURCE_DESC1 desc1 = {};
        m_Device.GetResourceDesc(m_Desc, (D3D12_RESOURCE_DESC&)desc1);

        const D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_COMMON;
        bool isRenderableSurface = desc1.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        if (memory->IsDummy()) {
            HRESULT hr = m_Device->CreateCommittedResource3(&heapDesc.Properties, heapFlagsFixed, &desc1, initialLayout, isRenderableSurface ? &clearValue : nullptr, nullptr, NO_CASTABLE_FORMATS, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreateCommittedResource3()");
        } else {
            HRESULT hr = m_Device->CreatePlacedResource2(*memory, offset, &desc1, initialLayout, isRenderableSurface ? &clearValue : nullptr, NO_CASTABLE_FORMATS, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreatePlacedResource2()");
        }
    } else
#endif
    { // TODO: by design textures should not be created in UPLOAD/READBACK heaps, since they can't be mapped. But what about a wrapped texture?
        D3D12_RESOURCE_DESC desc = {};
        m_Device.GetResourceDesc(m_Desc, desc);

        const D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
        bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        if (memory->IsDummy()) {
            HRESULT hr = m_Device->CreateCommittedResource(&heapDesc.Properties, heapFlagsFixed, &desc, initialState, isRenderableSurface ? &clearValue : nullptr, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommittedResource()");
        } else {
            HRESULT hr = m_Device->CreatePlacedResource(*memory, offset, &desc, initialState, isRenderableSurface ? &clearValue : nullptr, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreatePlacedResource()");
        }
    }

    // Priority
    D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(memory->GetPriority());
    if (m_Device.GetVersion() >= 1 && residencyPriority != 0) {
        ID3D12Pageable* obj = m_Texture.GetInterface();
        HRESULT hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority()");
    }

    return Result::SUCCESS;
}
