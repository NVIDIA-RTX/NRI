// © 2021 NVIDIA Corporation

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

Result DescriptorD3D11::Create(const Texture1DViewDesc& textureViewDesc) {
    const TextureD3D11& textureD3D11 = *(TextureD3D11*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureD3D11.GetDesc();

    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    HRESULT hr = E_INVALIDARG;
    switch (textureViewDesc.viewType) {
        case Texture1DViewType::SHADER_RESOURCE_1D: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1D.MipLevels = mipNum;
            desc.Format = format;

            hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case Texture1DViewType::SHADER_RESOURCE_1D_ARRAY: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1DArray.MipLevels = mipNum;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;
            desc.Format = format;

            hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MipSlice = textureViewDesc.mipOffset;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
        } break;
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D_ARRAY: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
        } break;
        case Texture1DViewType::COLOR_ATTACHMENT: {
            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;
            desc.Format = format;

            hr = m_Device->CreateRenderTargetView(textureD3D11, &desc, (ID3D11RenderTargetView**)&m_Descriptor);
        } break;
        case Texture1DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture1DViewType::DEPTH_STENCIL_READONLY: {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;
            desc.Format = format;

            if (textureViewDesc.viewType == Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
            else if (textureViewDesc.viewType == Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_STENCIL;
            else if (textureViewDesc.viewType == Texture1DViewType::DEPTH_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

            hr = m_Device->CreateDepthStencilView(textureD3D11, &desc, (ID3D11DepthStencilView**)&m_Descriptor);
        } break;
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    m_Format = textureViewDesc.format;
    m_SubresourceInfo.Initialize(&textureD3D11, textureViewDesc.mipOffset, mipNum, textureViewDesc.layerOffset, layerNum);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const Texture2DViewDesc& textureViewDesc) {
    const TextureD3D11& textureD3D11 = *(TextureD3D11*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureD3D11.GetDesc();

    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    HRESULT hr = E_INVALIDARG;
    switch (textureViewDesc.viewType) {
        case Texture2DViewType::SHADER_RESOURCE_2D: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1)
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            else {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2D.MipLevels = mipNum;
            }
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case Texture2DViewType::SHADER_RESOURCE_2D_ARRAY: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = layerNum;
            } else {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2DArray.MipLevels = mipNum;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = layerNum;
            }
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case Texture2DViewType::SHADER_RESOURCE_CUBE: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            desc.TextureCube.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCube.MipLevels = mipNum;
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            desc.TextureCubeArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCubeArray.MipLevels = mipNum;
            desc.TextureCubeArray.First2DArrayFace = textureViewDesc.layerOffset;
            desc.TextureCubeArray.NumCubes = textureViewDesc.layerNum / 6;
            desc.Format = GetShaderFormatForDepth(format);

            hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = textureViewDesc.mipOffset;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
        } break;
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D_ARRAY: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture2DArray.ArraySize = layerNum;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
        } break;
        case Texture2DViewType::COLOR_ATTACHMENT: {
            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = layerNum;
            } else {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = layerNum;
            }
            desc.Format = format;

            hr = m_Device->CreateRenderTargetView(textureD3D11, &desc, (ID3D11RenderTargetView**)&m_Descriptor);
        } break;
        case Texture2DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture2DViewType::DEPTH_STENCIL_READONLY: {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = layerNum;
            } else {
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = layerNum;
            }
            desc.Format = format;

            if (textureViewDesc.viewType == Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
            else if (textureViewDesc.viewType == Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_STENCIL;
            else if (textureViewDesc.viewType == Texture2DViewType::DEPTH_STENCIL_READONLY)
                desc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

            hr = m_Device->CreateDepthStencilView(textureD3D11, &desc, (ID3D11DepthStencilView**)&m_Descriptor);
        } break;
        case Texture2DViewType::SHADING_RATE_ATTACHMENT: {
#if NRI_ENABLE_NVAPI
            if (m_Device.HasNvExt()) {
                NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC desc = {NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC_VER};
                desc.Format = format;
                desc.ViewDimension = NV_SRRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;

                NvAPI_Status status = NvAPI_D3D11_CreateShadingRateResourceView(m_Device.GetNativeObject(), textureD3D11, &desc, (ID3D11NvShadingRateResourceView**)&m_Descriptor);
                if (status == NVAPI_OK)
                    hr = S_OK;
            }
#endif

        } break;
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    m_Format = textureViewDesc.format;
    m_SubresourceInfo.Initialize(&textureD3D11, textureViewDesc.mipOffset, mipNum, textureViewDesc.layerOffset, layerNum);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const Texture3DViewDesc& textureViewDesc) {
    const TextureD3D11& textureD3D11 = *(TextureD3D11*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureD3D11.GetDesc();

    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t sliceNum = textureViewDesc.sliceNum == REMAINING ? (textureDesc.depth - textureViewDesc.sliceOffset) : textureViewDesc.sliceNum;

    HRESULT hr = E_INVALIDARG;
    switch (textureViewDesc.viewType) {
        case Texture3DViewType::SHADER_RESOURCE_3D: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture3D.MipLevels = mipNum;
            desc.Format = format;

            hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case Texture3DViewType::SHADER_RESOURCE_STORAGE_3D: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = sliceNum;
            desc.Format = format;

            hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
        } break;
        case Texture3DViewType::COLOR_ATTACHMENT: {
            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = sliceNum;
            desc.Format = format;

            hr = m_Device->CreateRenderTargetView(textureD3D11, &desc, (ID3D11RenderTargetView**)&m_Descriptor);
        } break;
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    m_Format = textureViewDesc.format;
    m_SubresourceInfo.Initialize(&textureD3D11, textureViewDesc.mipOffset, mipNum, 0, 1);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferD3D11& bufferD3D11 = *(BufferD3D11*)bufferViewDesc.buffer;
    const BufferDesc& bufferDesc = bufferD3D11.GetDesc();
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
    uint32_t elementOffset = (uint32_t)(bufferViewDesc.offset / elementSize);
    uint32_t elementNum = (uint32_t)(size / elementSize);

    HRESULT hr = E_INVALIDARG;
    switch (bufferViewDesc.viewType) {
        case BufferViewType::CONSTANT: {
            m_Descriptor = bufferD3D11;
            hr = S_OK;
        } break;
        case BufferViewType::SHADER_RESOURCE: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
            desc.BufferEx.FirstElement = elementOffset;
            desc.BufferEx.NumElements = elementNum;
            desc.BufferEx.Flags = isRaw ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;

            hr = m_Device->CreateShaderResourceView(bufferD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case BufferViewType::SHADER_RESOURCE_STORAGE: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = elementOffset;
            desc.Buffer.NumElements = elementNum;
            desc.Buffer.Flags = isRaw ? D3D11_BUFFER_UAV_FLAG_RAW : 0;

            hr = m_Device->CreateUnorderedAccessView(bufferD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
        } break;
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    };

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    m_Format = patchedFormat;
    m_SubresourceInfo.Initialize(&bufferD3D11, elementOffset, elementNum);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const SamplerDesc& samplerDesc) {
    D3D11_SAMPLER_DESC desc = {};
    FillSamplerDesc(samplerDesc, desc);

    HRESULT hr = m_Device->CreateSamplerState(&desc, (ID3D11SamplerState**)&m_Descriptor);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateSamplerState");

    return Result::SUCCESS;
}
