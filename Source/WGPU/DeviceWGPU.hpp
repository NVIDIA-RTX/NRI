// © 2026 NVIDIA Corporation

struct RequestAdapterContext {
    WGPUAdapter adapter = nullptr;
    WGPURequestAdapterStatus status = WGPURequestAdapterStatus_Error;
    bool done = false;
};

struct RequestDeviceContext {
    WGPUDevice device = nullptr;
    WGPURequestDeviceStatus status = WGPURequestDeviceStatus_Error;
    bool done = false;
};

static void OnAdapterRequested(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView, void* userdata1, void*) {
    RequestAdapterContext& context = *(RequestAdapterContext*)userdata1;
    context.status = status;
    context.adapter = adapter;
    context.done = true;
}

static void OnDeviceRequested(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView, void* userdata1, void*) {
    RequestDeviceContext& context = *(RequestDeviceContext*)userdata1;
    context.status = status;
    context.device = device;
    context.done = true;
}

static void WaitForAsyncRequest(WGPUInstance instance, const bool& done) {
    // TODO: This is a busy wait around async WGPU requests. Prefer a blocking/event-based path if wgpu-native exposes one.
    while (!done) {
        wgpuInstanceProcessEvents(instance);
        std::this_thread::yield();
    }
}

DeviceWGPU::DeviceWGPU(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks)
    : DeviceBase(callbacks, allocationCallbacks)
    , m_QueueFamilies{
          Vector<QueueWGPU*>(GetStdAllocator()),
          Vector<QueueWGPU*>(GetStdAllocator()),
          Vector<QueueWGPU*>(GetStdAllocator()),
      }
    , m_HostCopyContexts(GetStdAllocator()) {
    m_Desc.graphicsAPI = GraphicsAPI::WGPU;
    m_Desc.nriVersion = NRI_VERSION;
}

DeviceWGPU::~DeviceWGPU() {
    WaitIdle();

    for (HostCopyContextWGPU* context : m_HostCopyContexts) {
        if (context->readbackBuffer)
            wgpuBufferRelease(context->readbackBuffer);
        Destroy(GetAllocationCallbacks(), context);
    }

    for (auto& queueFamily : m_QueueFamilies) {
        for (QueueWGPU* queue : queueFamily)
            Destroy(GetAllocationCallbacks(), queue);
    }

    if (m_Queue)
        wgpuQueueRelease(m_Queue);
    if (m_Device)
        wgpuDeviceRelease(m_Device);
    if (m_Adapter)
        wgpuAdapterRelease(m_Adapter);
    if (m_Instance)
        wgpuInstanceRelease(m_Instance);
}

Result DeviceWGPU::Create(const DeviceCreationDesc& desc) {
    m_BindingOffsets = desc.vkBindingOffsets;

    Result result = CreateInstanceAndDevice(desc);
    if (result != Result::SUCCESS)
        return result;

    FillDesc(*desc.adapterDesc);

    for (uint32_t i = 0; i < desc.queueFamilyNum; i++) {
        const QueueFamilyDesc& queueFamilyDesc = desc.queueFamilies[i];
        Vector<QueueWGPU*>& queueFamily = m_QueueFamilies[(uint32_t)queueFamilyDesc.queueType];

        for (uint32_t j = 0; j < queueFamilyDesc.queueNum; j++) {
            QueueWGPU* queue = Allocate<QueueWGPU>(GetAllocationCallbacks(), *this);
            if (!queue)
                return Result::OUT_OF_MEMORY;

            result = queue->Create(queueFamilyDesc.queueType, j);
            if (result != Result::SUCCESS) {
                Destroy(GetAllocationCallbacks(), queue);
                return result;
            }

            queueFamily.push_back(queue);
        }
    }

    return FillFunctionTable(m_iCore);
}

