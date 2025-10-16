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

static inline bool IsRayTracingShaderStageValid(StageBits shaderStages, StageBits allowedStages) {
    uint32_t x = (uint32_t)(shaderStages & allowedStages);
    uint32_t n = 0;
    while (x) {
        n += x & 1;
        x >>= 1;
    }

    return n == 1;
}

static inline Dim_t GetMaxMipNum(uint16_t w, uint16_t h, uint16_t d) {
    Dim_t mipNum = 1;

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

DeviceVal::DeviceVal(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks, DeviceBase& device)
    : DeviceBase(callbacks, allocationCallbacks, NRI_OBJECT_SIGNATURE)
    , m_Impl(*(Device*)&device)
    , m_MemoryTypeMap(GetStdAllocator()) {
}

DeviceVal::~DeviceVal() {
    for (size_t i = 0; i < m_Queues.size(); i++)
        Destroy(m_Queues[i]);

    if (m_Name) {
        const auto& allocationCallbacks = GetAllocationCallbacks();
        allocationCallbacks.Free(allocationCallbacks.userArg, m_Name);
    }

    ((DeviceBase*)&m_Impl)->Destruct();
}

bool DeviceVal::Create() {
    const DeviceBase& deviceBaseImpl = (DeviceBase&)m_Impl;

    Result result = deviceBaseImpl.FillFunctionTable(m_iCoreImpl);
    RETURN_ON_FAILURE(this, result == Result::SUCCESS, false, "Failed to get 'CoreInterface' interface");

    result = deviceBaseImpl.FillFunctionTable(m_iHelperImpl);
    RETURN_ON_FAILURE(this, result == Result::SUCCESS, false, "Failed to get 'HelperInterface' interface");

    m_IsExtSupported.lowLatency = deviceBaseImpl.FillFunctionTable(m_iLowLatencyImpl) == Result::SUCCESS;
    m_IsExtSupported.meshShader = deviceBaseImpl.FillFunctionTable(m_iMeshShaderImpl) == Result::SUCCESS;
    m_IsExtSupported.rayTracing = deviceBaseImpl.FillFunctionTable(m_iRayTracingImpl) == Result::SUCCESS;
    m_IsExtSupported.swapChain = deviceBaseImpl.FillFunctionTable(m_iSwapChainImpl) == Result::SUCCESS;
    m_IsExtSupported.wrapperD3D11 = deviceBaseImpl.FillFunctionTable(m_iWrapperD3D11Impl) == Result::SUCCESS;
    m_IsExtSupported.wrapperD3D12 = deviceBaseImpl.FillFunctionTable(m_iWrapperD3D12Impl) == Result::SUCCESS;
    m_IsExtSupported.wrapperVK = deviceBaseImpl.FillFunctionTable(m_iWrapperVKImpl) == Result::SUCCESS;

    m_Desc = GetDesc();

    return FillFunctionTable(m_iCore) == Result::SUCCESS;
}

void DeviceVal::RegisterMemoryType(MemoryType memoryType, MemoryLocation memoryLocation) {
    ExclusiveScope lock(m_Lock);

    m_MemoryTypeMap[memoryType] = memoryLocation;
}

void DeviceVal::Destruct() {
    Destroy(GetAllocationCallbacks(), this);
}

NRI_INLINE Result DeviceVal::CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    RETURN_ON_FAILURE(this, swapChainDesc.queue != nullptr, Result::INVALID_ARGUMENT, "'queue' is NULL");
    RETURN_ON_FAILURE(this, swapChainDesc.width != 0, Result::INVALID_ARGUMENT, "'width' is 0");
    RETURN_ON_FAILURE(this, swapChainDesc.height != 0, Result::INVALID_ARGUMENT, "'height' is 0");
    RETURN_ON_FAILURE(this, swapChainDesc.textureNum != 0, Result::INVALID_ARGUMENT, "'textureNum' is invalid");
    RETURN_ON_FAILURE(this, swapChainDesc.format < SwapChainFormat::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    auto swapChainDescImpl = swapChainDesc;
    swapChainDescImpl.queue = NRI_GET_IMPL(Queue, swapChainDesc.queue);

    SwapChain* swapChainImpl = nullptr;
    Result result = m_iSwapChainImpl.CreateSwapChain(m_Impl, swapChainDescImpl, swapChainImpl);

    swapChain = nullptr;
    if (result == Result::SUCCESS)
        swapChain = (SwapChain*)Allocate<SwapChainVal>(GetAllocationCallbacks(), *this, swapChainImpl, swapChainDesc);

    return result;
}

NRI_INLINE void DeviceVal::DestroySwapChain(SwapChain* swapChain) {
    m_iSwapChainImpl.DestroySwapChain(NRI_GET_IMPL(SwapChain, swapChain));
    Destroy((SwapChainVal*)swapChain);
}

NRI_INLINE Result DeviceVal::GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    RETURN_ON_FAILURE(this, queueType < QueueType::MAX_NUM, Result::INVALID_ARGUMENT, "'queueType' is invalid");

    Queue* queueImpl = nullptr;
    Result result = m_iCoreImpl.GetQueue(m_Impl, queueType, queueIndex, queueImpl);

    queue = nullptr;
    if (result == Result::SUCCESS) {
        const uint32_t index = (uint32_t)queueType;
        if (!m_Queues[index])
            m_Queues[index] = Allocate<QueueVal>(GetAllocationCallbacks(), *this, queueImpl);

        queue = (Queue*)m_Queues[index];
    }

    return result;
}

NRI_INLINE Result DeviceVal::WaitIdle() {
    return GetCoreInterfaceImpl().DeviceWaitIdle(&m_Impl);
}

NRI_INLINE Result DeviceVal::CreateCommandAllocator(const Queue& queue, CommandAllocator*& commandAllocator) {
    auto queueImpl = NRI_GET_IMPL(Queue, &queue);

    CommandAllocator* commandAllocatorImpl = nullptr;
    Result result = m_iCoreImpl.CreateCommandAllocator(*queueImpl, commandAllocatorImpl);

    commandAllocator = nullptr;
    if (result == Result::SUCCESS)
        commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorVal>(GetAllocationCallbacks(), *this, commandAllocatorImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    RETURN_ON_FAILURE(this, descriptorPoolDesc.mutableMaxNum == 0 || GetDesc().features.mutableDescriptorType, Result::INVALID_ARGUMENT, "'features.mutableDescriptorType' is false");

    DescriptorPool* descriptorPoolImpl = nullptr;
    Result result = m_iCoreImpl.CreateDescriptorPool(m_Impl, descriptorPoolDesc, descriptorPoolImpl);

    descriptorPool = nullptr;
    if (result == Result::SUCCESS)
        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetAllocationCallbacks(), *this, descriptorPoolImpl, descriptorPoolDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.size != 0, Result::INVALID_ARGUMENT, "'size' is 0");

    Buffer* bufferImpl = nullptr;
    Result result = m_iCoreImpl.CreateBuffer(m_Impl, bufferDesc, bufferImpl);

    buffer = nullptr;
    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, false);

    return result;
}

