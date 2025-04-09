// © 2021 NVIDIA Corporation

static uint8_t QueryLatestDevice(ComPtr<ID3D12DeviceBest>& in, ComPtr<ID3D12DeviceBest>& out) {
    static const IID versions[] = {
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
        __uuidof(ID3D12Device14),
        __uuidof(ID3D12Device13),
        __uuidof(ID3D12Device12),
        __uuidof(ID3D12Device11),
        __uuidof(ID3D12Device10),
        __uuidof(ID3D12Device9),
        __uuidof(ID3D12Device8),
        __uuidof(ID3D12Device7),
        __uuidof(ID3D12Device6),
#endif
        __uuidof(ID3D12Device5),
        __uuidof(ID3D12Device4),
        __uuidof(ID3D12Device3),
        __uuidof(ID3D12Device2),
        __uuidof(ID3D12Device1),
        __uuidof(ID3D12Device),
    };
    const uint8_t n = (uint8_t)GetCountOf(versions);

    uint8_t i = 0;
    for (; i < n; i++) {
        HRESULT hr = in->QueryInterface(versions[i], (void**)&out);
        if (SUCCEEDED(hr))
            break;
    }

    return n - i - 1;
}

static inline uint64_t HashRootSignatureAndStride(ID3D12RootSignature* rootSignature, uint32_t stride) {
    CHECK(stride < 4096, "Only stride < 4096 supported by encoding");
    return ((uint64_t)stride << 52ull) | ((uint64_t)rootSignature & ((1ull << 52) - 1));
}

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
typedef ID3D12InfoQueue1 ID3D12InfoQueueBest;

static void __stdcall MessageCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR message, void* context) {
    MaybeUnused(id, category);

    Message messageType = Message::INFO;
    if (severity < D3D12_MESSAGE_SEVERITY_WARNING)
        messageType = Message::ERROR;
    else if (severity == D3D12_MESSAGE_SEVERITY_WARNING)
        messageType = Message::WARNING;

    DeviceD3D12& device = *(DeviceD3D12*)context;
    device.ReportMessage(messageType, __FILE__, __LINE__, "%s", message);
}

#else
typedef ID3D12InfoQueue ID3D12InfoQueueBest;
#endif

#if NRI_ENABLE_D3D_EXTENSIONS

static void __stdcall NvapiMessageCallback(void* context, NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY severity, const char* messageCode, const char* message, const char* messageDetails) {
    Message messageType = Message::INFO;
    if (severity == NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_ERROR)
        messageType = Message::ERROR;
    else if (severity == NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_WARNING)
        messageType = Message::WARNING;

    DeviceD3D12& device = *(DeviceD3D12*)context;
    device.ReportMessage(messageType, __FILE__, __LINE__, "%s: %s", messageCode, message);
    device.ReportMessage(messageType, __FILE__, __LINE__, "Details: %s", messageDetails);
}

#endif

static D3D12_RESOURCE_FLAGS GetBufferFlags(BufferUsageBits bufferUsage) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (bufferUsage & (BufferUsageBits::SHADER_RESOURCE_STORAGE | BufferUsageBits::SCRATCH_BUFFER))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (bufferUsage & (BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE | BufferUsageBits::MICROMAP_STORAGE))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;

    return flags;
}

static D3D12_RESOURCE_FLAGS GetTextureFlags(TextureUsageBits textureUsage) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (textureUsage & TextureUsageBits::SHADER_RESOURCE_STORAGE)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (textureUsage & TextureUsageBits::COLOR_ATTACHMENT)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if (textureUsage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if (!(textureUsage & TextureUsageBits::SHADER_RESOURCE))
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    return flags;
}

DeviceD3D12::DeviceD3D12(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks)
    : DeviceBase(callbacks, allocationCallbacks)
    , m_DescriptorHeaps(GetStdAllocator())
    , m_FreeDescriptors(GetStdAllocator())
    , m_DrawCommandSignatures(GetStdAllocator())
    , m_DrawIndexedCommandSignatures(GetStdAllocator())
    , m_DrawMeshCommandSignatures(GetStdAllocator())
    , m_QueueFamilies{
          Vector<QueueD3D12*>(GetStdAllocator()),
          Vector<QueueD3D12*>(GetStdAllocator()),
          Vector<QueueD3D12*>(GetStdAllocator()),
      } {
    m_FreeDescriptors.resize(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, Vector<DescriptorHandle>(GetStdAllocator()));

    m_Desc.graphicsAPI = GraphicsAPI::D3D12;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;
}

DeviceD3D12::~DeviceD3D12() {
    for (auto& queueFamily : m_QueueFamilies) {
        for (uint32_t i = 0; i < queueFamily.size(); i++)
            Destroy<QueueD3D12>(queueFamily[i]);
    }

#if NRI_ENABLE_D3D_EXTENSIONS
    if (HasAmdExt() && !m_IsWrapped)
        m_AmdExt.DestroyDeviceD3D12(m_AmdExt.context, m_Device, nullptr);
#endif
}

