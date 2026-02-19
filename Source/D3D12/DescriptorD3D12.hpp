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

static inline uint32_t GetComponentSwizzle(ComponentSwizzle componentSwizzle, uint32_t channelIndex) {
    switch (componentSwizzle) {
        case ComponentSwizzle::ZERO:
            return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
        case ComponentSwizzle::ONE:
            return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
        case ComponentSwizzle::R:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
        case ComponentSwizzle::G:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
        case ComponentSwizzle::B:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
        case ComponentSwizzle::A:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
        default:
            return channelIndex;
    }
}

static inline uint32_t GetComponentMapping(const ComponentMapping& componentMapping) {
    uint32_t r = GetComponentSwizzle(componentMapping.r, 0);
    uint32_t g = GetComponentSwizzle(componentMapping.g, 1);
    uint32_t b = GetComponentSwizzle(componentMapping.b, 2);
    uint32_t a = GetComponentSwizzle(componentMapping.a, 3);

    return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(r, g, b, a);
}

Result DescriptorD3D12::Create(const Texture1DViewDesc& textureViewDesc) {
    const TextureD3D12& textureD3D12 = *(TextureD3D12*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureD3D12.GetDesc();

    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.layerOffset = textureViewDesc.layerOffset;
    m_ViewDesc.texture.layerNum = layerNum;
    m_ViewDesc.texture.mipOffset = textureViewDesc.mipOffset;
    m_ViewDesc.texture.mipNum = mipNum;

    m_Format = textureViewDesc.format;
    m_Resource = textureD3D12;

    switch (textureViewDesc.viewType) {
        case Texture1DViewType::SHADER_RESOURCE: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            desc.Shader4ComponentMapping = GetComponentMapping(textureViewDesc.components);
            desc.Texture1D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1D.MipLevels = mipNum;

            return CreateShaderResourceView(textureD3D12, desc);
        }
        case Texture1DViewType::SHADER_RESOURCE_ARRAY: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            desc.Shader4ComponentMapping = GetComponentMapping(textureViewDesc.components);
            desc.Texture1DArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture1DArray.MipLevels = mipNum;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;

            return CreateShaderResourceView(textureD3D12, desc);
        }
        case Texture1DViewType::SHADER_RESOURCE_STORAGE: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MipSlice = textureViewDesc.mipOffset;

            return CreateUnorderedAccessView(textureD3D12, desc);
        }
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_ARRAY: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;

            return CreateUnorderedAccessView(textureD3D12, desc);
        }
        case Texture1DViewType::COLOR_ATTACHMENT: {
            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;

            return CreateRenderTargetView(textureD3D12, desc);
        }
        case Texture1DViewType::DEPTH_STENCIL_ATTACHMENT: {
            D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            desc.Texture1DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture1DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture1DArray.ArraySize = layerNum;

            if (textureViewDesc.readonlyPlanes & PlaneBits::DEPTH)
                desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
            if (textureViewDesc.readonlyPlanes & PlaneBits::STENCIL)
                desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;

            return CreateDepthStencilView(textureD3D12, desc);
        }
        default:
            NRI_CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const Texture2DViewDesc& textureViewDesc) {
    const TextureD3D12& textureD3D12 = (TextureD3D12&)*textureViewDesc.texture;
    const TextureDesc& textureDesc = textureD3D12.GetDesc();

    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.layerOffset = textureViewDesc.layerOffset;
    m_ViewDesc.texture.layerNum = layerNum;
    m_ViewDesc.texture.mipOffset = textureViewDesc.mipOffset;
    m_ViewDesc.texture.mipNum = mipNum;

    m_Format = textureViewDesc.format;
    m_Resource = textureD3D12;

    switch (textureViewDesc.viewType) {
        case Texture2DViewType::INPUT_ATTACHMENT:
        case Texture2DViewType::SHADER_RESOURCE: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = GetComponentMapping(textureViewDesc.components);
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            } else {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2D.MipLevels = mipNum;
                desc.Texture2D.PlaneSlice = GetPlaneIndex(textureViewDesc.format);
            }

            return CreateShaderResourceView(textureD3D12, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_ARRAY: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = GetComponentMapping(textureViewDesc.components);
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = layerNum;
            } else {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MostDetailedMip = textureViewDesc.mipOffset;
                desc.Texture2DArray.MipLevels = mipNum;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = layerNum;
                desc.Texture2DArray.PlaneSlice = GetPlaneIndex(textureViewDesc.format);
            }

            return CreateShaderResourceView(textureD3D12, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_CUBE: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = GetComponentMapping(textureViewDesc.components);
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            desc.TextureCube.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCube.MipLevels = mipNum;

            return CreateShaderResourceView(textureD3D12, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = GetShaderFormatForDepth(format);
            desc.Shader4ComponentMapping = GetComponentMapping(textureViewDesc.components);
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            desc.TextureCubeArray.MostDetailedMip = textureViewDesc.mipOffset;
            desc.TextureCubeArray.MipLevels = mipNum;
            desc.TextureCubeArray.First2DArrayFace = textureViewDesc.layerOffset;
            desc.TextureCubeArray.NumCubes = layerNum / 6;

            return CreateShaderResourceView(textureD3D12, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_STORAGE: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture2D.PlaneSlice = GetPlaneIndex(textureViewDesc.format);

            return CreateUnorderedAccessView(textureD3D12, desc);
        }
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_ARRAY: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
            desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
            desc.Texture2DArray.ArraySize = layerNum;
            desc.Texture2DArray.PlaneSlice = GetPlaneIndex(textureViewDesc.format);

            return CreateUnorderedAccessView(textureD3D12, desc);
        }
        case Texture2DViewType::COLOR_ATTACHMENT: {
            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = format;
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = layerNum;
            } else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = layerNum;
                desc.Texture2DArray.PlaneSlice = GetPlaneIndex(textureViewDesc.format);
            }

            return CreateRenderTargetView(textureD3D12, desc);
        }
        case Texture2DViewType::DEPTH_STENCIL_ATTACHMENT: {
            D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format = format;
            if (textureDesc.sampleNum > 1) {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                desc.Texture2DMSArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DMSArray.ArraySize = layerNum;
            } else {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = textureViewDesc.mipOffset;
                desc.Texture2DArray.FirstArraySlice = textureViewDesc.layerOffset;
                desc.Texture2DArray.ArraySize = layerNum;
            }

            if (textureViewDesc.readonlyPlanes & PlaneBits::DEPTH)
                desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
            if (textureViewDesc.readonlyPlanes & PlaneBits::STENCIL)
                desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;

            return CreateDepthStencilView(textureD3D12, desc);
        }
        case Texture2DViewType::SHADING_RATE_ATTACHMENT:
            return Result::SUCCESS; // a resource view is not needed
        default:
            NRI_CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const Texture3DViewDesc& textureViewDesc) {
    const TextureD3D12& textureD3D12 = (TextureD3D12&)*textureViewDesc.texture;
    const TextureDesc& textureDesc = textureD3D12.GetDesc();

    DXGI_FORMAT format = GetDxgiFormat(textureViewDesc.format).typed;
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t sliceNum = textureViewDesc.sliceNum == REMAINING ? (textureDesc.depth - textureViewDesc.sliceOffset) : textureViewDesc.sliceNum;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.layerOffset = 0;
    m_ViewDesc.texture.layerNum = 1;
    m_ViewDesc.texture.mipOffset = textureViewDesc.mipOffset;
    m_ViewDesc.texture.mipNum = mipNum;

    m_Format = textureViewDesc.format;
    m_Resource = textureD3D12;

    switch (textureViewDesc.viewType) {
        case Texture3DViewType::SHADER_RESOURCE: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            desc.Shader4ComponentMapping = GetComponentMapping(textureViewDesc.components);
            desc.Texture3D.MostDetailedMip = textureViewDesc.mipOffset;
            desc.Texture3D.MipLevels = mipNum;

            return CreateShaderResourceView(textureD3D12, desc);
        }
        case Texture3DViewType::SHADER_RESOURCE_STORAGE: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = sliceNum;

            return CreateUnorderedAccessView(textureD3D12, desc);
        }
        case Texture3DViewType::COLOR_ATTACHMENT: {
            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = textureViewDesc.mipOffset;
            desc.Texture3D.FirstWSlice = textureViewDesc.sliceOffset;
            desc.Texture3D.WSize = sliceNum;

            return CreateRenderTargetView(textureD3D12, desc);
        }
        default:
            NRI_CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferD3D12& bufferD3D12 = *((BufferD3D12*)bufferViewDesc.buffer);
    const BufferDesc& bufferDesc = bufferD3D12.GetDesc();
    uint64_t size = bufferViewDesc.size == WHOLE_SIZE ? bufferDesc.size : bufferViewDesc.size;

    Format patchedFormat = Format::UNKNOWN;
    uint32_t structureStride = 0;
    bool isRaw = false;

    if (bufferViewDesc.viewType == BufferViewType::CONSTANT)
        patchedFormat = Format::RGBA32_SFLOAT;
    else if (bufferViewDesc.viewType == BufferViewType::SHADER_RESOURCE_STRUCTURED || bufferViewDesc.viewType == BufferViewType::SHADER_RESOURCE_STORAGE_STRUCTURED)
        structureStride = bufferViewDesc.structureStride ? bufferViewDesc.structureStride : bufferDesc.structureStride;
    else if (bufferViewDesc.viewType == BufferViewType::SHADER_RESOURCE_BYTE_ADDRESS || bufferViewDesc.viewType == BufferViewType::SHADER_RESOURCE_STORAGE_BYTE_ADDRESS) {
        patchedFormat = Format::R32_UINT;
        isRaw = true;
    } else
        patchedFormat = bufferViewDesc.format;

    const DxgiFormat& format = GetDxgiFormat(patchedFormat);
    const FormatProps& formatProps = GetFormatProps(patchedFormat);
    uint32_t elementSize = structureStride ? structureStride : formatProps.stride;
    uint64_t elementOffset = (uint32_t)(bufferViewDesc.offset / elementSize);
    uint32_t elementNum = (uint32_t)(size / elementSize);

    m_ViewDesc.bufferGPUVA = bufferD3D12.GetDeviceAddress() + bufferViewDesc.offset;

    m_Format = patchedFormat;
    m_Resource = bufferD3D12;

    switch (bufferViewDesc.viewType) {
        case BufferViewType::CONSTANT: {
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
            desc.BufferLocation = m_ViewDesc.bufferGPUVA;
            desc.SizeInBytes = (uint32_t)size;

            return CreateConstantBufferView(desc);
        }
        case BufferViewType::SHADER_RESOURCE:
        case BufferViewType::SHADER_RESOURCE_STRUCTURED:
        case BufferViewType::SHADER_RESOURCE_BYTE_ADDRESS: {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Buffer.FirstElement = elementOffset;
            desc.Buffer.NumElements = elementNum;
            desc.Buffer.StructureByteStride = isRaw ? 0 : structureStride;
            desc.Buffer.Flags = isRaw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

            return CreateShaderResourceView(bufferD3D12, desc);
        }
        case BufferViewType::SHADER_RESOURCE_STORAGE:
        case BufferViewType::SHADER_RESOURCE_STORAGE_STRUCTURED:
        case BufferViewType::SHADER_RESOURCE_STORAGE_BYTE_ADDRESS: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = isRaw ? format.typeless : format.typed;
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = elementOffset;
            desc.Buffer.NumElements = elementNum;
            desc.Buffer.StructureByteStride = isRaw ? 0 : structureStride;
            desc.Buffer.CounterOffsetInBytes = 0; // TODO: needed?
            desc.Buffer.Flags = isRaw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

            return CreateUnorderedAccessView(bufferD3D12, desc);
        }
        default:
            NRI_CHECK(false, "Unexpected");
            return Result::INVALID_ARGUMENT;
    }
}

