// © 2021 NVIDIA Corporation

static inline bool IsShaderStageValid(StageBits shaderStages, uint32_t& uniqueShaderStages, StageBits allowedStages) {
    uint32_t x = (uint32_t)(shaderStages & allowedStages);
    uint32_t n = 0;
    while (x) {
        n += x & 1;
        x >>= 1;
    }

    x = (uint32_t)shaderStages;
    bool isUnique = (uniqueShaderStages & x) == 0;
    uniqueShaderStages |= x;

    return n == 1 && isUnique;
}

static inline Mip_t GetMaxMipNum(uint16_t w, uint16_t h, uint16_t d) {
    Mip_t mipNum = 1;

    while (w > 1 || h > 1 || d > 1) {
        if (w > 1)
            w >>= 1;

        if (h > 1)
            h >>= 1;

        if (d > 1)
            d >>= 1;

        mipNum++;
    }

    return mipNum;
}

void ConvertGeometryObjectsVal(GeometryObject* destObjects, const GeometryObject* sourceObjects, uint32_t objectNum);
QueryType GetQueryTypeVK(uint32_t queryTypeVK);

DeviceVal::DeviceVal(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks, DeviceBase& device)
    : DeviceBase(callbacks, allocationCallbacks, NRI_OBJECT_SIGNATURE)
    , m_Impl(*(Device*)&device)
    , m_MemoryTypeMap(GetStdAllocator()) {
}

DeviceVal::~DeviceVal() {
    for (size_t i = 0; i < m_Queues.size(); i++)
        Destroy(GetAllocationCallbacks(), m_Queues[i]);

    if (m_Name) {
        const auto& allocationCallbacks = GetAllocationCallbacks();
        allocationCallbacks.Free(allocationCallbacks.userArg, m_Name);
    }

    ((DeviceBase*)&m_Impl)->Destruct();
}

bool DeviceVal::Create() {
    const DeviceBase& deviceBaseImpl = (DeviceBase&)m_Impl;

    if (deviceBaseImpl.FillFunctionTable(m_iCore) != Result::SUCCESS) {
        REPORT_ERROR(this, "Failed to get 'CoreInterface' interface");
        return false;
    }

    if (deviceBaseImpl.FillFunctionTable(m_iHelper) != Result::SUCCESS) {
        REPORT_ERROR(this, "Failed to get 'HelperInterface' interface");
        return false;
    }

    if (deviceBaseImpl.FillFunctionTable(m_iResourceAllocator) != Result::SUCCESS) {
        REPORT_ERROR(this, "Failed to get 'ResourceAllocatorInterface' interface");
        return false;
    }

    m_IsExtSupported.lowLatency = deviceBaseImpl.FillFunctionTable(m_iLowLatency) == Result::SUCCESS;
    m_IsExtSupported.meshShader = deviceBaseImpl.FillFunctionTable(m_iMeshShader) == Result::SUCCESS;
    m_IsExtSupported.rayTracing = deviceBaseImpl.FillFunctionTable(m_iRayTracing) == Result::SUCCESS;
    m_IsExtSupported.swapChain = deviceBaseImpl.FillFunctionTable(m_iSwapChain) == Result::SUCCESS;
    m_IsExtSupported.wrapperD3D11 = deviceBaseImpl.FillFunctionTable(m_iWrapperD3D11) == Result::SUCCESS;
    m_IsExtSupported.wrapperD3D12 = deviceBaseImpl.FillFunctionTable(m_iWrapperD3D12) == Result::SUCCESS;
    m_IsExtSupported.wrapperVK = deviceBaseImpl.FillFunctionTable(m_iWrapperVK) == Result::SUCCESS;
    
    m_Desc = GetDesc();

    return FillFunctionTable(m_iCoreVal) == Result::SUCCESS;
}

void DeviceVal::RegisterMemoryType(MemoryType memoryType, MemoryLocation memoryLocation) {
    ExclusiveScope lockScope(m_Lock);
    m_MemoryTypeMap[memoryType] = memoryLocation;
}

void DeviceVal::Destruct() {
    Destroy(GetAllocationCallbacks(), this);
}

NRI_INLINE Result DeviceVal::CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    RETURN_ON_FAILURE(this, swapChainDesc.queue != nullptr, Result::INVALID_ARGUMENT, "'queue' is NULL");
    RETURN_ON_FAILURE(this, swapChainDesc.width != 0, Result::INVALID_ARGUMENT, "'width' is 0");
    RETURN_ON_FAILURE(this, swapChainDesc.height != 0, Result::INVALID_ARGUMENT, "'height' is 0");
    RETURN_ON_FAILURE(this, swapChainDesc.textureNum > 0, Result::INVALID_ARGUMENT, "'textureNum' is invalid");
    RETURN_ON_FAILURE(this, swapChainDesc.format < SwapChainFormat::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    auto swapChainDescImpl = swapChainDesc;
    swapChainDescImpl.queue = NRI_GET_IMPL(Queue, swapChainDesc.queue);

    SwapChain* swapChainImpl;
    Result result = m_iSwapChain.CreateSwapChain(m_Impl, swapChainDescImpl, swapChainImpl);

    if (result == Result::SUCCESS)
        swapChain = (SwapChain*)Allocate<SwapChainVal>(GetAllocationCallbacks(), *this, swapChainImpl, swapChainDesc);

    return result;
}

NRI_INLINE void DeviceVal::DestroySwapChain(SwapChain& swapChain) {
    m_iSwapChain.DestroySwapChain(*NRI_GET_IMPL(SwapChain, &swapChain));
    Destroy(GetAllocationCallbacks(), (SwapChainVal*)&swapChain);
}

NRI_INLINE Result DeviceVal::GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    RETURN_ON_FAILURE(this, queueType < QueueType::MAX_NUM, Result::INVALID_ARGUMENT, "'queueType' is invalid");

    Queue* queueImpl;
    Result result = m_iCore.GetQueue(m_Impl, queueType, queueIndex, queueImpl);

    if (result == Result::SUCCESS) {
        const uint32_t index = (uint32_t)queueType;
        if (!m_Queues[index])
            m_Queues[index] = Allocate<QueueVal>(GetAllocationCallbacks(), *this, queueImpl);

        queue = (Queue*)m_Queues[index];
    }

    return result;
}