Result DeviceWGPU::CreateInstanceAndDevice(const DeviceCreationDesc& desc) {
    WGPUInstanceExtras instanceExtras = {};
    instanceExtras.chain.sType = (WGPUSType)WGPUSType_InstanceExtras;
    // TODO: Backend selection is left to wgpu-native. Forcing DX12 was observed to crash during early WGPU backend profiling.
    instanceExtras.backends = WGPUInstanceBackend_Primary;
    instanceExtras.flags = desc.enableGraphicsAPIValidation ? WGPUInstanceFlag_Debugging : WGPUInstanceFlag_Empty;

    WGPUInstanceFeatureName instanceFeatures[] = {WGPUInstanceFeatureName_ShaderSourceSPIRV};

    WGPUInstanceDescriptor instanceDesc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    instanceDesc.nextInChain = &instanceExtras.chain;
    instanceDesc.requiredFeatureCount = GetCountOf(instanceFeatures);
    instanceDesc.requiredFeatures = instanceFeatures;

    m_Instance = wgpuCreateInstance(&instanceDesc);
    if (!m_Instance)
        return Result::FAILURE;

    WGPURequestAdapterOptions adapterOptions = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;

    RequestAdapterContext adapterContext = {};
    WGPURequestAdapterCallbackInfo adapterCallbackInfo = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    adapterCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    adapterCallbackInfo.callback = OnAdapterRequested;
    adapterCallbackInfo.userdata1 = &adapterContext;

    wgpuInstanceRequestAdapter(m_Instance, &adapterOptions, adapterCallbackInfo);
    WaitForAsyncRequest(m_Instance, adapterContext.done);

    if (adapterContext.status != WGPURequestAdapterStatus_Success || !adapterContext.adapter)
        return Result::FAILURE;

    m_Adapter = adapterContext.adapter;

    std::array<WGPUFeatureName, 48> requiredFeatures = {};
    size_t requiredFeatureNum = 0;
    // TODO: Root constants rely on the wgpu-native "immediates" extension, not core WebGPU.
    requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_Immediates;

    if (wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures))
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures;
    if (wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_TextureFormat16bitNorm))
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_TextureFormat16bitNorm;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TextureCompressionBC))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TextureCompressionBC;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TextureCompressionETC2))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TextureCompressionETC2;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TextureCompressionASTC))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TextureCompressionASTC;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_IndirectFirstInstance))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_IndirectFirstInstance;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_Depth32FloatStencil8))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_Depth32FloatStencil8;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_ClipDistances))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_ClipDistances;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_BGRA8UnormStorage))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_BGRA8UnormStorage;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_ShaderF16))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_ShaderF16;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_RG11B10UfloatRenderable))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_RG11B10UfloatRenderable;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_Float32Filterable))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_Float32Filterable;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_Float32Blendable))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_Float32Blendable;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_DualSourceBlending))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_DualSourceBlending;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_Subgroups))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_Subgroups;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TextureFormatsTier1))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TextureFormatsTier1;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TextureFormatsTier2))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TextureFormatsTier2;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_PrimitiveIndex))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_PrimitiveIndex;
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TextureComponentSwizzle))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TextureComponentSwizzle;
    if (wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_TextureBindingArray))
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_TextureBindingArray;
    if (wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_SampledTextureAndStorageBufferArrayNonUniformIndexing))
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_SampledTextureAndStorageBufferArrayNonUniformIndexing;
    if (wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_StorageResourceBindingArray))
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_StorageResourceBindingArray;
    if (wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_BufferBindingArray))
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_BufferBindingArray;
    if (wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_StorageTextureArrayNonUniformIndexing))
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_StorageTextureArrayNonUniformIndexing;

    bool isTimestampQuerySupported = wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TimestampQuery) == WGPU_TRUE;
    bool isTimestampQueryInsideEncodersSupported = wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsideEncoders) == WGPU_TRUE;
    bool isTimestampQueryInsidePassesSupported = wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsidePasses) == WGPU_TRUE;
    if (isTimestampQuerySupported && isTimestampQueryInsideEncodersSupported && isTimestampQueryInsidePassesSupported) {
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TimestampQuery;
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsideEncoders;
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsidePasses;
    }

    WGPULimits requiredLimits = WGPU_LIMITS_INIT;
    requiredLimits.maxImmediateSize = 256;

    WGPUDeviceExtras deviceExtras = {};
    deviceExtras.chain.sType = (WGPUSType)WGPUSType_DeviceExtras;

    WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
    deviceDesc.nextInChain = &deviceExtras.chain;
    deviceDesc.requiredFeatureCount = requiredFeatureNum;
    deviceDesc.requiredFeatures = requiredFeatures.data();
    deviceDesc.requiredLimits = &requiredLimits;

    RequestDeviceContext deviceContext = {};
    WGPURequestDeviceCallbackInfo deviceCallbackInfo = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    deviceCallbackInfo.callback = OnDeviceRequested;
    deviceCallbackInfo.userdata1 = &deviceContext;

    wgpuAdapterRequestDevice(m_Adapter, &deviceDesc, deviceCallbackInfo);
    WaitForAsyncRequest(m_Instance, deviceContext.done);

    if (deviceContext.status != WGPURequestDeviceStatus_Success || !deviceContext.device)
        return Result::FAILURE;

    m_Device = deviceContext.device;
    m_Queue = wgpuDeviceGetQueue(m_Device);
    m_IsSubgroupsSupported = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_Subgroups) == WGPU_TRUE;
    m_IsTimestampQueryInsidePassesSupported = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_TimestampQuery) == WGPU_TRUE
        && wgpuDeviceHasFeature(m_Device, (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsideEncoders) == WGPU_TRUE
        && wgpuDeviceHasFeature(m_Device, (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsidePasses) == WGPU_TRUE;

    return m_Queue ? Result::SUCCESS : Result::FAILURE;
}

void DeviceWGPU::FillDesc(const AdapterDesc& adapterDesc) {
    m_Desc.adapterDesc = adapterDesc;
    m_Desc.adapterDesc.queueNum[(uint32_t)QueueType::GRAPHICS] = 1;
    m_Desc.adapterDesc.queueNum[(uint32_t)QueueType::COMPUTE] = 0;
    m_Desc.adapterDesc.queueNum[(uint32_t)QueueType::COPY] = 0;

    WGPULimits limits = WGPU_LIMITS_INIT;
    wgpuDeviceGetLimits(m_Device, &limits);

    // TODO: Compatibility placeholder. WebGPU/WGSL does not expose a D3D-style shader model.
    m_Desc.shaderModel = NriShaderModel(6, 0);

    m_Desc.viewport.maxNum = 1;
    m_Desc.viewport.boundsMin = -32768;
    m_Desc.viewport.boundsMax = 32767;

    m_Desc.dimensions.attachmentMaxDim = (Dim_t)std::min<uint32_t>(limits.maxTextureDimension2D, UINT16_MAX);
    m_Desc.dimensions.attachmentLayerMaxNum = (Dim_t)std::min<uint32_t>(limits.maxTextureArrayLayers, UINT16_MAX);
    m_Desc.dimensions.texture1DMaxDim = (Dim_t)std::min<uint32_t>(limits.maxTextureDimension1D, UINT16_MAX);
    m_Desc.dimensions.texture2DMaxDim = (Dim_t)std::min<uint32_t>(limits.maxTextureDimension2D, UINT16_MAX);
    m_Desc.dimensions.texture3DMaxDim = (Dim_t)std::min<uint32_t>(limits.maxTextureDimension3D, UINT16_MAX);
    m_Desc.dimensions.textureLayerMaxNum = (Dim_t)std::min<uint32_t>(limits.maxTextureArrayLayers, UINT16_MAX);
    m_Desc.dimensions.typedBufferMaxDim = 128 * 1024 * 1024;

    m_Desc.precision.viewportBits = 8;
    m_Desc.precision.subPixelBits = 8;
    m_Desc.precision.subTexelBits = 8;
    m_Desc.precision.mipmapBits = 8;

    m_Desc.memory.deviceUploadHeapSize = 0;
    m_Desc.memory.bufferMaxSize = limits.maxBufferSize;
    m_Desc.memory.allocationMaxSize = limits.maxBufferSize;
    m_Desc.memory.allocationMaxNum = uint32_t(-1);
    m_Desc.memory.samplerAllocationMaxNum = 4096;
    m_Desc.memory.constantBufferMaxRange = (uint32_t)limits.maxUniformBufferBindingSize;
    m_Desc.memory.storageBufferMaxRange = (uint32_t)std::min<uint64_t>(limits.maxStorageBufferBindingSize, UINT32_MAX);
    m_Desc.memory.bufferTextureGranularity = 1;

    m_Desc.memoryAlignment.uploadBufferTextureRow = 256;
    m_Desc.memoryAlignment.uploadBufferTextureSlice = 1;
    m_Desc.memoryAlignment.bufferShaderResourceOffset = (uint32_t)limits.minStorageBufferOffsetAlignment;
    m_Desc.memoryAlignment.constantBufferOffset = (uint32_t)limits.minUniformBufferOffsetAlignment;
    m_Desc.memoryAlignment.scratchBufferOffset = 1;
    m_Desc.memoryAlignment.shaderBindingTable = 1;
    m_Desc.memoryAlignment.accelerationStructureOffset = 1;
    m_Desc.memoryAlignment.micromapOffset = 1;

    m_Desc.pipelineLayout.descriptorSetMaxNum = limits.maxBindGroups;
    m_Desc.pipelineLayout.rootConstantMaxSize = 256;
    m_Desc.pipelineLayout.rootDescriptorMaxNum = 8;

    m_Desc.descriptorSet.samplerMaxNum = limits.maxSamplersPerShaderStage;
    m_Desc.descriptorSet.constantBufferMaxNum = limits.maxUniformBuffersPerShaderStage;
    m_Desc.descriptorSet.storageBufferMaxNum = limits.maxStorageBuffersPerShaderStage;
    m_Desc.descriptorSet.textureMaxNum = limits.maxSampledTexturesPerShaderStage;
    m_Desc.descriptorSet.storageTextureMaxNum = limits.maxStorageTexturesPerShaderStage;

    m_Desc.shaderStage.descriptorSamplerMaxNum = limits.maxSamplersPerShaderStage;
    m_Desc.shaderStage.descriptorConstantBufferMaxNum = limits.maxUniformBuffersPerShaderStage;
    m_Desc.shaderStage.descriptorStorageBufferMaxNum = limits.maxStorageBuffersPerShaderStage;
    m_Desc.shaderStage.descriptorTextureMaxNum = limits.maxSampledTexturesPerShaderStage;
    m_Desc.shaderStage.descriptorStorageTextureMaxNum = limits.maxStorageTexturesPerShaderStage;
    m_Desc.shaderStage.resourceMaxNum = limits.maxBindingsPerBindGroup;

    m_Desc.shaderStage.vertex.attributeMaxNum = limits.maxVertexAttributes;
    m_Desc.shaderStage.vertex.streamMaxNum = limits.maxVertexBuffers;
    m_Desc.shaderStage.vertex.outputComponentMaxNum = 60;

    m_Desc.shaderStage.fragment.inputComponentMaxNum = 60;
    m_Desc.shaderStage.fragment.attachmentMaxNum = limits.maxColorAttachments;
    m_Desc.shaderStage.fragment.dualSourceAttachmentMaxNum = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_DualSourceBlending) == WGPU_TRUE ? 1 : 0;

    m_Desc.shaderStage.compute.dispatchMaxDim[0] = limits.maxComputeWorkgroupsPerDimension;
    m_Desc.shaderStage.compute.dispatchMaxDim[1] = limits.maxComputeWorkgroupsPerDimension;
    m_Desc.shaderStage.compute.dispatchMaxDim[2] = limits.maxComputeWorkgroupsPerDimension;
    m_Desc.shaderStage.compute.workGroupInvocationMaxNum = limits.maxComputeInvocationsPerWorkgroup;
    m_Desc.shaderStage.compute.workGroupMaxDim[0] = limits.maxComputeWorkgroupSizeX;
    m_Desc.shaderStage.compute.workGroupMaxDim[1] = limits.maxComputeWorkgroupSizeY;
    m_Desc.shaderStage.compute.workGroupMaxDim[2] = limits.maxComputeWorkgroupSizeZ;
    m_Desc.shaderStage.compute.sharedMemoryMaxSize = limits.maxComputeWorkgroupStorageSize;

    if (m_IsSubgroupsSupported) {
        WGPUAdapterInfo adapterInfo = WGPU_ADAPTER_INFO_INIT;
        wgpuAdapterGetInfo(m_Adapter, &adapterInfo);
        m_Desc.wave.laneMinNum = adapterInfo.subgroupMinSize;
        m_Desc.wave.laneMaxNum = adapterInfo.subgroupMaxSize;
        wgpuAdapterInfoFreeMembers(adapterInfo);

        m_Desc.wave.waveOpsStages = StageBits::COMPUTE_SHADER;
    }

    m_Desc.wave.derivativeOpsStages = StageBits::FRAGMENT_SHADER;

    if (m_IsTimestampQueryInsidePassesSupported) {
        float timestampPeriod = wgpuQueueGetTimestampPeriod(m_Queue);
        m_Desc.other.timestampFrequencyHz = timestampPeriod > 0.0f ? uint64_t(1e9 / double(timestampPeriod) + 0.5) : 1;
    } else
        m_Desc.other.timestampFrequencyHz = 1;

    m_Desc.other.drawIndirectMaxNum = 1;
    m_Desc.other.samplerLodBiasMax = 16.0f;
    m_Desc.other.samplerAnisotropyMax = 16;
    m_Desc.other.texelOffsetMin = -8;
    m_Desc.other.texelOffsetMax = 7;
    m_Desc.other.texelGatherOffsetMin = -8;
    m_Desc.other.texelGatherOffsetMax = 7;
    m_Desc.other.clipDistanceMaxNum = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_ClipDistances) == WGPU_TRUE ? 8 : 0;
    m_Desc.other.cullDistanceMaxNum = 0;
    m_Desc.other.combinedClipAndCullDistanceMaxNum = m_Desc.other.clipDistanceMaxNum;
    m_Desc.other.viewMaxNum = 1;

    m_Desc.tiers.resourceBinding = 0;
    m_Desc.tiers.bindless = 0;
    m_Desc.tiers.memory = 1;

    // TODO: Unsupported WebGPU features are intentionally left false/zero in "DeviceDesc"; add explicit caps only when WGPU can back the NRI behavior.
    m_Desc.features.swapChain = true;
    m_Desc.features.textureCompressionBC = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_TextureCompressionBC) == WGPU_TRUE;
    m_Desc.features.textureCompressionETC2 = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_TextureCompressionETC2) == WGPU_TRUE;
    m_Desc.features.textureCompressionASTC = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_TextureCompressionASTC) == WGPU_TRUE;
    m_Desc.features.shaderBytecodeSPIRV = true;
    m_Desc.features.shaderBytecodeWGSL = true;
    m_Desc.features.timestamp = m_IsTimestampQueryInsidePassesSupported;
    m_Desc.features.getMemoryDesc2 = true;
    m_Desc.features.componentSwizzle = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_TextureComponentSwizzle) == WGPU_TRUE;
    m_Desc.features.rootConstantsOffset = true;
    m_Desc.shaderFeatures.nativeF16 = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_ShaderF16) == WGPU_TRUE;
    m_Desc.shaderFeatures.drawParameters = wgpuDeviceHasFeature(m_Device, WGPUFeatureName_IndirectFirstInstance) == WGPU_TRUE;
}

