// © 2021 NVIDIA Corporation

#define SET_CONSTANT_BUFFERS1(xy, stage) \
    if (IsShaderVisible(bindingRange.shaderStages, stage)) \
    deferredContext->xy##SetConstantBuffers1(bindingRange.baseSlot, bindingRange.descriptorNum, (ID3D11Buffer**)descriptors, constantFirst, rootConstantNum)

#define SET_CONSTANT_BUFFERS(xy, stage) \
    if (IsShaderVisible(bindingRange.shaderStages, stage)) \
    deferredContext->xy##SetConstantBuffers(bindingRange.baseSlot, bindingRange.descriptorNum, (ID3D11Buffer**)descriptors)

#define SET_SHADER_RESOURCES(xy, stage) \
    if (IsShaderVisible(bindingRange.shaderStages, stage)) \
    deferredContext->xy##SetShaderResources(bindingRange.baseSlot, bindingRange.descriptorNum, (ID3D11ShaderResourceView**)descriptors)

#define SET_SAMPLERS(xy, stage) \
    if (IsShaderVisible(bindingRange.shaderStages, stage)) \
    deferredContext->xy##SetSamplers(bindingRange.baseSlot, bindingRange.descriptorNum, (ID3D11SamplerState**)descriptors)

#define SET_CONSTANT_BUFFER(xy, stage) \
    if (IsShaderVisible(cb.shaderStages, stage)) \
    deferredContext->xy##SetConstantBuffers(cb.slot, 1, (ID3D11Buffer**)&cb.buffer)

#define SET_SAMPLER(xy, stage) \
    if (IsShaderVisible(ss.shaderStages, stage)) \
    deferredContext->xy##SetSamplers(ss.slot, 1, (ID3D11SamplerState**)&ss.sampler)

// see StageSlots
constexpr std::array<DescriptorTypeDX11, (size_t)DescriptorType::MAX_NUM> g_RemapDescriptorTypeToIndex = {
    DescriptorTypeDX11::SAMPLER,  // SAMPLER
    DescriptorTypeDX11::CONSTANT, // CONSTANT_BUFFER
    DescriptorTypeDX11::RESOURCE, // TEXTURE
    DescriptorTypeDX11::STORAGE,  // STORAGE_TEXTURE
    DescriptorTypeDX11::RESOURCE, // BUFFER
    DescriptorTypeDX11::STORAGE,  // STORAGE_BUFFER
    DescriptorTypeDX11::RESOURCE, // STRUCTURED_BUFFER
    DescriptorTypeDX11::STORAGE,  // STORAGE_STRUCTURED_BUFFER
    DescriptorTypeDX11::RESOURCE, // ACCELERATION_STRUCTURE
};
VALIDATE_ARRAY(g_RemapDescriptorTypeToIndex);

static inline DescriptorTypeDX11 GetDescriptorTypeIndex(DescriptorType type) {
    return g_RemapDescriptorTypeToIndex[(uint32_t)type];
}

static inline bool IsShaderVisible(StageBits shaderVisibility, StageBits stage) {
    return shaderVisibility & stage;
}

static inline StageBits GetShaderVisibility(StageBits shaderStages, StageBits pipelineLayoutShaderStages) {
    if (shaderStages == StageBits::ALL)
        shaderStages = StageBits::ALL_SHADERS;
    if (pipelineLayoutShaderStages == StageBits::ALL)
        pipelineLayoutShaderStages = StageBits::ALL_SHADERS;

    return (StageBits)(shaderStages & pipelineLayoutShaderStages);
}