NRI_INLINE Result DeviceVal::CreateTexture(const TextureDesc& textureDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureDesc.format > Format::UNKNOWN && textureDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");
    RETURN_ON_FAILURE(this, textureDesc.width != 0, Result::INVALID_ARGUMENT, "'width' is 0");

    Dim_t maxMipNum = GetMaxMipNum(textureDesc.width, textureDesc.height, textureDesc.depth);
    RETURN_ON_FAILURE(this, textureDesc.mipNum <= maxMipNum, Result::INVALID_ARGUMENT, "'mipNum=%u' can't be > %u", textureDesc.mipNum, maxMipNum);

    constexpr TextureUsageBits attachmentBits = TextureUsageBits::COLOR_ATTACHMENT | TextureUsageBits::DEPTH_STENCIL_ATTACHMENT | TextureUsageBits::SHADING_RATE_ATTACHMENT;
    RETURN_ON_FAILURE(this, textureDesc.sharingMode != SharingMode::EXCLUSIVE || (textureDesc.usage & attachmentBits), Result::INVALID_ARGUMENT, "'EXCLUSIVE' is needed only for attachments to enable DCC on some HW");

    Texture* textureImpl = nullptr;
    Result result = m_iCoreImpl.CreateTexture(m_Impl, textureDesc, textureImpl);

    texture = nullptr;
    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, false);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    RETURN_ON_FAILURE(this, bufferViewDesc.buffer != nullptr, Result::INVALID_ARGUMENT, "'buffer' is NULL");
    RETURN_ON_FAILURE(this, bufferViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");
    RETURN_ON_FAILURE(this, bufferViewDesc.viewType < BufferViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");

    const BufferVal& bufferVal = *(BufferVal*)bufferViewDesc.buffer;
    const BufferDesc& bufferDesc = bufferVal.GetDesc();

    RETURN_ON_FAILURE(this, bufferVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'bufferViewDesc.buffer' is not bound to memory");

    RETURN_ON_FAILURE(this, bufferViewDesc.offset + bufferViewDesc.size <= bufferDesc.size, Result::INVALID_ARGUMENT, "'offset=%" PRIu64 "' + 'size=%" PRIu64 "' must be <= buffer 'size=%" PRIu64 "'", bufferViewDesc.offset, bufferViewDesc.size, bufferDesc.size);

    auto bufferViewDescImpl = bufferViewDesc;
    bufferViewDescImpl.buffer = NRI_GET_IMPL(Buffer, bufferViewDesc.buffer);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCoreImpl.CreateBufferView(bufferViewDescImpl, descriptorImpl);

    bufferView = nullptr;
    if (result == Result::SUCCESS)
        bufferView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, bufferViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture1DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");
    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    const TextureVal& textureVal = *(TextureVal*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureVal.GetDesc();

    RETURN_ON_FAILURE(this, textureVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'textureViewDesc.texture' is not bound to memory");

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "'mipOffset=%u' + 'mipNum=%u' must be <= texture 'mipNum=%u'", textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.layerOffset + textureViewDesc.layerNum <= textureDesc.layerNum, Result::INVALID_ARGUMENT,
        "'layerOffset=%u' + 'layerNum=%u' must be <= texture 'layerNum=%u'", textureViewDesc.layerOffset, textureViewDesc.layerNum, textureDesc.layerNum);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCoreImpl.CreateTexture1DView(textureViewDescImpl, descriptorImpl);

    textureView = nullptr;
    if (result == Result::SUCCESS)
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, textureViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture2DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");
    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    const TextureVal& textureVal = *(TextureVal*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureVal.GetDesc();

    RETURN_ON_FAILURE(this, textureVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'textureViewDesc.texture' is not bound to memory");

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "'mipOffset=%u' + 'mipNum=%u' must be <= texture 'mipNum=%u'", textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.layerOffset + textureViewDesc.layerNum <= textureDesc.layerNum, Result::INVALID_ARGUMENT,
        "'layerOffset=%u' + 'layerNum=%u' must be <= texture 'layerNum=%u'", textureViewDesc.layerOffset, textureViewDesc.layerNum, textureDesc.layerNum);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCoreImpl.CreateTexture2DView(textureViewDescImpl, descriptorImpl);

    textureView = nullptr;
    if (result == Result::SUCCESS)
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, textureViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture3DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "'viewType' is invalid");
    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");

    const TextureVal& textureVal = *(TextureVal*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureVal.GetDesc();

    RETURN_ON_FAILURE(this, textureVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'textureViewDesc.texture' is not bound to memory");

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "'mipOffset=%u' + 'mipNum=%u' must be <= texture 'mipNum=%u'", textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.sliceOffset + textureViewDesc.sliceNum <= textureDesc.depth, Result::INVALID_ARGUMENT,
        "'sliceOffset=%u' + 'sliceNum=%u' must be <= texture 'depth=%u'", textureViewDesc.sliceOffset, textureViewDesc.sliceNum, textureDesc.depth);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    Result result = m_iCoreImpl.CreateTexture3DView(textureViewDescImpl, descriptorImpl);

    textureView = nullptr;
    if (result == Result::SUCCESS)
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, descriptorImpl, textureViewDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    RETURN_ON_FAILURE(this, samplerDesc.filters.mag < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.mag' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.min < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.min' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.mip < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.mip' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.ext < ReductionMode::MAX_NUM, Result::INVALID_ARGUMENT, "'filters.ext' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.u < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "'addressModes.u' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.v < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "'addressModes.v' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.w < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "'addressModes.w' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.compareOp < CompareOp::MAX_NUM, Result::INVALID_ARGUMENT, "'compareOp' is invalid");

    if (samplerDesc.filters.ext != ReductionMode::AVERAGE)
        RETURN_ON_FAILURE(this, GetDesc().features.textureFilterMinMax, Result::INVALID_ARGUMENT, "'features.textureFilterMinMax' is false");

    if ((samplerDesc.addressModes.u != AddressMode::CLAMP_TO_BORDER && samplerDesc.addressModes.v != AddressMode::CLAMP_TO_BORDER && samplerDesc.addressModes.w != AddressMode::CLAMP_TO_BORDER) && (samplerDesc.borderColor.ui.x != 0 || samplerDesc.borderColor.ui.y != 0 || samplerDesc.borderColor.ui.z != 0 || samplerDesc.borderColor.ui.w != 0))
        REPORT_WARNING(this, "'borderColor' is provided, but 'CLAMP_TO_BORDER' is not requested");

    Descriptor* samplerImpl = nullptr;
    Result result = m_iCoreImpl.CreateSampler(m_Impl, samplerDesc, samplerImpl);

    sampler = nullptr;
    if (result == Result::SUCCESS)
        sampler = (Descriptor*)Allocate<DescriptorVal>(GetAllocationCallbacks(), *this, samplerImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
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

        RETURN_ON_FAILURE(this, n == i, Result::INVALID_ARGUMENT, "'descriptorSets[%u].registerSpace=%u' is already in use", i, descriptorSetDesc.registerSpace);
        spaces[i] = descriptorSetDesc.registerSpace;

        rangeNum += descriptorSetDesc.rangeNum;
    }

    if (pipelineLayoutDesc.rootDescriptorNum || pipelineLayoutDesc.rootSamplerNum) {
        uint32_t n = 0;
        for (; n < pipelineLayoutDesc.descriptorSetNum && spaces[n] != pipelineLayoutDesc.rootRegisterSpace; n++)
            ;

        RETURN_ON_FAILURE(this, n == pipelineLayoutDesc.descriptorSetNum, Result::INVALID_ARGUMENT, "'registerSpace=%u' is already in use", pipelineLayoutDesc.rootRegisterSpace);
    }

    for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorNum; i++) {
        const RootDescriptorDesc& rootDescriptorDesc = pipelineLayoutDesc.rootDescriptors[i];

        bool isDescriptorTypeValid = rootDescriptorDesc.descriptorType == DescriptorType::CONSTANT_BUFFER
            || rootDescriptorDesc.descriptorType == DescriptorType::STRUCTURED_BUFFER
            || rootDescriptorDesc.descriptorType == DescriptorType::STORAGE_STRUCTURED_BUFFER
            || rootDescriptorDesc.descriptorType == DescriptorType::ACCELERATION_STRUCTURE;

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
    origSettings.enableD3D12DrawParametersEmulation = (pipelineLayoutDesc.flags & PipelineLayoutBits::ENABLE_D3D12_DRAW_PARAMETERS_EMULATION) != 0 && (pipelineLayoutDesc.shaderStages & StageBits::VERTEX_SHADER) != 0;

    PipelineLayoutSettingsDesc fittedSettings = FitPipelineLayoutSettingsIntoDeviceLimits(GetDesc(), origSettings);
    RETURN_ON_FAILURE(this, origSettings.descriptorSetNum == fittedSettings.descriptorSetNum, Result::INVALID_ARGUMENT, "total number of descriptor sets (=%u) exceeds device limits", origSettings.descriptorSetNum);
    RETURN_ON_FAILURE(this, origSettings.descriptorRangeNum == fittedSettings.descriptorRangeNum, Result::INVALID_ARGUMENT, "total number of descriptor ranges (=%u) exceeds device limits", origSettings.descriptorRangeNum);
    RETURN_ON_FAILURE(this, origSettings.rootConstantSize == fittedSettings.rootConstantSize, Result::INVALID_ARGUMENT, "total size of root constants (=%u) exceeds device limits", origSettings.rootConstantSize);
    RETURN_ON_FAILURE(this, origSettings.rootDescriptorNum == fittedSettings.rootDescriptorNum, Result::INVALID_ARGUMENT, "total number of root descriptors (=%u) exceeds device limits", origSettings.rootDescriptorNum);

    PipelineLayout* pipelineLayoutImpl = nullptr;
    Result result = m_iCoreImpl.CreatePipelineLayout(m_Impl, pipelineLayoutDesc, pipelineLayoutImpl);

    pipelineLayout = nullptr;
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
        if (shaderDesc->stage == StageBits::VERTEX_SHADER || shaderDesc->stage == StageBits::MESH_SHADER)
            hasEntryPoint = true;

        RETURN_ON_FAILURE(this, shaderDesc->stage & shaderStages, Result::INVALID_ARGUMENT, "'shaders[%u].stage' is not enabled in the pipeline layout", i);
        RETURN_ON_FAILURE(this, shaderDesc->bytecode != nullptr, Result::INVALID_ARGUMENT, "'shaders[%u].bytecode' is invalid", i);
        RETURN_ON_FAILURE(this, shaderDesc->size != 0, Result::INVALID_ARGUMENT, "'shaders[%u].size' is 0", i);
        RETURN_ON_FAILURE(this, IsShaderStageValid(shaderDesc->stage, uniqueShaderStages, StageBits::GRAPHICS_SHADERS), Result::INVALID_ARGUMENT, "'shaders[%u].stage' must include only 1 graphics shader stage, unique for the entire pipeline", i);
    }
    RETURN_ON_FAILURE(this, hasEntryPoint, Result::INVALID_ARGUMENT, "a VERTEX or MESH shader is not provided");

    for (uint32_t i = 0; i < graphicsPipelineDesc.outputMerger.colorNum; i++) {
        const ColorAttachmentDesc* color = graphicsPipelineDesc.outputMerger.colors + i;
        RETURN_ON_FAILURE(this, color->format > Format::UNKNOWN && color->format < Format::BC1_RGBA_UNORM, Result::INVALID_ARGUMENT, "'outputMerger->color[%u].format=%u' is invalid", i, color->format);
    }

    if (graphicsPipelineDesc.rasterization.conservativeRaster)
        RETURN_ON_FAILURE(this, GetDesc().tiers.conservativeRaster, Result::INVALID_ARGUMENT, "'tiers.conservativeRaster' must be > 0");

    if (graphicsPipelineDesc.rasterization.lineSmoothing)
        RETURN_ON_FAILURE(this, GetDesc().features.lineSmoothing, Result::INVALID_ARGUMENT, "'features.lineSmoothing' is false");

    if (graphicsPipelineDesc.rasterization.shadingRate)
        RETURN_ON_FAILURE(this, GetDesc().tiers.shadingRate, Result::INVALID_ARGUMENT, "'tiers.shadingRate' must be > 0");

    if (graphicsPipelineDesc.multisample && graphicsPipelineDesc.multisample->sampleLocations)
        RETURN_ON_FAILURE(this, GetDesc().tiers.sampleLocations, Result::INVALID_ARGUMENT, "'tiers.sampleLocations' must be > 0");

    if (graphicsPipelineDesc.outputMerger.depth.boundsTest)
        RETURN_ON_FAILURE(this, GetDesc().features.depthBoundsTest, Result::INVALID_ARGUMENT, "'features.depthBoundsTest' is false");

    if (graphicsPipelineDesc.outputMerger.logicOp != LogicOp::NONE)
        RETURN_ON_FAILURE(this, GetDesc().features.logicOp, Result::INVALID_ARGUMENT, "'features.logicOp' is false");

    if (graphicsPipelineDesc.outputMerger.viewMask != 0)
        RETURN_ON_FAILURE(this, GetDesc().features.flexibleMultiview || GetDesc().features.layerBasedMultiview || GetDesc().features.viewportBasedMultiview, Result::INVALID_ARGUMENT, "multiview is not supported");

    auto graphicsPipelineDescImpl = graphicsPipelineDesc;
    graphicsPipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, graphicsPipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iCoreImpl.CreateGraphicsPipeline(m_Impl, graphicsPipelineDescImpl, pipelineImpl);

    pipeline = nullptr;
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
    Result result = m_iCoreImpl.CreateComputePipeline(m_Impl, computePipelineDescImpl, pipelineImpl);

    pipeline = nullptr;
    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl, computePipelineDesc);

    return result;
}

NRI_INLINE Result DeviceVal::CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    RETURN_ON_FAILURE(this, queryPoolDesc.queryType < QueryType::MAX_NUM, Result::INVALID_ARGUMENT, "'queryType' is invalid");
    RETURN_ON_FAILURE(this, queryPoolDesc.capacity > 0, Result::INVALID_ARGUMENT, "'capacity' is 0");

    if (queryPoolDesc.queryType == QueryType::TIMESTAMP_COPY_QUEUE) {
        RETURN_ON_FAILURE(this, GetDesc().features.copyQueueTimestamp, Result::INVALID_ARGUMENT, "'features.copyQueueTimestamp' is false");
    } else if (queryPoolDesc.queryType == QueryType::PIPELINE_STATISTICS) {
        RETURN_ON_FAILURE(this, GetDesc().features.pipelineStatistics, Result::INVALID_ARGUMENT, "'features.pipelineStatistics' is false");
    } else if (queryPoolDesc.queryType == QueryType::ACCELERATION_STRUCTURE_SIZE || queryPoolDesc.queryType == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE) {
        RETURN_ON_FAILURE(this, GetDesc().features.rayTracing, Result::INVALID_ARGUMENT, "'features.rayTracing' is false");
    } else if (queryPoolDesc.queryType == QueryType::MICROMAP_COMPACTED_SIZE) {
        RETURN_ON_FAILURE(this, GetDesc().features.micromap, Result::INVALID_ARGUMENT, "'features.micromap' is false");
    }

    QueryPool* queryPoolImpl = nullptr;
    Result result = m_iCoreImpl.CreateQueryPool(m_Impl, queryPoolDesc, queryPoolImpl);

    queryPool = nullptr;
    if (result == Result::SUCCESS)
        queryPool = (QueryPool*)Allocate<QueryPoolVal>(GetAllocationCallbacks(), *this, queryPoolImpl, queryPoolDesc.queryType, queryPoolDesc.capacity);

    return result;
}

NRI_INLINE Result DeviceVal::CreateFence(uint64_t initialValue, Fence*& fence) {
    Fence* fenceImpl = nullptr;
    Result result = m_iCoreImpl.CreateFence(m_Impl, initialValue, fenceImpl);

    fence = nullptr;
    if (result == Result::SUCCESS)
        fence = (Fence*)Allocate<FenceVal>(GetAllocationCallbacks(), *this, fenceImpl);

    return result;
}

NRI_INLINE void DeviceVal::DestroyCommandBuffer(CommandBuffer* commandBuffer) {
    m_iCoreImpl.DestroyCommandBuffer(NRI_GET_IMPL(CommandBuffer, commandBuffer));
    Destroy((CommandBufferVal*)commandBuffer);
}

NRI_INLINE void DeviceVal::DestroyCommandAllocator(CommandAllocator* commandAllocator) {
    m_iCoreImpl.DestroyCommandAllocator(NRI_GET_IMPL(CommandAllocator, commandAllocator));
    Destroy((CommandAllocatorVal*)commandAllocator);
}

NRI_INLINE void DeviceVal::DestroyDescriptorPool(DescriptorPool* descriptorPool) {
    m_iCoreImpl.DestroyDescriptorPool(NRI_GET_IMPL(DescriptorPool, descriptorPool));
    Destroy((DescriptorPoolVal*)descriptorPool);
}

NRI_INLINE void DeviceVal::DestroyBuffer(Buffer* buffer) {
    m_iCoreImpl.DestroyBuffer(NRI_GET_IMPL(Buffer, buffer));
    Destroy((BufferVal*)buffer);
}

NRI_INLINE void DeviceVal::DestroyTexture(Texture* texture) {
    m_iCoreImpl.DestroyTexture(NRI_GET_IMPL(Texture, texture));
    Destroy((TextureVal*)texture);
}

NRI_INLINE void DeviceVal::DestroyDescriptor(Descriptor* descriptor) {
    m_iCoreImpl.DestroyDescriptor(NRI_GET_IMPL(Descriptor, descriptor));
    Destroy((DescriptorVal*)descriptor);
}

NRI_INLINE void DeviceVal::DestroyPipelineLayout(PipelineLayout* pipelineLayout) {
    m_iCoreImpl.DestroyPipelineLayout(NRI_GET_IMPL(PipelineLayout, pipelineLayout));
    Destroy((PipelineLayoutVal*)pipelineLayout);
}

NRI_INLINE void DeviceVal::DestroyPipeline(Pipeline* pipeline) {
    m_iCoreImpl.DestroyPipeline(NRI_GET_IMPL(Pipeline, pipeline));
    Destroy((PipelineVal*)pipeline);
}

NRI_INLINE void DeviceVal::DestroyQueryPool(QueryPool* queryPool) {
    m_iCoreImpl.DestroyQueryPool(NRI_GET_IMPL(QueryPool, queryPool));
    Destroy((QueryPoolVal*)queryPool);
}

NRI_INLINE void DeviceVal::DestroyFence(Fence* fence) {
    m_iCoreImpl.DestroyFence(NRI_GET_IMPL(Fence, fence));
    Destroy((FenceVal*)fence);
}

NRI_INLINE Result DeviceVal::CreateCommittedBuffer(MemoryLocation memoryLocation, float priority, const BufferDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, priority >= -1.0f && priority <= 1.0f, Result::INVALID_ARGUMENT, "'priority' outside of [-1; 1] range");
    RETURN_ON_FAILURE(this, bufferDesc.size != 0, Result::INVALID_ARGUMENT, "'size' is 0");

    Buffer* bufferImpl = nullptr;
    Result result = m_iCoreImpl.CreateCommittedBuffer(m_Impl, memoryLocation, priority, bufferDesc, bufferImpl);

    buffer = nullptr;
    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateCommittedTexture(MemoryLocation memoryLocation, float priority, const TextureDesc& textureDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, priority >= -1.0f && priority <= 1.0f, Result::INVALID_ARGUMENT, "'priority' outside of [-1; 1] range");
    RETURN_ON_FAILURE(this, textureDesc.format > Format::UNKNOWN && textureDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");
    RETURN_ON_FAILURE(this, textureDesc.width != 0, Result::INVALID_ARGUMENT, "'width' is 0");

    Dim_t maxMipNum = GetMaxMipNum(textureDesc.width, textureDesc.height, textureDesc.depth);
    RETURN_ON_FAILURE(this, textureDesc.mipNum <= maxMipNum, Result::INVALID_ARGUMENT, "'mipNum=%u' can't be > %u", textureDesc.mipNum, maxMipNum);

    constexpr TextureUsageBits attachmentBits = TextureUsageBits::COLOR_ATTACHMENT | TextureUsageBits::DEPTH_STENCIL_ATTACHMENT | TextureUsageBits::SHADING_RATE_ATTACHMENT;
    RETURN_ON_FAILURE(this, textureDesc.sharingMode != SharingMode::EXCLUSIVE || (textureDesc.usage & attachmentBits), Result::INVALID_ARGUMENT, "'EXCLUSIVE' is needed only for attachments to enable DCC on some HW");

    Texture* textureImpl = nullptr;
    Result result = m_iCoreImpl.CreateCommittedTexture(m_Impl, memoryLocation, priority, textureDesc, textureImpl);

    texture = nullptr;
    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateCommittedMicromap(MemoryLocation memoryLocation, float priority, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    RETURN_ON_FAILURE(this, priority >= -1.0f && priority <= 1.0f, Result::INVALID_ARGUMENT, "'priority' outside of [-1; 1] range");
    RETURN_ON_FAILURE(this, micromapDesc.usageNum != 0, Result::INVALID_ARGUMENT, "'usageNum' is 0");

    Micromap* micromapImpl = nullptr;
    Result result = m_iRayTracingImpl.CreateCommittedMicromap(m_Impl, memoryLocation, priority, micromapDesc, micromapImpl);

    micromap = nullptr;
    if (result == Result::SUCCESS)
        micromap = (Micromap*)Allocate<MicromapVal>(GetAllocationCallbacks(), *this, micromapImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateCommittedAccelerationStructure(MemoryLocation memoryLocation, float priority, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, priority >= -1.0f && priority <= 1.0f, Result::INVALID_ARGUMENT, "'priority' outside of [-1; 1] range");
    RETURN_ON_FAILURE(this, accelerationStructureDesc.geometryOrInstanceNum != 0, Result::INVALID_ARGUMENT, "'geometryOrInstanceNum' is 0");

    // Convert desc
    uint32_t geometryNum = 0;
    uint32_t micromapNum = 0;

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        geometryNum = accelerationStructureDesc.geometryOrInstanceNum;

        for (uint32_t i = 0; i < geometryNum; i++) {
            const BottomLevelGeometryDesc& geometryDesc = accelerationStructureDesc.geometries[i];

            if (geometryDesc.type == BottomLevelGeometryType::TRIANGLES && geometryDesc.triangles.micromap)
                micromapNum++;
        }
    }

    Scratch<BottomLevelGeometryDesc> geometriesImplScratch = AllocateScratch(*this, BottomLevelGeometryDesc, geometryNum);
    Scratch<BottomLevelMicromapDesc> micromapsImplScratch = AllocateScratch(*this, BottomLevelMicromapDesc, micromapNum);

    BottomLevelGeometryDesc* geometriesImpl = geometriesImplScratch;
    BottomLevelMicromapDesc* micromapsImpl = micromapsImplScratch;

    auto accelerationStructureDescImpl = accelerationStructureDesc;
    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        accelerationStructureDescImpl.geometries = geometriesImplScratch;
        ConvertBotomLevelGeometries(accelerationStructureDesc.geometries, geometryNum, geometriesImpl, micromapsImpl);
    }

    // Create
    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iRayTracingImpl.CreateCommittedAccelerationStructure(m_Impl, memoryLocation, priority, accelerationStructureDescImpl, accelerationStructureImpl);

    accelerationStructure = nullptr;
    if (result == Result::SUCCESS)
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreatePlacedBuffer(Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.size != 0, Result::INVALID_ARGUMENT, "'size' is 0");

    if (memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        if (!memoryVal.IsWrapped() && GetDesc().features.getMemoryDesc2) {
            MemoryDesc memoryDesc = {};
            m_iCoreImpl.GetBufferMemoryDesc2(m_Impl, bufferDesc, memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || offset == 0, Result::INVALID_ARGUMENT, "'offset' must be 0 for dedicated allocation");
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'alignment' is 0");
            RETURN_ON_FAILURE(this, offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'offset' is misaligned");
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'offset' is invalid");
        }
    } else
        RETURN_ON_FAILURE(this, offset < (uint64_t)MemoryLocation::MAX_NUM, Result::INVALID_ARGUMENT, "'offset' is not a valid 'MemoryLocation'");

    // Create
    Memory* memoryImpl = NRI_GET_IMPL(Memory, memory);

    Buffer* bufferImpl = nullptr;
    Result result = m_iCoreImpl.CreatePlacedBuffer(m_Impl, memoryImpl, offset, bufferDesc, bufferImpl);

    buffer = nullptr;
    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, !memory);

    // Update
    if (buffer && memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        memoryVal.Bind(*(BufferVal*)buffer);
    }

    return result;
}

NRI_INLINE Result DeviceVal::CreatePlacedTexture(Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureDesc.format > Format::UNKNOWN && textureDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "'format' is invalid");
    RETURN_ON_FAILURE(this, textureDesc.width != 0, Result::INVALID_ARGUMENT, "'width' is 0");

    Dim_t maxMipNum = GetMaxMipNum(textureDesc.width, textureDesc.height, textureDesc.depth);
    RETURN_ON_FAILURE(this, textureDesc.mipNum <= maxMipNum, Result::INVALID_ARGUMENT, "'mipNum=%u' can't be > %u", textureDesc.mipNum, maxMipNum);

    constexpr TextureUsageBits attachmentBits = TextureUsageBits::COLOR_ATTACHMENT | TextureUsageBits::DEPTH_STENCIL_ATTACHMENT | TextureUsageBits::SHADING_RATE_ATTACHMENT;
    RETURN_ON_FAILURE(this, textureDesc.sharingMode != SharingMode::EXCLUSIVE || (textureDesc.usage & attachmentBits), Result::INVALID_ARGUMENT, "'EXCLUSIVE' is needed only for attachments to enable DCC on some HW");

    if (memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        if (!memoryVal.IsWrapped() && GetDesc().features.getMemoryDesc2) {
            MemoryDesc memoryDesc = {};
            m_iCoreImpl.GetTextureMemoryDesc2(m_Impl, textureDesc, memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || offset == 0, Result::INVALID_ARGUMENT, "'offset' must be 0 for dedicated allocation");
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'alignment' is 0");
            RETURN_ON_FAILURE(this, offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'offset' is misaligned");
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'offset' is invalid");
        }
    } else
        RETURN_ON_FAILURE(this, offset < (uint64_t)MemoryLocation::MAX_NUM, Result::INVALID_ARGUMENT, "'offset' is not a valid 'MemoryLocation'");

    // Create
    Memory* memoryImpl = NRI_GET_IMPL(Memory, memory);

    Texture* textureImpl = nullptr;
    Result result = m_iCoreImpl.CreatePlacedTexture(m_Impl, memoryImpl, offset, textureDesc, textureImpl);

    texture = nullptr;
    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, !memory);

    // Update
    if (texture && memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        memoryVal.Bind(*(TextureVal*)texture);
    }

    return result;
}

NRI_INLINE Result DeviceVal::CreatePlacedMicromap(Memory* memory, uint64_t offset, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    RETURN_ON_FAILURE(this, micromapDesc.usageNum != 0, Result::INVALID_ARGUMENT, "'usageNum' is 0");

    if (memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        if (!memoryVal.IsWrapped() && GetDesc().features.getMemoryDesc2) {
            MemoryDesc memoryDesc = {};
            m_iRayTracingImpl.GetMicromapMemoryDesc2(m_Impl, micromapDesc, memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || offset == 0, Result::INVALID_ARGUMENT, "'offset' must be 0 for dedicated allocation");
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'alignment' is 0");
            RETURN_ON_FAILURE(this, offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'offset' is misaligned");
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'offset' is invalid");
        }
    } else
        RETURN_ON_FAILURE(this, offset < (uint64_t)MemoryLocation::MAX_NUM, Result::INVALID_ARGUMENT, "'offset' is not a valid 'MemoryLocation'");

    // Create
    Memory* memoryImpl = NRI_GET_IMPL(Memory, memory);

    Micromap* micromapImpl = nullptr;
    Result result = m_iRayTracingImpl.CreatePlacedMicromap(m_Impl, memoryImpl, offset, micromapDesc, micromapImpl);

    micromap = nullptr;
    if (result == Result::SUCCESS)
        micromap = (Micromap*)Allocate<MicromapVal>(GetAllocationCallbacks(), *this, micromapImpl, !memory);

    // Update
    if (micromap && memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        memoryVal.Bind(*(MicromapVal*)micromap);
    }

    return result;
}

NRI_INLINE Result DeviceVal::CreatePlacedAccelerationStructure(Memory* memory, uint64_t offset, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.geometryOrInstanceNum != 0, Result::INVALID_ARGUMENT, "'geometryOrInstanceNum' is 0");

    if (memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        if (!memoryVal.IsWrapped() && GetDesc().features.getMemoryDesc2) {
            MemoryDesc memoryDesc = {};
            m_iRayTracingImpl.GetAccelerationStructureMemoryDesc2(m_Impl, accelerationStructureDesc, memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || offset == 0, Result::INVALID_ARGUMENT, "'offset' must be 0 for dedicated allocation");
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'alignment' is 0");
            RETURN_ON_FAILURE(this, offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'offset' is misaligned");
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'offset' is invalid");
        }
    } else
        RETURN_ON_FAILURE(this, offset < (uint64_t)MemoryLocation::MAX_NUM, Result::INVALID_ARGUMENT, "'offset' is not a valid 'MemoryLocation'");

    // Convert desc
    uint32_t geometryNum = 0;
    uint32_t micromapNum = 0;

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        geometryNum = accelerationStructureDesc.geometryOrInstanceNum;

        for (uint32_t i = 0; i < geometryNum; i++) {
            const BottomLevelGeometryDesc& geometryDesc = accelerationStructureDesc.geometries[i];

            if (geometryDesc.type == BottomLevelGeometryType::TRIANGLES && geometryDesc.triangles.micromap)
                micromapNum++;
        }
    }

    Scratch<BottomLevelGeometryDesc> geometriesImplScratch = AllocateScratch(*this, BottomLevelGeometryDesc, geometryNum);
    Scratch<BottomLevelMicromapDesc> micromapsImplScratch = AllocateScratch(*this, BottomLevelMicromapDesc, micromapNum);

    BottomLevelGeometryDesc* geometriesImpl = geometriesImplScratch;
    BottomLevelMicromapDesc* micromapsImpl = micromapsImplScratch;

    auto accelerationStructureDescImpl = accelerationStructureDesc;
    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        accelerationStructureDescImpl.geometries = geometriesImplScratch;
        ConvertBotomLevelGeometries(accelerationStructureDesc.geometries, geometryNum, geometriesImpl, micromapsImpl);
    }

    // Create
    Memory* memoryImpl = NRI_GET_IMPL(Memory, memory);

    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iRayTracingImpl.CreatePlacedAccelerationStructure(m_Impl, memoryImpl, offset, accelerationStructureDescImpl, accelerationStructureImpl);

    accelerationStructure = nullptr;
    if (result == Result::SUCCESS)
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, !memory);

    // Update
    if (accelerationStructure && memory) {
        MemoryVal& memoryVal = *(MemoryVal*)memory;
        memoryVal.Bind(*(AccelerationStructureVal*)accelerationStructure);
    }

    return result;
}