Result DeviceD3D12::Create(const DeviceCreationDesc& desc, const DeviceCreationD3D12Desc& descD3D12) {
    m_IsWrapped = descD3D12.d3d12Device != nullptr;

    // Get adapter description as early as possible for meaningful error reporting
    m_Desc.adapterDesc = *desc.adapterDesc;

    // IMPORTANT: Must be called before the D3D12 device is created, or the D3D12 runtime removes the device
    if (desc.enableGraphicsAPIValidation) {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            debugController->EnableDebugLayer();
    }

    { // Get adapter
        ComPtr<IDXGIFactory4> dxgiFactory;
        HRESULT hr = CreateDXGIFactory2(desc.enableGraphicsAPIValidation ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgiFactory));
        RETURN_ON_BAD_HRESULT(this, hr, "CreateDXGIFactory2()");

        LUID luid = *(LUID*)&desc.adapterDesc->luid;
        hr = dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&m_Adapter));
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIFactory4::EnumAdapterByLuid()");
    }

    // Extensions
    InitializePixExt();
    if (m_Desc.adapterDesc.vendor == Vendor::NVIDIA)
        InitializeNvExt(descD3D12.isNVAPILoaded, descD3D12.d3d12Device != nullptr);
    else if (m_Desc.adapterDesc.vendor == Vendor::AMD)
        InitializeAmdExt(descD3D12.agsContext, descD3D12.d3d12Device != nullptr);

    // Device
    ComPtr<ID3D12DeviceBest> deviceTemp = (ID3D12DeviceBest*)descD3D12.d3d12Device;
    if (!m_IsWrapped) {
#if NRI_ENABLE_D3D_EXTENSIONS
        bool isShaderAtomicsI64Supported = false;
        bool isShaderClockSupported = false;
        uint32_t d3dShaderExtRegister = desc.d3dShaderExtRegister ? desc.d3dShaderExtRegister : NRI_SHADER_EXT_REGISTER;

        if (HasAmdExt()) {
            AGSDX12DeviceCreationParams deviceCreationParams = {};
            deviceCreationParams.pAdapter = m_Adapter;
            deviceCreationParams.iid = __uuidof(ID3D12DeviceBest);
            deviceCreationParams.FeatureLevel = D3D_FEATURE_LEVEL_11_0;

            AGSDX12ExtensionParams extensionsParams = {};
            extensionsParams.uavSlot = d3dShaderExtRegister;

            AGSDX12ReturnedParams agsParams = {};
            AGSReturnCode result = m_AmdExt.CreateDeviceD3D12(m_AmdExt.context, &deviceCreationParams, &extensionsParams, &agsParams);
            RETURN_ON_FAILURE(this, result == AGS_SUCCESS, Result::FAILURE, "agsDriverExtensionsDX11_CreateDevice() failed: %d", (int32_t)result);

            deviceTemp = (ID3D12DeviceBest*)agsParams.pDevice;
            isShaderAtomicsI64Supported = agsParams.extensionsSupported.intrinsics19;
            isShaderClockSupported = agsParams.extensionsSupported.shaderClock;
        } else {
#endif
            HRESULT hr = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&deviceTemp);
            RETURN_ON_BAD_HRESULT(this, hr, "D3D12CreateDevice()");

#if NRI_ENABLE_D3D_EXTENSIONS
            if (HasNvExt()) {
                REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D12_SetNvShaderExtnSlotSpace(deviceTemp, d3dShaderExtRegister, 0));
                REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_UINT64_ATOMIC, &isShaderAtomicsI64Supported));
                REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_GET_SPECIAL, &isShaderClockSupported));
            }
        }

        // Start filling here to avoid passing additional arguments into "FillDesc"
        m_Desc.shaderFeatures.atomicsI64 = isShaderAtomicsI64Supported;
        m_Desc.shaderFeatures.clock = isShaderClockSupported;
#endif
    }

    m_Version = QueryLatestDevice(deviceTemp, m_Device);
    REPORT_INFO(this, "Using ID3D12Device%u", m_Version);

    if (desc.enableGraphicsAPIValidation) {
        ComPtr<ID3D12InfoQueueBest> pInfoQueue;
        HRESULT hr = m_Device->QueryInterface(&pInfoQueue);

        if (SUCCEEDED(hr)) {
            hr = pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            RETURN_ON_BAD_HRESULT(this, hr, "ID3D12InfoQueue::SetBreakOnSeverity()");

            hr = pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            RETURN_ON_BAD_HRESULT(this, hr, "ID3D12InfoQueue::SetBreakOnSeverity()");

            // TODO: this code is currently needed to disable known false-positive errors reported by the debug layer
            D3D12_MESSAGE_ID disableMessageIDs[] = {
                // It's almost impossible to match. Doesn't hurt perf on modern HW
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                // Descriptor validation doesn't understand acceleration structures used outside of RAYGEN shaders
                D3D12_MESSAGE_ID_COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH,
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.pIDList = disableMessageIDs;
            filter.DenyList.NumIDs = GetCountOf(disableMessageIDs);
            hr = pInfoQueue->AddStorageFilterEntries(&filter);
            RETURN_ON_BAD_HRESULT(this, hr, "ID3D12InfoQueue::AddStorageFilterEntries()");

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
            DWORD cookie = 0;
            hr = pInfoQueue->RegisterMessageCallback(MessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &cookie);
            RETURN_ON_BAD_HRESULT(this, hr, "ID3D12InfoQueue1::RegisterMessageCallback()");
#endif
        }
    }

    // Create queues
    memset(m_Desc.adapterDesc.queueNum, 0, sizeof(m_Desc.adapterDesc.queueNum)); // patch to reflect available queues
    if (m_IsWrapped) {
        for (uint32_t i = 0; i < descD3D12.queueFamilyNum; i++) {
            const QueueFamilyD3D12Desc& queueFamilyDesc = descD3D12.queueFamilies[i];
            auto& queueFamily = m_QueueFamilies[(size_t)queueFamilyDesc.queueType];

            for (uint32_t j = 0; j < queueFamilyDesc.queueNum; j++) {
                QueueD3D12* queue = nullptr;
                Result result = Result::FAILURE;
                if (queueFamilyDesc.d3d12Queues) {
                    ID3D12CommandQueue* commandQueue = queueFamilyDesc.d3d12Queues[j];
                    result = CreateImplementation<QueueD3D12>(queue, commandQueue);
                } else
                    result = CreateImplementation<QueueD3D12>(queue, queueFamilyDesc.queueType, 0.0f);

                if (result == Result::SUCCESS)
                    queueFamily.push_back(queue);
            }

            m_Desc.adapterDesc.queueNum[(size_t)queueFamilyDesc.queueType] = queueFamilyDesc.queueNum;
        }
    } else {
        for (uint32_t i = 0; i < desc.queueFamilyNum; i++) {
            const QueueFamilyDesc& queueFamilyDesc = desc.queueFamilies[i];
            auto& queueFamily = m_QueueFamilies[(size_t)queueFamilyDesc.queueType];

            for (uint32_t j = 0; j < queueFamilyDesc.queueNum; j++) {
                float priority = queueFamilyDesc.queuePriorities ? queueFamilyDesc.queuePriorities[j] : 0.0f;

                QueueD3D12* queue = nullptr;
                Result result = CreateImplementation<QueueD3D12>(queue, queueFamilyDesc.queueType, priority);
                if (result == Result::SUCCESS)
                    queueFamily.push_back(queue);
            }

            m_Desc.adapterDesc.queueNum[(size_t)queueFamilyDesc.queueType] = queueFamilyDesc.queueNum;
        }
    }

    // NV-specific ray tracing features
#if NRI_ENABLE_D3D_EXTENSIONS
    if (HasNvExt()) {
        { // Check position fetch support
            bool isSupported = false;
            REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_RT_TRIANGLE_OBJECT_POSITIONS, &isSupported));
            m_Desc.shaderFeatures.rayTracingPositionFetch = isSupported;
        }

        // Enable ray tracing validation
        if (desc.enableD3D12RayTracingValidation) {
            NvAPI_Status status = NvAPI_D3D12_EnableRaytracingValidation(m_Device, NVAPI_D3D12_RAYTRACING_VALIDATION_FLAG_NONE);
            if (status == NVAPI_OK) {
                void* unused = nullptr;
                REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D12_RegisterRaytracingValidationMessageCallback(m_Device, NvapiMessageCallback, this, &unused));
                // TODO: add "NvAPI_D3D12_FlushRaytracingValidationMessages" somewhere?
            }
        }
    }