Result PipelineLayoutD3D11::Create(const PipelineLayoutDesc& pipelineLayoutDesc) {
    // Descriptor sets
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& set = pipelineLayoutDesc.descriptorSets[i];

        BindingSet bindingSet = {};
        bindingSet.startRange = (uint32_t)m_BindingRanges.size();

        // Descriptor ranges
        for (uint32_t j = 0; j < set.rangeNum; j++) {
            const DescriptorRangeDesc& range = set.ranges[j];

            // Add binding range
            BindingRange bindingRange = {};
            bindingRange.baseSlot = range.baseRegisterIndex;
            bindingRange.descriptorOffset = bindingSet.descriptorNum;
            bindingRange.descriptorNum = range.descriptorNum;
            bindingRange.descriptorType = GetDescriptorTypeIndex(range.descriptorType);
            bindingRange.shaderStages = GetShaderVisibility(range.shaderStages, pipelineLayoutDesc.shaderStages);

            m_BindingRanges.push_back(bindingRange);

            bindingSet.descriptorNum += bindingRange.descriptorNum;
        }

        bindingSet.endRange = (uint32_t)m_BindingRanges.size();

        // Add binding set
        m_BindingSets.push_back(bindingSet);
    }

    // Root constants
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++) {
        const RootConstantDesc& rootConstantDesc = pipelineLayoutDesc.rootConstants[i];

        ConstantBuffer cb = {};
        cb.shaderStages = GetShaderVisibility(rootConstantDesc.shaderStages, pipelineLayoutDesc.shaderStages);
        cb.slot = rootConstantDesc.registerIndex;

        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth = Align(rootConstantDesc.size, 16);

        HRESULT hr = m_Device->CreateBuffer(&desc, nullptr, &cb.buffer);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateBuffer");

        m_ConstantBuffers.push_back(cb);
    }

    // Root descriptors
    m_RootBindingOffset = (uint32_t)m_BindingSets.size();

    for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorNum; i++) {
        const RootDescriptorDesc& rootDescriptorSetDesc = pipelineLayoutDesc.rootDescriptors[i];

        BindingSet bindingSet = {};
        bindingSet.startRange = (uint32_t)m_BindingRanges.size();

        BindingRange bindingRange = {};
        bindingRange.baseSlot = rootDescriptorSetDesc.registerIndex;
        bindingRange.descriptorOffset = 0;
        bindingRange.descriptorNum = 1;
        bindingRange.descriptorType = GetDescriptorTypeIndex(rootDescriptorSetDesc.descriptorType);
        bindingRange.shaderStages = GetShaderVisibility(rootDescriptorSetDesc.shaderStages, pipelineLayoutDesc.shaderStages);
        m_BindingRanges.push_back(bindingRange);

        bindingSet.descriptorNum = 1;
        bindingSet.endRange = (uint32_t)m_BindingRanges.size();

        // Add binding set
        m_BindingSets.push_back(bindingSet);
    }

    // Root samplers
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootSamplerNum; i++) {
        const RootSamplerDesc& rootSamplerDesc = pipelineLayoutDesc.rootSamplers[i];

        RootSampler ss = {};
        ss.slot = rootSamplerDesc.registerIndex;
        ss.shaderStages = GetShaderVisibility(rootSamplerDesc.shaderStages, pipelineLayoutDesc.shaderStages);

        D3D11_SAMPLER_DESC desc = {};
        FillSamplerDesc(rootSamplerDesc.desc, desc);

        HRESULT hr = m_Device->CreateSamplerState(&desc, &ss.sampler);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateSamplerState");

        m_RootSamplers.push_back(ss);
    }

    return Result::SUCCESS;
}

void PipelineLayoutD3D11::Bind(ID3D11DeviceContextBest* deferredContext) {
    for (size_t i = 0; i < m_ConstantBuffers.size(); i++) {
        const ConstantBuffer& cb = m_ConstantBuffers[i];

        SET_CONSTANT_BUFFER(VS, StageBits::VERTEX_SHADER);
        SET_CONSTANT_BUFFER(HS, StageBits::TESS_CONTROL_SHADER);
        SET_CONSTANT_BUFFER(DS, StageBits::TESS_EVALUATION_SHADER);
        SET_CONSTANT_BUFFER(GS, StageBits::GEOMETRY_SHADER);
        SET_CONSTANT_BUFFER(PS, StageBits::FRAGMENT_SHADER);
        SET_CONSTANT_BUFFER(CS, StageBits::COMPUTE_SHADER);
    }

    for (size_t i = 0; i < m_RootSamplers.size(); i++) {
        const RootSampler& ss = m_RootSamplers[i];

        SET_SAMPLER(VS, StageBits::VERTEX_SHADER);
        SET_SAMPLER(HS, StageBits::TESS_CONTROL_SHADER);
        SET_SAMPLER(DS, StageBits::TESS_EVALUATION_SHADER);
        SET_SAMPLER(GS, StageBits::GEOMETRY_SHADER);
        SET_SAMPLER(PS, StageBits::FRAGMENT_SHADER);
        SET_SAMPLER(CS, StageBits::COMPUTE_SHADER);
    }
}

