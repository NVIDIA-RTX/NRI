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

static inline uint32_t GetPlaneIndex(Format format) { // TODO: still unclear, is it needed for a stencil-only SRV?
    // https://microsoft.github.io/DirectX-Specs/d3d/PlanarDepthStencilDDISpec.html
    switch (format) {
        case Format::X32_G8_UINT_X24:
        case Format::X24_G8_UINT:
            return 1;

        default:
            return 0;
    }
}

Result DescriptorD3D12::Create(const Texture1DViewDesc& textureViewDesc) {
    const TextureD3D12& texture = (TextureD3D12&)*textureViewDesc.texture;
    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;

    const TextureDesc& textureDesc = texture.GetDesc();
    Dim_t remainingMips = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t remainingLayers = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    switch (textureViewDesc.viewType) {
        case Texture1DViewType::SHADER_RESOURCE_1D: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Texture1D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1D.MipLevels = remainingMips;

            return CreateShaderResourceView(texture, desc);
        }
        case Texture1DViewType::SHADER_RESOURCE_1D_ARRAY: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Texture1DArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1DArray.MipLevels = remainingMips;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;

            return CreateShaderResourceView(texture, desc);
        }
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MipSlice = textureViewDesc.mipOffset;

            return CreateUnorderedAccessView(texture, desc, textureViewDesc.format);
        }
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D_ARRAY: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;

            return CreateUnorderedAccessView(texture, desc, textureViewDesc.format);
        }
        case Texture1DViewType::COLOR_ATTACHMENT: {
            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;

            return CreateRenderTargetView(texture, desc);
        }
        case Texture1DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture1DViewType::DEPTH_STENCIL_READONLY: {
            D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = remainingLayers;

            if (textureViewDesc.viewType == Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT)
                desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
            else if (textureViewDesc.viewType == Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY)
                desc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
            else if (textureViewDesc.viewType == Texture1DViewType::DEPTH_STENCIL_READONLY)
                desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;

            return CreateDepthStencilView(texture, desc);
        }
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const Texture2DViewDesc& textureViewDesc) {
    const TextureD3D12& texture = (TextureD3D12&)*textureViewDesc.texture;
    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;

    const TextureDesc& textureDesc = texture.GetDesc();
    Dim_t remainingMips = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t remainingLayers = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    switch (textureViewDesc.viewType) {
        case Texture2DViewType::SHADER_RESOURCE_2D: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            } else {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2D.MipLevels = remainingMips;
                desc.Texture2D.PlaneSlice = GetPlaneIndex(textureViewDesc.format);
            }

            return CreateShaderResourceView(texture, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_2D_ARRAY: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = remainingLayers;
            } else {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2DArray.MipLevels = remainingMips;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = remainingLayers;
                desc.Texture2DArray.PlaneSlice = GetPlaneIndex(textureViewDesc.format);
            }

            return CreateShaderResourceView(texture, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_CUBE: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            desc.TextureCube.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCube.MipLevels = remainingMips;

            return CreateShaderResourceView(texture, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            desc.TextureCubeArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCubeArray.MipLevels = remainingMips;
            desc.TextureCubeArray.First2DArrayFace = textureViewDesc.layerOffset;
            desc.TextureCubeArray.NumCubes = remainingLayers / 6;

            return CreateShaderResourceView(texture, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture2D.PlaneSlice = GetPlaneIndex(textureViewDesc.format);

            return CreateUnorderedAccessView(texture, desc, textureViewDesc.format);
        }
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D_ARRAY: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture2DArray.ArraySize = remainingLayers;
            desc.Texture2DArray.PlaneSlice = GetPlaneIndex(textureViewDesc.format);

            return CreateUnorderedAccessView(texture, desc, textureViewDesc.format);
        }
        case Texture2DViewType::COLOR_ATTACHMENT: {
            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = format;
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = remainingLayers;
            } else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = remainingLayers;
                desc.Texture2DArray.PlaneSlice = GetPlaneIndex(textureViewDesc.format);
            }

            return CreateRenderTargetView(texture, desc);
        }
        case Texture2DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture2DViewType::DEPTH_STENCIL_READONLY: {
            D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format = format;
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = remainingLayers;
            } else {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = remainingLayers;
            }

            if (textureViewDesc.viewType == Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT)
                desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
            else if (textureViewDesc.viewType == Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY)
                desc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
            else if (textureViewDesc.viewType == Texture2DViewType::DEPTH_STENCIL_READONLY)
                desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;

            return CreateDepthStencilView(texture, desc);
        }
        case Texture2DViewType::SHADING_RATE_ATTACHMENT: {
            m_Resource = texture; // a resource view is not needed
            return Result::SUCCESS;
        }
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const Texture3DViewDesc& textureViewDesc) {
    const TextureD3D12& texture = (TextureD3D12&)*textureViewDesc.texture;
    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;

    const TextureDesc& textureDesc = texture.GetDesc();
    Dim_t remainingMips = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;

    switch (textureViewDesc.viewType) {
        case Texture3DViewType::SHADER_RESOURCE_3D: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Texture3D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture3D.MipLevels = remainingMips;

            return CreateShaderResourceView(texture, desc);
        }
        case Texture3DViewType::SHADER_RESOURCE_STORAGE_3D: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = textureViewDesc.sliceNum;

            return CreateUnorderedAccessView(texture, desc, textureViewDesc.format);
        }
        case Texture3DViewType::COLOR_ATTACHMENT: {
            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = textureViewDesc.sliceNum;

            return CreateRenderTargetView(texture, desc);
        }
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferD3D12& buffer = *((BufferD3D12*)bufferViewDesc.buffer);
    const BufferDesc& bufferDesc = buffer.GetDesc();
    uint64_t size = bufferViewDesc.size == WHOLE_SIZE ? bufferDesc.size : bufferViewDesc.size;

    uint32_t structureStride = 0;
    if (bufferViewDesc.format == Format::UNKNOWN)
        structureStride = bufferViewDesc.structureStride ? bufferViewDesc.structureStride : bufferDesc.structureStride;
    bool isRaw = structureStride == 4;

    Format patchedFormat = isRaw ? Format::R32_UINT : bufferViewDesc.format;
    const DxgiFormat& format = GetDxgiFormat(patchedFormat);
    const FormatProps& formatProps = GetFormatProps(patchedFormat);
    uint32_t elementSize = structureStride ? structureStride : formatProps.stride;
    uint64_t elementOffset = (uint32_t)(bufferViewDesc.offset / elementSize);
    uint32_t elementNum = (uint32_t)(size / elementSize);

    m_BufferLocation = buffer.GetGPUVA() + bufferViewDesc.offset;
    m_BufferViewType = bufferViewDesc.viewType;

    switch (bufferViewDesc.viewType) {
        case BufferViewType::CONSTANT: {
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
            desc.BufferLocation = m_BufferLocation;
            desc.SizeInBytes = (uint32_t)size;

            return CreateConstantBufferView(desc);
        }
        case BufferViewType::SHADER_RESOURCE: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Buffer.FirstElement = elementOffset;
            desc.Buffer.NumElements = elementNum;
            desc.Buffer.StructureByteStride = isRaw ? 0 : structureStride;
            desc.Buffer.Flags = isRaw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

            return CreateShaderResourceView(buffer, desc);
        }
        case BufferViewType::SHADER_RESOURCE_STORAGE: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = elementOffset;
            desc.Buffer.NumElements = elementNum;
            desc.Buffer.StructureByteStride = isRaw ? 0 : structureStride;
            desc.Buffer.CounterOffsetInBytes = 0; // TODO: needed?
            desc.Buffer.Flags = isRaw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

            if (isRaw || structureStride)
                m_IsIntegerFormat = true;

            return CreateUnorderedAccessView(buffer, desc, bufferViewDesc.format);
        }
        default:
            CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const AccelerationStructureD3D12& accelerationStructure) {
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.RaytracingAccelerationStructure.Location = accelerationStructure.GetHandle();

    m_BufferLocation = desc.RaytracingAccelerationStructure.Location;
    m_IsAccelerationStructure = true;

    return CreateShaderResourceView(nullptr, desc);
}

