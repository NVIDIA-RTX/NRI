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
      } {
    m_Desc.graphicsAPI = GraphicsAPI::WGPU;
    m_Desc.nriVersion = NRI_VERSION;
}

DeviceWGPU::~DeviceWGPU() {
    WaitIdle();

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

    WGPUFeatureName requiredFeatures[16] = {};
    size_t requiredFeatureNum = 0;
    // TODO: Root constants rely on the wgpu-native "immediates" extension, not core WebGPU.
    requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_Immediates;

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
    if (wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_BGRA8UnormStorage))
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_BGRA8UnormStorage;

    bool isTimestampQuerySupported = wgpuAdapterHasFeature(m_Adapter, WGPUFeatureName_TimestampQuery) == WGPU_TRUE;
    bool isTimestampQueryInsideEncodersSupported = wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsideEncoders) == WGPU_TRUE;
    bool isTimestampQueryInsidePassesSupported = wgpuAdapterHasFeature(m_Adapter, (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsidePasses) == WGPU_TRUE;
    if (isTimestampQuerySupported && isTimestampQueryInsideEncodersSupported && isTimestampQueryInsidePassesSupported) {
        requiredFeatures[requiredFeatureNum++] = WGPUFeatureName_TimestampQuery;
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsideEncoders;
        requiredFeatures[requiredFeatureNum++] = (WGPUFeatureName)WGPUNativeFeature_TimestampQueryInsidePasses;
    }

    WGPUNativeLimits nativeLimits = {};
    nativeLimits.chain.sType = (WGPUSType)WGPUSType_NativeLimits;
    nativeLimits.maxImmediateSize = 256;

    WGPULimits requiredLimits = WGPU_LIMITS_INIT;
    requiredLimits.nextInChain = &nativeLimits.chain;
    requiredLimits.maxImmediateSize = 256;

    WGPUDeviceExtras deviceExtras = {};
    deviceExtras.chain.sType = (WGPUSType)WGPUSType_DeviceExtras;

    WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
    deviceDesc.nextInChain = &deviceExtras.chain;
    deviceDesc.requiredFeatureCount = requiredFeatureNum;
    deviceDesc.requiredFeatures = requiredFeatures;
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

    m_Desc.shaderStage.compute.dispatchMaxDim[0] = limits.maxComputeWorkgroupsPerDimension;
    m_Desc.shaderStage.compute.dispatchMaxDim[1] = limits.maxComputeWorkgroupsPerDimension;
    m_Desc.shaderStage.compute.dispatchMaxDim[2] = limits.maxComputeWorkgroupsPerDimension;
    m_Desc.shaderStage.compute.workGroupInvocationMaxNum = limits.maxComputeInvocationsPerWorkgroup;
    m_Desc.shaderStage.compute.workGroupMaxDim[0] = limits.maxComputeWorkgroupSizeX;
    m_Desc.shaderStage.compute.workGroupMaxDim[1] = limits.maxComputeWorkgroupSizeY;
    m_Desc.shaderStage.compute.workGroupMaxDim[2] = limits.maxComputeWorkgroupSizeZ;
    m_Desc.shaderStage.compute.sharedMemoryMaxSize = limits.maxComputeWorkgroupStorageSize;

    WGPUAdapterInfo adapterInfo = WGPU_ADAPTER_INFO_INIT;
    wgpuAdapterGetInfo(m_Adapter, &adapterInfo);
    m_Desc.wave.laneMinNum = adapterInfo.subgroupMinSize;
    m_Desc.wave.laneMaxNum = adapterInfo.subgroupMaxSize;
    wgpuAdapterInfoFreeMembers(adapterInfo);

    m_Desc.wave.waveOpsStages = StageBits::COMPUTE_SHADER;
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
    m_Desc.other.clipDistanceMaxNum = 8;
    m_Desc.other.cullDistanceMaxNum = 8;
    m_Desc.other.combinedClipAndCullDistanceMaxNum = 8;
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
    m_Desc.features.rootConstantsOffset = true;
    m_Desc.shaderFeatures.drawParameters = true;
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

FormatSupportBits DeviceWGPU::GetFormatSupport(Format format) const {
    if (IsBCFormat(format) && !m_Desc.features.textureCompressionBC)
        return FormatSupportBits::UNSUPPORTED;
    if (IsETC2Format(format) && !m_Desc.features.textureCompressionETC2)
        return FormatSupportBits::UNSUPPORTED;
    if (IsASTCFormat(format) && !m_Desc.features.textureCompressionASTC)
        return FormatSupportBits::UNSUPPORTED;
    if (format == Format::D32_SFLOAT_S8_UINT && wgpuDeviceHasFeature(m_Device, WGPUFeatureName_Depth32FloatStencil8) != WGPU_TRUE)
        return FormatSupportBits::UNSUPPORTED;

    FormatSupportBits support = GetFormatSupportWGPU(format);
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