NRI_INLINE Result DeviceVal::AllocateMemory(const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    RETURN_ON_FAILURE(this, allocateMemoryDesc.size != 0, Result::INVALID_ARGUMENT, "'size' is 0");
    RETURN_ON_FAILURE(this, allocateMemoryDesc.priority >= -1.0f && allocateMemoryDesc.priority <= 1.0f, Result::INVALID_ARGUMENT, "'priority' outside of [-1; 1] range");

    std::unordered_map<MemoryType, MemoryLocation>::iterator it;
    std::unordered_map<MemoryType, MemoryLocation>::iterator end;
    {
        ExclusiveScope lock(m_Lock);
        it = m_MemoryTypeMap.find(allocateMemoryDesc.type);
        end = m_MemoryTypeMap.end();
    }

    RETURN_ON_FAILURE(this, it != end, Result::FAILURE, "'memoryType' is invalid");

    Memory* memoryImpl = nullptr;
    Result result = m_iCoreImpl.AllocateMemory(m_Impl, allocateMemoryDesc, memoryImpl);

    memory = nullptr;
    if (result == Result::SUCCESS)
        memory = (Memory*)Allocate<MemoryVal>(GetAllocationCallbacks(), *this, memoryImpl, allocateMemoryDesc.size, it->second);

    return result;
}