Result DescriptorD3D12::Create(const SamplerDesc& samplerDesc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_Handle);
    if (result != Result::SUCCESS)
        return result;

    m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_SAMPLER_DESC2 desc = {};
    desc.Filter = GetFilter(samplerDesc);
    desc.AddressU = GetAddressMode(samplerDesc.addressModes.u);
    desc.AddressV = GetAddressMode(samplerDesc.addressModes.v);
    desc.AddressW = GetAddressMode(samplerDesc.addressModes.w);
    desc.MipLODBias = samplerDesc.mipBias;
    desc.MaxAnisotropy = samplerDesc.anisotropy;
    desc.ComparisonFunc = GetCompareOp(samplerDesc.compareOp);
    desc.MinLOD = samplerDesc.mipMin;
    desc.MaxLOD = samplerDesc.mipMax;

    if (samplerDesc.unnormalizedCoordinates)
        desc.Flags |= D3D12_SAMPLER_FLAG_NON_NORMALIZED_COORDINATES;

    if (samplerDesc.isInteger) {
        desc.UintBorderColor[0] = samplerDesc.borderColor.ui.x;
        desc.UintBorderColor[1] = samplerDesc.borderColor.ui.y;
        desc.UintBorderColor[2] = samplerDesc.borderColor.ui.z;
        desc.UintBorderColor[3] = samplerDesc.borderColor.ui.w;

        desc.Flags |= D3D12_SAMPLER_FLAG_UINT_BORDER_COLOR;
    } else {
        desc.FloatBorderColor[0] = samplerDesc.borderColor.f.x;
        desc.FloatBorderColor[1] = samplerDesc.borderColor.f.y;
        desc.FloatBorderColor[2] = samplerDesc.borderColor.f.z;
        desc.FloatBorderColor[3] = samplerDesc.borderColor.f.w;
    }

    m_Device->CreateSampler2(&desc, {m_DescriptorHandleCPU});
