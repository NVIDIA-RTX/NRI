// © 2021 NVIDIA Corporation

static inline void BuildDescriptorSetMapping(const DescriptorSetDesc& descriptorSetDesc, DescriptorSetMapping& descriptorSetMapping) {
    descriptorSetMapping.descriptorRangeMappings.resize(descriptorSetDesc.rangeNum);

    for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
        const DescriptorRangeDesc& descriptorRangeDesc = descriptorSetDesc.ranges[i];
        D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = GetDescriptorHeapType(descriptorRangeDesc.descriptorType);

        descriptorSetMapping.descriptorRangeMappings[i].descriptorHeapType = (DescriptorHeapType)descriptorHeapType;
        descriptorSetMapping.descriptorRangeMappings[i].heapOffset = descriptorSetMapping.descriptorNum[descriptorHeapType];
        descriptorSetMapping.descriptorRangeMappings[i].descriptorNum = descriptorRangeDesc.descriptorNum;

        descriptorSetMapping.descriptorNum[descriptorHeapType] += descriptorRangeDesc.descriptorNum;
    }
}

static inline D3D12_DESCRIPTOR_RANGE_FLAGS GetDescriptorRangeFlags(const DescriptorRangeDesc& descriptorRangeDesc) {
    // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#flags-added-in-root-signature-version-11
    D3D12_DESCRIPTOR_RANGE_FLAGS descriptorRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    // "PARTIALLY_BOUND" implies relaxed requirements and validation
    // "ALLOW_UPDATE_AFTER_SET" allows descriptor updates after "bind"
    if (descriptorRangeDesc.flags & (DescriptorRangeBits::PARTIALLY_BOUND | DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET))
        descriptorRangeFlags |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    // "ALLOW_UPDATE_AFTER_SET" additionally allows to change data, pointed to by descriptors. Samplers are always "DATA_STATIC"
    if (descriptorRangeDesc.descriptorType != DescriptorType::SAMPLER) {
        if (descriptorRangeDesc.flags & DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET)
            descriptorRangeFlags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
        else
            descriptorRangeFlags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
    }

    return descriptorRangeFlags;
}

static inline D3D12_ROOT_SIGNATURE_FLAGS GetRootSignatureStageFlags(const PipelineLayoutDesc& pipelineLayoutDesc, const DeviceD3D12& device) {
    D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    StageBits shaderStages = pipelineLayoutDesc.shaderStages == StageBits::ALL ? StageBits::ALL_SHADERS : pipelineLayoutDesc.shaderStages;

    if (shaderStages & StageBits::VERTEX_SHADER) {
        // The optimization is minor (based on https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_root_signature_flags)
        flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // TODO: do we care if a vertex shader doesn't use IA?
    } else
        flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

    if (!(shaderStages & StageBits::TESS_CONTROL_SHADER))
        flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
    if (!(shaderStages & StageBits::TESS_EVALUATION_SHADER))
        flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
    if (!(shaderStages & StageBits::GEOMETRY_SHADER))
        flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    if (!(shaderStages & StageBits::FRAGMENT_SHADER))
        flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // Windows versions prior to 20H1 (which introduced DirectX Ultimate) can produce errors when the following flags are added. To avoid this, we
    // only add these mesh shading pipeline flags when the device (and thus Windows) supports mesh shading
    if (device.GetDesc().features.meshShader) {
        if (!(shaderStages & StageBits::TASK_SHADER))
            flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
        if (!(shaderStages & StageBits::MESH_SHADER))
            flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
    }

    if (pipelineLayoutDesc.flags & PipelineLayoutBits::SAMPLER_HEAP_DIRECTLY_INDEXED)
        flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

    if (pipelineLayoutDesc.flags & PipelineLayoutBits::RESOURCE_HEAP_DIRECTLY_INDEXED)
        flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    return flags;
}

Result PipelineLayoutD3D12::Create(const PipelineLayoutDesc& pipelineLayoutDesc) {
    uint32_t rangeNum = 0;
    uint32_t rangeMaxNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++)
        rangeMaxNum += pipelineLayoutDesc.descriptorSets[i].rangeNum;

    StdAllocator<uint8_t>& allocator = m_Device.GetStdAllocator();
    m_DescriptorSetMappings.resize(pipelineLayoutDesc.descriptorSetNum, DescriptorSetMapping(allocator));

    Scratch<D3D12_DESCRIPTOR_RANGE1> ranges = AllocateScratch(m_Device, D3D12_DESCRIPTOR_RANGE1, rangeMaxNum);
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    Scratch<D3D12_STATIC_SAMPLER_DESC1> staticSamplers = AllocateScratch(m_Device, D3D12_STATIC_SAMPLER_DESC1, pipelineLayoutDesc.rootSamplerNum);
#else
    Scratch<D3D12_STATIC_SAMPLER_DESC> staticSamplers = AllocateScratch(m_Device, D3D12_STATIC_SAMPLER_DESC, pipelineLayoutDesc.rootSamplerNum);
