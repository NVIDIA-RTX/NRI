// © 2026 NVIDIA Corporation

DeviceMetal::DeviceMetal(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks)
    : DeviceBase(callbacks, allocationCallbacks), m_ResidencyReferences(GetStdAllocator()), m_InternalShaders(*this) {
    m_Desc.graphicsAPI = GraphicsAPI::METAL;
    m_Desc.nriVersion = NRI_VERSION;
}

DeviceMetal::~DeviceMetal() {
    for (auto& queues : m_Queues) {
        for (QueueMetal* queue : queues)
            Destroy(GetAllocationCallbacks(), queue);
    }

    if (m_Constants)
        m_Constants->release();

    if (m_Compiler)
        m_Compiler->release();

    if (m_Residency)
        m_Residency->release();

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (m_TessellatorTables)
        m_TessellatorTables->release();
#endif

    if (m_Device)
        m_Device->release();
}

Result DeviceMetal::Create(const DeviceCreationDesc& desc, const DeviceCreationMetalDesc& metalDesc) {
    if (desc.enableGraphicsAPIValidation)
        NRI_REPORT_WARNING(this, "'enableGraphicsAPIValidation' is ignored, use 'MTL_DEBUG_LAYER=1'");

    m_Device = (MTL::Device*)metalDesc.mtlDevice;

    if (m_Device)
        m_Device->retain();
    else {
        // "uid.low" holds "registryID" (see "GetAdapterDescMetal")
        NS::Array* devices = MTL::CopyAllDevices();

        if (devices) {
            for (NS::UInteger i = 0; i < devices->count() && !m_Device; i++) {
                MTL::Device* candidate = devices->object<MTL::Device>(i);

                if (candidate->registryID() == desc.adapterDesc->uid.low)
                    m_Device = candidate->retain();
            }

            devices->release();
        }

        // The adapter may come from another API (i.e. merged by name)
        if (!m_Device)
            m_Device = MTL::CreateSystemDefaultDevice();

        if (!m_Device)
            return Result::UNSUPPORTED;
    }

    if (!m_Device->supportsFamily(MTL::GPUFamilyMetal4))
        return Result::UNSUPPORTED;

    {
        AutoreleasePoolMetal autoreleasePool;

        MTL::ResidencySetDescriptor* residencyDesc = MTL::ResidencySetDescriptor::alloc()->init();
        NS::Error* error = nullptr;
        m_Residency = m_Device->newResidencySet(residencyDesc, &error);
        residencyDesc->release();

        if (!m_Residency) {
            NRI_REPORT_ERROR(this, "newResidencySet() failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

            return Result::OUT_OF_MEMORY;
        }

        // Pipelines are built by Metal 4 compilers (thread-safe). Pipelines created with a pipeline cache use the cache's compiler
        MTL4::CompilerDescriptor* compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
        m_Compiler = m_Device->newCompiler(compilerDesc, &error);
        compilerDesc->release();

        if (!m_Compiler) {
            NRI_REPORT_ERROR(this, "newCompiler() failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

            return Result::FAILURE;
        }
    }

    Result result = m_InternalShaders.Create();

    if (result != Result::SUCCESS)
        return result;

    m_Constants = m_Device->newBuffer(64, MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked);

    if (!m_Constants)
        return Result::OUT_OF_MEMORY;

    m_ConstantsAddress = m_Constants->gpuAddress();

    uint16_t* constants = (uint16_t*)m_Constants->contents();
    memset(constants, 0, 64);
    constants[8] = 1;
    constants[16] = 2;
    AddResidency(m_Constants);

    FillDesc(*desc.adapterDesc);
    memset(m_Desc.adapterDesc.queueNum, 0, sizeof(m_Desc.adapterDesc.queueNum));

    for (uint32_t i = 0; i < desc.queueFamilyNum; i++) {
        const QueueFamilyDesc& queueFamily = desc.queueFamilies[i];
        const uint32_t queueType = (uint32_t)queueFamily.queueType;

        if (queueType >= QUEUE_TYPE_NUM_METAL)
            return Result::UNSUPPORTED;

        // Wrapped native queues ("queueFamilies" match)
        void* const* nativeQueues = i < metalDesc.queueFamilyNum ? metalDesc.queueFamilies[i].mtl4Queues : nullptr;

        // Duplicate families append queues
        uint32_t& queueNum = m_Desc.adapterDesc.queueNum[queueType];
        const uint32_t firstQueue = queueNum;
        const uint32_t newQueueNum = std::min(queueNum + queueFamily.queueNum, QUEUE_NUM_PER_TYPE_METAL);

        for (; queueNum < newQueueNum; queueNum++) {
            QueueMetal*& queue = m_Queues[queueType][queueNum];
            MTL4::CommandQueue* nativeQueue = nativeQueues ? (MTL4::CommandQueue*)nativeQueues[queueNum - firstQueue] : nullptr;

            result = nativeQueue ? CreateImplementation<QueueMetal>(queue, nativeQueue) : CreateImplementation<QueueMetal>(queue);

            if (result != Result::SUCCESS)
                return result;
        }
    }

    FillFunctionTable(m_Core);

    return Result::SUCCESS;
}

void DeviceMetal::AddResidency(MTL::Allocation* allocation) {
    std::lock_guard<std::mutex> lock(m_ResidencyLock);

    if (m_ResidencyReferences[allocation]++ == 0) {
        m_Residency->addAllocation(allocation);
        m_IsResidencyDirty.store(true, std::memory_order_release);
    }
}

void DeviceMetal::RemoveResidency(MTL::Allocation* allocation) {
    std::lock_guard<std::mutex> lock(m_ResidencyLock);

    auto it = m_ResidencyReferences.find(allocation);
    NRI_CHECK(it != m_ResidencyReferences.end(), "Unexpected residency removal");

    if (it == m_ResidencyReferences.end())
        return;

    if (--it->second == 0) {
        m_Residency->removeAllocation(allocation);
        m_ResidencyReferences.erase(it);
        m_IsResidencyDirty.store(true, std::memory_order_release);
    }
}

void DeviceMetal::CommitResidency() {
    // Changes made before a submission (in the app's order) are visible here
    if (!m_IsResidencyDirty.load(std::memory_order_acquire))
        return;

    std::lock_guard<std::mutex> lock(m_ResidencyLock);

    if (m_IsResidencyDirty.load(std::memory_order_relaxed)) {
        m_Residency->commit();
        m_IsResidencyDirty.store(false, std::memory_order_relaxed);
    }
}

void DeviceMetal::AddQueueResidencySet(MTL::ResidencySet* residencySet) {
    for (uint32_t i = 0; i < QUEUE_TYPE_NUM_METAL; i++) {
        for (uint32_t j = 0; j < m_Desc.adapterDesc.queueNum[i]; j++)
            m_Queues[i][j]->GetNativeObject()->addResidencySet(residencySet);
    }
}

void DeviceMetal::RemoveQueueResidencySet(MTL::ResidencySet* residencySet) {
    for (uint32_t i = 0; i < QUEUE_TYPE_NUM_METAL; i++) {
        for (uint32_t j = 0; j < m_Desc.adapterDesc.queueNum[i]; j++)
            m_Queues[i][j]->GetNativeObject()->removeResidencySet(residencySet);
    }
}

#if NRI_ENABLE_METAL_SHADER_CONVERTER
MTL::GPUAddress DeviceMetal::GetTessellatorTables() {
    std::lock_guard<std::mutex> lock(m_TessellatorTablesLock);

    if (!m_TessellatorTables) {
        m_TessellatorTables = m_Device->newBuffer(IRRuntimeTessellatorTablesSize(), MTL::ResourceStorageModeShared);

        if (!m_TessellatorTables)
            return 0;

        IRRuntimeLoadTessellatorTables(m_TessellatorTables);
        AddResidency(m_TessellatorTables);
    }

    return m_TessellatorTables->gpuAddress();
}
#endif

Result DeviceMetal::GetQueue(QueueType type, uint32_t index, Queue*& queue) {
    queue = nullptr;

    const uint32_t queueType = (uint32_t)type;

    if (queueType >= QUEUE_TYPE_NUM_METAL || !m_Desc.adapterDesc.queueNum[queueType])
        return Result::UNSUPPORTED;

    if (index >= m_Desc.adapterDesc.queueNum[queueType])
        return Result::INVALID_ARGUMENT;

    queue = (Queue*)m_Queues[queueType][index];

    return Result::SUCCESS;
}

template <typename Implementation, typename Interface, typename Desc>
static Result CreatePlacedImplementationMetal(DeviceMetal& device, Memory* memory, uint64_t offset, const Desc& desc, Interface*& entity) {
    // "memory = nullptr" means committed, "offset" holds "MemoryLocation"
    if (!memory)
        return device.CreateImplementation<Implementation>(entity, desc, (MemoryLocation)offset);

    Result result = device.CreateImplementation<Implementation>(entity, desc);

    if (result != Result::SUCCESS)
        return result;

    result = ((Implementation*)entity)->Bind(*(MemoryMetal*)memory, offset);

    if (result != Result::SUCCESS) {
        Destroy((Implementation*)entity);
        entity = nullptr;
    }

    return result;
}

Result DeviceMetal::CreatePlacedBuffer(Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return CreatePlacedImplementationMetal<BufferMetal>(*this, memory, offset, bufferDesc, buffer);
}

Result DeviceMetal::CreatePlacedTexture(Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture) {
    return CreatePlacedImplementationMetal<TextureMetal>(*this, memory, offset, textureDesc, texture);
}

Result DeviceMetal::CreatePlacedAccelerationStructure(Memory* memory, uint64_t offset, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return CreatePlacedImplementationMetal<AccelerationStructureMetal>(*this, memory, offset, accelerationStructureDesc, accelerationStructure);
}

Result DeviceMetal::WaitIdle() {
    for (uint32_t i = 0; i < QUEUE_TYPE_NUM_METAL; i++) {
        for (uint32_t j = 0; j < m_Desc.adapterDesc.queueNum[i]; j++) {
            Result result = m_Queues[i][j]->WaitIdle();

            if (result != Result::SUCCESS)
                return result;
        }
    }

    return Result::SUCCESS;
}

Result DeviceMetal::QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const {
    MaybeUnused(memoryLocation);

    // Apple silicon shares one memory budget across device and host-visible allocations
    videoMemoryInfo.budgetSize = m_Device->recommendedMaxWorkingSetSize();
    videoMemoryInfo.usageSize = m_Device->currentAllocatedSize();

    return Result::SUCCESS;
}

void DeviceMetal::Destruct() {
    Destroy(GetAllocationCallbacks(), this);
}

void DeviceMetal::FillDesc(const AdapterDesc& adapterDesc) {
    m_Desc.adapterDesc = adapterDesc;
    m_Desc.viewport.maxNum = 16;
    m_Desc.viewport.boundsMin = -32768;
    m_Desc.viewport.boundsMax = 32767;
    const uint32_t textureMaxDim = m_Device->supportsFamily(MTL::GPUFamilyApple10) ? 32768 : 16384;
    m_Desc.dimensions.attachmentMaxDim = textureMaxDim;
    m_Desc.dimensions.attachmentLayerMaxNum = 2048;
    m_Desc.dimensions.texture1DMaxDim = textureMaxDim;
    m_Desc.dimensions.texture2DMaxDim = textureMaxDim;
    m_Desc.dimensions.texture3DMaxDim = 2048;
    m_Desc.dimensions.textureLayerMaxNum = 2048;
    m_Desc.dimensions.typedBufferMaxDim = 256 * 1024 * 1024;
    // All memory is host-visible and device-local with unified memory
    m_Desc.memory.deviceUploadHeapSize = m_Device->hasUnifiedMemory() ? m_Device->recommendedMaxWorkingSetSize() : 0;
    m_Desc.memory.allocationMaxSize = m_Device->recommendedMaxWorkingSetSize();
    m_Desc.memory.bufferMaxSize = m_Device->maxBufferLength();
    m_Desc.memory.allocationMaxNum = UINT32_MAX;
    // Unique argument buffer samplers per app
    m_Desc.memory.samplerAllocationMaxNum = (uint32_t)m_Device->maxArgumentBufferSamplerCount();
    m_Desc.memory.constantBufferMaxRange = (uint32_t)std::min<uint64_t>(m_Desc.memory.bufferMaxSize, UINT32_MAX);
    m_Desc.memory.storageBufferMaxRange = (uint32_t)std::min<uint64_t>(m_Desc.memory.bufferMaxSize, UINT32_MAX);
    m_Desc.memory.bufferTextureGranularity = 1;
    m_Desc.memoryAlignment.uploadBufferTextureRow = 256;
    m_Desc.memoryAlignment.uploadBufferTextureSlice = 256;
    m_Desc.memoryAlignment.bufferShaderResourceOffset = 16;

    for (uint32_t i = 0; i < (uint32_t)Format::MAX_NUM; i++) {
        if (GetFormatSupportMetal(*m_Device, (Format)i) & FormatSupportBits::BUFFER) {
            const uint32_t alignment = (uint32_t)m_Device->minimumTextureBufferAlignmentForPixelFormat(GetPixelFormatMetal((Format)i));
            m_Desc.memoryAlignment.bufferShaderResourceOffset = std::max(m_Desc.memoryAlignment.bufferShaderResourceOffset, alignment);
        }
    }

    m_Desc.memoryAlignment.constantBufferOffset = 4;
    m_Desc.pipelineLayout.descriptorSetMaxNum = 8;
    m_Desc.pipelineLayout.rootConstantMaxSize = 256;
    m_Desc.pipelineLayout.rootDescriptorMaxNum = 31;
    m_Desc.pipelineLayout.rootSamplerMaxNum = 16;
    m_Desc.descriptorHeap.resourceMaxNum = 1000000;
    // Per-stage argument buffer sampler limits from the Metal feature set tables, bounded by the unique sampler limit
    m_Desc.descriptorHeap.samplerMaxNum = std::min((uint32_t)m_Device->maxArgumentBufferSamplerCount(), m_Device->supportsFamily(MTL::GPUFamilyApple9) ? 500000u : 996u);
    m_Desc.descriptorHeap.rootConstantMaxSize = 256;
    m_Desc.descriptorHeap.rootDescriptorMaxNum = 31;
    m_Desc.descriptorHeap.rootSamplerMaxNum = 16;
    m_Desc.descriptorSet.samplerMaxNum = m_Desc.descriptorHeap.samplerMaxNum;
    m_Desc.descriptorSet.constantBufferMaxNum = 1000000;
    m_Desc.descriptorSet.storageBufferMaxNum = 1000000;
    m_Desc.descriptorSet.textureMaxNum = 1000000;
    m_Desc.descriptorSet.storageTextureMaxNum = 1000000;
    m_Desc.descriptorSet.updateAfterSet.samplerMaxNum = m_Desc.descriptorHeap.samplerMaxNum;
    m_Desc.descriptorSet.updateAfterSet.constantBufferMaxNum = 1000000;
    m_Desc.descriptorSet.updateAfterSet.storageBufferMaxNum = 1000000;
    m_Desc.descriptorSet.updateAfterSet.textureMaxNum = 1000000;
    m_Desc.descriptorSet.updateAfterSet.storageTextureMaxNum = 1000000;
    m_Desc.shaderStage.descriptorSamplerMaxNum = 16;
    m_Desc.shaderStage.descriptorConstantBufferMaxNum = 31;
    m_Desc.shaderStage.descriptorStorageBufferMaxNum = 1000000;
    m_Desc.shaderStage.descriptorTextureMaxNum = 1000000;
    m_Desc.shaderStage.descriptorStorageTextureMaxNum = 1000000;
    m_Desc.shaderStage.resourceMaxNum = 1000000;
    m_Desc.shaderStage.updateAfterSet.descriptorSamplerMaxNum = m_Desc.descriptorHeap.samplerMaxNum;
    m_Desc.shaderStage.updateAfterSet.descriptorConstantBufferMaxNum = 1000000;
    m_Desc.shaderStage.updateAfterSet.descriptorStorageBufferMaxNum = 1000000;
    m_Desc.shaderStage.updateAfterSet.descriptorTextureMaxNum = 1000000;
    m_Desc.shaderStage.updateAfterSet.descriptorStorageTextureMaxNum = 1000000;
    m_Desc.shaderStage.updateAfterSet.resourceMaxNum = 1000000;
    m_Desc.shaderStage.vertex.attributeMaxNum = 31;
    m_Desc.shaderStage.vertex.streamMaxNum = 25;
    m_Desc.shaderStage.vertex.outputComponentMaxNum = 124;
    m_Desc.shaderStage.fragment.inputComponentMaxNum = 124;
    m_Desc.shaderStage.fragment.attachmentMaxNum = 8;
    m_Desc.shaderStage.fragment.dualSourceAttachmentMaxNum = 1;
    const MTL::Size maxThreads = m_Device->maxThreadsPerThreadgroup();
    const uint32_t threadgroupMemoryMaxSize = (uint32_t)m_Device->maxThreadgroupMemoryLength();
    m_Desc.shaderStage.compute.dispatchMaxDim[0] = UINT32_MAX;
    m_Desc.shaderStage.compute.dispatchMaxDim[1] = UINT32_MAX;
    m_Desc.shaderStage.compute.dispatchMaxDim[2] = UINT32_MAX;
    m_Desc.shaderStage.compute.workGroupInvocationMaxNum = (uint32_t)maxThreads.width;
    m_Desc.shaderStage.compute.workGroupMaxDim[0] = (uint32_t)maxThreads.width;
    m_Desc.shaderStage.compute.workGroupMaxDim[1] = (uint32_t)maxThreads.height;
    m_Desc.shaderStage.compute.workGroupMaxDim[2] = (uint32_t)maxThreads.depth;
    m_Desc.shaderStage.compute.sharedMemoryMaxSize = threadgroupMemoryMaxSize;
    m_Desc.shaderStage.task.dispatchMaxDim[0] = UINT32_MAX;
    m_Desc.shaderStage.task.dispatchMaxDim[1] = UINT32_MAX;
    m_Desc.shaderStage.task.dispatchMaxDim[2] = UINT32_MAX;
    m_Desc.shaderStage.task.workGroupInvocationMaxNum = (uint32_t)maxThreads.width;
    m_Desc.shaderStage.task.workGroupMaxDim[0] = (uint32_t)maxThreads.width;
    m_Desc.shaderStage.task.workGroupMaxDim[1] = (uint32_t)maxThreads.height;
    m_Desc.shaderStage.task.workGroupMaxDim[2] = (uint32_t)maxThreads.depth;
    m_Desc.shaderStage.task.sharedMemoryMaxSize = threadgroupMemoryMaxSize;
    m_Desc.shaderStage.task.payloadMaxSize = 16 * 1024;
    m_Desc.shaderStage.task.dispatchWorkGroupMaxNum = UINT32_MAX;
    m_Desc.shaderStage.mesh.dispatchMaxDim[0] = UINT32_MAX;
    m_Desc.shaderStage.mesh.dispatchMaxDim[1] = UINT32_MAX;
    m_Desc.shaderStage.mesh.dispatchMaxDim[2] = UINT32_MAX;
    m_Desc.shaderStage.mesh.workGroupInvocationMaxNum = (uint32_t)maxThreads.width;
    m_Desc.shaderStage.mesh.workGroupMaxDim[0] = (uint32_t)maxThreads.width;
    m_Desc.shaderStage.mesh.workGroupMaxDim[1] = (uint32_t)maxThreads.height;
    m_Desc.shaderStage.mesh.workGroupMaxDim[2] = (uint32_t)maxThreads.depth;
    m_Desc.shaderStage.mesh.sharedMemoryMaxSize = threadgroupMemoryMaxSize;
    m_Desc.shaderStage.mesh.outputVerticesMaxNum = 256;
    m_Desc.shaderStage.mesh.outputPrimitiveMaxNum = 512;
    // Metal's mesh unique-scalar limit includes the position output.
    m_Desc.shaderStage.mesh.outputComponentMaxNum = 124;
    // Apple7/Apple8 limit mesh grids to 1024 threadgroups per draw.
    m_Desc.shaderStage.mesh.dispatchWorkGroupMaxNum = m_Device->supportsFamily(MTL::GPUFamilyApple10) ? 4194303 : (m_Device->supportsFamily(MTL::GPUFamilyApple9) ? 1048575 : 1024);
    m_Desc.wave.laneMinNum = 32;
    m_Desc.wave.laneMaxNum = 32;
    m_Desc.wave.waveOpsStages = StageBits::ALL_SHADERS;
    m_Desc.wave.quadOpsStages = StageBits::FRAGMENT_SHADER | StageBits::COMPUTE_SHADER;
    m_Desc.wave.derivativeOpsStages = StageBits::FRAGMENT_SHADER | StageBits::COMPUTE_SHADER | StageBits::TASK_SHADER | StageBits::MESH_SHADER;

    for (uint32_t count = 1; count <= 32 && m_Device->supportsVertexAmplificationCount(count); count++)
        m_Desc.other.viewMaxNum = (uint8_t)count;

    // Indirect draws are issued one by one, "drawNum" bounds the CPU loop and the per-call upload of filtered arguments and roots
    m_Desc.other.drawIndirectMaxNum = DRAW_INDIRECT_MAX_NUM_METAL;
    m_Desc.other.samplerAnisotropyMax = 16.0f;
    m_Desc.tiers.resourceBinding = 2;
    m_Desc.tiers.bindless = 2;
    m_Desc.tiers.memory = 1;
    m_Desc.tiers.sampleLocations = 1;
    m_Desc.features.descriptorHeap = true;
    m_Desc.features.swapChain = true;
    // Drawables keep the swap chain size, "CAMetalLayer" scales them to the layer bounds
    m_Desc.features.resizableSwapChain = true;
    m_Desc.features.waitableSwapChain = true;
    m_Desc.features.presentFromCompute = true;
    m_Desc.features.layerBasedMultiview = m_Desc.other.viewMaxNum > 1;
    m_Desc.features.viewportBasedMultiview = m_Desc.other.viewMaxNum > 1;
    m_Desc.features.enhancedBarriers = true;
    // Metal has no texture layouts
    m_Desc.features.unifiedTextureLayouts = true;
    m_Desc.features.getMemoryDesc2 = true;
    m_Desc.features.resourceAliasing = true;
    m_Desc.features.componentSwizzle = true;
    m_Desc.features.constantAlphaBlendFactors = true;
    m_Desc.features.independentFrontAndBackStencilReferenceAndMasks = true;
    m_Desc.features.dynamicDepthBias = true;
    m_Desc.features.depthBoundsTest = m_Device->supportsFamily(MTL::GPUFamilyApple10);
    m_Desc.features.rectColorClears = true;
    m_Desc.features.rectDepthStencilClears = true;
    m_Desc.features.regionResolve = true;
    m_Desc.features.resolveOpMinMax = true;
    m_Desc.features.rootConstantsOffset = true;
    m_Desc.features.nonConstantBufferRootDescriptorOffset = true;
    m_Desc.features.textureCompressionBC = m_Device->supportsBCTextureCompression();
    m_Desc.features.textureCompressionETC2 = true;
    m_Desc.features.textureCompressionASTC = true;
    m_Desc.features.shaderBytecodeMETALLIB = true;
    // Mesh shaders (including indirect "drawMeshThreadgroups") are Apple7+
    m_Desc.features.meshShader = m_Device->supportsFamily(MTL::GPUFamilyApple7);
    m_Desc.features.drawIndirectCount = true;
    m_Desc.features.occlusion = true;
    m_Desc.features.timestamp = true;
    m_Desc.features.timestampCopyQueue = true;
    m_Desc.features.calibratedTimestamps = true;
    m_Desc.features.pipelineCache = true;
    m_Desc.features.pipelineCacheControl = true;
    m_Desc.features.extendedDynamicState = true;
    m_Desc.features.mutableDescriptorType = true;
    // Counter heap timestamps are in GPU ticks, not nanoseconds
    m_Desc.other.timestampFrequencyHz = m_Device->queryTimestampFrequency();
    m_Desc.other.clipDistanceMaxNum = 8;
    m_Desc.other.combinedClipAndCullDistanceMaxNum = 8;
    m_Desc.other.texelOffsetMin = -8;
    m_Desc.other.texelOffsetMax = 7;
    m_Desc.other.texelGatherOffsetMin = -8;
    m_Desc.other.texelGatherOffsetMax = 7;
    m_Desc.other.samplerLodBiasMax = 15.984375f; // Largest positive S4.6 native sampler bias.
    // Metal 4 devices meet Apple7's baseline; Converter also supports these
    // scalar types and SM6 wave/packed-dot intrinsics.
    m_Desc.shaderFeatures.nativeI16 = true;
    m_Desc.shaderFeatures.nativeF16 = true;
    m_Desc.shaderFeatures.nativeI64 = true;
    m_Desc.shaderFeatures.atomicsF32 = true;
    // MSL has only 64-bit "atomic_min/max", Converter rejects 64-bit "InterlockedAdd/Exchange/Or/..."
    m_Desc.shaderFeatures.atomicsI64 = false;
    m_Desc.shaderFeatures.barycentric = true;
    m_Desc.shaderFeatures.rasterizedOrderedView = true;
    m_Desc.shaderFeatures.storageReadWithoutFormat = true;
    m_Desc.shaderFeatures.storageWriteWithoutFormat = true;
    m_Desc.shaderFeatures.waveQuery = true;
    m_Desc.shaderFeatures.waveVote = true;
    m_Desc.shaderFeatures.waveShuffle = true;
    m_Desc.shaderFeatures.waveArithmetic = true;
    m_Desc.shaderFeatures.waveReduction = true;
    m_Desc.shaderFeatures.waveQuad = true;
    m_Desc.shaderFeatures.integerDotProduct = true;
    m_Desc.shaderFeatures.unnormalizedCoordinates = true;
    m_Desc.shaderFeatures.viewportIndex = true;
    m_Desc.shaderFeatures.layerIndex = true;
    // DXIL uses Metal Shader Converter framebuffer fetches. Native MSL can use
    // color inputs (for example, `float4 value [[color(1)]]`) directly.
    m_Desc.shaderFeatures.inputAttachments = true;
    m_Desc.shaderFeatures.drawParameters = true; // root data emulation works for native and converted shaders (see "NRI.metal")
    m_Desc.shaderFeatures.drawIndex = true;
    m_Desc.features.flexibleMultiview = m_Desc.other.viewMaxNum > 1;
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    m_Desc.shaderStage.vertex.attributeMaxNum = CONVERTED_VERTEX_ATTRIBUTE_NUM;
    m_Desc.shaderModel = NriShaderModel(6, 8);
    m_Desc.features.shaderBytecodeDXIL = true;
    // Converter GS/TS emulation dispatches object threadgroups proportionally to the primitive count, which quickly exceeds the
    // 1024 threadgroups per mesh grid limit of Apple7/Apple8
    m_Desc.features.geometryShader = m_Device->supportsFamily(MTL::GPUFamilyApple9);
    m_Desc.features.tessellationShader = m_Desc.features.geometryShader;

    // Ray tracing pipelines are DXIL-only (Converter's DXR emulation). Metal 4 acceleration structures require Apple9+ (despite "supportsRaytracing")
    if (m_Device->supportsFamily(MTL::GPUFamilyApple9)) {
        m_Desc.tiers.rayTracing = 2;
        m_Desc.memoryAlignment.scratchBufferOffset = 256;
        m_Desc.memoryAlignment.shaderBindingTable = 64;
        m_Desc.shaderStage.rayTracing.shaderGroupIdentifierSize = sizeof(IRShaderIdentifier);
        m_Desc.shaderStage.rayTracing.shaderBindingTableMaxStride = 4096;
        m_Desc.shaderStage.rayTracing.recursionMaxDepth = 31;
        m_Desc.accelerationStructure.primitiveMaxNum = 1u << 24;
        m_Desc.accelerationStructure.geometryMaxNum = 1u << 20;
        m_Desc.accelerationStructure.instanceMaxNum = 1u << 20;
    }
#endif
}