#else
    D3D12_SAMPLER_DESC desc = {};
    desc.Filter = GetFilter(samplerDesc);
    desc.AddressU = GetAddressMode(samplerDesc.addressModes.u);
    desc.AddressV = GetAddressMode(samplerDesc.addressModes.v);
    desc.AddressW = GetAddressMode(samplerDesc.addressModes.w);
    desc.MipLODBias = samplerDesc.mipBias;
    desc.MaxAnisotropy = samplerDesc.anisotropy;
    desc.ComparisonFunc = GetCompareOp(samplerDesc.compareOp);
    desc.MinLOD = samplerDesc.mipMin;
    desc.MaxLOD = samplerDesc.mipMax;

    if (!samplerDesc.isInteger) { // TODO: the spec is not clear about the behavior, keep black
        desc.BorderColor[0] = samplerDesc.borderColor.f.x;
        desc.BorderColor[1] = samplerDesc.borderColor.f.y;
        desc.BorderColor[2] = samplerDesc.borderColor.f.z;
        desc.BorderColor[3] = samplerDesc.borderColor.f.w;
    }

    m_Device->CreateSampler(&desc, {m_DescriptorHandleCPU});
#endif

    return Result::SUCCESS;
}

Result DescriptorD3D12::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Handle);
    if (result == Result::SUCCESS) {
        m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
        m_Device->CreateConstantBufferView(&desc, {m_DescriptorHandleCPU});
    }

    return result;
}

Result DescriptorD3D12::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Handle);
    if (result == Result::SUCCESS) {
        m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
        m_Device->CreateShaderResourceView(resource, &desc, {m_DescriptorHandleCPU});
        m_Resource = resource;
    }

    return result;
}

Result DescriptorD3D12::CreateUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, Format format) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Handle);
    if (result == Result::SUCCESS) {
        m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
        m_Device->CreateUnorderedAccessView(resource, nullptr, &desc, {m_DescriptorHandleCPU});
        m_Resource = resource;
        m_IsIntegerFormat = GetFormatProps(format).isInteger;
    }

    return result;
}

Result DescriptorD3D12::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_Handle);
    if (result == Result::SUCCESS) {
        m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
        m_Device->CreateRenderTargetView(resource, &desc, {m_DescriptorHandleCPU});
        m_Resource = resource;
    }

    return result;
}

Result DescriptorD3D12::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_Handle);
    if (result == Result::SUCCESS) {
        m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
        m_Device->CreateDepthStencilView(resource, &desc, {m_DescriptorHandleCPU});
        m_Resource = resource;
    }

    return result;
}
