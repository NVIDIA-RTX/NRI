// © 2021 NVIDIA Corporation

Result TextureD3D11::Allocate(MemoryLocation memoryLocation, float priority) {
    CHECK(!m_Texture, "Unexpected");

    const DxgiFormat& dxgiFormat = GetDxgiFormat(m_Desc.format);

    uint32_t bindFlags = 0;
    if (m_Desc.usage & TextureUsageBits::SHADER_RESOURCE)
        bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (m_Desc.usage & TextureUsageBits::SHADER_RESOURCE_STORAGE)
        bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    if (m_Desc.usage & TextureUsageBits::COLOR_ATTACHMENT)
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (m_Desc.usage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT)
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;

    D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
    uint32_t cpuAccessFlags = 0;
    switch (memoryLocation) {
        case MemoryLocation::DEVICE_UPLOAD:
        case MemoryLocation::HOST_UPLOAD:
            usage = D3D11_USAGE_DYNAMIC;
            cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
            break;
        case MemoryLocation::HOST_READBACK:
            usage = D3D11_USAGE_STAGING;
            cpuAccessFlags = D3D11_CPU_ACCESS_READ;
            break;
        default:
            break;
    }

    HRESULT hr = E_INVALIDARG;
    if (m_Desc.type == TextureType::TEXTURE_1D) {
        D3D11_TEXTURE1D_DESC desc = {};
        desc.Width = m_Desc.width;
        desc.MipLevels = m_Desc.mipNum;
        desc.ArraySize = m_Desc.layerNum;
        desc.Format = dxgiFormat.typeless;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = cpuAccessFlags;

        hr = m_Device->CreateTexture1D(&desc, nullptr, (ID3D11Texture1D**)&m_Texture);
    } else if (m_Desc.type == TextureType::TEXTURE_3D) {
        D3D11_TEXTURE3D_DESC desc = {};
        desc.Width = m_Desc.width;
        desc.Height = m_Desc.height;
        desc.Depth = m_Desc.depth;
        desc.MipLevels = m_Desc.mipNum;
        desc.Format = dxgiFormat.typeless;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = cpuAccessFlags;

        hr = m_Device->CreateTexture3D(&desc, nullptr, (ID3D11Texture3D**)&m_Texture);
    } else {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = m_Desc.width;
        desc.Height = m_Desc.height;
        desc.MipLevels = m_Desc.mipNum;
        desc.ArraySize = m_Desc.layerNum;
        desc.Format = dxgiFormat.typeless;
        desc.SampleDesc.Count = m_Desc.sampleNum;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = cpuAccessFlags;

        if (m_Desc.sampleNum == 1 && desc.Width == desc.Height && (m_Desc.layerNum % 6 == 0))
            desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // TODO: valid assumption?

        hr = m_Device->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&m_Texture);
    }

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateTextureXx");

    // Priority
    uint32_t evictionPriority = ConvertPriority(priority);
    if (evictionPriority != 0)
        m_Texture->SetEvictionPriority(evictionPriority);

    return Result::SUCCESS;
}

Result TextureD3D11::Create(const TextureDesc& textureDesc) {
    m_Desc = FixTextureDesc(textureDesc);

    return Result::SUCCESS;
}

Result TextureD3D11::Create(const TextureD3D11Desc& textureD3D11Desc) {
    if (!GetTextureDesc(textureD3D11Desc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Texture = textureD3D11Desc.d3d11Resource;

    return Result::SUCCESS;
}

uint32_t TextureD3D11::GetMipmappedSize(const TextureDesc& textureDesc) {
    bool isCompressed = textureDesc.format >= Format::BC1_RGBA_UNORM && textureDesc.format <= Format::BC7_RGBA_SRGB;

    uint32_t w = GetDimension(GraphicsAPI::D3D11, textureDesc, 0, 0);
    uint32_t h = GetDimension(GraphicsAPI::D3D11, textureDesc, 1, 0);
    uint32_t d = GetDimension(GraphicsAPI::D3D11, textureDesc, 2, 0);
    uint32_t mipNum = textureDesc.mipNum;
    uint32_t size = 0;

    while (mipNum) {
        if (isCompressed)
            size += ((w + 3) >> 2) * ((h + 3) >> 2) * d;
        else
            size += w * h * d;

        if (w == 1 && h == 1 && d == 1)
            break;

        if (d > 1)
            d >>= 1;

        if (w > 1)
            w >>= 1;

        if (h > 1)
            h >>= 1;

        mipNum--;
    }

    const FormatProps& formatProps = GetFormatProps(textureDesc.format);
    size *= formatProps.stride;
    size *= std::max(textureDesc.sampleNum, (Sample_t)1);
    size *= std::max(textureDesc.layerNum, (Dim_t)1);

    return size;
}