Result DescriptorD3D12::Create(const AccelerationStructureD3D12& accelerationStructure) {
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.RaytracingAccelerationStructure.Location = accelerationStructure.GetHandle();

    m_ViewDesc.bufferGPUVA = desc.RaytracingAccelerationStructure.Location;

    m_Resource = *accelerationStructure.GetBuffer();

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

    m_Type = DescriptorType::SAMPLER;

    return Result::SUCCESS;
}

Result DescriptorD3D12::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Handle);
    if (result != Result::SUCCESS)
        return result;

    m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
    m_Device->CreateConstantBufferView(&desc, {m_DescriptorHandleCPU});

    m_Type = DescriptorType::CONSTANT_BUFFER;

    return result;
}

Result DescriptorD3D12::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Handle);
    if (result != Result::SUCCESS)
        return result;

    m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
    m_Device->CreateShaderResourceView(resource, &desc, {m_DescriptorHandleCPU});

    if (desc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
        m_Type = DescriptorType::ACCELERATION_STRUCTURE;
    else if (desc.ViewDimension == D3D12_SRV_DIMENSION_BUFFER) {
        bool isStructured = desc.Buffer.StructureByteStride != 0;
        bool isByteAddress = (desc.Buffer.Flags & D3D12_BUFFER_SRV_FLAG_RAW) != 0;

        m_Type = (isStructured || isByteAddress) ? DescriptorType::STRUCTURED_BUFFER : DescriptorType::BUFFER;
    } else
        m_Type = DescriptorType::TEXTURE;

    return result;
}