#endif

    { // Create zero buffer
        D3D12_RESOURCE_DESC zeroBufferDesc = {};
        zeroBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        zeroBufferDesc.Width = desc.d3dZeroBufferSize ? desc.d3dZeroBufferSize : ZERO_BUFFER_DEFAULT_SIZE;
        zeroBufferDesc.Height = 1;
        zeroBufferDesc.DepthOrArraySize = 1;
        zeroBufferDesc.MipLevels = 1;
        zeroBufferDesc.SampleDesc.Count = 1;
        zeroBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        zeroBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = NODE_MASK;
        heapProps.VisibleNodeMask = NODE_MASK;

        HRESULT hr = m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &zeroBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_ZeroBuffer));
        RETURN_ON_BAD_HRESULT(this, hr, "ID3D12Device::CreateCommittedResource()");
    }

    // Fill desc
    FillDesc();

    // Create indirect command signatures
    m_DispatchCommandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, sizeof(DispatchDesc), nullptr);
    if (m_Desc.tiers.rayTracing >= 2)
        m_DispatchRaysCommandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS, sizeof(DispatchRaysIndirectDesc), nullptr);

    return FillFunctionTable(m_iCore);
}

void DeviceD3D12::FillDesc() {
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options) failed, result = 0x%08X!", hr);
    m_Desc.tiers.memory = options.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2 ? 1 : 0;

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options1) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options2) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options3) failed, result = 0x%08X!", hr);
    m_Desc.features.copyQueueTimestamp = options3.CopyQueueTimestampQueriesSupported;

    D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options4) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options5) failed, result = 0x%08X!", hr);
    m_Desc.features.rayTracing = options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
    if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_1_0)
        m_Desc.tiers.rayTracing = 1;
    else if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_1_1)
        m_Desc.tiers.rayTracing = 2;

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options6) failed, result = 0x%08X!", hr);
    m_Desc.tiers.shadingRate = (uint8_t)options6.VariableShadingRateTier;
    m_Desc.other.shadingRateAttachmentTileSize = (uint8_t)options6.ShadingRateImageTileSize;
    m_Desc.features.additionalShadingRates = options6.AdditionalShadingRatesSupported;

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options7) failed, result = 0x%08X!", hr);
    m_Desc.features.meshShader = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    // Minimum supported client: Windows 10 Build 20348 (or Agility SDK)
    D3D12_FEATURE_DATA_D3D12_OPTIONS8 options8 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &options8, sizeof(options8));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options8) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options9) failed, result = 0x%08X!", hr);
    m_Desc.features.meshShaderPipelineStats = options9.MeshShaderPipelineStatsSupported;

    // Minimum supported client: Windows 11 Build 22000 (or Agility SDK)
    D3D12_FEATURE_DATA_D3D12_OPTIONS10 options10 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &options10, sizeof(options10));
    if (FAILED(hr))
        REPORT_ERROR(this, "ID3D12Device::CheckFeatureSupport(options10) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS11 options11 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &options11, sizeof(options11));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options11) failed, result = 0x%08X!", hr);

    // Minimum supported client: Windows 11 22H2 (or Agility SDK)
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options12) failed, result = 0x%08X!", hr);
    m_Desc.features.enchancedBarrier = options12.EnhancedBarriersSupported;

    D3D12_FEATURE_DATA_D3D12_OPTIONS13 options13 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &options13, sizeof(options13));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options13) failed, result = 0x%08X!", hr);
    m_Desc.memoryAlignment.uploadBufferTextureRow = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    m_Desc.memoryAlignment.uploadBufferTextureSlice = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    m_Desc.features.viewportOriginBottomLeft = options13.InvertedViewportHeightFlipsYSupported ? 1 : 0;

    // Minimum supported client: Agility SDK
    D3D12_FEATURE_DATA_D3D12_OPTIONS14 options14 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS14, &options14, sizeof(options14));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options14) failed, result = 0x%08X!", hr);
    m_Desc.features.independentFrontAndBackStencilReferenceAndMasks = options14.IndependentFrontAndBackStencilRefMaskSupported ? true : false;

    D3D12_FEATURE_DATA_D3D12_OPTIONS15 options15 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS15, &options15, sizeof(options15));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options15) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options16) failed, result = 0x%08X!", hr);
    m_Desc.memory.deviceUploadHeapSize = options16.GPUUploadHeapSupported ? m_Desc.adapterDesc.videoMemorySize : 0;
    m_Desc.features.dynamicDepthBias = options16.DynamicDepthBiasSupported;

    D3D12_FEATURE_DATA_D3D12_OPTIONS17 options17 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS17, &options17, sizeof(options17));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options17) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options18) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS19 options19 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &options19, sizeof(options19));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options19) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS20 options20 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS20, &options20, sizeof(options20));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options20) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options21) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_TIGHT_ALIGNMENT tightAlignment = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_TIGHT_ALIGNMENT, &tightAlignment, sizeof(tightAlignment));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(tightAlignment) failed, result = 0x%08X!", hr);
    m_TightAlignmentTier = (uint8_t)tightAlignment.SupportTier;
#else
    m_Desc.uploadBufferTextureRowAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    m_Desc.uploadBufferTextureSliceAlignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