#endif

    Vector<D3D12_ROOT_PARAMETER1> rootParameters(allocator);

    // Draw parameters emulation
    bool enableDrawParametersEmulation = (pipelineLayoutDesc.flags & PipelineLayoutBits::ENABLE_D3D12_DRAW_PARAMETERS_EMULATION) != 0 && (pipelineLayoutDesc.shaderStages & StageBits::VERTEX_SHADER) != 0;
    if (enableDrawParametersEmulation) {
        D3D12_ROOT_PARAMETER1 rootParam = {};
        rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParam.Constants.ShaderRegister = 0;
        rootParam.Constants.RegisterSpace = NRI_BASE_ATTRIBUTES_EMULATION_SPACE;
        rootParam.Constants.Num32BitValues = 2;

        rootParameters.push_back(rootParam);
    }

    // Descriptor sets
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];
        BuildDescriptorSetMapping(descriptorSetDesc, m_DescriptorSetMappings[i]);

        // Ranges
        D3D12_ROOT_PARAMETER1 rootTable = {};
        rootTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        uint32_t heapIndex = 0;
        uint32_t groupedRangeNum = 0;
        D3D12_DESCRIPTOR_RANGE_TYPE groupedRangeType = {};

        for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
            const DescriptorRangeDesc& descriptorRangeDesc = descriptorSetDesc.ranges[j];
            auto& descriptorRangeMapping = m_DescriptorSetMappings[i].descriptorRangeMappings[j];

            D3D12_SHADER_VISIBILITY shaderVisibility = GetShaderVisibility(descriptorRangeDesc.shaderStages);
            D3D12_DESCRIPTOR_RANGE_TYPE rangeType = GetDescriptorRangesType(descriptorRangeDesc.descriptorType);

            // Try to merge
            if (groupedRangeNum) {
                if (rootTable.ShaderVisibility != shaderVisibility || groupedRangeType != rangeType || descriptorRangeMapping.descriptorHeapType != heapIndex) {
                    rootTable.DescriptorTable.NumDescriptorRanges = groupedRangeNum;
                    rootParameters.push_back(rootTable);

                    rangeNum += groupedRangeNum;
                    groupedRangeNum = 0;
                }
            }

            groupedRangeType = rangeType;
            heapIndex = (uint32_t)descriptorRangeMapping.descriptorHeapType;

            descriptorRangeMapping.rootParameterIndex = groupedRangeNum ? ROOT_PARAMETER_UNUSED : (RootParameterIndexType)rootParameters.size();

            rootTable.ShaderVisibility = shaderVisibility;
            rootTable.DescriptorTable.pDescriptorRanges = &ranges[rangeNum];

            D3D12_DESCRIPTOR_RANGE1& descriptorRange = ranges[rangeNum + groupedRangeNum];
            descriptorRange = {};
            descriptorRange.RangeType = rangeType;
            descriptorRange.NumDescriptors = descriptorRangeDesc.descriptorNum;
            descriptorRange.BaseShaderRegister = descriptorRangeDesc.baseRegisterIndex;
            descriptorRange.RegisterSpace = descriptorSetDesc.registerSpace;
            descriptorRange.Flags = GetDescriptorRangeFlags(descriptorRangeDesc);
            descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            groupedRangeNum++;
        }

        // Finalize merging
        if (groupedRangeNum) {
            rootTable.DescriptorTable.NumDescriptorRanges = groupedRangeNum;
            rootParameters.push_back(rootTable);
            rangeNum += groupedRangeNum;
        }
    }

    // Root constants
    if (pipelineLayoutDesc.rootConstantNum) {
        m_BaseRootConstant = (uint32_t)rootParameters.size();

        for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++) {
            const RootConstantDesc& rootConstantDesc = pipelineLayoutDesc.rootConstants[i];

            D3D12_ROOT_PARAMETER1 rootConstants = {};
            rootConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            rootConstants.ShaderVisibility = GetShaderVisibility(rootConstantDesc.shaderStages);
            rootConstants.Constants.ShaderRegister = rootConstantDesc.registerIndex;
            rootConstants.Constants.RegisterSpace = pipelineLayoutDesc.rootRegisterSpace;
            rootConstants.Constants.Num32BitValues = rootConstantDesc.size / 4;

            rootParameters.push_back(rootConstants);
        }
    }

    // Root descriptors
    if (pipelineLayoutDesc.rootDescriptorNum) {
        m_BaseRootDescriptor = (uint32_t)rootParameters.size();

        for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorNum; i++) {
            const RootDescriptorDesc& rootDescriptorDesc = pipelineLayoutDesc.rootDescriptors[i];

            D3D12_ROOT_PARAMETER1 rootDescriptor = {};
            rootDescriptor.ShaderVisibility = GetShaderVisibility(rootDescriptorDesc.shaderStages);
            rootDescriptor.Descriptor.ShaderRegister = rootDescriptorDesc.registerIndex;
            rootDescriptor.Descriptor.RegisterSpace = pipelineLayoutDesc.rootRegisterSpace;
            rootDescriptor.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE; // TODO: better flags?

            if (rootDescriptorDesc.descriptorType == DescriptorType::CONSTANT_BUFFER)
                rootDescriptor.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            else if (rootDescriptorDesc.descriptorType == DescriptorType::STORAGE_STRUCTURED_BUFFER)
                rootDescriptor.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
            else
                rootDescriptor.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;

            rootParameters.push_back(rootDescriptor);
        }
    }

    // Root samplers
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootSamplerNum; i++) {
        const RootSamplerDesc& rootSamplerDesc = pipelineLayoutDesc.rootSamplers[i];
        const SamplerDesc& samplerDesc = rootSamplerDesc.desc;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        D3D12_STATIC_SAMPLER_DESC1& staticSamplerDesc = staticSamplers[i];
#else
        D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc = staticSamplers[i];
#endif
        staticSamplerDesc = {};
        staticSamplerDesc.Filter = GetFilter(samplerDesc);
        staticSamplerDesc.AddressU = GetAddressMode(samplerDesc.addressModes.u);
        staticSamplerDesc.AddressV = GetAddressMode(samplerDesc.addressModes.v);
        staticSamplerDesc.AddressW = GetAddressMode(samplerDesc.addressModes.w);
        staticSamplerDesc.MipLODBias = samplerDesc.mipBias;
        staticSamplerDesc.MaxAnisotropy = samplerDesc.anisotropy;
        staticSamplerDesc.ComparisonFunc = GetCompareOp(samplerDesc.compareOp);
        staticSamplerDesc.MinLOD = samplerDesc.mipMin;
        staticSamplerDesc.MaxLOD = samplerDesc.mipMax;
        staticSamplerDesc.ShaderRegister = rootSamplerDesc.registerIndex;
        staticSamplerDesc.RegisterSpace = pipelineLayoutDesc.rootRegisterSpace;
        staticSamplerDesc.ShaderVisibility = GetShaderVisibility(rootSamplerDesc.shaderStages);

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        if (samplerDesc.unnormalizedCoordinates)
            staticSamplerDesc.Flags |= D3D12_SAMPLER_FLAG_NON_NORMALIZED_COORDINATES;
#endif

        const Color& borderColor = samplerDesc.borderColor;
        bool isZeroColor = borderColor.ui.x == 0 && borderColor.ui.y == 0 && borderColor.ui.z == 0;
        bool isZeroAlpha = borderColor.ui.w == 0;
        if (isZeroColor && isZeroAlpha)
            staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        else if (isZeroColor)
            staticSamplerDesc.BorderColor = samplerDesc.isInteger ? D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK_UINT : D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        else
            staticSamplerDesc.BorderColor = samplerDesc.isInteger ? D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE_UINT : D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    }

    // Root signature
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureVersionedDesc = {};
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    rootSignatureVersionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_2;
    D3D12_ROOT_SIGNATURE_DESC2& rootSignatureDesc = rootSignatureVersionedDesc.Desc_1_2;
