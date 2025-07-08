// © 2021 NVIDIA Corporation

static inline D3D11_TEXTURE_ADDRESS_MODE GetAddressMode(AddressMode mode) {
    return (D3D11_TEXTURE_ADDRESS_MODE)(D3D11_TEXTURE_ADDRESS_WRAP + (uint32_t)mode);
}

static inline D3D11_FILTER GetFilterIsotropic(Filter mip, Filter magnification, Filter minification, ReductionMode filterExt, bool useComparison) {
    uint32_t combinedMask = mip == Filter::LINEAR ? 0x1 : 0;
    combinedMask |= magnification == Filter::LINEAR ? 0x4 : 0;
    combinedMask |= minification == Filter::LINEAR ? 0x10 : 0;

    if (useComparison)
        combinedMask |= 0x80;
    else if (filterExt == ReductionMode::MIN)
        combinedMask |= 0x100;
    else if (filterExt == ReductionMode::MAX)
        combinedMask |= 0x180;

    return (D3D11_FILTER)combinedMask;
}

static inline DXGI_FORMAT GetShaderFormatForDepth(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:
            return format;
    }
}

static inline D3D11_FILTER GetFilterAnisotropic(ReductionMode filterExt, bool useComparison) {
    if (filterExt == ReductionMode::MIN)
        return D3D11_FILTER_MINIMUM_ANISOTROPIC;
    else if (filterExt == ReductionMode::MAX)
        return D3D11_FILTER_MAXIMUM_ANISOTROPIC;

    return useComparison ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
}