#endif

    // Feature level
    const std::array<D3D_FEATURE_LEVEL, 5> levelsList = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_2,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {};
    levels.NumFeatureLevels = (uint32_t)levelsList.size();
    levels.pFeatureLevelsRequested = levelsList.data();
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS) failed, result = 0x%08X!", hr);

    // Timestamp frequency
    uint64_t timestampFrequency = 0;
    {
        Queue* queue = nullptr;
        Result result = GetQueue(QueueType::GRAPHICS, 0, queue);
        if (result == Result::SUCCESS) {
            ID3D12CommandQueue* queueD3D12 = *(QueueD3D12*)queue;
            queueD3D12->GetTimestampFrequency(&timestampFrequency);
        }
    }

    // Shader model
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {D3D_HIGHEST_SHADER_MODEL};
    for (; shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_0; (*(uint32_t*)&shaderModel.HighestShaderModel)--) {
        hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
        if (SUCCEEDED(hr))
            break;
    }
    if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0)
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;

    m_Desc.shaderModel = (uint8_t)((shaderModel.HighestShaderModel / 0xF) * 10 + (shaderModel.HighestShaderModel & 0xF));

    m_Desc.viewport.maxNum = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    m_Desc.viewport.boundsMin = D3D12_VIEWPORT_BOUNDS_MIN;
    m_Desc.viewport.boundsMax = D3D12_VIEWPORT_BOUNDS_MAX;

    m_Desc.multisampling.zeroAttachmentsSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.multisampling.attachmentColorSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.multisampling.attachmentDepthSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.multisampling.attachmentStencilSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.multisampling.textureColorSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.multisampling.textureDepthSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.multisampling.textureStencilSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.multisampling.textureIntegerSampleMaxNum = 1;
    m_Desc.multisampling.storageTextureSampleMaxNum = 1;

    m_Desc.dimensions.attachmentMaxDim = D3D12_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH;
    m_Desc.dimensions.attachmentLayerMaxNum = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.dimensions.texture1DMaxDim = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    m_Desc.dimensions.texture2DMaxDim = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Desc.dimensions.texture3DMaxDim = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Desc.dimensions.textureLayerMaxNum = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.dimensions.typedBufferMaxDim = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;

    m_Desc.precision.viewportBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.precision.subPixelBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.precision.subTexelBits = D3D12_SUBTEXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.precision.mipmapBits = D3D12_MIP_LOD_FRACTIONAL_BIT_COUNT;

    m_Desc.memory.allocationMaxNum = 0xFFFFFFFF;
    m_Desc.memory.samplerAllocationMaxNum = D3D12_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
    m_Desc.memory.constantBufferMaxRange = D3D12_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    m_Desc.memory.storageBufferMaxRange = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;
    m_Desc.memory.bufferTextureGranularity = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
    m_Desc.memory.bufferMaxSize = D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ull * 1024ull;

    m_Desc.memoryAlignment.shaderBindingTable = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    m_Desc.memoryAlignment.bufferShaderResourceOffset = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
    m_Desc.memoryAlignment.constantBufferOffset = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    m_Desc.memoryAlignment.scratchBufferOffset = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
    m_Desc.memoryAlignment.accelerationStructureOffset = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;

    m_Desc.pipelineLayout.descriptorSetMaxNum = ROOT_SIGNATURE_DWORD_NUM / 1;
    m_Desc.pipelineLayout.rootConstantMaxSize = sizeof(uint32_t) * ROOT_SIGNATURE_DWORD_NUM / 1;
    m_Desc.pipelineLayout.rootDescriptorMaxNum = ROOT_SIGNATURE_DWORD_NUM / 2;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
    if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1) {
        m_Desc.descriptorSet.samplerMaxNum = 16;
        m_Desc.descriptorSet.constantBufferMaxNum = 14;
        m_Desc.descriptorSet.storageBufferMaxNum = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? 64 : 8;
        m_Desc.descriptorSet.textureMaxNum = 128;
    } else if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2) {
        m_Desc.descriptorSet.samplerMaxNum = 2048;
        m_Desc.descriptorSet.constantBufferMaxNum = 14;
        m_Desc.descriptorSet.storageBufferMaxNum = 64;
        m_Desc.descriptorSet.textureMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
    } else {
        m_Desc.descriptorSet.samplerMaxNum = 2048;
        m_Desc.descriptorSet.constantBufferMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
        m_Desc.descriptorSet.storageBufferMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
        m_Desc.descriptorSet.textureMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
    }
    m_Desc.descriptorSet.storageTextureMaxNum = m_Desc.descriptorSet.storageBufferMaxNum;

    m_Desc.descriptorSet.updateAfterSet.samplerMaxNum = m_Desc.descriptorSet.samplerMaxNum;
    m_Desc.descriptorSet.updateAfterSet.constantBufferMaxNum = m_Desc.descriptorSet.constantBufferMaxNum;
    m_Desc.descriptorSet.updateAfterSet.storageBufferMaxNum = m_Desc.descriptorSet.storageBufferMaxNum;
    m_Desc.descriptorSet.updateAfterSet.textureMaxNum = m_Desc.descriptorSet.textureMaxNum;
    m_Desc.descriptorSet.updateAfterSet.storageTextureMaxNum = m_Desc.descriptorSet.storageTextureMaxNum;

    m_Desc.shaderStage.descriptorSamplerMaxNum = m_Desc.descriptorSet.samplerMaxNum;
    m_Desc.shaderStage.descriptorConstantBufferMaxNum = m_Desc.descriptorSet.constantBufferMaxNum;
    m_Desc.shaderStage.descriptorStorageBufferMaxNum = m_Desc.descriptorSet.storageBufferMaxNum;
    m_Desc.shaderStage.descriptorTextureMaxNum = m_Desc.descriptorSet.textureMaxNum;
    m_Desc.shaderStage.descriptorStorageTextureMaxNum = m_Desc.descriptorSet.storageTextureMaxNum;
    m_Desc.shaderStage.resourceMaxNum = m_Desc.descriptorSet.textureMaxNum;

    m_Desc.shaderStage.updateAfterSet.descriptorSamplerMaxNum = m_Desc.shaderStage.descriptorSamplerMaxNum;
    m_Desc.shaderStage.updateAfterSet.descriptorConstantBufferMaxNum = m_Desc.shaderStage.descriptorConstantBufferMaxNum;
    m_Desc.shaderStage.updateAfterSet.descriptorStorageBufferMaxNum = m_Desc.shaderStage.descriptorStorageBufferMaxNum;
    m_Desc.shaderStage.updateAfterSet.descriptorTextureMaxNum = m_Desc.shaderStage.descriptorTextureMaxNum;
    m_Desc.shaderStage.updateAfterSet.descriptorStorageTextureMaxNum = m_Desc.shaderStage.descriptorStorageTextureMaxNum;
    m_Desc.shaderStage.updateAfterSet.resourceMaxNum = m_Desc.shaderStage.resourceMaxNum;

    m_Desc.shaderStage.vertex.attributeMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.shaderStage.vertex.streamMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.shaderStage.vertex.outputComponentMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 4;

    m_Desc.shaderStage.tesselationControl.generationMaxLevel = D3D12_HS_MAXTESSFACTOR_UPPER_BOUND;
    m_Desc.shaderStage.tesselationControl.patchPointMaxNum = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
    m_Desc.shaderStage.tesselationControl.perVertexInputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.tesselationControl.perVertexOutputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.tesselationControl.perPatchOutputComponentMaxNum = D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS;
    m_Desc.shaderStage.tesselationControl.totalOutputComponentMaxNum
        = m_Desc.shaderStage.tesselationControl.patchPointMaxNum * m_Desc.shaderStage.tesselationControl.perVertexOutputComponentMaxNum
        + m_Desc.shaderStage.tesselationControl.perPatchOutputComponentMaxNum;

    m_Desc.shaderStage.tesselationEvaluation.inputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.tesselationEvaluation.outputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;

    m_Desc.shaderStage.geometry.invocationMaxNum = D3D12_GS_MAX_INSTANCE_COUNT;
    m_Desc.shaderStage.geometry.inputComponentMaxNum = D3D12_GS_INPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.geometry.outputComponentMaxNum = D3D12_GS_OUTPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.geometry.outputVertexMaxNum = D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
    m_Desc.shaderStage.geometry.totalOutputComponentMaxNum = D3D12_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT;

    m_Desc.shaderStage.fragment.inputComponentMaxNum = D3D12_PS_INPUT_REGISTER_COUNT * D3D12_PS_INPUT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.fragment.attachmentMaxNum = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Desc.shaderStage.fragment.dualSourceAttachmentMaxNum = 1;

    m_Desc.shaderStage.compute.sharedMemoryMaxSize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
    m_Desc.shaderStage.compute.workGroupMaxNum[0] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.shaderStage.compute.workGroupMaxNum[1] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.shaderStage.compute.workGroupMaxNum[2] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.shaderStage.compute.workGroupInvocationMaxNum = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    m_Desc.shaderStage.compute.workGroupMaxDim[0] = D3D12_CS_THREAD_GROUP_MAX_X;
    m_Desc.shaderStage.compute.workGroupMaxDim[1] = D3D12_CS_THREAD_GROUP_MAX_Y;
    m_Desc.shaderStage.compute.workGroupMaxDim[2] = D3D12_CS_THREAD_GROUP_MAX_Z;

    m_Desc.shaderStage.rayTracing.shaderGroupIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_Desc.shaderStage.rayTracing.tableMaxStride = D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE;
    m_Desc.shaderStage.rayTracing.recursionMaxDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;

    m_Desc.shaderStage.meshControl.sharedMemoryMaxSize = 32 * 1024;
    m_Desc.shaderStage.meshControl.workGroupInvocationMaxNum = 128;
    m_Desc.shaderStage.meshControl.payloadMaxSize = 16 * 1024;

    m_Desc.shaderStage.meshEvaluation.outputVerticesMaxNum = 256;
    m_Desc.shaderStage.meshEvaluation.outputPrimitiveMaxNum = 256;
    m_Desc.shaderStage.meshEvaluation.outputComponentMaxNum = 128;
    m_Desc.shaderStage.meshEvaluation.sharedMemoryMaxSize = 28 * 1024;
    m_Desc.shaderStage.meshEvaluation.workGroupInvocationMaxNum = 128;

    m_Desc.other.timestampFrequencyHz = timestampFrequency;
    //m_Desc.other.micromapSubdivisionMaxLevel = D3D12_RAYTRACING_OPACITY_MICROMAP_OC1_MAX_SUBDIVISION_LEVEL;
    m_Desc.other.drawIndirectMaxNum = (1ull << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1;
    m_Desc.other.samplerLodBiasMax = D3D12_MIP_LOD_BIAS_MAX;
    m_Desc.other.samplerAnisotropyMax = D3D12_DEFAULT_MAX_ANISOTROPY;
    m_Desc.other.texelOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.other.texelOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.other.texelGatherOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.other.texelGatherOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.other.clipDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.other.cullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.other.combinedClipAndCullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.other.viewMaxNum = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED ? D3D12_MAX_VIEW_INSTANCE_COUNT : 1;

    m_Desc.tiers.conservativeRaster = (uint8_t)options.ConservativeRasterizationTier;
    m_Desc.tiers.sampleLocations = (uint8_t)options2.ProgrammableSamplePositionsTier;

    if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 && shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6)
        m_Desc.tiers.bindless = 2;
    else if (levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_12_0)
        m_Desc.tiers.bindless = 1;

    m_Desc.features.getMemoryDesc2 = true;
    m_Desc.features.swapChain = HasOutput();
    m_Desc.features.lowLatency = HasNvExt();
    m_Desc.features.micromap = m_Desc.tiers.rayTracing >= 3;

    m_Desc.features.textureFilterMinMax = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? true : false;
    m_Desc.features.logicFunc = options.OutputMergerLogicOp != 0;
    m_Desc.features.depthBoundsTest = options2.DepthBoundsTestSupported != 0;
    m_Desc.features.drawIndirectCount = true;
    m_Desc.features.lineSmoothing = true;
    m_Desc.features.regionResolve = true;
    m_Desc.features.flexibleMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
    m_Desc.features.layerBasedMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
    m_Desc.features.viewportBasedMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
    m_Desc.features.waitableSwapChain = true; // TODO: swap chain version >= 2?

    bool isShaderAtomicsF16Supported = false;
    bool isShaderAtomicsF32Supported = false;