NRI_INLINE Result DeviceVal::CreateCommandAllocator(const Queue& queue, CommandAllocator*& commandAllocator) {
    auto queueImpl = NRI_GET_IMPL(Queue, &queue);

    CommandAllocator* commandAllocatorImpl = nullptr;
    Result result = m_iCore.CreateCommandAllocator(*queueImpl, commandAllocatorImpl);

    if (result == Result::SUCCESS)
        commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorVal>(GetAllocationCallbacks(), *this, commandAllocatorImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    DescriptorPool* descriptorPoolImpl = nullptr;
    Result result = m_iCore.CreateDescriptorPool(m_Impl, descriptorPoolDesc, descriptorPoolImpl);

    if (result == Result::SUCCESS)
        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetAllocationCallbacks(), *this, descriptorPoolImpl, descriptorPoolDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.size != 0, Result::INVALID_ARGUMENT, "'size' is 0");

    Buffer* bufferImpl = nullptr;
    Result result = m_iCore.CreateBuffer(m_Impl, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, false);

    return result;
}

NRI_INLINE Result DeviceVal::AllocateBuffer(const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.desc.size != 0, Result::INVALID_ARGUMENT, "'size' is 0");

    Buffer* bufferImpl = nullptr;
    Result result = m_iResourceAllocator.AllocateBuffer(m_Impl, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateTexture(const TextureDesc& textureDesc, Texture*& texture) {
    Mip_t maxMipNum = GetMaxMipNum(textureDesc.width, textureDesc.height, textureDesc.depth);

    RETURN_ON_FAILURE(this, textureDesc.format > Format::UNKNOWN && textureDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");
    RETURN_ON_FAILURE(this, textureDesc.width != 0, Result::INVALID_ARGUMENT, "'width' is 0");
    RETURN_ON_FAILURE(this, textureDesc.mipNum <= maxMipNum, Result::INVALID_ARGUMENT, "'mipNum=%u' can't be > %u", textureDesc.mipNum, maxMipNum);

    Texture* textureImpl = nullptr;
    Result result = m_iCore.CreateTexture(m_Impl, textureDesc, textureImpl);

    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, false);

    return result;
}

NRI_INLINE Result DeviceVal::AllocateTexture(const AllocateTextureDesc& textureDesc, Texture*& texture) {
    Mip_t maxMipNum = GetMaxMipNum(textureDesc.desc.width, textureDesc.desc.height, textureDesc.desc.depth);

    RETURN_ON_FAILURE(this, textureDesc.desc.format > Format::UNKNOWN && textureDesc.desc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'desc.format' is invalid");
    RETURN_ON_FAILURE(this, textureDesc.desc.width != 0, Result::INVALID_ARGUMENT, "'desc.width' is 0");
    RETURN_ON_FAILURE(this, textureDesc.desc.mipNum <= maxMipNum, Result::INVALID_ARGUMENT, "'desc.mipNum=%u' can't be > %u", textureDesc.desc.mipNum, maxMipNum);

    Texture* textureImpl = nullptr;
    Result result = m_iResourceAllocator.AllocateTexture(m_Impl, textureDesc, textureImpl);

    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    RETURN_ON_FAILURE(this, bufferViewDesc.buffer != nullptr, Result::INVALID_ARGUMENT, "'buffer' is NULL");
    RETURN_ON_FAILURE(this, bufferViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");
    RETURN_ON_FAILURE(this, bufferViewDesc.viewType < BufferViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");

    const BufferDesc& bufferDesc = ((BufferVal*)bufferViewDesc.buffer)->GetDesc();
    RETURN_ON_FAILURE(this, bufferViewDesc.offset + bufferViewDesc.size <= bufferDesc.size, Result::INVALID_ARGUMENT, "'offset=%llu' + 'size=%llu' must be <= buffer 'size = %llu'", bufferViewDesc.offset, bufferViewDesc.size, bufferDesc.size);

    auto bufferViewDescImpl = bufferViewDesc;
    bufferViewDescImpl.buffer = NRI_GET_IMPL(Buffer, bufferViewDesc.buffer);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCore.CreateBufferView(bufferViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS)
        bufferView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, bufferViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture1DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");
    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    const TextureDesc& textureDesc = ((TextureVal*)textureViewDesc.texture)->GetDesc();

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "'mipOffset=%u' + 'mipNum=%u' must be <= texture 'mipNum=%u'", textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.layerOffset + textureViewDesc.layerNum <= textureDesc.layerNum, Result::INVALID_ARGUMENT,
        "'layerOffset=%u' + 'layerNum=%u' must be <= texture 'layerNum=%u'", textureViewDesc.layerOffset, textureViewDesc.layerNum, textureDesc.layerNum);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCore.CreateTexture1DView(textureViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS)
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, textureViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture2DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");
    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    const TextureDesc& textureDesc = ((TextureVal*)textureViewDesc.texture)->GetDesc();

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "'mipOffset=%u' + 'mipNum=%u' must be <= texture 'mipNum=%u'", textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.layerOffset + textureViewDesc.layerNum <= textureDesc.layerNum, Result::INVALID_ARGUMENT,
        "'layerOffset=%u' + 'layerNum=%u' must be <= texture 'layerNum=%u'", textureViewDesc.layerOffset, textureViewDesc.layerNum, textureDesc.layerNum);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCore.CreateTexture2DView(textureViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS)
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, textureViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture3DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");
    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    const TextureDesc& textureDesc = ((TextureVal*)textureViewDesc.texture)->GetDesc();

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "'mipOffset=%u' + 'mipNum=%u' must be <= texture 'mipNum=%u'", textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.sliceOffset + textureViewDesc.sliceNum <= textureDesc.depth, Result::INVALID_ARGUMENT,
        "'sliceOffset=%u' + 'sliceNum=%u' must be <= texture 'depth=%u'", textureViewDesc.sliceOffset, textureViewDesc.sliceNum, textureDesc.depth);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCore.CreateTexture3DView(textureViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS)
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, textureViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    RETURN_ON_FAILURE(this, samplerDesc.filters.mag < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.mag' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.min < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.min' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.mip < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.mip' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.ext < FilterExt::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.ext' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.u < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "'addressModes.u' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.v < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "'addressModes.v' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.w < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "'addressModes.w' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.compareFunc < CompareFunc::MAX_NUM, Result::INVALID_ARGUMENT, "'compareFunc' is invalid");

    if (samplerDesc.filters.ext != FilterExt::NONE)
        RETURN_ON_FAILURE(this, GetDesc().isTextureFilterMinMaxSupported, Result::UNSUPPORTED, "'isTextureFilterMinMaxSupported' is false");

    if ((samplerDesc.addressModes.u != AddressMode::CLAMP_TO_BORDER && samplerDesc.addressModes.v != AddressMode::CLAMP_TO_BORDER && samplerDesc.addressModes.w != AddressMode::CLAMP_TO_BORDER) && (samplerDesc.borderColor.ui.x != 0 || samplerDesc.borderColor.ui.y != 0 || samplerDesc.borderColor.ui.z != 0 && samplerDesc.borderColor.ui.w != 0))
        REPORT_WARNING(this, "'borderColor' is provided, but 'CLAMP_TO_BORDER' is not requested");

    Descriptor* samplerImpl = nullptr;
    Result result = m_iCore.CreateSampler(m_Impl, samplerDesc, samplerImpl);

    if (result == Result::SUCCESS)
        sampler = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, samplerImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    bool isGraphics = pipelineLayoutDesc.shaderStages & StageBits::GRAPHICS_SHADERS;
    bool isCompute = pipelineLayoutDesc.shaderStages & StageBits::COMPUTE_SHADER;
    bool isRayTracing = pipelineLayoutDesc.shaderStages & StageBits::RAY_TRACING_SHADERS;
    uint32_t supportedTypes = (uint32_t)isGraphics + (uint32_t)isCompute + (uint32_t)isRayTracing;

    RETURN_ON_FAILURE(this, supportedTypes > 0, Result::INVALID_ARGUMENT, "'shaderStages' doesn't include any shader stages");
    RETURN_ON_FAILURE(this, supportedTypes == 1, Result::INVALID_ARGUMENT, "'shaderStages' is invalid, it can't be compatible with more than one type of pipeline");
    RETURN_ON_FAILURE(this, pipelineLayoutDesc.shaderStages != StageBits::NONE, Result::INVALID_ARGUMENT, "'shaderStages' can't be 'NONE'");

    Scratch<uint32_t> spaces = AllocateScratch(*this, uint32_t, pipelineLayoutDesc.descriptorSetNum);

    uint32_t rangeNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];

        for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
            const DescriptorRangeDesc& range = descriptorSetDesc.ranges[j];

            RETURN_ON_FAILURE(this, range.descriptorNum > 0, Result::INVALID_ARGUMENT, "'descriptorSets[%u].ranges[%u].descriptorNum' is 0", i, j);
            RETURN_ON_FAILURE(this, range.descriptorType < DescriptorType::MAX_NUM, Result::INVALID_ARGUMENT, "'descriptorSets[%u].ranges[%u].descriptorType' is invalid", i, j);

            if (range.shaderStages != StageBits::ALL) {
                const uint32_t filteredVisibilityMask = range.shaderStages & pipelineLayoutDesc.shaderStages;

                RETURN_ON_FAILURE(this, (uint32_t)range.shaderStages == filteredVisibilityMask, Result::INVALID_ARGUMENT, "'descriptorSets[%u].ranges[%u].shaderStages' is not compatible with 'shaderStages'", i, j);
            }
        }

        uint32_t n = 0;
        for (; n < i && spaces[n] != descriptorSetDesc.registerSpace; n++)
            ;

        RETURN_ON_FAILURE(this, n == i, Result::INVALID_ARGUMENT, "'descriptorSets[%u].registerSpace = %u' is already in use", i, descriptorSetDesc.registerSpace);
        spaces[i] = descriptorSetDesc.registerSpace;

        rangeNum += descriptorSetDesc.rangeNum;
    }

    if (pipelineLayoutDesc.rootDescriptorNum) {
        uint32_t n = 0;
        for (; n < pipelineLayoutDesc.descriptorSetNum && spaces[n] != pipelineLayoutDesc.rootRegisterSpace; n++)
            ;

        RETURN_ON_FAILURE(this, n == pipelineLayoutDesc.descriptorSetNum, Result::INVALID_ARGUMENT, "'registerSpace = %u' is already in use", pipelineLayoutDesc.rootRegisterSpace);
    }

    for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorNum; i++) {
        const RootDescriptorDesc& rootDescriptorDesc = pipelineLayoutDesc.rootDescriptors[i];

        bool isDescriptorTypeValid = rootDescriptorDesc.descriptorType == DescriptorType::CONSTANT_BUFFER
            || rootDescriptorDesc.descriptorType == DescriptorType::STRUCTURED_BUFFER
            || rootDescriptorDesc.descriptorType == DescriptorType::STORAGE_STRUCTURED_BUFFER;
        RETURN_ON_FAILURE(this, isDescriptorTypeValid, Result::INVALID_ARGUMENT, "'rootDescriptors[%u].descriptorType' must be one of 'CONSTANT_BUFFER', 'STRUCTURED_BUFFER' or 'STORAGE_STRUCTURED_BUFFER'", i);
    }

    uint32_t rootConstantSize = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++)
        rootConstantSize += pipelineLayoutDesc.rootConstants[i].size;

    PipelineLayoutSettingsDesc origSettings = {};
    origSettings.descriptorSetNum = pipelineLayoutDesc.descriptorSetNum;
    origSettings.descriptorRangeNum = rangeNum;
    origSettings.rootConstantSize = rootConstantSize;
    origSettings.rootDescriptorNum = pipelineLayoutDesc.rootDescriptorNum;
    origSettings.enableD3D12DrawParametersEmulation = pipelineLayoutDesc.enableD3D12DrawParametersEmulation && (pipelineLayoutDesc.shaderStages & StageBits::VERTEX_SHADER) != 0;

    PipelineLayoutSettingsDesc fittedSettings = FitPipelineLayoutSettingsIntoDeviceLimits(GetDesc(), origSettings);
    RETURN_ON_FAILURE(this, origSettings.descriptorSetNum == fittedSettings.descriptorSetNum, Result::UNSUPPORTED, "total number of descriptor sets (=%u) exceeds device limits", origSettings.descriptorSetNum);
    RETURN_ON_FAILURE(this, origSettings.descriptorRangeNum == fittedSettings.descriptorRangeNum, Result::UNSUPPORTED, "total number of descriptor ranges (=%u) exceeds device limits", origSettings.descriptorRangeNum);
    RETURN_ON_FAILURE(this, origSettings.rootConstantSize == fittedSettings.rootConstantSize, Result::UNSUPPORTED, "total size of root constants (=%u) exceeds device limits", origSettings.rootConstantSize);
    RETURN_ON_FAILURE(this, origSettings.rootDescriptorNum == fittedSettings.rootDescriptorNum, Result::UNSUPPORTED, "total number of root descriptors (=%u) exceeds device limits", origSettings.rootDescriptorNum);

    PipelineLayout* pipelineLayoutImpl = nullptr;
    Result result = m_iCore.CreatePipelineLayout(m_Impl, pipelineLayoutDesc, pipelineLayoutImpl);

    if (result == Result::SUCCESS)
        pipelineLayout = (PipelineLayout*)Allocate<PipelineLayoutVal>(GetAllocationCallbacks(), *this, pipelineLayoutImpl, pipelineLayoutDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, graphicsPipelineDesc.pipelineLayout != nullptr, Result::INVALID_ARGUMENT, "'pipelineLayout' is NULL");
    RETURN_ON_FAILURE(this, graphicsPipelineDesc.shaders != nullptr, Result::INVALID_ARGUMENT, "'shaders' is NULL");
    RETURN_ON_FAILURE(this, graphicsPipelineDesc.shaderNum > 0, Result::INVALID_ARGUMENT, "'shaderNum' is 0");

    const PipelineLayoutVal& pipelineLayout = *(PipelineLayoutVal*)graphicsPipelineDesc.pipelineLayout;
    const StageBits shaderStages = pipelineLayout.GetPipelineLayoutDesc().shaderStages;
    bool hasEntryPoint = false;
    uint32_t uniqueShaderStages = 0;
    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
        const ShaderDesc* shaderDesc = graphicsPipelineDesc.shaders + i;
        if (shaderDesc->stage == StageBits::VERTEX_SHADER || shaderDesc->stage == StageBits::MESH_CONTROL_SHADER)
            hasEntryPoint = true;

        RETURN_ON_FAILURE(this, shaderDesc->stage & shaderStages, Result::INVALID_ARGUMENT, "'shaders[%u].stage' is not enabled in the pipeline layout", i);
        RETURN_ON_FAILURE(this, shaderDesc->bytecode != nullptr, Result::INVALID_ARGUMENT, "'shaders[%u].bytecode' is invalid", i);
        RETURN_ON_FAILURE(this, shaderDesc->size != 0, Result::INVALID_ARGUMENT, "'shaders[%u].size' is 0", i);
        RETURN_ON_FAILURE(this, IsShaderStageValid(shaderDesc->stage, uniqueShaderStages, StageBits::GRAPHICS_SHADERS), Result::INVALID_ARGUMENT, "'shaders[%u].stage' must include only 1 graphics shader stage, unique for the entire pipeline", i);
    }
    RETURN_ON_FAILURE(this, hasEntryPoint, Result::INVALID_ARGUMENT, "a VERTEX or MESH_CONTROL shader is not provided");

    for (uint32_t i = 0; i < graphicsPipelineDesc.outputMerger.colorNum; i++) {
        const ColorAttachmentDesc* color = graphicsPipelineDesc.outputMerger.colors + i;
        RETURN_ON_FAILURE(this, color->format > Format::UNKNOWN && color->format < Format::BC1_RGBA_UNORM, Result::INVALID_ARGUMENT, "'outputMerger->color[%u].format = %u' is invalid", i, color->format);
    }

    if (graphicsPipelineDesc.vertexInput) {
        for (uint32_t i = 0; i < graphicsPipelineDesc.vertexInput->attributeNum; i++) {
            const VertexAttributeDesc* attribute = graphicsPipelineDesc.vertexInput->attributes + i;
            uint32_t size = GetFormatProps(attribute->format).stride;
            uint32_t stride = graphicsPipelineDesc.vertexInput->streams[attribute->streamIndex].stride;
            RETURN_ON_FAILURE(this, attribute->offset + size <= stride, Result::INVALID_ARGUMENT,
                "'inputAssembly->attributes[%u]' is out of bounds of 'inputAssembly->streams[%u]' (stride = %u)", i, attribute->streamIndex, stride);
        }
    }

    if (graphicsPipelineDesc.rasterization.conservativeRaster)
        RETURN_ON_FAILURE(this, GetDesc().conservativeRasterTier, Result::UNSUPPORTED, "'conservativeRasterTier' must be > 0");

    if (graphicsPipelineDesc.rasterization.lineSmoothing)
        RETURN_ON_FAILURE(this, GetDesc().isLineSmoothingSupported, Result::UNSUPPORTED, "'isLineSmoothingSupported' is false");

    if (graphicsPipelineDesc.rasterization.shadingRate)
        RETURN_ON_FAILURE(this, GetDesc().shadingRateTier, Result::UNSUPPORTED, "'shadingRateTier' must be > 0");

    if (graphicsPipelineDesc.multisample && graphicsPipelineDesc.multisample->sampleLocations)
        RETURN_ON_FAILURE(this, GetDesc().sampleLocationsTier, Result::UNSUPPORTED, "'sampleLocationsTier' must be > 0");

    if (graphicsPipelineDesc.outputMerger.depth.boundsTest)
        RETURN_ON_FAILURE(this, GetDesc().isDepthBoundsTestSupported, Result::UNSUPPORTED, "'isDepthBoundsTestSupported' is false");

    if (graphicsPipelineDesc.outputMerger.logicFunc != LogicFunc::NONE)
        RETURN_ON_FAILURE(this, GetDesc().isLogicFuncSupported, Result::UNSUPPORTED, "'isLogicFuncSupported' is false");

    auto graphicsPipelineDescImpl = graphicsPipelineDesc;
    graphicsPipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, graphicsPipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iCore.CreateGraphicsPipeline(m_Impl, graphicsPipelineDescImpl, pipelineImpl);

    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl, graphicsPipelineDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, computePipelineDesc.pipelineLayout != nullptr, Result::INVALID_ARGUMENT, "'pipelineLayout' is NULL");
    RETURN_ON_FAILURE(this, computePipelineDesc.shader.size != 0, Result::INVALID_ARGUMENT, "'shader.size' is 0");
    RETURN_ON_FAILURE(this, computePipelineDesc.shader.bytecode != nullptr, Result::INVALID_ARGUMENT, "'shader.bytecode' is NULL");
    RETURN_ON_FAILURE(this, computePipelineDesc.shader.stage == StageBits::COMPUTE_SHADER, Result::INVALID_ARGUMENT, "'shader.stage' must be 'StageBits::COMPUTE_SHADER'");

    auto computePipelineDescImpl = computePipelineDesc;
    computePipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, computePipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iCore.CreateComputePipeline(m_Impl, computePipelineDescImpl, pipelineImpl);

    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl, computePipelineDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    RETURN_ON_FAILURE(this, queryPoolDesc.queryType < QueryType::MAX_NUM, Result::INVALID_ARGUMENT, "'queryType' is invalid");
    RETURN_ON_FAILURE(this, queryPoolDesc.capacity > 0, Result::INVALID_ARGUMENT, "'capacity' is 0");

    if (queryPoolDesc.queryType == QueryType::TIMESTAMP_COPY_QUEUE)
        RETURN_ON_FAILURE(this, GetDesc().isCopyQueueTimestampSupported, Result::UNSUPPORTED, "'isCopyQueueTimestampSupported' is false");

    QueryPool* queryPoolImpl = nullptr;
    Result result = m_iCore.CreateQueryPool(m_Impl, queryPoolDesc, queryPoolImpl);

    if (result == Result::SUCCESS)
        queryPool = (QueryPool*)Allocate<QueryPoolVal>(GetAllocationCallbacks(), *this, queryPoolImpl, queryPoolDesc.queryType, queryPoolDesc.capacity);

    return result;
}