Result DescriptorD3D11::Create(const Texture1DViewDesc& textureViewDesc) {
    const TextureD3D11& texture = *(TextureD3D11*)textureViewDesc.texture;
    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;

    const TextureDesc& textureDesc = texture.GetDesc();
    Dim_t remainingMips = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t remainingLayers = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    HRESULT hr = E_INVALIDARG;
    switch (textureViewDesc.viewType) {
        case Texture1DViewType::SHADER_RESOURCE_1D: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1D.MipLevels = remainingMips;
            desc.Format = format;

            hr = m_Device->CreateShaderResourceView(texture, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;
        } break;
        case Texture1DViewType::SHADER_RESOURCE_1D_ARRAY: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1DArray.MipLevels = remainingMips;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;
            desc.Format = format;

            hr = m_Device->CreateShaderResourceView(texture, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;
        } break;
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MipSlice = textureViewDesc.mipOffset;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(texture, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::STORAGE;
        } break;
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D_ARRAY: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(texture, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::STORAGE;
        } break;
        case Texture1DViewType::COLOR_ATTACHMENT: {
            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;
            desc.Format = format;

            hr = m_Device->CreateRenderTargetView(texture, &desc, (ID3D11RenderTargetView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;
        } break;
        case Texture1DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture1DViewType::DEPTH_STENCIL_READONLY: {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;
            desc.Format = format;

            if (textureViewDesc.viewType == Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
            else if (textureViewDesc.viewType == Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_STENCIL;
            else if (textureViewDesc.viewType == Texture1DViewType::DEPTH_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

            hr = m_Device->CreateDepthStencilView(texture, &desc, (ID3D11DepthStencilView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;
        } break;
    };

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    const FormatProps& formatProps = GetFormatProps(textureViewDesc.format);
    m_IsIntegerFormat = formatProps.isInteger;
    m_SubresourceInfo.Initialize(textureViewDesc.texture, textureViewDesc.mipOffset, remainingMips, textureViewDesc.layerOffset, remainingLayers);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const Texture2DViewDesc& textureViewDesc) {
    const TextureD3D11& texture = *(TextureD3D11*)textureViewDesc.texture;
    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;

    const TextureDesc& textureDesc = texture.GetDesc();
    Dim_t remainingMips = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t remainingLayers = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    HRESULT hr = E_INVALIDARG;
    switch (textureViewDesc.viewType) {
        case Texture2DViewType::SHADER_RESOURCE_2D: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1)
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            else {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2D.MipLevels = remainingMips;
            }
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(texture, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;

            break;
        }
        case Texture2DViewType::SHADER_RESOURCE_2D_ARRAY: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = remainingLayers;
            } else {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2DArray.MipLevels = remainingMips;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = remainingLayers;
            }
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(texture, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;

            break;
        }
        case Texture2DViewType::SHADER_RESOURCE_CUBE: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            desc.TextureCube.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCube.MipLevels = remainingMips;
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(texture, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;

            break;
        }
        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            desc.TextureCubeArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCubeArray.MipLevels = remainingMips;
            desc.TextureCubeArray.First2DArrayFace = textureViewDesc.layerOffset;
            desc.TextureCubeArray.NumCubes = textureViewDesc.layerNum / 6;
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(texture, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;

            break;
        }
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = textureViewDesc.mipOffset;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(texture, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::STORAGE;

            break;
        }
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D_ARRAY: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture2DArray.ArraySize = remainingLayers;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(texture, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::STORAGE;

            break;
        }
        case Texture2DViewType::COLOR_ATTACHMENT: {
            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = remainingLayers;
            } else {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = remainingLayers;
            }
            desc.Format = format;

            hr = m_Device->CreateRenderTargetView(texture, &desc, (ID3D11RenderTargetView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;

            break;
        }
        case Texture2DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture2DViewType::DEPTH_STENCIL_READONLY: {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = remainingLayers;
            } else {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = remainingLayers;
            }
            desc.Format = format;

            if (textureViewDesc.viewType == Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
            else if (textureViewDesc.viewType == Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_STENCIL;
            else if (textureViewDesc.viewType == Texture2DViewType::DEPTH_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

            hr = m_Device->CreateDepthStencilView(texture, &desc, (ID3D11DepthStencilView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;

            break;
        }
        case Texture2DViewType::SHADING_RATE_ATTACHMENT: {
#if NRI_ENABLE_D3D_EXTENSIONS
            if (m_Device.HasNvExt()) {
                NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC desc = {NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC_VER};
                desc.Format = format;
                desc.ViewDimension = NV_SRRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;

                NvAPI_Status status = NvAPI_D3D11_CreateShadingRateResourceView(m_Device.GetNativeObject(), texture, &desc, (ID3D11NvShadingRateResourceView**)&m_Descriptor);
                if (status == NVAPI_OK)
                    hr = S_OK;

                m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;
            }
#endif

            break;
        }
    };

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    const FormatProps& formatProps = GetFormatProps(textureViewDesc.format);
    m_IsIntegerFormat = formatProps.isInteger;
    m_SubresourceInfo.Initialize(textureViewDesc.texture, textureViewDesc.mipOffset, remainingMips, textureViewDesc.layerOffset, remainingLayers);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const Texture3DViewDesc& textureViewDesc) {
    const TextureD3D11& texture = *(TextureD3D11*)textureViewDesc.texture;
    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;

    const TextureDesc& textureDesc = texture.GetDesc();
    Dim_t remainingMips = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;

    HRESULT hr = E_INVALIDARG;
    switch (textureViewDesc.viewType) {
        case Texture3DViewType::SHADER_RESOURCE_3D: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture3D.MipLevels = remainingMips;
            desc.Format = format;

            hr = m_Device->CreateShaderResourceView(texture, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;
        } break;
        case Texture3DViewType::SHADER_RESOURCE_STORAGE_3D: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = textureViewDesc.sliceNum;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(texture, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::STORAGE;
        } break;
        case Texture3DViewType::COLOR_ATTACHMENT: {
            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = textureViewDesc.sliceNum;
            desc.Format = format;

            hr = m_Device->CreateRenderTargetView(texture, &desc, (ID3D11RenderTargetView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;
        } break;
    };

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    const FormatProps& formatProps = GetFormatProps(textureViewDesc.format);
    m_IsIntegerFormat = formatProps.isInteger;
    m_SubresourceInfo.Initialize(textureViewDesc.texture, textureViewDesc.mipOffset, remainingMips, textureViewDesc.sliceOffset, textureViewDesc.sliceNum);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferD3D11& buffer = *(BufferD3D11*)bufferViewDesc.buffer;
    const BufferDesc& bufferDesc = buffer.GetDesc();
    uint64_t size = bufferViewDesc.size == WHOLE_SIZE ? bufferDesc.size : bufferViewDesc.size;

    // D3D11 requires "structureStride" passed during creation, but we violate the spec and treat "structured" buffers as "raw" to allow multiple views creation for a single buffer
    uint32_t structureStride = 0;
    if (bufferViewDesc.format == Format::UNKNOWN)
        structureStride = bufferDesc.structureStride == 4 ? 4 : bufferViewDesc.structureStride;
    bool isRaw = structureStride == 4;

    Format patchedFormat = bufferViewDesc.format;
    if (bufferViewDesc.viewType == BufferViewType::CONSTANT) {
        patchedFormat = Format::RGBA32_SFLOAT;

        if (bufferViewDesc.offset != 0 && m_Device.GetVersion() == 0)
            REPORT_ERROR(&m_Device, "Constant buffers with non-zero offsets require 11.1+ feature level!");
    } else if (structureStride)
        patchedFormat = isRaw ? Format::R32_UINT : Format::UNKNOWN;

    const DxgiFormat& format = GetDxgiFormat(patchedFormat);
    const FormatProps& formatProps = GetFormatProps(patchedFormat);
    uint32_t elementSize = structureStride ? structureStride : formatProps.stride;
    m_ElementOffset = (uint32_t)(bufferViewDesc.offset / elementSize);
    m_ElementNum = (uint32_t)(size / elementSize);

    HRESULT hr = E_INVALIDARG;
    switch (bufferViewDesc.viewType) {
        case BufferViewType::CONSTANT: {
            m_Descriptor = buffer;
            hr = S_OK;
            m_Type = DescriptorTypeDX11::CONSTANT;
        } break;
        case BufferViewType::SHADER_RESOURCE: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
            desc.BufferEx.FirstElement = m_ElementOffset;
            desc.BufferEx.NumElements = m_ElementNum;
            desc.BufferEx.Flags = isRaw ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;

            hr = m_Device->CreateShaderResourceView(buffer, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::RESOURCE;
        } break;
        case BufferViewType::SHADER_RESOURCE_STORAGE: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = m_ElementOffset;
            desc.Buffer.NumElements = m_ElementNum;
            desc.Buffer.Flags = isRaw ? D3D11_BUFFER_UAV_FLAG_RAW : 0;

            hr = m_Device->CreateUnorderedAccessView(buffer, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);

            m_Type = DescriptorTypeDX11::STORAGE;
        } break;
    };

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    m_IsIntegerFormat = formatProps.isInteger;
    m_SubresourceInfo.Initialize(bufferViewDesc.buffer);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const SamplerDesc& samplerDesc) {
    bool isAnisotropy = samplerDesc.anisotropy > 1;
    bool isComparison = samplerDesc.compareOp != CompareOp::NONE;

    D3D11_SAMPLER_DESC desc = {};
    desc.AddressU = GetAddressMode(samplerDesc.addressModes.u);
    desc.AddressV = GetAddressMode(samplerDesc.addressModes.v);
    desc.AddressW = GetAddressMode(samplerDesc.addressModes.w);
    desc.MipLODBias = samplerDesc.mipBias;
    desc.MaxAnisotropy = samplerDesc.anisotropy;
    desc.ComparisonFunc = GetD3D11ComparisonFuncFromCompareOp(samplerDesc.compareOp);
    desc.MinLOD = samplerDesc.mipMin;
    desc.MaxLOD = samplerDesc.mipMax;
    desc.Filter = isAnisotropy
        ? GetFilterAnisotropic(samplerDesc.filters.ext, isComparison)
        : GetFilterIsotropic(samplerDesc.filters.mip, samplerDesc.filters.mag, samplerDesc.filters.min, samplerDesc.filters.ext, isComparison);

    if (!samplerDesc.isInteger) { // TODO: the spec is not clear about the behavior, keep black
        desc.BorderColor[0] = samplerDesc.borderColor.f.x;
        desc.BorderColor[1] = samplerDesc.borderColor.f.y;
        desc.BorderColor[2] = samplerDesc.borderColor.f.z;
        desc.BorderColor[3] = samplerDesc.borderColor.f.w;
    }

    HRESULT hr = m_Device->CreateSamplerState(&desc, (ID3D11SamplerState**)&m_Descriptor);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateSamplerState");

    m_Type = DescriptorTypeDX11::SAMPLER;

    return m_Descriptor ? Result::SUCCESS : Result::FAILURE;
}

DescriptorD3D11::DescriptorD3D11(DeviceD3D11& device, ID3D11ShaderResourceView* resource)
    : m_Descriptor(resource)
    , m_Device(device) {
    m_Type = DescriptorTypeDX11::RESOURCE;
}

DescriptorD3D11::DescriptorD3D11(DeviceD3D11& device, ID3D11UnorderedAccessView* storage)
    : m_Descriptor(storage)
    , m_Device(device) {
    m_Type = DescriptorTypeDX11::STORAGE;
}

DescriptorD3D11::DescriptorD3D11(DeviceD3D11& device, ID3D11RenderTargetView* randerTarget)
    : m_Descriptor(randerTarget)
    , m_Device(device) {
    m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;
}

DescriptorD3D11::DescriptorD3D11(DeviceD3D11& device, ID3D11DepthStencilView* depthStencil)
    : m_Descriptor(depthStencil)
    , m_Device(device) {
    m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;
}

DescriptorD3D11::DescriptorD3D11(DeviceD3D11& device, ID3D11Buffer* constantBuffer, uint32_t elementOffset, uint32_t elementNum)
    : m_Descriptor(constantBuffer)
    , m_ElementOffset(elementOffset)
    , m_ElementNum(elementNum)
    , m_Device(device) {
    m_Type = DescriptorTypeDX11::CONSTANT;
}

DescriptorD3D11::DescriptorD3D11(DeviceD3D11& device, ID3D11SamplerState* sampler)
    : m_Descriptor(sampler)
    , m_Device(device) {
    m_Type = DescriptorTypeDX11::SAMPLER;
}