#if NRI_ENABLE_D3D_EXTENSIONS
    if (HasNvExt()) {
        REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP16_ATOMIC, &isShaderAtomicsF16Supported));
        REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP32_ATOMIC, &isShaderAtomicsF32Supported));
    }
#endif

    m_Desc.shaderFeatures.nativeI16 = options4.Native16BitShaderOpsSupported;
    m_Desc.shaderFeatures.nativeF16 = options4.Native16BitShaderOpsSupported;
    m_Desc.shaderFeatures.nativeI64 = options1.Int64ShaderOps;
    m_Desc.shaderFeatures.nativeF64 = options.DoublePrecisionFloatShaderOps;
    m_Desc.shaderFeatures.atomicsF16 = isShaderAtomicsF16Supported;
    m_Desc.shaderFeatures.atomicsF32 = isShaderAtomicsF32Supported;
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    m_Desc.shaderFeatures.atomicsI64 = m_Desc.shaderFeatures.atomicsI64 || options9.AtomicInt64OnTypedResourceSupported || options9.AtomicInt64OnGroupSharedSupported || options11.AtomicInt64OnDescriptorHeapResourceSupported;
#endif

    m_Desc.shaderFeatures.viewportIndex = options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
    m_Desc.shaderFeatures.layerIndex = options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
    m_Desc.shaderFeatures.rasterizedOrderedView = options.ROVsSupported;
    m_Desc.shaderFeatures.barycentric = options3.BarycentricsSupported;
}

void DeviceD3D12::InitializeNvExt(bool isNVAPILoadedInApp, bool isImported) {
    MaybeUnused(isNVAPILoadedInApp, isImported);
#if NRI_ENABLE_D3D_EXTENSIONS
    if (GetModuleHandleA("renderdoc.dll") != nullptr) {
        REPORT_WARNING(this, "NVAPI is disabled, because RenderDoc library has been loaded");
        return;
    }

    if (isImported && !isNVAPILoadedInApp)
        REPORT_WARNING(this, "NVAPI is disabled, because it's not loaded on the application side");
    else {
        NvAPI_Status status = NvAPI_Initialize();
        if (status != NVAPI_OK)
            REPORT_ERROR(this, "NvAPI_Initialize(): failed, result = %d!", (int32_t)status);
        m_NvExt.available = (status == NVAPI_OK);
    }
#endif
}