NRI_INLINE Result DeviceVal::CreateFence(uint64_t initialValue, Fence*& fence) {
    Fence* fenceImpl;
    Result result = m_iCore.CreateFence(m_Impl, initialValue, fenceImpl);

    if (result == Result::SUCCESS)
        fence = (Fence*)Allocate<FenceVal>(GetAllocationCallbacks(), *this, fenceImpl);

    return result;
}

NRI_INLINE void DeviceVal::DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    m_iCore.DestroyCommandBuffer(*NRI_GET_IMPL(CommandBuffer, &commandBuffer));
    Destroy(GetAllocationCallbacks(), (CommandBufferVal*)&commandBuffer);
}

NRI_INLINE void DeviceVal::DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    m_iCore.DestroyCommandAllocator(*NRI_GET_IMPL(CommandAllocator, &commandAllocator));
    Destroy(GetAllocationCallbacks(), (CommandAllocatorVal*)&commandAllocator);
}

NRI_INLINE void DeviceVal::DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    m_iCore.DestroyDescriptorPool(*NRI_GET_IMPL(DescriptorPool, &descriptorPool));
    Destroy(GetAllocationCallbacks(), (DescriptorPoolVal*)&descriptorPool);
}

NRI_INLINE void DeviceVal::DestroyBuffer(Buffer& buffer) {
    m_iCore.DestroyBuffer(*NRI_GET_IMPL(Buffer, &buffer));
    Destroy(GetAllocationCallbacks(), (BufferVal*)&buffer);
}