#else
    rootSignatureVersionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc = rootSignatureVersionedDesc.Desc_1_1;
#endif
    rootSignatureDesc.NumParameters = (UINT)rootParameters.size();
    rootSignatureDesc.pParameters = rootParameters.empty() ? nullptr : rootParameters.data();
    rootSignatureDesc.NumStaticSamplers = pipelineLayoutDesc.rootSamplerNum;
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.Flags = GetRootSignatureStageFlags(pipelineLayoutDesc, m_Device);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = S_OK;
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    ComPtr<ID3D12DeviceConfiguration> deviceConfig;
    m_Device->QueryInterface(IID_PPV_ARGS(&deviceConfig));

    if (deviceConfig)
        hr = deviceConfig->SerializeVersionedRootSignature(&rootSignatureVersionedDesc, &rootSignatureBlob, &errorBlob);
    else
#endif
        hr = D3D12SerializeVersionedRootSignature(&rootSignatureVersionedDesc, &rootSignatureBlob, &errorBlob);

    if (errorBlob)
        REPORT_ERROR(&m_Device, "SerializeVersionedRootSignature(): %s", (char*)errorBlob->GetBufferPointer());

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "SerializeVersionedRootSignature");

    hr = m_Device->CreateRootSignature(NODE_MASK, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateRootSignature");

    m_DrawParametersEmulation = enableDrawParametersEmulation;
    if (pipelineLayoutDesc.shaderStages & StageBits::VERTEX_SHADER) {
        Result result = m_Device.CreateDefaultDrawSignatures(m_RootSignature.GetInterface(), enableDrawParametersEmulation);
        RETURN_ON_FAILURE(&m_Device, result == Result::SUCCESS, result, "Failed to create draw signature for pipeline layout");
    }

    return Result::SUCCESS;
}