Result DescriptorD3D12::CreateUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Handle);
    if (result != Result::SUCCESS)
        return result;

    m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
    m_Device->CreateUnorderedAccessView(resource, nullptr, &desc, {m_DescriptorHandleCPU});

    if (desc.ViewDimension == D3D12_UAV_DIMENSION_BUFFER) {
        bool isStructured = desc.Buffer.StructureByteStride != 0;
        bool isByteAddress = (desc.Buffer.Flags & D3D12_BUFFER_UAV_FLAG_RAW) != 0;

        m_Type = (isStructured || isByteAddress) ? DescriptorType::STORAGE_STRUCTURED_BUFFER : DescriptorType::STORAGE_BUFFER;
    } else
        m_Type = DescriptorType::STORAGE_TEXTURE;

    return result;
}

Result DescriptorD3D12::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_Handle);
    if (result != Result::SUCCESS)
        return result;

    m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
    m_Device->CreateRenderTargetView(resource, &desc, {m_DescriptorHandleCPU});

    m_Type = DescriptorType::MAX_NUM;

    return result;
}

Result DescriptorD3D12::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc) {
    Result result = m_Device.GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_Handle);
    if (result != Result::SUCCESS)
        return result;

    m_DescriptorHandleCPU = m_Device.GetDescriptorHandleCPU(m_Handle);
    m_Device->CreateDepthStencilView(resource, &desc, {m_DescriptorHandleCPU});

    m_Type = DescriptorType::MAX_NUM;

    return result;
}