void DeviceD3D12::InitializeAmdExt(AGSContext* agsContext, bool isImported) {
    MaybeUnused(agsContext, isImported);
#if NRI_ENABLE_D3D_EXTENSIONS
    if (isImported && !agsContext) {
        REPORT_WARNING(this, "AMDAGS is disabled, because 'agsContext' is not provided");
        return;
    }

    // Load library
    Library* agsLibrary = LoadSharedLibrary("amd_ags_x64.dll");
    if (!agsLibrary) {
        REPORT_WARNING(this, "AMDAGS is disabled, because 'amd_ags_x64' is not found");
        return;
    }

    // Get functions
    m_AmdExt.Initialize = (AGS_INITIALIZE)GetSharedLibraryFunction(*agsLibrary, "agsInitialize");
    m_AmdExt.Deinitialize = (AGS_DEINITIALIZE)GetSharedLibraryFunction(*agsLibrary, "agsDeInitialize");
    m_AmdExt.CreateDeviceD3D12 = (AGS_DRIVEREXTENSIONSDX12_CREATEDEVICE)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX12_CreateDevice");
    m_AmdExt.DestroyDeviceD3D12 = (AGS_DRIVEREXTENSIONSDX12_DESTROYDEVICE)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX12_DestroyDevice");

    // Verify
    const void** functionArray = (const void**)&m_AmdExt;
    const size_t functionArraySize = 4;
    size_t i = 0;
    for (; i < functionArraySize && functionArray[i] != nullptr; i++)
        ;

    if (i != functionArraySize) {
        REPORT_WARNING(this, "AMDAGS is disabled, because not all functions are found in the DLL");
        UnloadSharedLibrary(*agsLibrary);

        return;
    }

    // Initialize
    AGSGPUInfo gpuInfo = {};
    AGSConfiguration config = {};
    if (!agsContext) {
        const AGSReturnCode result = m_AmdExt.Initialize(AGS_CURRENT_VERSION, &config, &agsContext, &gpuInfo);
        if (result != AGS_SUCCESS || !agsContext) {
            REPORT_ERROR(this, "Failed to initialize AMDAGS: %d", (int32_t)result);
            UnloadSharedLibrary(*agsLibrary);

            return;
        }
    }

    m_AmdExt.library = agsLibrary;
    m_AmdExt.context = agsContext;
#endif
}

void DeviceD3D12::InitializePixExt() {
    // Load library
    Library* pixLibrary = LoadSharedLibrary("WinPixEventRuntime.dll");
    if (!pixLibrary)
        return;

    // Get functions
    m_Pix.BeginEventOnCommandList = (PIX_BEGINEVENTONCOMMANDLIST)GetSharedLibraryFunction(*pixLibrary, "PIXBeginEventOnCommandList");
    m_Pix.EndEventOnCommandList = (PIX_ENDEVENTONCOMMANDLIST)GetSharedLibraryFunction(*pixLibrary, "PIXEndEventOnCommandList");
    m_Pix.SetMarkerOnCommandList = (PIX_SETMARKERONCOMMANDLIST)GetSharedLibraryFunction(*pixLibrary, "PIXSetMarkerOnCommandList");
    m_Pix.BeginEventOnQueue = (PIX_BEGINEVENTONCOMMANDQUEUE)GetSharedLibraryFunction(*pixLibrary, "PIXBeginEventOnQueue");
    m_Pix.EndEventOnQueue = (PIX_ENDEVENTONCOMMANDQUEUE)GetSharedLibraryFunction(*pixLibrary, "PIXEndEventOnQueue");
    m_Pix.SetMarkerOnQueue = (PIX_SETMARKERONCOMMANDQUEUE)GetSharedLibraryFunction(*pixLibrary, "PIXSetMarkerOnQueue");

    // Verify
    const void** functionArray = (const void**)&m_Pix;
    const size_t functionArraySize = 6;
    size_t i = 0;
    for (; i < functionArraySize && functionArray[i] != nullptr; i++)
        ;

    if (i != functionArraySize) {
        REPORT_WARNING(this, "PIX is disabled, because not all functions are found in the DLL");
        UnloadSharedLibrary(*pixLibrary);

        return;
    }

    m_Pix.library = pixLibrary;
}

Result DeviceD3D12::GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, DescriptorHandle& descriptorHandle) {
    ExclusiveScope lock1(m_FreeDescriptorLocks[type]);

    descriptorHandle = {};

    // Create a new descriptor heap if there is no a free descriptor
    auto& freeDescriptors = m_FreeDescriptors[type];
    if (freeDescriptors.empty()) {
        ExclusiveScope lock2(m_DescriptorHeapLock);

        // Can't create a new heap because the index doesn't fit into "DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM" bits
        size_t heapIndex = m_DescriptorHeaps.size();
        if (heapIndex >= (1 << DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM))
            return Result::OUT_OF_MEMORY;

        // Create a new batch of descriptors
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = type;
        desc.NumDescriptors = DESCRIPTORS_BATCH_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = NODE_MASK;

        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        HRESULT hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
        RETURN_ON_BAD_HRESULT(this, hr, "ID3D12Device::CreateDescriptorHeap()");

        DescriptorHeapDesc descriptorHeapDesc = {};
        descriptorHeapDesc.heap = descriptorHeap;
        descriptorHeapDesc.basePointerCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        descriptorHeapDesc.descriptorSize = m_Device->GetDescriptorHandleIncrementSize(type);
        m_DescriptorHeaps.push_back(descriptorHeapDesc);

        // Add the new batch of free descriptors
        freeDescriptors.reserve(desc.NumDescriptors);

        for (uint32_t i = 0; i < desc.NumDescriptors; i++) {
            DescriptorHandle handle = {};
            handle.heapType = type;
            handle.heapIndex = heapIndex;
            handle.heapOffset = i;

            freeDescriptors.push_back(handle);
        }
    }

    // Reserve and return one descriptor
    descriptorHandle = freeDescriptors.back();
    freeDescriptors.pop_back();

    return Result::SUCCESS;
}

void DeviceD3D12::FreeDescriptorHandle(const DescriptorHandle& descriptorHandle) {
    ExclusiveScope lock(m_FreeDescriptorLocks[descriptorHandle.heapType]);

    auto& freeDescriptors = m_FreeDescriptors[descriptorHandle.heapType];
    freeDescriptors.push_back(descriptorHandle);
}

DescriptorPointerCPU DeviceD3D12::GetDescriptorPointerCPU(const DescriptorHandle& descriptorHandle) {
    ExclusiveScope lock(m_DescriptorHeapLock);

    const DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeaps[descriptorHandle.heapIndex];
    DescriptorPointerCPU descriptorPointerCPU = descriptorHeapDesc.basePointerCPU + descriptorHandle.heapOffset * descriptorHeapDesc.descriptorSize;

    return descriptorPointerCPU;
}