void DeviceWGPU::Destruct() {
    Destroy(GetAllocationCallbacks(), this);
}

static bool IsBCFormat(Format format) {
    switch (format) {
        case Format::BC1_RGBA_UNORM:
        case Format::BC1_RGBA_SRGB:
        case Format::BC2_RGBA_UNORM:
        case Format::BC2_RGBA_SRGB:
        case Format::BC3_RGBA_UNORM:
        case Format::BC3_RGBA_SRGB:
        case Format::BC4_R_UNORM:
        case Format::BC4_R_SNORM:
        case Format::BC5_RG_UNORM:
        case Format::BC5_RG_SNORM:
        case Format::BC6H_RGB_UFLOAT:
        case Format::BC6H_RGB_SFLOAT:
        case Format::BC7_RGBA_UNORM:
        case Format::BC7_RGBA_SRGB:
            return true;
        default:
            return false;
    }
}

static bool IsETC2Format(Format format) {
    switch (format) {
        case Format::ETC2_RGB8_UNORM:
        case Format::ETC2_RGB8_SRGB:
        case Format::ETC2_RGB8_A1_UNORM:
        case Format::ETC2_RGB8_A1_SRGB:
        case Format::ETC2_RGB8_A8_UNORM:
        case Format::ETC2_RGB8_A8_SRGB:
        case Format::ETC2_R11_UNORM:
        case Format::ETC2_R11_SNORM:
        case Format::ETC2_R11_G11_UNORM:
        case Format::ETC2_R11_G11_SNORM:
            return true;
        default:
            return false;
    }
}