NRI_INLINE void DeviceVal::DestroyTexture(Texture& texture) {
    m_iCore.DestroyTexture(*NRI_GET_IMPL(Texture, &texture));
    Destroy(GetAllocationCallbacks(), (TextureVal*)&texture);
}

NRI_INLINE void DeviceVal::DestroyDescriptor(Descriptor& descriptor) {
    m_iCore.DestroyDescriptor(*NRI_GET_IMPL(Descriptor, &descriptor));
    Destroy(GetAllocationCallbacks(), (DescriptorVal*)&descriptor);
}

NRI_INLINE void DeviceVal::DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    m_iCore.DestroyPipelineLayout(*NRI_GET_IMPL(PipelineLayout, &pipelineLayout));
    Destroy(GetAllocationCallbacks(), (PipelineLayoutVal*)&pipelineLayout);
}

NRI_INLINE void DeviceVal::DestroyPipeline(Pipeline& pipeline) {
    m_iCore.DestroyPipeline(*NRI_GET_IMPL(Pipeline, &pipeline));
    Destroy(GetAllocationCallbacks(), (PipelineVal*)&pipeline);
}

NRI_INLINE void DeviceVal::DestroyQueryPool(QueryPool& queryPool) {
    m_iCore.DestroyQueryPool(*NRI_GET_IMPL(QueryPool, &queryPool));
    Destroy(GetAllocationCallbacks(), (QueryPoolVal*)&queryPool);
}