void PipelineLayoutD3D12::SetDescriptorSet(ID3D12GraphicsCommandList* graphicsCommandList, BindPoint bindPoint, const SetDescriptorSetDesc& setDescriptorSetDesc) const {
    bool isGraphics = bindPoint == BindPoint::GRAPHICS;
    const DescriptorSetD3D12& descriptorSetD3D12 = *(DescriptorSetD3D12*)setDescriptorSetDesc.descriptorSet;
    const DescriptorSetMapping& descriptorSetMapping = m_DescriptorSetMappings[setDescriptorSetDesc.setIndex];
    uint32_t rangeNum = (uint32_t)descriptorSetMapping.descriptorRangeMappings.size();

    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeMapping& descriptorRangeMapping = descriptorSetMapping.descriptorRangeMappings[i];

        RootParameterIndexType rootParameterIndex = descriptorRangeMapping.rootParameterIndex;
        if (rootParameterIndex == ROOT_PARAMETER_UNUSED)
            continue;

        DescriptorHandleGPU descriptorHandleGPU = descriptorSetD3D12.GetDescriptorHandleGPU(i, 0);
        if (isGraphics)
            graphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, {descriptorHandleGPU});
        else
            graphicsCommandList->SetComputeRootDescriptorTable(rootParameterIndex, {descriptorHandleGPU});
    }
}

void PipelineLayoutD3D12::SetRootConstants(ID3D12GraphicsCommandList* graphicsCommandList, BindPoint bindPoint, const SetRootConstantsDesc& setRootConstantsDesc) const {
    bool isGraphics = bindPoint == BindPoint::GRAPHICS;
    uint32_t rootParameterIndex = m_BaseRootConstant + setRootConstantsDesc.rootConstantIndex;
    uint32_t num = setRootConstantsDesc.size / 4;
    uint32_t offset = setRootConstantsDesc.offset / 4;

    // TODO: push constants in VK is a global state, visible for any bind point. But "bindPoint" is used explicitly,
    // because using shader visibility associated with the root constant instead is inefficient if "ALL" is used.
    if (isGraphics)
        graphicsCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, num, setRootConstantsDesc.data, offset);
    else
        graphicsCommandList->SetComputeRoot32BitConstants(rootParameterIndex, num, setRootConstantsDesc.data, offset);
}

void PipelineLayoutD3D12::SetRootDescriptor(ID3D12GraphicsCommandList* graphicsCommandList, BindPoint bindPoint, const SetRootDescriptorDesc& setRootDescriptorDesc) const {
    bool isGraphics = bindPoint == BindPoint::GRAPHICS;
    uint32_t rootParameterIndex = m_BaseRootDescriptor + setRootDescriptorDesc.rootDescriptorIndex;
    const DescriptorD3D12& descriptorD3D12 = *(DescriptorD3D12*)setRootDescriptorDesc.descriptor;
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation = descriptorD3D12.GetGPUVA() + setRootDescriptorDesc.offset;

    BufferViewType bufferViewType = descriptorD3D12.GetBufferViewType();
    if (bufferViewType == BufferViewType::CONSTANT) {
        if (isGraphics)
            graphicsCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, bufferLocation);
        else
            graphicsCommandList->SetComputeRootConstantBufferView(rootParameterIndex, bufferLocation);
    } else if (bufferViewType == BufferViewType::SHADER_RESOURCE || descriptorD3D12.IsAccelerationStructure()) {
        if (isGraphics)
            graphicsCommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, bufferLocation);
        else
            graphicsCommandList->SetComputeRootShaderResourceView(rootParameterIndex, bufferLocation);
    } else if (bufferViewType == BufferViewType::SHADER_RESOURCE_STORAGE) {
        if (isGraphics)
            graphicsCommandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, bufferLocation);
        else
            graphicsCommandList->SetComputeRootUnorderedAccessView(rootParameterIndex, bufferLocation);
    } else
        CHECK(false, "Unexpected");
}