static bool IsASTCFormat(Format format) {
    switch (format) {
        case Format::ASTC_4X4_UNORM:
        case Format::ASTC_4X4_SRGB:
        case Format::ASTC_5X4_UNORM:
        case Format::ASTC_5X4_SRGB:
        case Format::ASTC_5X5_UNORM:
        case Format::ASTC_5X5_SRGB:
        case Format::ASTC_6X5_UNORM:
        case Format::ASTC_6X5_SRGB:
        case Format::ASTC_6X6_UNORM:
        case Format::ASTC_6X6_SRGB:
        case Format::ASTC_8X5_UNORM:
        case Format::ASTC_8X5_SRGB:
        case Format::ASTC_8X6_UNORM:
        case Format::ASTC_8X6_SRGB:
        case Format::ASTC_8X8_UNORM:
        case Format::ASTC_8X8_SRGB:
        case Format::ASTC_10X5_UNORM:
        case Format::ASTC_10X5_SRGB:
        case Format::ASTC_10X6_UNORM:
        case Format::ASTC_10X6_SRGB:
        case Format::ASTC_10X8_UNORM:
        case Format::ASTC_10X8_SRGB:
        case Format::ASTC_10X10_UNORM:
        case Format::ASTC_10X10_SRGB:
        case Format::ASTC_12X10_UNORM:
        case Format::ASTC_12X10_SRGB:
        case Format::ASTC_12X12_UNORM:
        case Format::ASTC_12X12_SRGB:
            return true;
        default:
            return false;
    }
}

