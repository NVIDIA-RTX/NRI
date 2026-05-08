// © 2021 NVIDIA Corporation

static inline DXGI_FORMAT GetPatchedShaderResourceViewFormat(DXGI_FORMAT format, PlaneBits planes) {
    switch (format) {
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return (planes & PlaneBits::STENCIL) ? DXGI_FORMAT_X24_TYPELESS_G8_UINT : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return (planes & PlaneBits::STENCIL) ? DXGI_FORMAT_X32_TYPELESS_G8X24_UINT : DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:
            return format;
    }
}

Result DescriptorD3D11::Create(const TextureViewDesc& textureViewDesc) {
    const TextureD3D11& textureD3D11 = *(TextureD3D11*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureD3D11.GetDesc();

    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;
    Dim_t sliceNum = textureViewDesc.sliceNum == REMAINING ? (textureDesc.depth - textureViewDesc.sliceOffset) : textureViewDesc.sliceNum;

    HRESULT hr = E_INVALIDARG;

    if (textureDesc.type == TextureType::TEXTURE_1D) {
        switch (textureViewDesc.type) {
            case TextureView::TEXTURE: {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture1D.MipLevels = mipNum;
                desc.Format = format;

                hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
            } break;
            case TextureView::TEXTURE_ARRAY: {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture1DArray.MipLevels = mipNum;
                desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture1DArray.ArraySize = layerNum;
                desc.Format = format;

                hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
            } break;
            case TextureView::STORAGE_TEXTURE: {
                D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = textureViewDesc.mipOffset;
                desc.Format = format;

                hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
            } break;
            case TextureView::STORAGE_TEXTURE_ARRAY: {
                D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture1DArray.ArraySize = layerNum;
                desc.Format = format;

                hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
            } break;
            case TextureView::COLOR_ATTACHMENT: {
                D3D11_RENDER_TARGET_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture1DArray.ArraySize = layerNum;
                desc.Format = format;

                hr = m_Device->CreateRenderTargetView(textureD3D11, &desc, (ID3D11RenderTargetView**)&m_Descriptor);
            } break;
            case TextureView::DEPTH_STENCIL_ATTACHMENT: {
                D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture1DArray.ArraySize = layerNum;
                desc.Format = format;

                bool isDepthReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::DEPTH) == 0;
                bool isStencilReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::STENCIL) == 0;
                if (isDepthReadonly)
                    desc.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
                if (isStencilReadonly)
                    desc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;

                hr = m_Device->CreateDepthStencilView(textureD3D11, &desc, (ID3D11DepthStencilView**)&m_Descriptor);
            } break;
            default:
                NRI_CHECK(false, "Unexpected 'textureViewDesc.type'");
                return Result::INVALID_ARGUMENT;
        }
    } else if (textureDesc.type == TextureType::TEXTURE_2D) {
        switch (textureViewDesc.type) {
            case TextureView::SUBPASS_INPUT:
            case TextureView::TEXTURE: {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
                if (textureDesc.sampleNum > 1)
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                else {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MostDetailedMip = textureViewDesc.mipOffset;
                    desc.Texture2D.MipLevels = mipNum;
                }
                desc.Format = GetPatchedShaderResourceViewFormat(format, textureViewDesc.planes);

                hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
            } break;
            case TextureView::TEXTURE_ARRAY: {
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
                desc.Format = GetPatchedShaderResourceViewFormat(format, textureViewDesc.planes);

                hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
            } break;
            case TextureView::TEXTURE_CUBE: {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                desc.TextureCube.MostDetailedMip = textureViewDesc.mipOffset;
                desc.TextureCube.MipLevels = mipNum;
                desc.Format = GetPatchedShaderResourceViewFormat(format, textureViewDesc.planes);

                hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
            } break;
            case TextureView::TEXTURE_CUBE_ARRAY: {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                desc.TextureCubeArray.MostDetailedMip = textureViewDesc.mipOffset;
                desc.TextureCubeArray.MipLevels = mipNum;
                desc.TextureCubeArray.First2DArrayFace = textureViewDesc.layerOffset;
                desc.TextureCubeArray.NumCubes = textureViewDesc.layerNum / 6;
                desc.Format = GetPatchedShaderResourceViewFormat(format, textureViewDesc.planes);

                hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
            } break;
            case TextureView::STORAGE_TEXTURE: {
                D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = textureViewDesc.mipOffset;
                desc.Format = format;

                hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
            } break;
            case TextureView::STORAGE_TEXTURE_ARRAY: {
                D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = layerNum;
                desc.Format = format;

                hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
            } break;
            case TextureView::COLOR_ATTACHMENT: {
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
            case TextureView::DEPTH_STENCIL_ATTACHMENT: {
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

                bool isDepthReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::DEPTH) == 0;
                bool isStencilReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::STENCIL) == 0;
                if (isDepthReadonly)
                    desc.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
                if (isStencilReadonly)
                    desc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;

                hr = m_Device->CreateDepthStencilView(textureD3D11, &desc, (ID3D11DepthStencilView**)&m_Descriptor);
            } break;
            case TextureView::SHADING_RATE_ATTACHMENT: {
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
                NRI_CHECK(false, "Unexpected 'textureViewDesc.type'");
                return Result::INVALID_ARGUMENT;
        }
    } else {
        switch (textureViewDesc.type) {
            case TextureView::TEXTURE: {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                desc.Texture3D.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture3D.MipLevels = mipNum;
                desc.Format = format;

                hr = m_Device->CreateShaderResourceView(textureD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
            } break;
            case TextureView::STORAGE_TEXTURE: {
                D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
                desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
                desc.Texture3D.WSize = sliceNum;
                desc.Format = format;

                hr = m_Device->CreateUnorderedAccessView(textureD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
            } break;
            case TextureView::COLOR_ATTACHMENT: {
                D3D11_RENDER_TARGET_VIEW_DESC desc = {};
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
                desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
                desc.Texture3D.WSize = sliceNum;
                desc.Format = format;

                hr = m_Device->CreateRenderTargetView(textureD3D11, &desc, (ID3D11RenderTargetView**)&m_Descriptor);
            } break;
            default:
                NRI_CHECK(false, "Unexpected 'textureViewDesc.type'");
                return Result::INVALID_ARGUMENT;
        }
    }

    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    m_Format = textureViewDesc.format;
    m_SubresourceInfo.Initialize(&textureD3D11, textureViewDesc.mipOffset, mipNum, textureViewDesc.layerOffset, layerNum);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferD3D11& bufferD3D11 = *(BufferD3D11*)bufferViewDesc.buffer;
    const BufferDesc& bufferDesc = bufferD3D11.GetDesc();
    uint64_t size = bufferViewDesc.size == WHOLE_SIZE ? bufferDesc.size : bufferViewDesc.size;

    Format patchedFormat = Format::UNKNOWN;
    uint32_t structureStride = 0;
    bool isRaw = false;

    if (bufferViewDesc.type == BufferView::CONSTANT_BUFFER) {
        patchedFormat = Format::RGBA32_SFLOAT;

        if (bufferViewDesc.offset != 0 && m_Device.GetVersion() == 0)
            NRI_REPORT_ERROR(&m_Device, "Constant buffers with non-zero offsets require 11.1+ feature level!");
    } else if (bufferViewDesc.type == BufferView::STRUCTURED_BUFFER || bufferViewDesc.type == BufferView::STORAGE_STRUCTURED_BUFFER) {
        if (bufferViewDesc.structureStride != bufferDesc.structureStride) {
            // D3D11 requires "structureStride" passed during creation, but we violate the spec and treat "structured" buffers as "raw" to allow multiple views creation for a single buffer // TODO: this may not work on some HW!
            patchedFormat = Format::R32_UINT;
            isRaw = true;
        } else
            structureStride = bufferDesc.structureStride;
    } else if (bufferViewDesc.type == BufferView::BYTE_ADDRESS_BUFFER || bufferViewDesc.type == BufferView::STORAGE_BYTE_ADDRESS_BUFFER) {
        patchedFormat = Format::R32_UINT;
        isRaw = true;
    } else
        patchedFormat = bufferViewDesc.format;

    const DxgiFormat& format = GetDxgiFormat(patchedFormat);
    const FormatProps& formatProps = GetFormatProps(patchedFormat);
    uint32_t elementSize = structureStride ? structureStride : formatProps.stride;
    uint32_t elementOffset = (uint32_t)(bufferViewDesc.offset / elementSize);
    uint32_t elementNum = (uint32_t)(size / elementSize);

    HRESULT hr = E_INVALIDARG;
    switch (bufferViewDesc.type) {
        case BufferView::CONSTANT_BUFFER: {
            m_Descriptor = bufferD3D11;
            hr = S_OK;
        } break;
        case BufferView::BUFFER:
        case BufferView::STRUCTURED_BUFFER:
        case BufferView::BYTE_ADDRESS_BUFFER: {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
            desc.BufferEx.FirstElement = elementOffset;
            desc.BufferEx.NumElements = elementNum;
            desc.BufferEx.Flags = isRaw ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;

            hr = m_Device->CreateShaderResourceView(bufferD3D11, &desc, (ID3D11ShaderResourceView**)&m_Descriptor);
        } break;
        case BufferView::STORAGE_BUFFER:
        case BufferView::STORAGE_STRUCTURED_BUFFER:
        case BufferView::STORAGE_BYTE_ADDRESS_BUFFER: {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = elementOffset;
            desc.Buffer.NumElements = elementNum;
            desc.Buffer.Flags = isRaw ? D3D11_BUFFER_UAV_FLAG_RAW : 0;

            hr = m_Device->CreateUnorderedAccessView(bufferD3D11, &desc, (ID3D11UnorderedAccessView**)&m_Descriptor);
        } break;
        default:
            NRI_CHECK(false, "Unexpected 'bufferViewDesc.type'");
            return Result::INVALID_ARGUMENT;
    };

    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateXxxView");

    m_Format = patchedFormat;
    m_SubresourceInfo.Initialize(&bufferD3D11, elementOffset, elementNum);

    return Result::SUCCESS;
}

Result DescriptorD3D11::Create(const SamplerDesc& samplerDesc) {
    D3D11_SAMPLER_DESC desc = {};
    FillSamplerDesc(samplerDesc, desc);

    HRESULT hr = m_Device->CreateSamplerState(&desc, (ID3D11SamplerState**)&m_Descriptor);
    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateSamplerState");

    return Result::SUCCESS;
}