NRI_INLINE void DeviceVal::FreeMemory(Memory* memory) {
    MemoryVal* memoryVal = (MemoryVal*)memory;
    if (!memoryVal)
        return;

    if (memoryVal->HasBoundResources()) {
        memoryVal->ReportBoundResources();
        REPORT_ERROR(this, "some resources are still bound to the memory");
        return;
    }

    m_iCoreImpl.FreeMemory(NRI_GET_IMPL(Memory, memory));

    Destroy(memoryVal);
}

NRI_INLINE void DeviceVal::CopyDescriptorRanges(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum) {
    Scratch<CopyDescriptorRangeDesc> copyDescriptorSetDescsImpl = AllocateScratch(*this, CopyDescriptorRangeDesc, copyDescriptorRangeDescNum);
    for (uint32_t i = 0; i < copyDescriptorRangeDescNum; i++) {
        const CopyDescriptorRangeDesc& copyDescriptorSetDesc = copyDescriptorRangeDescs[i];

        RETURN_ON_FAILURE(this, copyDescriptorSetDesc.dstDescriptorSet != nullptr, ReturnVoid(), "'[%u].dstDescriptorSet' is NULL", i);
        RETURN_ON_FAILURE(this, copyDescriptorSetDesc.srcDescriptorSet != nullptr, ReturnVoid(), "'[%u].srcDescriptorSet' is NULL", i);

        DescriptorSetVal& dstSetVal = *(DescriptorSetVal*)copyDescriptorSetDesc.dstDescriptorSet;
        DescriptorSetVal& srcSetVal = *(DescriptorSetVal*)copyDescriptorSetDesc.srcDescriptorSet;

        const DescriptorSetDesc& dstSetDesc = dstSetVal.GetDesc();
        const DescriptorSetDesc& srcSetDesc = srcSetVal.GetDesc();

        RETURN_ON_FAILURE(this, copyDescriptorSetDesc.dstRangeIndex < dstSetDesc.rangeNum, ReturnVoid(), "'[%u].dstRangeIndex = %u' is out of bounds", copyDescriptorSetDesc.dstRangeIndex, i);
        RETURN_ON_FAILURE(this, copyDescriptorSetDesc.srcRangeIndex < srcSetDesc.rangeNum, ReturnVoid(), "'[%u].srcRangeIndex = %u' is out of bounds", copyDescriptorSetDesc.srcRangeIndex, i);

        const DescriptorRangeDesc& dstRangeDesc = dstSetDesc.ranges[copyDescriptorSetDesc.dstRangeIndex];
        const DescriptorRangeDesc& srcRangeDesc = srcSetDesc.ranges[copyDescriptorSetDesc.srcRangeIndex];

        uint32_t descriptorNum = copyDescriptorSetDesc.descriptorNum;
        if (!descriptorNum)
            descriptorNum = srcRangeDesc.descriptorNum;

        RETURN_ON_FAILURE(this, copyDescriptorSetDesc.dstBaseDescriptor + descriptorNum <= dstRangeDesc.descriptorNum, ReturnVoid(),
            "'[%u].dstBaseDescriptor = %u + [%u].descriptorNum = %u' is greater than 'descriptorNum = %u' in the range (descriptorType=%s)",
            i, copyDescriptorSetDesc.dstBaseDescriptor, i, descriptorNum, dstRangeDesc.descriptorNum, GetDescriptorTypeName(dstRangeDesc.descriptorType));

        RETURN_ON_FAILURE(this, copyDescriptorSetDesc.srcBaseDescriptor + descriptorNum <= srcRangeDesc.descriptorNum, ReturnVoid(),
            "'[%u].srcBaseDescriptor = %u + [%u].descriptorNum = %u' is greater than 'descriptorNum = %u' in the range (descriptorType=%s)",
            i, copyDescriptorSetDesc.srcBaseDescriptor, i, descriptorNum, srcRangeDesc.descriptorNum, GetDescriptorTypeName(srcRangeDesc.descriptorType));

        auto& copyDescriptorSetDescImpl = copyDescriptorSetDescsImpl[i];
        copyDescriptorSetDescImpl = copyDescriptorSetDesc;
        copyDescriptorSetDescImpl.dstDescriptorSet = NRI_GET_IMPL(DescriptorSet, copyDescriptorSetDesc.dstDescriptorSet);
        copyDescriptorSetDescImpl.srcDescriptorSet = NRI_GET_IMPL(DescriptorSet, copyDescriptorSetDesc.srcDescriptorSet);
    }

    GetCoreInterfaceImpl().CopyDescriptorRanges(copyDescriptorSetDescsImpl, copyDescriptorRangeDescNum);
}