void PipelineLayoutD3D11::SetRootConstants(ID3D11DeviceContextBest* deferredContext, const SetRootConstantsDesc& setRootConstantsDesc) const {
    const ConstantBuffer& cb = m_ConstantBuffers[setRootConstantsDesc.rootConstantIndex];

    deferredContext->UpdateSubresource(cb.buffer, 0, nullptr, setRootConstantsDesc.data, 0, 0);
}

void PipelineLayoutD3D11::SetDescriptorSet(BindPoint bindPoint, BindingState& currentBindingState, ID3D11DeviceContextBest* deferredContext, uint32_t setIndex, const DescriptorSetD3D11* descriptorSet, const DescriptorD3D11* descriptor, uint32_t bufferOffset) const {
    bool isGraphics = bindPoint == BindPoint::GRAPHICS;
    const BindingSet& bindingSet = m_BindingSets[setIndex];
    bool isStorageRebindNeededInGraphics = false;

    Scratch<uint8_t> scratch = AllocateScratch(m_Device, uint8_t, bindingSet.descriptorNum * (sizeof(void*) + sizeof(uint32_t) * 2));
    uint8_t* ptr = scratch;

    void** descriptors = (void**)ptr;
    ptr += bindingSet.descriptorNum * sizeof(void*);

    uint32_t* constantFirst = (uint32_t*)ptr;
    ptr += bindingSet.descriptorNum * sizeof(uint32_t);

    uint32_t* rootConstantNum = (uint32_t*)ptr;

    for (uint32_t j = bindingSet.startRange; j < bindingSet.endRange; j++) {
        const BindingRange& bindingRange = m_BindingRanges[j];

        uint32_t hasNonZeroOffset = 0;
        uint32_t descriptorIndex = bindingRange.descriptorOffset;

        for (uint32_t i = 0; i < bindingRange.descriptorNum; i++) {
            if (descriptorSet)
                descriptor = descriptorSet->GetDescriptor(descriptorIndex++);

            if (descriptor) {
                descriptors[i] = *descriptor;

                if (bindingRange.descriptorType == DescriptorTypeDX11::CONSTANT) {
                    uint32_t offset = descriptor->GetElementOffset() + (bufferOffset >> 4);
                    hasNonZeroOffset |= offset;

                    constantFirst[i] = offset;
                    rootConstantNum[i] = descriptor->GetElementNum();
                } else if (bindingRange.descriptorType == DescriptorTypeDX11::STORAGE)
                    currentBindingState.TrackSubresource_UnbindIfNeeded_PostponeGraphicsStorageBinding(deferredContext, descriptor->GetSubresourceInfo(), *descriptor, bindingRange.baseSlot + i, isGraphics, true);
                else if (bindingRange.descriptorType == DescriptorTypeDX11::RESOURCE)
                    currentBindingState.TrackSubresource_UnbindIfNeeded_PostponeGraphicsStorageBinding(deferredContext, descriptor->GetSubresourceInfo(), *descriptor, bindingRange.baseSlot + i, isGraphics, false);
            } else {
                descriptors[i] = nullptr;
                constantFirst[i] = 0;
                rootConstantNum[i] = 0;
            }
        }

        if (bindingRange.descriptorType == DescriptorTypeDX11::CONSTANT) {
            if (hasNonZeroOffset) {
                if (m_Device.GetVersion() < 1)
                    REPORT_ERROR(&m_Device, "Constant buffers with non-zero offsets require 11.1+ feature level");

                if (isGraphics) {
                    SET_CONSTANT_BUFFERS1(VS, StageBits::VERTEX_SHADER);
                    SET_CONSTANT_BUFFERS1(HS, StageBits::TESS_CONTROL_SHADER);
                    SET_CONSTANT_BUFFERS1(DS, StageBits::TESS_EVALUATION_SHADER);
                    SET_CONSTANT_BUFFERS1(GS, StageBits::GEOMETRY_SHADER);
                    SET_CONSTANT_BUFFERS1(PS, StageBits::FRAGMENT_SHADER);
                } else {
                    SET_CONSTANT_BUFFERS1(CS, StageBits::COMPUTE_SHADER);
                }
            } else {
                if (isGraphics) {
                    SET_CONSTANT_BUFFERS(VS, StageBits::VERTEX_SHADER);
                    SET_CONSTANT_BUFFERS(HS, StageBits::TESS_CONTROL_SHADER);
                    SET_CONSTANT_BUFFERS(DS, StageBits::TESS_EVALUATION_SHADER);
                    SET_CONSTANT_BUFFERS(GS, StageBits::GEOMETRY_SHADER);
                    SET_CONSTANT_BUFFERS(PS, StageBits::FRAGMENT_SHADER);
                } else {
                    SET_CONSTANT_BUFFERS(CS, StageBits::COMPUTE_SHADER);
                }
            }
        } else if (bindingRange.descriptorType == DescriptorTypeDX11::RESOURCE) {
            if (isGraphics) {
                SET_SHADER_RESOURCES(VS, StageBits::VERTEX_SHADER);
                SET_SHADER_RESOURCES(HS, StageBits::TESS_CONTROL_SHADER);
                SET_SHADER_RESOURCES(DS, StageBits::TESS_EVALUATION_SHADER);
                SET_SHADER_RESOURCES(GS, StageBits::GEOMETRY_SHADER);
                SET_SHADER_RESOURCES(PS, StageBits::FRAGMENT_SHADER);
            } else {
                SET_SHADER_RESOURCES(CS, StageBits::COMPUTE_SHADER);
            }
        } else if (bindingRange.descriptorType == DescriptorTypeDX11::SAMPLER) {
            if (isGraphics) {
                SET_SAMPLERS(VS, StageBits::VERTEX_SHADER);
                SET_SAMPLERS(HS, StageBits::TESS_CONTROL_SHADER);
                SET_SAMPLERS(DS, StageBits::TESS_EVALUATION_SHADER);
                SET_SAMPLERS(GS, StageBits::GEOMETRY_SHADER);
                SET_SAMPLERS(PS, StageBits::FRAGMENT_SHADER);
            } else {
                SET_SAMPLERS(CS, StageBits::COMPUTE_SHADER);
            }
        } else if (bindingRange.descriptorType == DescriptorTypeDX11::STORAGE) {
            if (isGraphics)
                isStorageRebindNeededInGraphics = true;
            else if (IsShaderVisible(bindingRange.shaderStages, StageBits::COMPUTE_SHADER))
                deferredContext->CSSetUnorderedAccessViews(bindingRange.baseSlot, bindingRange.descriptorNum, (ID3D11UnorderedAccessView**)descriptors, nullptr);
        }
    }

    // UAVs are visible from any stage on DX11.1, but can be bound only to OM or CS
    if (isStorageRebindNeededInGraphics) {
        // Find last "non NULL" slot
        size_t i = currentBindingState.graphicsStorageDescriptors.size() - 1;
        for (; i >= 0; i--) {
            if (currentBindingState.graphicsStorageDescriptors[i])
                break;
        }

        uint32_t num = (uint32_t)(i + 1);
        ID3D11UnorderedAccessView** storages = currentBindingState.graphicsStorageDescriptors.data();

        deferredContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, 0, num, storages, nullptr);
    }
}