NRI_INLINE void DeviceVal::DestroyFence(Fence& fence) {
    m_iCore.DestroyFence(*NRI_GET_IMPL(Fence, &fence));
    Destroy(GetAllocationCallbacks(), (FenceVal*)&fence);
}

NRI_INLINE Result DeviceVal::AllocateMemory(const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    RETURN_ON_FAILURE(this, allocateMemoryDesc.size > 0, Result::INVALID_ARGUMENT, "'size' is 0");
    RETURN_ON_FAILURE(this, allocateMemoryDesc.priority >= -1.0f && allocateMemoryDesc.priority <= 1.0f, Result::INVALID_ARGUMENT, "'priority' outside of [-1; 1] range");

    std::unordered_map<MemoryType, MemoryLocation>::iterator it;
    std::unordered_map<MemoryType, MemoryLocation>::iterator end;
    {
        ExclusiveScope lockScope(m_Lock);
        it = m_MemoryTypeMap.find(allocateMemoryDesc.type);
        end = m_MemoryTypeMap.end();
    }

    RETURN_ON_FAILURE(this, it != end, Result::FAILURE, "'memoryType' is invalid");

    Memory* memoryImpl;
    Result result = m_iCore.AllocateMemory(m_Impl, allocateMemoryDesc, memoryImpl);

    if (result == Result::SUCCESS)
        memory = (Memory*)Allocate<MemoryVal>(GetAllocationCallbacks(), *this, memoryImpl, allocateMemoryDesc.size, it->second);

    return result;
}

NRI_INLINE Result DeviceVal::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    Scratch<BufferMemoryBindingDesc> memoryBindingDescsImpl = AllocateScratch(*this, BufferMemoryBindingDesc, memoryBindingDescNum);
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        BufferMemoryBindingDesc& destDesc = memoryBindingDescsImpl[i];
        const BufferMemoryBindingDesc& srcDesc = memoryBindingDescs[i];

        RETURN_ON_FAILURE(this, srcDesc.buffer != nullptr, Result::INVALID_ARGUMENT, "'[%u].buffer' is NULL", i);
        RETURN_ON_FAILURE(this, srcDesc.memory != nullptr, Result::INVALID_ARGUMENT, "'[%u].memory' is NULL", i);

        MemoryVal& memory = (MemoryVal&)*srcDesc.memory;
        BufferVal& buffer = (BufferVal&)*srcDesc.buffer;

        RETURN_ON_FAILURE(this, !buffer.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'[%u].buffer' is already bound to memory", i);

        destDesc = srcDesc;
        destDesc.memory = memory.GetImpl();
        destDesc.buffer = buffer.GetImpl();

        // Skip validation if memory has been created from GAPI object using a wrapper extension
        if (memory.GetMemoryLocation() == MemoryLocation::MAX_NUM)
            continue;

        MemoryDesc memoryDesc = {};
        GetCoreInterface().GetBufferMemoryDesc(*buffer.GetImpl(), memory.GetMemoryLocation(), memoryDesc);

        RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || srcDesc.offset == 0, Result::INVALID_ARGUMENT, "'[%u].offset' must be zero for dedicated allocation", i);
        RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'[%u].alignment' is 0", i);
        RETURN_ON_FAILURE(this, srcDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'[%u].offset' is misaligned", i);

        const uint64_t rangeMax = srcDesc.offset + memoryDesc.size;
        const bool memorySizeIsUnknown = memory.GetSize() == 0;

        RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memory.GetSize(), Result::INVALID_ARGUMENT, "'[%u].offset' is invalid", i);
    }

    Result result = m_iCore.BindBufferMemory(m_Impl, memoryBindingDescsImpl, memoryBindingDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
            MemoryVal& memory = *(MemoryVal*)memoryBindingDescs[i].memory;
            memory.BindBuffer(*(BufferVal*)memoryBindingDescs[i].buffer);
        }
    }

    return result;
}