NRI_INLINE void DeviceVal::UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum) {
    uint32_t descriptorNum = 0;
    for (uint32_t i = 0; i < updateDescriptorRangeDescNum; i++)
        descriptorNum += updateDescriptorRangeDescs[i].descriptorNum;

    Scratch<UpdateDescriptorRangeDesc> updateDescriptorRangeDescsImpl = AllocateScratch(*this, UpdateDescriptorRangeDesc, updateDescriptorRangeDescNum);
    Scratch<Descriptor*> descriptorsImpl = AllocateScratch(*this, Descriptor*, descriptorNum);

    uint32_t descriptorOffset = 0;
    for (uint32_t i = 0; i < updateDescriptorRangeDescNum; i++) {
        const UpdateDescriptorRangeDesc& updateDescriptorRangeDesc = updateDescriptorRangeDescs[i];
        const DescriptorSetVal& set = *(DescriptorSetVal*)updateDescriptorRangeDesc.descriptorSet;
        const DescriptorSetDesc& setDesc = set.GetDesc();

        RETURN_ON_FAILURE(this, updateDescriptorRangeDesc.rangeIndex < setDesc.rangeNum, ReturnVoid(), "'rangeIndex = %u' is out of 'rangeNum = %u' in the set", updateDescriptorRangeDesc.rangeIndex, setDesc.rangeNum);

        const DescriptorRangeDesc& rangeDesc = setDesc.ranges[updateDescriptorRangeDesc.rangeIndex];

        RETURN_ON_FAILURE(this, updateDescriptorRangeDesc.descriptorNum != 0, ReturnVoid(), "'[%u].descriptorNum' is 0", i);
        RETURN_ON_FAILURE(this, updateDescriptorRangeDesc.descriptors != nullptr, ReturnVoid(), "'[%u].descriptors' is NULL", i);
        RETURN_ON_FAILURE(this, updateDescriptorRangeDesc.baseDescriptor + updateDescriptorRangeDesc.descriptorNum <= rangeDesc.descriptorNum, ReturnVoid(),
            "'[%u].baseDescriptor = %u + [%u].descriptorNum = %u' is greater than 'descriptorNum = %u' in the range (descriptorType=%s)",
            i, updateDescriptorRangeDesc.baseDescriptor, i, updateDescriptorRangeDesc.descriptorNum, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        auto& updateDescriptorRangeDescImpl = updateDescriptorRangeDescsImpl[i];
        updateDescriptorRangeDescImpl = updateDescriptorRangeDesc;
        updateDescriptorRangeDescImpl.descriptorSet = NRI_GET_IMPL(DescriptorSet, updateDescriptorRangeDesc.descriptorSet);
        updateDescriptorRangeDescImpl.descriptors = descriptorsImpl + descriptorOffset;

        Descriptor** descriptors = (Descriptor**)updateDescriptorRangeDescImpl.descriptors;
        for (uint32_t j = 0; j < updateDescriptorRangeDesc.descriptorNum; j++) {
            RETURN_ON_FAILURE(this, updateDescriptorRangeDesc.descriptors[j] != nullptr, ReturnVoid(), "'[%u].descriptors[%u]' is NULL", i, j);

            descriptors[j] = NRI_GET_IMPL(Descriptor, updateDescriptorRangeDesc.descriptors[j]);
        }

        descriptorOffset += updateDescriptorRangeDesc.descriptorNum;
    }

    GetCoreInterfaceImpl().UpdateDescriptorRanges(updateDescriptorRangeDescsImpl, updateDescriptorRangeDescNum);
}

NRI_INLINE FormatSupportBits DeviceVal::GetFormatSupport(Format format) const {
    return m_iCoreImpl.GetFormatSupport(m_Impl, format);
}

#if NRI_ENABLE_VK_SUPPORT

NRI_INLINE Result DeviceVal::CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator) {
    RETURN_ON_FAILURE(this, commandAllocatorVKDesc.vkCommandPool != 0, Result::INVALID_ARGUMENT, "'vkCommandPool' is NULL");
    RETURN_ON_FAILURE(this, commandAllocatorVKDesc.queueType < QueueType::MAX_NUM, Result::INVALID_ARGUMENT, "'queueType' is invalid");

    CommandAllocator* commandAllocatorImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateCommandAllocatorVK(m_Impl, commandAllocatorVKDesc, commandAllocatorImpl);

    commandAllocator = nullptr;
    if (result == Result::SUCCESS)
        commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorVal>(GetAllocationCallbacks(), *this, commandAllocatorImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateCommandBuffer(const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferVKDesc.vkCommandBuffer != 0, Result::INVALID_ARGUMENT, "'vkCommandBuffer' is NULL");
    RETURN_ON_FAILURE(this, commandBufferVKDesc.queueType < QueueType::MAX_NUM, Result::INVALID_ARGUMENT, "'queueType' is invalid");

    CommandBuffer* commandBufferImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateCommandBufferVK(m_Impl, commandBufferVKDesc, commandBufferImpl);

    commandBuffer = nullptr;
    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetAllocationCallbacks(), *this, commandBufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool) {
    RETURN_ON_FAILURE(this, descriptorPoolVKDesc.vkDescriptorPool != 0, Result::INVALID_ARGUMENT, "'vkDescriptorPool' is NULL");
    RETURN_ON_FAILURE(this, descriptorPoolVKDesc.descriptorSetMaxNum != 0, Result::INVALID_ARGUMENT, "'descriptorSetMaxNum' is 0");

    DescriptorPool* descriptorPoolImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateDescriptorPoolVK(m_Impl, descriptorPoolVKDesc, descriptorPoolImpl);

    descriptorPool = nullptr;
    if (result == Result::SUCCESS)
        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetAllocationCallbacks(), *this, descriptorPoolImpl, descriptorPoolVKDesc.descriptorSetMaxNum);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferVKDesc& bufferVKDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferVKDesc.vkBuffer != 0, Result::INVALID_ARGUMENT, "'vkBuffer' is NULL");
    RETURN_ON_FAILURE(this, bufferVKDesc.size > 0, Result::INVALID_ARGUMENT, "'bufferSize' is 0");

    Buffer* bufferImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateBufferVK(m_Impl, bufferVKDesc, bufferImpl);

    buffer = nullptr;
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
    Result result = m_iWrapperVKImpl.CreateTextureVK(m_Impl, textureVKDesc, textureImpl);

    texture = nullptr;
    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory) {
    RETURN_ON_FAILURE(this, memoryVKDesc.vkDeviceMemory != 0, Result::INVALID_ARGUMENT, "'vkDeviceMemory' is NULL");
    RETURN_ON_FAILURE(this, memoryVKDesc.size > 0, Result::INVALID_ARGUMENT, "'size' is 0");

    Memory* memoryImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateMemoryVK(m_Impl, memoryVKDesc, memoryImpl);

    memory = nullptr;
    if (result == Result::SUCCESS)
        memory = (Memory*)Allocate<MemoryVal>(GetAllocationCallbacks(), *this, memoryImpl, memoryVKDesc.size, MemoryLocation::MAX_NUM);

    return result;
}