static bool Is16BitNormFormat(Format format) {
    switch (format) {
        case Format::R16_UNORM:
        case Format::R16_SNORM:
        case Format::RG16_UNORM:
        case Format::RG16_SNORM:
        case Format::RGBA16_UNORM:
        case Format::RGBA16_SNORM:
            return true;
        default:
            return false;
    }
}

FormatSupportBits DeviceWGPU::GetFormatSupport(Format format) const {
    if (IsBCFormat(format) && !m_Desc.features.textureCompressionBC)
        return FormatSupportBits::UNSUPPORTED;
    if (IsETC2Format(format) && !m_Desc.features.textureCompressionETC2)
        return FormatSupportBits::UNSUPPORTED;
    if (IsASTCFormat(format) && !m_Desc.features.textureCompressionASTC)
        return FormatSupportBits::UNSUPPORTED;
    if (Is16BitNormFormat(format) && wgpuDeviceHasFeature(m_Device, (WGPUFeatureName)WGPUNativeFeature_TextureFormat16bitNorm) != WGPU_TRUE)
        return FormatSupportBits::UNSUPPORTED;
    if (format == Format::D32_SFLOAT_S8_UINT && wgpuDeviceHasFeature(m_Device, WGPUFeatureName_Depth32FloatStencil8) != WGPU_TRUE)
        return FormatSupportBits::UNSUPPORTED;

    FormatSupportBits support = GetFormatSupportWGPU(format);
    if (format == Format::R11_G11_B10_UFLOAT && wgpuDeviceHasFeature(m_Device, WGPUFeatureName_RG11B10UfloatRenderable) != WGPU_TRUE)
        support &= ~(FormatSupportBits::COLOR_ATTACHMENT | FormatSupportBits::MULTISAMPLE_4X | FormatSupportBits::MULTISAMPLE_RESOLVE | FormatSupportBits::BLEND);

    if (wgpuDeviceHasFeature(m_Device, WGPUFeatureName_Float32Blendable) == WGPU_TRUE) {
        if (format == Format::R32_SFLOAT || format == Format::RG32_SFLOAT || format == Format::RGBA32_SFLOAT)
            support |= FormatSupportBits::BLEND;
    }

    if (format == Format::BGRA8_UNORM && wgpuDeviceHasFeature(m_Device, WGPUFeatureName_BGRA8UnormStorage) == WGPU_TRUE)
        support |= FormatSupportBits::STORAGE_TEXTURE;

    return support;
}