NRI_INLINE Result DeviceVal::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    Scratch<TextureMemoryBindingDesc> memoryBindingDescsImpl = AllocateScratch(*this, TextureMemoryBindingDesc, memoryBindingDescNum);
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        TextureMemoryBindingDesc& destDesc = memoryBindingDescsImpl[i];
        const TextureMemoryBindingDesc& srcDesc = memoryBindingDescs[i];

        RETURN_ON_FAILURE(this, srcDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'[%u].texture' is NULL", i);
        RETURN_ON_FAILURE(this, srcDesc.memory != nullptr, Result::INVALID_ARGUMENT, "'[%u].memory' is NULL", i);

        MemoryVal& memory = (MemoryVal&)*srcDesc.memory;
        TextureVal& texture = (TextureVal&)*srcDesc.texture;

        RETURN_ON_FAILURE(this, !texture.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'[%u].texture' is already bound to memory", i);

        destDesc = srcDesc;
        destDesc.memory = memory.GetImpl();
        destDesc.texture = texture.GetImpl();

        // Skip validation if memory has been created from GAPI object using a wrapper extension
        if (memory.GetMemoryLocation() == MemoryLocation::MAX_NUM)
            continue;

        MemoryDesc memoryDesc = {};
        GetCoreInterface().GetTextureMemoryDesc(*texture.GetImpl(), memory.GetMemoryLocation(), memoryDesc);

        RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || srcDesc.offset == 0, Result::INVALID_ARGUMENT, "'[%u].offset' must be zero for dedicated allocation", i);
        RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'[%u].alignment' is 0", i);
        RETURN_ON_FAILURE(this, srcDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'[%u].offset' is misaligned", i);

        const uint64_t rangeMax = srcDesc.offset + memoryDesc.size;
        const bool memorySizeIsUnknown = memory.GetSize() == 0;

        RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memory.GetSize(), Result::INVALID_ARGUMENT, "'[%u].offset' is invalid", i);
    }

    Result result = m_iCore.BindTextureMemory(m_Impl, memoryBindingDescsImpl, memoryBindingDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
            MemoryVal& memory = *(MemoryVal*)memoryBindingDescs[i].memory;
            memory.BindTexture(*(TextureVal*)memoryBindingDescs[i].texture);
        }
    }

    return result;
}

NRI_INLINE void DeviceVal::FreeMemory(Memory& memory) {
    MemoryVal& memoryVal = (MemoryVal&)memory;

    if (memoryVal.HasBoundResources()) {
        memoryVal.ReportBoundResources();
        REPORT_ERROR(this, "some resources are still bound to the memory");
        return;
    }

    m_iCore.FreeMemory(*NRI_GET_IMPL(Memory, &memory));
    Destroy(GetAllocationCallbacks(), &memoryVal);
}

NRI_INLINE FormatSupportBits DeviceVal::GetFormatSupport(Format format) const {
    return m_iCore.GetFormatSupport(m_Impl, format);
}

#if NRI_ENABLE_VK_SUPPORT

NRI_INLINE Result DeviceVal::CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator) {
    RETURN_ON_FAILURE(this, commandAllocatorVKDesc.vkCommandPool != 0, Result::INVALID_ARGUMENT, "'vkCommandPool' is NULL");
    RETURN_ON_FAILURE(this, commandAllocatorVKDesc.queueType < QueueType::MAX_NUM, Result::INVALID_ARGUMENT, "'queueType' is invalid");

    CommandAllocator* commandAllocatorImpl = nullptr;
    Result result = m_iWrapperVK.CreateCommandAllocatorVK(m_Impl, commandAllocatorVKDesc, commandAllocatorImpl);

    if (result == Result::SUCCESS)
        commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorVal>(GetAllocationCallbacks(), *this, commandAllocatorImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateCommandBuffer(const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferVKDesc.vkCommandBuffer != 0, Result::INVALID_ARGUMENT, "'vkCommandBuffer' is NULL");
    RETURN_ON_FAILURE(this, commandBufferVKDesc.queueType < QueueType::MAX_NUM, Result::INVALID_ARGUMENT, "'queueType' is invalid");

    CommandBuffer* commandBufferImpl = nullptr;
    Result result = m_iWrapperVK.CreateCommandBufferVK(m_Impl, commandBufferVKDesc, commandBufferImpl);

    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetAllocationCallbacks(), *this, commandBufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool) {
    RETURN_ON_FAILURE(this, descriptorPoolVKDesc.vkDescriptorPool != 0, Result::INVALID_ARGUMENT, "'vkDescriptorPool' is NULL");
    RETURN_ON_FAILURE(this, descriptorPoolVKDesc.descriptorSetMaxNum != 0, Result::INVALID_ARGUMENT, "'descriptorSetMaxNum' is 0");

    DescriptorPool* descriptorPoolImpl = nullptr;
    Result result = m_iWrapperVK.CreateDescriptorPoolVK(m_Impl, descriptorPoolVKDesc, descriptorPoolImpl);

    if (result == Result::SUCCESS)
        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetAllocationCallbacks(), *this, descriptorPoolImpl, descriptorPoolVKDesc.descriptorSetMaxNum);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferVKDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.vkBuffer != 0, Result::INVALID_ARGUMENT, "'vkBuffer' is NULL");
    RETURN_ON_FAILURE(this, bufferDesc.size > 0, Result::INVALID_ARGUMENT, "'bufferSize' is 0");

    Buffer* bufferImpl = nullptr;
    Result result = m_iWrapperVK.CreateBufferVK(m_Impl, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateTexture(const TextureVKDesc& textureVKDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureVKDesc.vkImage != 0, Result::INVALID_ARGUMENT, "'vkImage' is NULL");
    RETURN_ON_FAILURE(this, nriConvertVKFormatToNRI(textureVKDesc.vkFormat) != Format::UNKNOWN, Result::INVALID_ARGUMENT, "'sampleNum' is 0");
    RETURN_ON_FAILURE(this, textureVKDesc.sampleNum > 0, Result::INVALID_ARGUMENT, "'sampleNum' is 0");
    RETURN_ON_FAILURE(this, textureVKDesc.layerNum > 0, Result::INVALID_ARGUMENT, "'layerNum' is 0");
    RETURN_ON_FAILURE(this, textureVKDesc.mipNum > 0, Result::INVALID_ARGUMENT, "'mipNum' is 0");

    Texture* textureImpl = nullptr;
    Result result = m_iWrapperVK.CreateTextureVK(m_Impl, textureVKDesc, textureImpl);

    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory) {
    RETURN_ON_FAILURE(this, memoryVKDesc.vkDeviceMemory != 0, Result::INVALID_ARGUMENT, "'vkDeviceMemory' is NULL");
    RETURN_ON_FAILURE(this, memoryVKDesc.size > 0, Result::INVALID_ARGUMENT, "'size' is 0");

    Memory* memoryImpl = nullptr;
    Result result = m_iWrapperVK.CreateMemoryVK(m_Impl, memoryVKDesc, memoryImpl);

    if (result == Result::SUCCESS)
        memory = (Memory*)Allocate<MemoryVal>(GetAllocationCallbacks(), *this, memoryImpl, memoryVKDesc.size, MemoryLocation::MAX_NUM);

    return result;
}