constexpr std::array<D3D12_HEAP_TYPE, (size_t)MemoryLocation::MAX_NUM> g_HeapTypes = {
    D3D12_HEAP_TYPE_DEFAULT, // DEVICE
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_HEAP_TYPE_GPU_UPLOAD, // DEVICE_UPLOAD (Prerequisite: D3D12_FEATURE_D3D12_OPTIONS16)
#else
    D3D12_HEAP_TYPE_UPLOAD, // DEVICE_UPLOAD (silent fallback to HOST_UPLOAD)
#endif
    D3D12_HEAP_TYPE_UPLOAD,   // HOST_UPLOAD
    D3D12_HEAP_TYPE_READBACK, // HOST_READBACK
};
VALIDATE_ARRAY(g_HeapTypes);

D3D12_HEAP_TYPE DeviceD3D12::GetHeapType(MemoryLocation memoryLocation) const {
    if (memoryLocation == MemoryLocation::DEVICE_UPLOAD && m_Desc.memory.deviceUploadHeapSize == 0)
        memoryLocation = MemoryLocation::HOST_UPLOAD;

    return g_HeapTypes[(size_t)memoryLocation];
}

void DeviceD3D12::GetResourceDesc(const BufferDesc& bufferDesc, D3D12_RESOURCE_DESC& desc) const {
    desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = bufferDesc.size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = GetBufferFlags(bufferDesc.usage);

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_TightAlignmentTier != 0)
        desc.Flags |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
#endif
}

void DeviceD3D12::GetResourceDesc(const TextureDesc& textureDesc, D3D12_RESOURCE_DESC& desc) const {
    uint16_t blockWidth = (uint16_t)GetFormatProps(textureDesc.format).blockWidth;
    const DxgiFormat& dxgiFormat = GetDxgiFormat(textureDesc.format);

    desc = {};
    desc.Dimension = GetResourceDimension(textureDesc.type);
    desc.Width = Align(textureDesc.width, blockWidth);
    desc.Height = Align(std::max(textureDesc.height, (Dim_t)1), blockWidth);
    desc.DepthOrArraySize = std::max(textureDesc.type == TextureType::TEXTURE_3D ? textureDesc.depth : textureDesc.layerNum, (Dim_t)1);
    desc.MipLevels = std::max(textureDesc.mipNum, (Mip_t)1);
    desc.Format = (textureDesc.usage & TextureUsageBits::SHADING_RATE_ATTACHMENT) ? dxgiFormat.typed : dxgiFormat.typeless;
    desc.SampleDesc.Count = std::max(textureDesc.sampleNum, (Sample_t)1);
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = GetTextureFlags(textureDesc.usage);

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_TightAlignmentTier > 1)
        desc.Flags |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
#endif
}

void DeviceD3D12::GetMemoryDesc(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc, MemoryDesc& memoryDesc) const {
    D3D12_HEAP_TYPE heapType = GetHeapType(memoryLocation);
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

    bool mustBeDedicated = false;
    if (m_Desc.tiers.memory == 0) {
        if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
            heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
        else if (resourceDesc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
            heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
            mustBeDedicated = true;
        } else
            heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
    }

    D3D12_RESOURCE_ALLOCATION_INFO resourceAllocationInfo = m_Device->GetResourceAllocationInfo(NODE_MASK, 1, &resourceDesc);

    MemoryTypeInfo memoryTypeInfo = {};
    memoryTypeInfo.heapFlags = (uint16_t)heapFlags;
    memoryTypeInfo.heapType = (uint8_t)heapType;
    memoryTypeInfo.mustBeDedicated = mustBeDedicated;

    memoryDesc = {};
    memoryDesc.size = resourceAllocationInfo.SizeInBytes;
    memoryDesc.alignment = (uint32_t)resourceAllocationInfo.Alignment;
    memoryDesc.type = Pack(memoryTypeInfo);
    memoryDesc.mustBeDedicated = mustBeDedicated;
}

void DeviceD3D12::GetAccelerationStructurePrebuildInfo(const AccelerationStructureDesc& accelerationStructureDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo) const {
    // Scratch memory
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

    Scratch<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs = AllocateScratch(*this, D3D12_RAYTRACING_GEOMETRY_DESC, geometryNum);
    Scratch<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC> trianglesDescs = AllocateScratch(*this, D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC, micromapNum);
    Scratch<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC> ommDescs = AllocateScratch(*this, D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC, micromapNum);

    // Get prebuild info
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS accelerationStructureInputs = {};
    accelerationStructureInputs.Type = GetAccelerationStructureType(accelerationStructureDesc.type);
    accelerationStructureInputs.Flags = GetAccelerationStructureFlags(accelerationStructureDesc.flags);
    accelerationStructureInputs.NumDescs = accelerationStructureDesc.geometryOrInstanceNum;
    accelerationStructureInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY; // TODO: D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS support?

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        accelerationStructureInputs.pGeometryDescs = geometryDescs;
        ConvertBotomLevelGeometries(accelerationStructureDesc.geometries, geometryNum, geometryDescs, trianglesDescs, ommDescs);
    }

    m_Device->GetRaytracingAccelerationStructurePrebuildInfo(&accelerationStructureInputs, &prebuildInfo);
}

void DeviceD3D12::GetMicromapPrebuildInfo(const MicromapDesc& micromapDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo) const {
    MaybeUnused(micromapDesc, prebuildInfo);
}

ComPtr<ID3D12CommandSignature> DeviceD3D12::CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride, ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation) {
    const bool isDrawArgument = enableDrawParametersEmulation && (type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW || type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED);

    D3D12_INDIRECT_ARGUMENT_DESC indirectArgumentDescs[2] = {};
    if (isDrawArgument) {
        // Draw base parameters emulation
        // Based on: https://github.com/google/dawn/blob/e72fa969ad72e42064cd33bd99572ea12b0bcdaf/src/dawn/native/d3d12/PipelineLayoutD3D12.cpp#L504
        indirectArgumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        indirectArgumentDescs[0].Constant.RootParameterIndex = 0;
        indirectArgumentDescs[0].Constant.DestOffsetIn32BitValues = 0;
        indirectArgumentDescs[0].Constant.Num32BitValuesToSet = 2;

        indirectArgumentDescs[1].Type = type;
    } else
        indirectArgumentDescs[0].Type = type;

    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
    commandSignatureDesc.NumArgumentDescs = isDrawArgument ? 2 : 1;
    commandSignatureDesc.pArgumentDescs = indirectArgumentDescs;
    commandSignatureDesc.NodeMask = NODE_MASK;
    commandSignatureDesc.ByteStride = stride;

    ComPtr<ID3D12CommandSignature> commandSignature = nullptr;
    HRESULT hr = m_Device->CreateCommandSignature(&commandSignatureDesc, isDrawArgument ? rootSignature : nullptr, IID_PPV_ARGS(&commandSignature));
    if (FAILED(hr))
        REPORT_ERROR(this, "ID3D12Device::CreateCommandSignature() failed, result = 0x%08X!", hr);

    return commandSignature;
}