Result DeviceWGPU::GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    const Vector<QueueWGPU*>& queueFamily = m_QueueFamilies[(uint32_t)queueType];
    if (queueFamily.empty())
        return Result::UNSUPPORTED;

    if (queueIndex < queueFamily.size()) {
        queue = (Queue*)queueFamily[queueIndex];
        return Result::SUCCESS;
    }

    return Result::INVALID_ARGUMENT;
}

Result DeviceWGPU::WaitIdle() {
    if (m_Device)
        wgpuDevicePoll(m_Device, WGPU_TRUE, nullptr);

    return Result::SUCCESS;
}

HostCopyLayoutWGPU DeviceWGPU::GetHostCopyLayout(const TextureWGPU& texture, const TextureRegionDesc& region, uint64_t& offset, bool alignForBufferCopy) const {
    const TextureDesc& textureDesc = texture.GetDesc();
    const FormatProps& formatProps = GetFormatProps(textureDesc.format);

    HostCopyLayoutWGPU layout = {};
    uint32_t width = region.width == WHOLE_SIZE ? GetDimension(GraphicsAPI::WGPU, textureDesc, 0, region.mipOffset) : region.width;
    uint32_t height = region.height == WHOLE_SIZE ? GetDimension(GraphicsAPI::WGPU, textureDesc, 1, region.mipOffset) : region.height;
    layout.width = Align(width, formatProps.blockWidth);
    layout.height = Align(height, formatProps.blockHeight);
    layout.depth = textureDesc.type == TextureType::TEXTURE_3D ? (region.depth == WHOLE_SIZE ? GetDimension(GraphicsAPI::WGPU, textureDesc, 2, region.mipOffset) : region.depth) : 1;
    layout.rowNum = (layout.height + formatProps.blockHeight - 1) / formatProps.blockHeight;
    layout.rowSize = ((layout.width + formatProps.blockWidth - 1) / formatProps.blockWidth) * formatProps.stride;
    layout.rowPitch = alignForBufferCopy ? Align(layout.rowSize, 256u) : layout.rowSize;
    layout.slicePitch = layout.rowPitch * layout.rowNum;

    uint64_t offsetAlignment = alignForBufferCopy ? std::lcm<uint64_t>(4, formatProps.stride) : 1;
    offset = Align(offset, offsetAlignment);
    layout.offset = offset;
    offset += uint64_t(layout.slicePitch) * layout.depth;

    return layout;
}