NRI_INLINE Result DeviceVal::CreateGraphicsPipeline(VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, vkPipeline != 0, Result::INVALID_ARGUMENT, "'vkPipeline' is NULL");

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iWrapperVK.CreateGraphicsPipelineVK(m_Impl, vkPipeline, pipelineImpl);

    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateComputePipeline(VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, vkPipeline != 0, Result::INVALID_ARGUMENT, "'vkPipeline' is NULL");

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iWrapperVK.CreateComputePipelineVK(m_Impl, vkPipeline, pipelineImpl);

    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool) {
    RETURN_ON_FAILURE(this, queryPoolVKDesc.vkQueryPool != 0, Result::INVALID_ARGUMENT, "'vkQueryPool' is NULL");

    QueryPool* queryPoolImpl = nullptr;
    Result result = m_iWrapperVK.CreateQueryPoolVK(m_Impl, queryPoolVKDesc, queryPoolImpl);

    if (result == Result::SUCCESS) {
        QueryType queryType = GetQueryTypeVK(queryPoolVKDesc.vkQueryType);
        queryPool = (QueryPool*)Allocate<QueryPoolVal>(GetAllocationCallbacks(), *this, queryPoolImpl, queryType, 0);
    }

    return result;
}

NRI_INLINE Result DeviceVal::CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.vkAccelerationStructure != 0, Result::INVALID_ARGUMENT, "'vkAccelerationStructure' is NULL");

    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iWrapperVK.CreateAccelerationStructureVK(m_Impl, accelerationStructureDesc, accelerationStructureImpl);

    if (result == Result::SUCCESS) {
        MemoryDesc memoryDesc = {};
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, true, memoryDesc);
    }

    return result;
}

#endif

#if NRI_ENABLE_D3D11_SUPPORT

NRI_INLINE Result DeviceVal::CreateCommandBuffer(const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferDesc.d3d11DeviceContext != nullptr, Result::INVALID_ARGUMENT, "'d3d11DeviceContext' is NULL");

    CommandBuffer* commandBufferImpl = nullptr;
    Result result = m_iWrapperD3D11.CreateCommandBufferD3D11(m_Impl, commandBufferDesc, commandBufferImpl);

    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetAllocationCallbacks(), *this, commandBufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferD3D11Desc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.d3d11Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d11Resource' is NULL");

    Buffer* bufferImpl = nullptr;
    Result result = m_iWrapperD3D11.CreateBufferD3D11(m_Impl, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateTexture(const TextureD3D11Desc& textureDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureDesc.d3d11Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d11Resource' is NULL");

    Texture* textureImpl = nullptr;
    Result result = m_iWrapperD3D11.CreateTextureD3D11(m_Impl, textureDesc, textureImpl);

    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

#endif

#if NRI_ENABLE_D3D12_SUPPORT

NRI_INLINE Result DeviceVal::CreateCommandBuffer(const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferDesc.d3d12CommandAllocator != nullptr, Result::INVALID_ARGUMENT, "'d3d12CommandAllocator' is NULL");
    RETURN_ON_FAILURE(this, commandBufferDesc.d3d12CommandList != nullptr, Result::INVALID_ARGUMENT, "'d3d12CommandList' is NULL");

    CommandBuffer* commandBufferImpl = nullptr;
    Result result = m_iWrapperD3D12.CreateCommandBufferD3D12(m_Impl, commandBufferDesc, commandBufferImpl);

    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetAllocationCallbacks(), *this, commandBufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptorPool(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool) {
    RETURN_ON_FAILURE(this, descriptorPoolD3D12Desc.d3d12ResourceDescriptorHeap || descriptorPoolD3D12Desc.d3d12SamplerDescriptorHeap, Result::INVALID_ARGUMENT, "'d3d12ResourceDescriptorHeap' and 'd3d12ResourceDescriptorHeap' are both NULL");

    DescriptorPool* descriptorPoolImpl = nullptr;
    Result result = m_iWrapperD3D12.CreateDescriptorPoolD3D12(m_Impl, descriptorPoolD3D12Desc, descriptorPoolImpl);

    if (result == Result::SUCCESS)
        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetAllocationCallbacks(), *this, descriptorPoolImpl, descriptorPoolD3D12Desc.descriptorSetMaxNum);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferD3D12Desc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d12Resource' is NULL");

    Buffer* bufferImpl = nullptr;
    Result result = m_iWrapperD3D12.CreateBufferD3D12(m_Impl, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateTexture(const TextureD3D12Desc& textureDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureDesc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d12Resource' is NULL");

    Texture* textureImpl = nullptr;
    Result result = m_iWrapperD3D12.CreateTextureD3D12(m_Impl, textureDesc, textureImpl);

    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateMemory(const MemoryD3D12Desc& memoryDesc, Memory*& memory) {
    RETURN_ON_FAILURE(this, memoryDesc.d3d12Heap != nullptr, Result::INVALID_ARGUMENT, "'d3d12Heap' is NULL");

    Memory* memoryImpl = nullptr;
    Result result = m_iWrapperD3D12.CreateMemoryD3D12(m_Impl, memoryDesc, memoryImpl);

    const uint64_t size = GetMemorySizeD3D12(memoryDesc);

    if (result == Result::SUCCESS)
        memory = (Memory*)Allocate<MemoryVal>(GetAllocationCallbacks(), *this, memoryImpl, size, MemoryLocation::MAX_NUM);

    return result;
}

NRI_INLINE Result DeviceVal::CreateAccelerationStructure(const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d12Resource' is NULL");

    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iWrapperD3D12.CreateAccelerationStructureD3D12(m_Impl, accelerationStructureDesc, accelerationStructureImpl);

    if (result == Result::SUCCESS) {
        MemoryDesc memoryDesc = {};
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, true, memoryDesc);
    }

    return result;
}

#endif

NRI_INLINE Result DeviceVal::CreatePipeline(const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, pipelineDesc.pipelineLayout != nullptr, Result::INVALID_ARGUMENT, "'pipelineLayout' is NULL");
    RETURN_ON_FAILURE(this, pipelineDesc.shaderLibrary != nullptr, Result::INVALID_ARGUMENT, "'shaderLibrary' is NULL");
    RETURN_ON_FAILURE(this, pipelineDesc.shaderGroupDescs != nullptr, Result::INVALID_ARGUMENT, "'shaderGroupDescs' is NULL");
    RETURN_ON_FAILURE(this, pipelineDesc.shaderGroupDescNum != 0, Result::INVALID_ARGUMENT, "'shaderGroupDescNum' is 0");
    RETURN_ON_FAILURE(this, pipelineDesc.recursionDepthMax != 0, Result::INVALID_ARGUMENT, "'recursionDepthMax' is 0");

    uint32_t uniqueShaderStages = 0;
    for (uint32_t i = 0; i < pipelineDesc.shaderLibrary->shaderNum; i++) {
        const ShaderDesc& shaderDesc = pipelineDesc.shaderLibrary->shaders[i];

        RETURN_ON_FAILURE(this, shaderDesc.bytecode != nullptr, Result::INVALID_ARGUMENT, "'shaderLibrary->shaders[%u].bytecode' is invalid", i);
        RETURN_ON_FAILURE(this, shaderDesc.size != 0, Result::INVALID_ARGUMENT, "'shaderLibrary->shaders[%u].size' is 0", i);
        RETURN_ON_FAILURE(this, IsShaderStageValid(shaderDesc.stage, uniqueShaderStages, StageBits::RAY_TRACING_SHADERS), Result::INVALID_ARGUMENT,
            "'shaderLibrary->shaders[%u].stage' must include only 1 ray tracing shader stage, unique for the entire pipeline", i);
    }

    auto pipelineDescImpl = pipelineDesc;
    pipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, pipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iRayTracing.CreateRayTracingPipeline(m_Impl, pipelineDescImpl, pipelineImpl);

    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.instanceOrGeometryObjectNum != 0, Result::INVALID_ARGUMENT, "'instanceOrGeometryObjectNum' is 0");

    auto accelerationStructureDescImpl = accelerationStructureDesc;

    uint32_t geometryObjectNum = accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL ? accelerationStructureDesc.instanceOrGeometryObjectNum : 0;
    Scratch<GeometryObject> objectImplArray = AllocateScratch(*this, GeometryObject, geometryObjectNum);

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        ConvertGeometryObjectsVal(objectImplArray, accelerationStructureDesc.geometryObjects, geometryObjectNum);
        accelerationStructureDescImpl.geometryObjects = objectImplArray;
    }

    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iRayTracing.CreateAccelerationStructure(m_Impl, accelerationStructureDescImpl, accelerationStructureImpl);

    if (result == Result::SUCCESS) {
        MemoryDesc memoryDesc = {};
        m_iRayTracing.GetAccelerationStructureMemoryDesc(*accelerationStructureImpl, MemoryLocation::DEVICE, memoryDesc);

        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, false, memoryDesc);
    }

    return result;
}