Result DeviceD3D12::CreateDefaultDrawSignatures(ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation) {
    ExclusiveScope lock(m_CommandSignatureLock);

    const uint32_t drawStride = enableDrawParametersEmulation ? sizeof(DrawBaseDesc) : sizeof(DrawDesc);
    const uint32_t drawIndexedStride = enableDrawParametersEmulation ? sizeof(DrawIndexedBaseDesc) : sizeof(DrawIndexedDesc);

    ComPtr<ID3D12CommandSignature> drawCommandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, drawStride, rootSignature, enableDrawParametersEmulation);
    if (!drawCommandSignature)
        return Result::FAILURE;

    auto key = HashRootSignatureAndStride(rootSignature, drawStride);
    m_DrawCommandSignatures.emplace(key, drawCommandSignature);

    ComPtr<ID3D12CommandSignature> drawIndexedCommandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, drawIndexedStride, rootSignature, enableDrawParametersEmulation);
    if (!drawIndexedCommandSignature)
        return Result::FAILURE;

    key = HashRootSignatureAndStride(rootSignature, drawIndexedStride);
    m_DrawIndexedCommandSignatures.emplace(key, drawIndexedCommandSignature);

    return Result::SUCCESS;
}

ID3D12CommandSignature* DeviceD3D12::GetDrawCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature) {
    ExclusiveScope lock(m_CommandSignatureLock);

    auto key = HashRootSignatureAndStride(rootSignature, stride);
    auto commandSignatureIt = m_DrawCommandSignatures.find(key);
    if (commandSignatureIt != m_DrawCommandSignatures.end())
        return commandSignatureIt->second;

    ComPtr<ID3D12CommandSignature> commandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, stride, rootSignature);
    m_DrawCommandSignatures[key] = commandSignature;

    return commandSignature;
}

ID3D12CommandSignature* DeviceD3D12::GetDrawIndexedCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature) {
    ExclusiveScope lock(m_CommandSignatureLock);

    auto key = HashRootSignatureAndStride(rootSignature, stride);
    auto commandSignatureIt = m_DrawIndexedCommandSignatures.find(key);
    if (commandSignatureIt != m_DrawIndexedCommandSignatures.end())
        return commandSignatureIt->second;

    ComPtr<ID3D12CommandSignature> commandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, stride, rootSignature);
    m_DrawIndexedCommandSignatures[key] = commandSignature;

    return commandSignature;
}

ID3D12CommandSignature* DeviceD3D12::GetDrawMeshCommandSignature(uint32_t stride) {
    ExclusiveScope lock(m_CommandSignatureLock);

    auto commandSignatureIt = m_DrawMeshCommandSignatures.find(stride);
    if (commandSignatureIt != m_DrawMeshCommandSignatures.end())
        return commandSignatureIt->second;

    ComPtr<ID3D12CommandSignature> commandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, stride, nullptr);
    m_DrawMeshCommandSignatures[stride] = commandSignature;

    return commandSignature;
}

ID3D12CommandSignature* DeviceD3D12::GetDispatchRaysCommandSignature() const {
    return m_DispatchRaysCommandSignature.GetInterface();
}

ID3D12CommandSignature* DeviceD3D12::GetDispatchCommandSignature() const {
    return m_DispatchCommandSignature.GetInterface();
}

void DeviceD3D12::Destruct() {
    Destroy(GetAllocationCallbacks(), this);
}

NRI_INLINE Result DeviceD3D12::GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    const auto& queueFamily = m_QueueFamilies[(uint32_t)queueType];
    if (queueFamily.empty())
        return Result::UNSUPPORTED;

    if (queueIndex < queueFamily.size()) {
        queue = (Queue*)m_QueueFamilies[(uint32_t)queueType].at(queueIndex);
        return Result::SUCCESS;
    }

    return Result::FAILURE;
}

NRI_INLINE Result DeviceD3D12::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        Result result = ((BufferD3D12*)memoryBindingDescs[i].buffer)->BindMemory((MemoryD3D12*)memoryBindingDescs[i].memory, memoryBindingDescs[i].offset);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceD3D12::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        Result result = ((TextureD3D12*)memoryBindingDescs[i].texture)->BindMemory((MemoryD3D12*)memoryBindingDescs[i].memory, memoryBindingDescs[i].offset);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceD3D12::BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        Result result = ((AccelerationStructureD3D12*)memoryBindingDescs[i].accelerationStructure)->BindMemory(memoryBindingDescs[i].memory, memoryBindingDescs[i].offset);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceD3D12::BindMicromapMemory(const MicromapMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        Result result = ((MicromapD3D12*)memoryBindingDescs[i].micromap)->BindMemory(memoryBindingDescs[i].memory, memoryBindingDescs[i].offset);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

NRI_INLINE FormatSupportBits DeviceD3D12::GetFormatSupport(Format format) const {
    FormatSupportBits mask = FormatSupportBits::UNSUPPORTED;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {GetDxgiFormat(format).typed};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));

    if (SUCCEEDED(hr)) {
#define UPDATE_SUPPORT_BITS(required, optional, bit) \
    if ((formatSupport.Support1 & (required)) == (required) && ((formatSupport.Support1 & (optional)) != 0 || (optional) == 0)) \
        mask |= bit;

        UPDATE_SUPPORT_BITS(0, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE | D3D12_FORMAT_SUPPORT1_SHADER_LOAD, FormatSupportBits::TEXTURE);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW, 0, FormatSupportBits::STORAGE_TEXTURE);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_RENDER_TARGET, 0, FormatSupportBits::COLOR_ATTACHMENT);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL, 0, FormatSupportBits::DEPTH_STENCIL_ATTACHMENT);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_BLENDABLE, 0, FormatSupportBits::BLEND);

        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_BUFFER, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE | D3D12_FORMAT_SUPPORT1_SHADER_LOAD, FormatSupportBits::BUFFER);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_BUFFER | D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW, 0, FormatSupportBits::STORAGE_BUFFER);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER, 0, FormatSupportBits::VERTEX_BUFFER);

#undef UPDATE_SUPPORT_BITS

#define UPDATE_SUPPORT_BITS(optional, bit) \
    if ((formatSupport.Support2 & (optional)) != 0) \
        mask |= bit;

        const uint32_t anyAtomics = D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX;

        if (mask & FormatSupportBits::STORAGE_TEXTURE)
            UPDATE_SUPPORT_BITS(anyAtomics, FormatSupportBits::STORAGE_TEXTURE_ATOMICS);

        if (mask & FormatSupportBits::STORAGE_BUFFER)
            UPDATE_SUPPORT_BITS(anyAtomics, FormatSupportBits::STORAGE_BUFFER_ATOMICS);

#undef UPDATE_SUPPORT_BITS
    }

    return mask;
}