Result DeviceWGPU::CopyHostMemoryToTexture(const CopyHostMemoryToTextureDesc* copyDescs, uint32_t copyDescNum) {
    if (!copyDescNum)
        return Result::SUCCESS;

    for (uint32_t i = 0; i < copyDescNum; i++) {
        const CopyHostMemoryToTextureDesc& copyDesc = copyDescs[i];
        const TextureWGPU& texture = *(TextureWGPU*)copyDesc.dstTexture;
        uint64_t ignoredOffset = 0;
        HostCopyLayoutWGPU copyLayout = GetHostCopyLayout(texture, copyDesc.dstRegion, ignoredOffset, false);
        uint32_t rowPitch = copyDesc.srcRowPitch ? copyDesc.srcRowPitch : copyLayout.rowSize;
        uint32_t slicePitch = copyDesc.srcSlicePitch ? copyDesc.srcSlicePitch : rowPitch * copyLayout.rowNum;
        size_t dataSize = (size_t)(uint64_t(copyLayout.depth - 1) * slicePitch + uint64_t(copyLayout.rowNum - 1) * rowPitch + copyLayout.rowSize);

        WGPUTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
        dst.texture = texture;
        dst.mipLevel = copyDesc.dstRegion.mipOffset;
        dst.origin.x = copyDesc.dstRegion.x;
        dst.origin.y = copyDesc.dstRegion.y;
        dst.origin.z = texture.GetDesc().type == TextureType::TEXTURE_3D ? copyDesc.dstRegion.z : copyDesc.dstRegion.layerOffset;
        dst.aspect = GetTextureAspect(copyDesc.dstRegion.planes);

        WGPUTexelCopyBufferInfo src = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
        src.layout.bytesPerRow = rowPitch;
        src.layout.rowsPerImage = slicePitch / rowPitch;

        WGPUExtent3D extent = {copyLayout.width, copyLayout.height, copyLayout.depth};
        wgpuQueueWriteTexture(m_Queue, &dst, copyDesc.srcData, dataSize, &src.layout, &extent);
    }

    return WaitIdle();
}