NRI_INLINE Result DeviceVal::CreatePipeline(const PipelineVKDesc& pipelineVKDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, pipelineVKDesc.vkPipeline != 0, Result::INVALID_ARGUMENT, "'vkPipeline' is NULL");

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreatePipelineVK(m_Impl, pipelineVKDesc, pipelineImpl);

    pipeline = nullptr;
    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool) {
    RETURN_ON_FAILURE(this, queryPoolVKDesc.vkQueryPool != 0, Result::INVALID_ARGUMENT, "'vkQueryPool' is NULL");

    QueryPool* queryPoolImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateQueryPoolVK(m_Impl, queryPoolVKDesc, queryPoolImpl);

    queryPool = nullptr;
    if (result == Result::SUCCESS) {
        QueryType queryType = GetQueryTypeVK(queryPoolVKDesc.vkQueryType);
        queryPool = (QueryPool*)Allocate<QueryPoolVal>(GetAllocationCallbacks(), *this, queryPoolImpl, queryType, 0);
    }

    return result;
}

NRI_INLINE Result DeviceVal::CreateFence(const FenceVKDesc& fenceVKDesc, Fence*& fence) {
    RETURN_ON_FAILURE(this, fenceVKDesc.vkTimelineSemaphore != 0, Result::INVALID_ARGUMENT, "'vkTimelineSemaphore' is NULL");

    Fence* fenceImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateFenceVK(m_Impl, fenceVKDesc, fenceImpl);

    fence = nullptr;
    if (result == Result::SUCCESS)
        fence = (Fence*)Allocate<FenceVal>(GetAllocationCallbacks(), *this, fenceImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureVKDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureVKDesc.vkAccelerationStructure != 0, Result::INVALID_ARGUMENT, "'vkAccelerationStructure' is NULL");

    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iWrapperVKImpl.CreateAccelerationStructureVK(m_Impl, accelerationStructureVKDesc, accelerationStructureImpl);

    accelerationStructure = nullptr;
    if (result == Result::SUCCESS)
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, true);

    return result;
}

#endif

#if NRI_ENABLE_D3D11_SUPPORT

NRI_INLINE Result DeviceVal::CreateCommandBuffer(const CommandBufferD3D11Desc& commandBufferD3D11Desc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferD3D11Desc.d3d11DeviceContext != nullptr, Result::INVALID_ARGUMENT, "'d3d11DeviceContext' is NULL");

    CommandBuffer* commandBufferImpl = nullptr;
    Result result = m_iWrapperD3D11Impl.CreateCommandBufferD3D11(m_Impl, commandBufferD3D11Desc, commandBufferImpl);

    commandBuffer = nullptr;
    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetAllocationCallbacks(), *this, commandBufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferD3D11Desc& bufferD3D11Desc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferD3D11Desc.d3d11Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d11Resource' is NULL");

    Buffer* bufferImpl = nullptr;
    Result result = m_iWrapperD3D11Impl.CreateBufferD3D11(m_Impl, bufferD3D11Desc, bufferImpl);

    buffer = nullptr;
    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateTexture(const TextureD3D11Desc& textureD3D11Desc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureD3D11Desc.d3d11Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d11Resource' is NULL");

    Texture* textureImpl = nullptr;
    Result result = m_iWrapperD3D11Impl.CreateTextureD3D11(m_Impl, textureD3D11Desc, textureImpl);

    texture = nullptr;
    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

#endif

#if NRI_ENABLE_D3D12_SUPPORT

NRI_INLINE Result DeviceVal::CreateCommandBuffer(const CommandBufferD3D12Desc& commandBufferD3D12Desc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferD3D12Desc.d3d12CommandList != nullptr, Result::INVALID_ARGUMENT, "'d3d12CommandList' is NULL");

    CommandBuffer* commandBufferImpl = nullptr;
    Result result = m_iWrapperD3D12Impl.CreateCommandBufferD3D12(m_Impl, commandBufferD3D12Desc, commandBufferImpl);

    commandBuffer = nullptr;
    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetAllocationCallbacks(), *this, commandBufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateDescriptorPool(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool) {
    RETURN_ON_FAILURE(this, descriptorPoolD3D12Desc.d3d12ResourceDescriptorHeap || descriptorPoolD3D12Desc.d3d12SamplerDescriptorHeap, Result::INVALID_ARGUMENT, "'d3d12ResourceDescriptorHeap' and 'd3d12ResourceDescriptorHeap' are both NULL");

    DescriptorPool* descriptorPoolImpl = nullptr;
    Result result = m_iWrapperD3D12Impl.CreateDescriptorPoolD3D12(m_Impl, descriptorPoolD3D12Desc, descriptorPoolImpl);

    descriptorPool = nullptr;
    if (result == Result::SUCCESS)
        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetAllocationCallbacks(), *this, descriptorPoolImpl, descriptorPoolD3D12Desc.descriptorSetMaxNum);

    return result;
}