NRI_INLINE Result DeviceVal::AllocateAccelerationStructure(const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.desc.instanceOrGeometryObjectNum != 0, Result::INVALID_ARGUMENT, "'instanceOrGeometryObjectNum' is 0");

    auto accelerationStructureDescImpl = accelerationStructureDesc;

    uint32_t geometryObjectNum = accelerationStructureDesc.desc.type == AccelerationStructureType::BOTTOM_LEVEL ? accelerationStructureDesc.desc.instanceOrGeometryObjectNum : 0;
    Scratch<GeometryObject> objectImplArray = AllocateScratch(*this, GeometryObject, geometryObjectNum);

    if (accelerationStructureDesc.desc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        ConvertGeometryObjectsVal(objectImplArray, accelerationStructureDesc.desc.geometryObjects, geometryObjectNum);
        accelerationStructureDescImpl.desc.geometryObjects = objectImplArray;
    }

    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iResourceAllocator.AllocateAccelerationStructure(m_Impl, accelerationStructureDescImpl, accelerationStructureImpl);

    if (result == Result::SUCCESS) {
        MemoryDesc memoryDesc = {};
        m_iRayTracing.GetAccelerationStructureMemoryDesc(*accelerationStructureImpl, MemoryLocation::DEVICE, memoryDesc);

        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, true, memoryDesc);
    }

    return result;
}

NRI_INLINE Result DeviceVal::BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    RETURN_ON_FAILURE(this, memoryBindingDescs != nullptr, Result::INVALID_ARGUMENT, "'' is NULL");

    Scratch<AccelerationStructureMemoryBindingDesc> memoryBindingDescsImpl = AllocateScratch(*this, AccelerationStructureMemoryBindingDesc, memoryBindingDescNum);
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        AccelerationStructureMemoryBindingDesc& destDesc = memoryBindingDescsImpl[i];
        const AccelerationStructureMemoryBindingDesc& srcDesc = memoryBindingDescs[i];

        MemoryVal& memory = (MemoryVal&)*srcDesc.memory;
        AccelerationStructureVal& accelerationStructure = (AccelerationStructureVal&)*srcDesc.accelerationStructure;
        const MemoryDesc& memoryDesc = accelerationStructure.GetMemoryDesc();

        RETURN_ON_FAILURE(this, !accelerationStructure.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'[%u].accelerationStructure' is already bound to memory", i);
        RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || srcDesc.offset == 0, Result::INVALID_ARGUMENT, "'[%u].offset' must be 0 for dedicated allocation", i);
        RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'[%u].alignment' is 0", i);
        RETURN_ON_FAILURE(this, srcDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'[%u].offset' is misaligned", i);

        const uint64_t rangeMax = srcDesc.offset + memoryDesc.size;
        const bool memorySizeIsUnknown = memory.GetSize() == 0;

        RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memory.GetSize(), Result::INVALID_ARGUMENT, "'[%u].offset' is invalid", i);

        destDesc = srcDesc;
        destDesc.memory = memory.GetImpl();
        destDesc.accelerationStructure = accelerationStructure.GetImpl();
    }

    Result result = m_iRayTracing.BindAccelerationStructureMemory(m_Impl, memoryBindingDescsImpl, memoryBindingDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
            MemoryVal& memory = *(MemoryVal*)memoryBindingDescs[i].memory;
            memory.BindAccelerationStructure(*(AccelerationStructureVal*)memoryBindingDescs[i].accelerationStructure);
        }
    }

    return result;
}

NRI_INLINE void DeviceVal::DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Destroy(GetAllocationCallbacks(), (AccelerationStructureVal*)&accelerationStructure);
}