Result DeviceWGPU::CopyTextureToHostMemory(const CopyTextureToHostMemoryDesc* copyDescs, uint32_t copyDescNum) {
    if (!copyDescNum)
        return Result::SUCCESS;

    HostCopyContextWGPU* context = nullptr;
    Result result = AcquireHostCopyContext(context);
    if (result != Result::SUCCESS)
        return result;

    Vector<HostCopyLayoutWGPU> layouts(GetStdAllocator());
    layouts.reserve(copyDescNum);

    uint64_t stagingSize = 0;
    for (uint32_t i = 0; i < copyDescNum; i++)
        layouts.push_back(GetHostCopyLayout(*(TextureWGPU*)copyDescs[i].srcTexture, copyDescs[i].srcRegion, stagingSize, true));

    result = EnsureReadbackBuffer(*context, stagingSize);

    WGPUCommandEncoder encoder = nullptr;
    WGPUCommandBuffer commandBuffer = nullptr;
    if (result == Result::SUCCESS) {
        WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
        encoder = wgpuDeviceCreateCommandEncoder(m_Device, &encoderDesc);
        if (!encoder)
            result = Result::FAILURE;
    }

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < copyDescNum; i++) {
            const CopyTextureToHostMemoryDesc& copyDesc = copyDescs[i];
            const TextureWGPU& texture = *(TextureWGPU*)copyDesc.srcTexture;
            const HostCopyLayoutWGPU& layout = layouts[i];

            WGPUTexelCopyTextureInfo src = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
            src.texture = texture;
            src.mipLevel = copyDesc.srcRegion.mipOffset;
            src.origin.x = copyDesc.srcRegion.x;
            src.origin.y = copyDesc.srcRegion.y;
            src.origin.z = texture.GetDesc().type == TextureType::TEXTURE_3D ? copyDesc.srcRegion.z : copyDesc.srcRegion.layerOffset;
            src.aspect = GetTextureAspect(copyDesc.srcRegion.planes);

            WGPUTexelCopyBufferInfo dst = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
            dst.buffer = context->readbackBuffer;
            dst.layout.offset = layout.offset;
            dst.layout.bytesPerRow = layout.rowPitch;
            dst.layout.rowsPerImage = layout.rowNum;

            WGPUExtent3D extent = {layout.width, layout.height, layout.depth};
            wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &extent);
        }

        WGPUCommandBufferDescriptor commandBufferDesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
        commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
        if (!commandBuffer)
            result = Result::FAILURE;
    }

    if (result == Result::SUCCESS)
        wgpuQueueSubmitForIndex(m_Queue, 1, &commandBuffer);

    if (commandBuffer)
        wgpuCommandBufferRelease(commandBuffer);
    if (encoder)
        wgpuCommandEncoderRelease(encoder);

    struct MapContext {
        bool completed;
        WGPUMapAsyncStatus status;
    } mapContext = {};

    if (result == Result::SUCCESS) {
        WGPUBufferMapCallbackInfo callbackInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        callbackInfo.userdata1 = &mapContext;
        callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
            MapContext& context = *(MapContext*)userdata1;
            context.completed = true;
            context.status = status;
        };

        wgpuBufferMapAsync(context->readbackBuffer, WGPUMapMode_Read, 0, (size_t)stagingSize, callbackInfo);
        while (!mapContext.completed) {
            wgpuDevicePoll(m_Device, WGPU_TRUE, nullptr);
            wgpuInstanceProcessEvents(m_Instance);
        }

        if (mapContext.status != WGPUMapAsyncStatus_Success)
            result = Result::FAILURE;
    }

    const uint8_t* stagingData = nullptr;
    if (result == Result::SUCCESS) {
        stagingData = (const uint8_t*)wgpuBufferGetConstMappedRange(context->readbackBuffer, 0, (size_t)stagingSize);
        if (!stagingData)
            result = Result::FAILURE;
    }

    for (uint32_t i = 0; result == Result::SUCCESS && i < copyDescNum; i++) {
        const CopyTextureToHostMemoryDesc& copyDesc = copyDescs[i];
        const HostCopyLayoutWGPU& layout = layouts[i];
        uint32_t dstRowPitch = copyDesc.dstRowPitch ? copyDesc.dstRowPitch : layout.rowSize;
        uint32_t dstSlicePitch = copyDesc.dstSlicePitch ? copyDesc.dstSlicePitch : dstRowPitch * layout.rowNum;

        for (uint32_t z = 0; z < layout.depth; z++) {
            for (uint32_t y = 0; y < layout.rowNum; y++) {
                const uint8_t* srcRow = stagingData + layout.offset + uint64_t(z) * layout.slicePitch + uint64_t(y) * layout.rowPitch;
                uint8_t* dstRow = (uint8_t*)copyDesc.dstData + uint64_t(z) * dstSlicePitch + uint64_t(y) * dstRowPitch;
                memcpy(dstRow, srcRow, layout.rowSize);
            }
        }
    }

    if (mapContext.status == WGPUMapAsyncStatus_Success)
        wgpuBufferUnmap(context->readbackBuffer);

    ReleaseHostCopyContext(*context);

    return result;
}

Result DeviceWGPU::AcquireHostCopyContext(HostCopyContextWGPU*& context) {
    ExclusiveScope lock(m_HostCopyContextLock);

    for (HostCopyContextWGPU* candidate : m_HostCopyContexts) {
        if (!candidate->isInUse) {
            candidate->isInUse = true;
            context = candidate;
            return Result::SUCCESS;
        }
    }

    context = Allocate<HostCopyContextWGPU>(GetAllocationCallbacks());
    if (!context)
        return Result::OUT_OF_MEMORY;

    context->isInUse = true;
    m_HostCopyContexts.push_back(context);

    return Result::SUCCESS;
}

void DeviceWGPU::ReleaseHostCopyContext(HostCopyContextWGPU& context) {
    ExclusiveScope lock(m_HostCopyContextLock);

    if (context.readbackBufferSize > MAX_CACHED_HOST_COPY_BUFFER_SIZE) {
        wgpuBufferRelease(context.readbackBuffer);
        context.readbackBuffer = nullptr;
        context.readbackBufferSize = 0;
    }

    context.isInUse = false;
}

Result DeviceWGPU::EnsureReadbackBuffer(HostCopyContextWGPU& context, uint64_t size) {
    if (size <= context.readbackBufferSize)
        return Result::SUCCESS;

    uint64_t newSize = size;
    if (size <= MAX_CACHED_HOST_COPY_BUFFER_SIZE && context.readbackBufferSize)
        newSize = std::min(std::max(context.readbackBufferSize * 2, size), MAX_CACHED_HOST_COPY_BUFFER_SIZE);
    newSize = Align(newSize, 4ull);

    WGPUBufferDescriptor bufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
    bufferDesc.size = newSize;
    bufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);
    if (!buffer)
        return Result::FAILURE;

    if (context.readbackBuffer)
        wgpuBufferRelease(context.readbackBuffer);

    context.readbackBuffer = buffer;
    context.readbackBufferSize = newSize;

    return Result::SUCCESS;
}