NRI_INLINE Result DeviceVal::CreateBuffer(const BufferD3D12Desc& bufferD3D12Desc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferD3D12Desc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d12Resource' is NULL");

    Buffer* bufferImpl = nullptr;
    Result result = m_iWrapperD3D12Impl.CreateBufferD3D12(m_Impl, bufferD3D12Desc, bufferImpl);

    buffer = nullptr;
    if (result == Result::SUCCESS)
        buffer = (Buffer*)Allocate<BufferVal>(GetAllocationCallbacks(), *this, bufferImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateTexture(const TextureD3D12Desc& textureD3D12Desc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureD3D12Desc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d12Resource' is NULL");

    Texture* textureImpl = nullptr;
    Result result = m_iWrapperD3D12Impl.CreateTextureD3D12(m_Impl, textureD3D12Desc, textureImpl);

    texture = nullptr;
    if (result == Result::SUCCESS)
        texture = (Texture*)Allocate<TextureVal>(GetAllocationCallbacks(), *this, textureImpl, true);

    return result;
}

NRI_INLINE Result DeviceVal::CreateMemory(const MemoryD3D12Desc& memoryD3D12Desc, Memory*& memory) {
    RETURN_ON_FAILURE(this, memoryD3D12Desc.d3d12Heap != nullptr, Result::INVALID_ARGUMENT, "'d3d12Heap' is NULL");

    Memory* memoryImpl = nullptr;
    Result result = m_iWrapperD3D12Impl.CreateMemoryD3D12(m_Impl, memoryD3D12Desc, memoryImpl);

    const uint64_t size = GetMemorySizeD3D12(memoryD3D12Desc);

    memory = nullptr;
    if (result == Result::SUCCESS)
        memory = (Memory*)Allocate<MemoryVal>(GetAllocationCallbacks(), *this, memoryImpl, size, MemoryLocation::MAX_NUM);

    return result;
}

NRI_INLINE Result DeviceVal::CreateFence(const FenceD3D12Desc& fenceD3D12Desc, Fence*& fence) {
    RETURN_ON_FAILURE(this, fenceD3D12Desc.d3d12Fence != 0, Result::INVALID_ARGUMENT, "'d3d12Fence' is NULL");

    Fence* fenceImpl = nullptr;
    Result result = m_iWrapperD3D12Impl.CreateFenceD3D12(m_Impl, fenceD3D12Desc, fenceImpl);

    fence = nullptr;
    if (result == Result::SUCCESS)
        fence = (Fence*)Allocate<FenceVal>(GetAllocationCallbacks(), *this, fenceImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateAccelerationStructure(const AccelerationStructureD3D12Desc& accelerationStructureD3D12Desc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureD3D12Desc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "'d3d12Resource' is NULL");

    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iWrapperD3D12Impl.CreateAccelerationStructureD3D12(m_Impl, accelerationStructureD3D12Desc, accelerationStructureImpl);

    accelerationStructure = nullptr;
    if (result == Result::SUCCESS)
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, true);

    return result;
}

#endif

NRI_INLINE Result DeviceVal::CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, rayTracingPipelineDesc.pipelineLayout != nullptr, Result::INVALID_ARGUMENT, "'pipelineLayout' is NULL");
    RETURN_ON_FAILURE(this, rayTracingPipelineDesc.shaderLibrary != nullptr, Result::INVALID_ARGUMENT, "'shaderLibrary' is NULL");
    RETURN_ON_FAILURE(this, rayTracingPipelineDesc.shaderGroups != nullptr, Result::INVALID_ARGUMENT, "'shaderGroups' is NULL");
    RETURN_ON_FAILURE(this, rayTracingPipelineDesc.shaderGroupNum != 0, Result::INVALID_ARGUMENT, "'shaderGroupNum' is 0");
    RETURN_ON_FAILURE(this, rayTracingPipelineDesc.recursionMaxDepth != 0, Result::INVALID_ARGUMENT, "'recursionDepthMax' is 0");

    for (uint32_t i = 0; i < rayTracingPipelineDesc.shaderLibrary->shaderNum; i++) {
        const ShaderDesc& shaderDesc = rayTracingPipelineDesc.shaderLibrary->shaders[i];

        RETURN_ON_FAILURE(this, shaderDesc.bytecode != nullptr, Result::INVALID_ARGUMENT, "'shaderLibrary->shaders[%u].bytecode' is NULL", i);
        RETURN_ON_FAILURE(this, shaderDesc.size != 0, Result::INVALID_ARGUMENT, "'shaderLibrary->shaders[%u].size' is 0", i);
        RETURN_ON_FAILURE(this, IsRayTracingShaderStageValid(shaderDesc.stage, StageBits::RAY_TRACING_SHADERS), Result::INVALID_ARGUMENT, "'shaderLibrary->shaders[%u].stage' must include only 1 ray tracing shader stage", i);
    }

    auto pipelineDescImpl = rayTracingPipelineDesc;
    pipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, rayTracingPipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    Result result = m_iRayTracingImpl.CreateRayTracingPipeline(m_Impl, pipelineDescImpl, pipelineImpl);

    pipeline = nullptr;
    if (result == Result::SUCCESS)
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetAllocationCallbacks(), *this, pipelineImpl);

    return result;
}

NRI_INLINE Result DeviceVal::CreateMicromap(const MicromapDesc& micromapDesc, Micromap*& micromap) {
    RETURN_ON_FAILURE(this, micromapDesc.usageNum != 0, Result::INVALID_ARGUMENT, "'usageNum' is 0");

    Micromap* micromapImpl = nullptr;
    Result result = m_iRayTracingImpl.CreateMicromap(m_Impl, micromapDesc, micromapImpl);

    micromap = nullptr;
    if (result == Result::SUCCESS)
        micromap = (Micromap*)Allocate<MicromapVal>(GetAllocationCallbacks(), *this, micromapImpl, false);

    return result;
}

NRI_INLINE Result DeviceVal::CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.geometryOrInstanceNum != 0, Result::INVALID_ARGUMENT, "'geometryOrInstanceNum' is 0");

    // Convert desc
    uint32_t geometryNum = 0;
    uint32_t micromapNum = 0;

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        geometryNum = accelerationStructureDesc.geometryOrInstanceNum;

        for (uint32_t i = 0; i < geometryNum; i++) {
            const BottomLevelGeometryDesc& geometryDesc = accelerationStructureDesc.geometries[i];

            if (geometryDesc.type == BottomLevelGeometryType::TRIANGLES && geometryDesc.triangles.micromap)
                micromapNum++;
        }
    }

    Scratch<BottomLevelGeometryDesc> geometriesImplScratch = AllocateScratch(*this, BottomLevelGeometryDesc, geometryNum);
    Scratch<BottomLevelMicromapDesc> micromapsImplScratch = AllocateScratch(*this, BottomLevelMicromapDesc, micromapNum);

    BottomLevelGeometryDesc* geometriesImpl = geometriesImplScratch;
    BottomLevelMicromapDesc* micromapsImpl = micromapsImplScratch;

    auto accelerationStructureDescImpl = accelerationStructureDesc;
    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        accelerationStructureDescImpl.geometries = geometriesImplScratch;
        ConvertBotomLevelGeometries(accelerationStructureDesc.geometries, geometryNum, geometriesImpl, micromapsImpl);
    }

    // Create
    AccelerationStructure* accelerationStructureImpl = nullptr;
    Result result = m_iRayTracingImpl.CreateAccelerationStructure(m_Impl, accelerationStructureDescImpl, accelerationStructureImpl);

    accelerationStructure = nullptr;
    if (result == Result::SUCCESS)
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetAllocationCallbacks(), *this, accelerationStructureImpl, false);

    return result;
}

NRI_INLINE Result DeviceVal::BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum) {
    Scratch<BindBufferMemoryDesc> bindBufferMemoryDescsImpl = AllocateScratch(*this, BindBufferMemoryDesc, bindBufferMemoryDescNum);
    for (uint32_t i = 0; i < bindBufferMemoryDescNum; i++) {
        const BindBufferMemoryDesc& bindBufferMemoryDesc = bindBufferMemoryDescs[i];
        MemoryVal& memoryVal = *(MemoryVal*)bindBufferMemoryDesc.memory;
        BufferVal& bufferVal = *(BufferVal*)bindBufferMemoryDesc.buffer;

        RETURN_ON_FAILURE(this, bindBufferMemoryDesc.buffer != nullptr, Result::INVALID_ARGUMENT, "'[%u].buffer' is NULL", i);
        RETURN_ON_FAILURE(this, bindBufferMemoryDesc.memory != nullptr, Result::INVALID_ARGUMENT, "'[%u].memory' is NULL", i);
        RETURN_ON_FAILURE(this, !bufferVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'[%u].buffer' is already bound to memory", i);

        BindBufferMemoryDesc& bindBufferMemoryDescImpl = bindBufferMemoryDescsImpl[i];
        bindBufferMemoryDescImpl = bindBufferMemoryDesc;
        bindBufferMemoryDescImpl.memory = memoryVal.GetImpl();
        bindBufferMemoryDescImpl.buffer = bufferVal.GetImpl();

        if (!memoryVal.IsWrapped()) {
            MemoryDesc memoryDesc = {};
            m_iCoreImpl.GetBufferMemoryDesc(*bufferVal.GetImpl(), memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = bindBufferMemoryDesc.offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || bindBufferMemoryDesc.offset == 0, Result::INVALID_ARGUMENT, "'[%u].offset' must be 0 for dedicated allocation", i);
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'[%u].alignment' is 0", i);
            RETURN_ON_FAILURE(this, bindBufferMemoryDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'[%u].offset' is misaligned", i);
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'[%u].offset' is invalid", i);
        }
    }

    Result result = m_iCoreImpl.BindBufferMemory(bindBufferMemoryDescsImpl, bindBufferMemoryDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < bindBufferMemoryDescNum; i++) {
            MemoryVal& memoryVal = *(MemoryVal*)bindBufferMemoryDescs[i].memory;
            memoryVal.Bind(*(BufferVal*)bindBufferMemoryDescs[i].buffer);
        }
    }

    return result;
}

NRI_INLINE Result DeviceVal::BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum) {
    Scratch<BindTextureMemoryDesc> bindTextureMemoryDescsImpl = AllocateScratch(*this, BindTextureMemoryDesc, bindTextureMemoryDescNum);
    for (uint32_t i = 0; i < bindTextureMemoryDescNum; i++) {
        const BindTextureMemoryDesc& bindTextureMemoryDesc = bindTextureMemoryDescs[i];
        MemoryVal& memoryVal = *(MemoryVal*)bindTextureMemoryDesc.memory;
        TextureVal& textureVal = *(TextureVal*)bindTextureMemoryDesc.texture;

        RETURN_ON_FAILURE(this, bindTextureMemoryDesc.texture != nullptr, Result::INVALID_ARGUMENT, "'[%u].texture' is NULL", i);
        RETURN_ON_FAILURE(this, bindTextureMemoryDesc.memory != nullptr, Result::INVALID_ARGUMENT, "'[%u].memory' is NULL", i);
        RETURN_ON_FAILURE(this, !textureVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'[%u].texture' is already bound to memory", i);

        BindTextureMemoryDesc& bindTextureMemoryDescImpl = bindTextureMemoryDescsImpl[i];
        bindTextureMemoryDescImpl = bindTextureMemoryDesc;
        bindTextureMemoryDescImpl.memory = memoryVal.GetImpl();
        bindTextureMemoryDescImpl.texture = textureVal.GetImpl();

        if (!memoryVal.IsWrapped()) {
            MemoryDesc memoryDesc = {};
            m_iCoreImpl.GetTextureMemoryDesc(*textureVal.GetImpl(), memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = bindTextureMemoryDesc.offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || bindTextureMemoryDesc.offset == 0, Result::INVALID_ARGUMENT, "'[%u].offset' must be 0 for dedicated allocation", i);
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'[%u].alignment' is 0", i);
            RETURN_ON_FAILURE(this, bindTextureMemoryDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'[%u].offset' is misaligned", i);
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'[%u].offset' is invalid", i);
        }
    }

    Result result = m_iCoreImpl.BindTextureMemory(bindTextureMemoryDescsImpl, bindTextureMemoryDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < bindTextureMemoryDescNum; i++) {
            MemoryVal& memoryVal = *(MemoryVal*)bindTextureMemoryDescs[i].memory;
            memoryVal.Bind(*(TextureVal*)bindTextureMemoryDescs[i].texture);
        }
    }

    return result;
}

NRI_INLINE Result DeviceVal::BindMicromapMemory(const BindMicromapMemoryDesc* bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum) {
    Scratch<BindMicromapMemoryDesc> bindMicromapMemoryDescsImpl = AllocateScratch(*this, BindMicromapMemoryDesc, bindMicromapMemoryDescNum);
    for (uint32_t i = 0; i < bindMicromapMemoryDescNum; i++) {
        const BindMicromapMemoryDesc& bindMicromapMemoryDesc = bindMicromapMemoryDescs[i];
        MemoryVal& memoryVal = *(MemoryVal*)bindMicromapMemoryDesc.memory;
        MicromapVal& micromapVal = *(MicromapVal*)bindMicromapMemoryDesc.micromap;

        RETURN_ON_FAILURE(this, !micromapVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'[%u].micromap' is already bound to memory", i);

        BindMicromapMemoryDesc& bindMicromapMemoryDescImpl = bindMicromapMemoryDescsImpl[i];
        bindMicromapMemoryDescImpl = bindMicromapMemoryDesc;
        bindMicromapMemoryDescImpl.memory = memoryVal.GetImpl();
        bindMicromapMemoryDescImpl.micromap = micromapVal.GetImpl();

        if (!memoryVal.IsWrapped()) {
            MemoryDesc memoryDesc = {};
            m_iRayTracingImpl.GetMicromapMemoryDesc(*micromapVal.GetImpl(), memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = bindMicromapMemoryDesc.offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || bindMicromapMemoryDesc.offset == 0, Result::INVALID_ARGUMENT, "'[%u].offset' must be 0 for dedicated allocation", i);
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'[%u].alignment' is 0", i);
            RETURN_ON_FAILURE(this, bindMicromapMemoryDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'[%u].offset' is misaligned", i);
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'[%u].offset' is invalid", i);
        }
    }

    Result result = m_iRayTracingImpl.BindMicromapMemory(bindMicromapMemoryDescsImpl, bindMicromapMemoryDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < bindMicromapMemoryDescNum; i++) {
            MemoryVal& memoryVal = *(MemoryVal*)bindMicromapMemoryDescs[i].memory;
            memoryVal.Bind(*(MicromapVal*)bindMicromapMemoryDescs[i].micromap);
        }
    }

    return result;
}

NRI_INLINE Result DeviceVal::BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum) {
    Scratch<BindAccelerationStructureMemoryDesc> memoryBindingDescsImpl = AllocateScratch(*this, BindAccelerationStructureMemoryDesc, bindAccelerationStructureMemoryDescNum);
    for (uint32_t i = 0; i < bindAccelerationStructureMemoryDescNum; i++) {
        const BindAccelerationStructureMemoryDesc& srcDesc = bindAccelerationStructureMemoryDescs[i];
        MemoryVal& memoryVal = (MemoryVal&)*srcDesc.memory;
        AccelerationStructureVal& accelerationStructureVal = (AccelerationStructureVal&)*srcDesc.accelerationStructure;

        RETURN_ON_FAILURE(this, !accelerationStructureVal.IsBoundToMemory(), Result::INVALID_ARGUMENT, "'[%u].accelerationStructure' is already bound to memory", i);

        BindAccelerationStructureMemoryDesc& destDesc = memoryBindingDescsImpl[i];
        destDesc = srcDesc;
        destDesc.memory = memoryVal.GetImpl();
        destDesc.accelerationStructure = accelerationStructureVal.GetImpl();

        if (!memoryVal.IsWrapped()) {
            MemoryDesc memoryDesc = {};
            m_iRayTracingImpl.GetAccelerationStructureMemoryDesc(*accelerationStructureVal.GetImpl(), memoryVal.GetMemoryLocation(), memoryDesc);

            const uint64_t rangeMax = srcDesc.offset + memoryDesc.size;
            const bool memorySizeIsUnknown = memoryVal.GetSize() == 0;

            RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || srcDesc.offset == 0, Result::INVALID_ARGUMENT, "'[%u].offset' must be 0 for dedicated allocation", i);
            RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "'[%u].alignment' is 0", i);
            RETURN_ON_FAILURE(this, srcDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "'[%u].offset' is misaligned", i);
            RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memoryVal.GetSize(), Result::INVALID_ARGUMENT, "'[%u].offset' is invalid", i);
        }
    }

    Result result = m_iRayTracingImpl.BindAccelerationStructureMemory(memoryBindingDescsImpl, bindAccelerationStructureMemoryDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < bindAccelerationStructureMemoryDescNum; i++) {
            MemoryVal& memoryVal = *(MemoryVal*)bindAccelerationStructureMemoryDescs[i].memory;
            memoryVal.Bind(*(AccelerationStructureVal*)bindAccelerationStructureMemoryDescs[i].accelerationStructure);
        }
    }

    return result;
}

NRI_INLINE void DeviceVal::DestroyAccelerationStructure(AccelerationStructure* accelerationStructure) {
    m_iRayTracingImpl.DestroyAccelerationStructure(NRI_GET_IMPL(AccelerationStructure, accelerationStructure));
    Destroy((AccelerationStructureVal*)accelerationStructure);
}

NRI_INLINE void DeviceVal::DestroyMicromap(Micromap* micromap) {
    m_iRayTracingImpl.DestroyMicromap(NRI_GET_IMPL(Micromap, micromap));
    Destroy((MicromapVal*)micromap);
}
