// © 2021 NVIDIA Corporation

uint8_t QueryLatestDeviceContext(ComPtr<ID3D11DeviceContextBest>& in, ComPtr<ID3D11DeviceContextBest>& out);

static uint8_t QueryLatestInterface(ComPtr<ID3D11DeviceBest>& in, ComPtr<ID3D11DeviceBest>& out) {
    static const IID versions[] = {
        __uuidof(ID3D11Device5),
        __uuidof(ID3D11Device4),
        __uuidof(ID3D11Device3),
        __uuidof(ID3D11Device2),
        __uuidof(ID3D11Device1),
        __uuidof(ID3D11Device),
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

DeviceD3D11::DeviceD3D11(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks)
    : DeviceBase(callbacks, allocationCallbacks)
    , m_QueueFamilies{
          Vector<QueueD3D11*>(GetStdAllocator()),
          Vector<QueueD3D11*>(GetStdAllocator()),
          Vector<QueueD3D11*>(GetStdAllocator()),
      } {
    m_Desc.graphicsAPI = GraphicsAPI::D3D11;
    m_Desc.nriVersion = NRI_VERSION;
}

DeviceD3D11::~DeviceD3D11() {
    if (m_ImmediateContext) {
        if (HasNvExt()) {
#if NRI_ENABLE_NVAPI
            NvAPI_Status status = NvAPI_D3D11_EndUAVOverlap(m_ImmediateContext);
            if (status != NVAPI_OK)
                REPORT_WARNING(this, "NvAPI_D3D11_EndUAVOverlap() failed!");
#endif
        } else if (HasAmdExt()) {
#if NRI_ENABLE_AMDAGS
            AGSReturnCode res = m_AmdExt.EndUAVOverlap(m_AmdExt.context, m_ImmediateContext);
            if (res != AGS_SUCCESS)
                REPORT_WARNING(this, "agsDriverExtensionsDX11_EndUAVOverlap() failed!");
#endif
        }
    }

    for (auto& queueFamily : m_QueueFamilies) {
        for (auto queue : queueFamily)
            Destroy<QueueD3D11>(queue);
    }

    DeleteCriticalSection(&m_CriticalSection);

#if NRI_ENABLE_AMDAGS
    if (HasAmdExt() && !m_IsWrapped)
        m_AmdExt.DestroyDeviceD3D11(m_AmdExt.context, m_Device, nullptr, m_ImmediateContext, nullptr);
#endif
}

Result DeviceD3D11::Create(const DeviceCreationDesc& desc, const DeviceCreationD3D11Desc& descD3D11) {
    m_IsWrapped = descD3D11.d3d11Device != nullptr;

    // Get adapter description as early as possible for meaningful error reporting
    m_Desc.adapterDesc = *desc.adapterDesc;

    // Get adapter
    if (m_IsWrapped) {
        ComPtr<IDXGIDevice> dxgiDevice;
        HRESULT hr = descD3D11.d3d11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        RETURN_ON_BAD_HRESULT(this, hr, "QueryInterface(IDXGIDevice)");

        hr = dxgiDevice->GetAdapter(&m_Adapter);
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIDevice::GetAdapter");
    } else {
        ComPtr<IDXGIFactory4> dxgiFactory;
        HRESULT hr = CreateDXGIFactory2(desc.enableGraphicsAPIValidation ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgiFactory));
        RETURN_ON_BAD_HRESULT(this, hr, "CreateDXGIFactory2");

        LUID luid = *(LUID*)&m_Desc.adapterDesc.uid.low;
        hr = dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&m_Adapter));
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIFactory4::EnumAdapterByLuid");
    }

    // Extensions
    if (m_Desc.adapterDesc.vendor == Vendor::NVIDIA)
        InitializeNvExt(descD3D11.disableNVAPIInitialization, m_IsWrapped);
    else if (m_Desc.adapterDesc.vendor == Vendor::AMD)
        InitializeAmdExt(descD3D11.agsContext, m_IsWrapped);

    // Device
    ComPtr<ID3D11DeviceBest> deviceTemp = (ID3D11DeviceBest*)descD3D11.d3d11Device;
    if (!m_IsWrapped) {
        UINT flags = desc.enableGraphicsAPIValidation ? D3D11_CREATE_DEVICE_DEBUG : 0;
        D3D_FEATURE_LEVEL levels[2] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
        uint32_t levelNum = GetCountOf(levels);
        bool isDepthBoundsTestSupported = false;
        bool isDrawIndirectCountSupported = false;
        bool isShaderAtomicsI64Supported = false;
        bool isShaderClockSupported = false;
        bool areWaveIntrinsicsSupported = false;

        uint32_t d3dShaderExtRegister = desc.d3dShaderExtRegister ? desc.d3dShaderExtRegister : NRI_SHADER_EXT_REGISTER;
        MaybeUnused(d3dShaderExtRegister);

        if (HasAmdExt()) {
#if NRI_ENABLE_AMDAGS
            AGSDX11DeviceCreationParams deviceCreationParams = {};
            deviceCreationParams.pAdapter = m_Adapter;
            deviceCreationParams.DriverType = D3D_DRIVER_TYPE_UNKNOWN;
            deviceCreationParams.Flags = flags;
            deviceCreationParams.pFeatureLevels = levels;
            deviceCreationParams.FeatureLevels = levelNum;
            deviceCreationParams.SDKVersion = D3D11_SDK_VERSION;

            AGSDX11ExtensionParams extensionsParams = {};
            extensionsParams.uavSlot = d3dShaderExtRegister;

            AGSDX11ReturnedParams agsParams = {};
            AGSReturnCode result = m_AmdExt.CreateDeviceD3D11(m_AmdExt.context, &deviceCreationParams, &extensionsParams, &agsParams);
            if (flags != 0 && result != AGS_SUCCESS) {
                // If Debug Layer is not available, try without D3D11_CREATE_DEVICE_DEBUG
                deviceCreationParams.Flags = 0;
                result = m_AmdExt.CreateDeviceD3D11(m_AmdExt.context, &deviceCreationParams, &extensionsParams, &agsParams);
            }

            RETURN_ON_FAILURE(this, result == AGS_SUCCESS, Result::FAILURE, "agsDriverExtensionsDX11_CreateDevice() returned %d", (int32_t)result);

            deviceTemp = (ID3D11DeviceBest*)agsParams.pDevice;
            isDepthBoundsTestSupported = agsParams.extensionsSupported.depthBoundsDeferredContexts;
            isDrawIndirectCountSupported = agsParams.extensionsSupported.multiDrawIndirectCountIndirect;
            isShaderAtomicsI64Supported = agsParams.extensionsSupported.intrinsics19;

            m_Desc.other.viewMaxNum = agsParams.extensionsSupported.multiView ? 4 : 1;
            m_Desc.features.viewportBasedMultiview = agsParams.extensionsSupported.multiView;
            m_Desc.shaderFeatures.barycentric = agsParams.extensionsSupported.intrinsics16;

            areWaveIntrinsicsSupported = agsParams.extensionsSupported.intrinsics16
                && agsParams.extensionsSupported.intrinsics17
                && agsParams.extensionsSupported.intrinsics19
                && agsParams.extensionsSupported.getWaveSize;
#endif
        } else {
            HRESULT hr = D3D11CreateDevice(m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, levels, levelNum, D3D11_SDK_VERSION, (ID3D11Device**)&deviceTemp, nullptr, nullptr);

            // If Debug Layer is not available, try without D3D11_CREATE_DEVICE_DEBUG
            if (flags && hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
                hr = D3D11CreateDevice(m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, levels, levelNum, D3D11_SDK_VERSION, (ID3D11Device**)&deviceTemp, nullptr, nullptr);

            RETURN_ON_BAD_HRESULT(this, hr, "D3D11CreateDevice");

            if (HasNvExt()) {
#if NRI_ENABLE_NVAPI
                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D_RegisterDevice(deviceTemp));
                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_SetNvShaderExtnSlot(deviceTemp, d3dShaderExtRegister));
                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_UINT64_ATOMIC, &isShaderAtomicsI64Supported));
                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_GET_SPECIAL, &isShaderClockSupported));

                bool isSupported = false;
                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_SHFL, &isSupported));
                areWaveIntrinsicsSupported = isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_SHFL_UP, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_SHFL_DOWN, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_SHFL_XOR, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_VOTE_ALL, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_VOTE_ANY, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_VOTE_BALLOT, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_GET_LANE_ID, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_MATCH_ANY, &isSupported));
                areWaveIntrinsicsSupported = areWaveIntrinsicsSupported && isSupported;

                isDepthBoundsTestSupported = true;
#endif
            }
        }

        // Start filling here to avoid passing additional arguments into "FillDesc"
        m_Desc.features.depthBoundsTest = isDepthBoundsTestSupported;
        m_Desc.features.drawIndirectCount = isDrawIndirectCountSupported;
        m_Desc.shaderFeatures.atomicsI64 = isShaderAtomicsI64Supported;
        m_Desc.shaderFeatures.clock = isShaderClockSupported;

        // If "all known" intrinsics are supported assume that "all possible" intrinsics are supported, since it's unclear which of them are really available via extensions
        m_Desc.shaderFeatures.waveQuery = areWaveIntrinsicsSupported;
        m_Desc.shaderFeatures.waveVote = areWaveIntrinsicsSupported;
        m_Desc.shaderFeatures.waveShuffle = areWaveIntrinsicsSupported;
        m_Desc.shaderFeatures.waveArithmetic = areWaveIntrinsicsSupported;
        m_Desc.shaderFeatures.waveReduction = areWaveIntrinsicsSupported;
        m_Desc.shaderFeatures.waveQuad = areWaveIntrinsicsSupported;
    }

    m_Version = QueryLatestInterface(deviceTemp, m_Device);

    if (desc.enableGraphicsAPIValidation) {
        ComPtr<ID3D11InfoQueue> pInfoQueue;
        HRESULT hr = m_Device->QueryInterface(&pInfoQueue);

        if (SUCCEEDED(hr)) {
            hr = pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            RETURN_ON_BAD_HRESULT(this, hr, "ID3D11InfoQueue::SetBreakOnSeverity");

            hr = pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            RETURN_ON_BAD_HRESULT(this, hr, "ID3D11InfoQueue::SetBreakOnSeverity");

            // TODO: this code is currently needed to disable known false-positive errors reported by the debug layer
            D3D11_MESSAGE_ID disableMessageIDs[] = {
                // Disobey the spec, but allow multiple structured views for a single buffer
                D3D11_MESSAGE_ID_DEVICE_SHADERRESOURCEVIEW_BUFFER_TYPE_MISMATCH,
            };

            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.pIDList = disableMessageIDs;
            filter.DenyList.NumIDs = GetCountOf(disableMessageIDs);
            hr = pInfoQueue->AddStorageFilterEntries(&filter);
            RETURN_ON_BAD_HRESULT(this, hr, "ID3D11InfoQueue::AddStorageFilterEntries");
        }
    }

    // Immediate context
    ComPtr<ID3D11DeviceContextBest> immediateContext;
    m_Device->GetImmediateContext((ID3D11DeviceContext**)&immediateContext);

    m_ImmediateContextVersion = QueryLatestDeviceContext(immediateContext, m_ImmediateContext);

    REPORT_INFO(this, "Using ID3D11Device%u and ID3D11DeviceContext%u", m_Version, m_ImmediateContextVersion);

    // Skip UAV barriers by default on the immediate context
    if (HasNvExt()) {
#if NRI_ENABLE_NVAPI
        NvAPI_Status status = NvAPI_D3D11_BeginUAVOverlap(m_ImmediateContext);
        if (status != NVAPI_OK)
            REPORT_WARNING(this, "NvAPI_D3D11_BeginUAVOverlap() failed!");
#endif
    } else if (HasAmdExt()) {
#if NRI_ENABLE_AMDAGS
        AGSReturnCode res = m_AmdExt.BeginUAVOverlap(m_AmdExt.context, m_ImmediateContext);
        if (res != AGS_SUCCESS)
            REPORT_WARNING(this, "agsDriverExtensionsDX11_BeginUAVOverlap() failed!");
#endif
    }

    // Threading
    D3D11_FEATURE_DATA_THREADING threadingCaps = {};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingCaps, sizeof(threadingCaps));
    if (FAILED(hr) || !threadingCaps.DriverConcurrentCreates)
        REPORT_WARNING(this, "Concurrent resource creation is not supported by the driver!");

    m_IsDeferredContextEmulated = !HasNvExt() || desc.enableD3D11CommandBufferEmulation;
    if (!threadingCaps.DriverCommandLists && !desc.enableD3D11CommandBufferEmulation) {
        REPORT_WARNING(this, "Deferred Contexts are not supported by the driver and will be emulated!");
        m_IsDeferredContextEmulated = true;
    }

    hr = m_ImmediateContext->QueryInterface(IID_PPV_ARGS(&m_Multithread));
    if (FAILED(hr)) {
        REPORT_WARNING(this, "ID3D11Multithread is not supported: a critical section will be used instead!");
        InitializeCriticalSection(&m_CriticalSection);
    } else
        m_Multithread->SetMultithreadProtected(true);

    // Create queues
    memset(m_Desc.adapterDesc.queueNum, 0, sizeof(m_Desc.adapterDesc.queueNum)); // patch to reflect available queues
    for (uint32_t i = 0; i < desc.queueFamilyNum; i++) {
        const QueueFamilyDesc& queueFamilyDesc = desc.queueFamilies[i];
        auto& queueFamily = m_QueueFamilies[(size_t)queueFamilyDesc.queueType];

        for (uint32_t j = 0; j < queueFamilyDesc.queueNum; j++) {
            QueueD3D11* queue = Allocate<QueueD3D11>(GetAllocationCallbacks(), *this);
            queueFamily.push_back(queue);
        }

        m_Desc.adapterDesc.queueNum[(size_t)queueFamilyDesc.queueType] = queueFamilyDesc.queueNum;
    }

    { // Create zero buffer
        D3D11_BUFFER_DESC zeroBufferDesc = {};
        zeroBufferDesc.ByteWidth = desc.d3dZeroBufferSize ? desc.d3dZeroBufferSize : ZERO_BUFFER_DEFAULT_SIZE;
        zeroBufferDesc.Usage = D3D11_USAGE_DEFAULT;

        auto& allocator = GetAllocationCallbacks();
        uint8_t* zeros = (uint8_t*)allocator.Allocate(allocator.userArg, zeroBufferDesc.ByteWidth, 16);
        memset(zeros, 0, zeroBufferDesc.ByteWidth);

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = zeros;

        hr = m_Device->CreateBuffer(&zeroBufferDesc, &data, &m_ZeroBuffer);
        RETURN_ON_BAD_HRESULT(this, hr, "ID3D11Device::CreateBuffer");

        allocator.Free(allocator.userArg, zeros);
    }

    // Fill desc
    FillDesc();

    return FillFunctionTable(m_iCore);
}

void DeviceD3D11::FillDesc() {
    D3D11_FEATURE_DATA_D3D11_OPTIONS options = {};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &options, sizeof(options));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS1 options1 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &options1, sizeof(options1));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options1) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS2 options2 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &options2, sizeof(options2));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options2) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 options3 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options3, sizeof(options3));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options3) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS4 options4 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS4, &options4, sizeof(options4));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options4) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS5 options5 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS5, &options5, sizeof(options5));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options5) failed, result = 0x%08X!", hr);

    uint64_t timestampFrequency = 0;
    {
        D3D11_QUERY_DESC queryDesc = {};
        queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

        ComPtr<ID3D11Query> query;
        hr = m_Device->CreateQuery(&queryDesc, &query);
        if (SUCCEEDED(hr)) {
            m_ImmediateContext->Begin(query);
            m_ImmediateContext->End(query);

            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data = {};
            while (m_ImmediateContext->GetData(query, &data, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0) == S_FALSE)
                ;

            timestampFrequency = data.Frequency;
        }
    }

    m_Desc.shaderModel = 51;

    m_Desc.viewport.maxNum = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    m_Desc.viewport.boundsMin = D3D11_VIEWPORT_BOUNDS_MIN;
    m_Desc.viewport.boundsMax = D3D11_VIEWPORT_BOUNDS_MAX;

    m_Desc.dimensions.attachmentMaxDim = D3D11_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH;
    m_Desc.dimensions.attachmentLayerMaxNum = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.dimensions.texture1DMaxDim = D3D11_REQ_TEXTURE1D_U_DIMENSION;
    m_Desc.dimensions.texture2DMaxDim = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Desc.dimensions.texture3DMaxDim = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Desc.dimensions.textureLayerMaxNum = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.dimensions.typedBufferMaxDim = 1 << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;

    m_Desc.precision.viewportBits = D3D11_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.precision.subPixelBits = D3D11_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.precision.subTexelBits = D3D11_SUBTEXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.precision.mipmapBits = D3D11_MIP_LOD_FRACTIONAL_BIT_COUNT;

    m_Desc.memory.allocationMaxNum = (uint32_t)(-1);
    m_Desc.memory.samplerAllocationMaxNum = D3D11_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
    m_Desc.memory.constantBufferMaxRange = D3D11_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    m_Desc.memory.storageBufferMaxRange = 1 << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;
    m_Desc.memory.bufferTextureGranularity = 1;
    m_Desc.memory.bufferMaxSize = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ull * 1024ull;

    m_Desc.memoryAlignment.uploadBufferTextureRow = 256;   // D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    m_Desc.memoryAlignment.uploadBufferTextureSlice = 512; // D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    m_Desc.memoryAlignment.bufferShaderResourceOffset = D3D11_RAW_UAV_SRV_BYTE_ALIGNMENT;
    m_Desc.memoryAlignment.constantBufferOffset = 256; // D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    m_Desc.pipelineLayout.descriptorSetMaxNum = ROOT_SIGNATURE_DWORD_NUM / 1;
    m_Desc.pipelineLayout.rootConstantMaxSize = sizeof(uint32_t) * ROOT_SIGNATURE_DWORD_NUM / 1;
    m_Desc.pipelineLayout.rootDescriptorMaxNum = ROOT_SIGNATURE_DWORD_NUM / 2;

    m_Desc.descriptorSet.samplerMaxNum = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
    m_Desc.descriptorSet.constantBufferMaxNum = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    m_Desc.descriptorSet.storageBufferMaxNum = m_Version >= 1 ? D3D11_1_UAV_SLOT_COUNT : D3D11_PS_CS_UAV_REGISTER_COUNT;
    m_Desc.descriptorSet.textureMaxNum = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
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

    m_Desc.shaderStage.vertex.attributeMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.shaderStage.vertex.streamMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.shaderStage.vertex.outputComponentMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 4;

    m_Desc.shaderStage.tesselationControl.generationMaxLevel = D3D11_HS_MAXTESSFACTOR_UPPER_BOUND;
    m_Desc.shaderStage.tesselationControl.patchPointMaxNum = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
    m_Desc.shaderStage.tesselationControl.perVertexInputComponentMaxNum = D3D11_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT * D3D11_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.tesselationControl.perVertexOutputComponentMaxNum = D3D11_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT * D3D11_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.tesselationControl.perPatchOutputComponentMaxNum = D3D11_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS;
    m_Desc.shaderStage.tesselationControl.totalOutputComponentMaxNum
        = m_Desc.shaderStage.tesselationControl.patchPointMaxNum * m_Desc.shaderStage.tesselationControl.perVertexOutputComponentMaxNum
        + m_Desc.shaderStage.tesselationControl.perPatchOutputComponentMaxNum;

    m_Desc.shaderStage.tesselationEvaluation.inputComponentMaxNum = D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.tesselationEvaluation.outputComponentMaxNum = D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;

    m_Desc.shaderStage.geometry.invocationMaxNum = D3D11_GS_MAX_INSTANCE_COUNT;
    m_Desc.shaderStage.geometry.inputComponentMaxNum = D3D11_GS_INPUT_REGISTER_COUNT * D3D11_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.geometry.outputComponentMaxNum = D3D11_GS_OUTPUT_REGISTER_COUNT * D3D11_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.geometry.outputVertexMaxNum = D3D11_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
    m_Desc.shaderStage.geometry.totalOutputComponentMaxNum = D3D11_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT;

    m_Desc.shaderStage.fragment.inputComponentMaxNum = D3D11_PS_INPUT_REGISTER_COUNT * D3D11_PS_INPUT_REGISTER_COMPONENTS;
    m_Desc.shaderStage.fragment.attachmentMaxNum = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Desc.shaderStage.fragment.dualSourceAttachmentMaxNum = 1;

    m_Desc.shaderStage.compute.workGroupMaxNum[0] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.shaderStage.compute.workGroupMaxNum[1] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.shaderStage.compute.workGroupMaxNum[2] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.shaderStage.compute.workGroupMaxDim[0] = D3D11_CS_THREAD_GROUP_MAX_X;
    m_Desc.shaderStage.compute.workGroupMaxDim[1] = D3D11_CS_THREAD_GROUP_MAX_Y;
    m_Desc.shaderStage.compute.workGroupMaxDim[2] = D3D11_CS_THREAD_GROUP_MAX_Z;
    m_Desc.shaderStage.compute.workGroupInvocationMaxNum = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    m_Desc.shaderStage.compute.sharedMemoryMaxSize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;

    m_Desc.wave.laneMinNum = m_Desc.adapterDesc.vendor == Vendor::AMD ? 64 : 32; // TODO: yes, AMD nowadays can do 32, but 64 seems to be a more generic match
    m_Desc.wave.laneMaxNum = m_Desc.adapterDesc.vendor == Vendor::AMD ? 64 : 32;
    m_Desc.wave.derivativeOpsStages = StageBits::FRAGMENT_SHADER;
    m_Desc.wave.waveOpsStages = StageBits::ALL_SHADERS;

    m_Desc.other.timestampFrequencyHz = timestampFrequency;
    m_Desc.other.drawIndirectMaxNum = (1ull << D3D11_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1;
    m_Desc.other.samplerLodBiasMax = D3D11_MIP_LOD_BIAS_MAX;
    m_Desc.other.samplerAnisotropyMax = D3D11_DEFAULT_MAX_ANISOTROPY;
    m_Desc.other.texelOffsetMin = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.other.texelOffsetMax = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.other.texelGatherOffsetMin = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.other.texelGatherOffsetMax = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.other.clipDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.other.cullDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.other.combinedClipAndCullDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;

    m_Desc.tiers.conservativeRaster = (uint8_t)options2.ConservativeRasterizationTier;

    bool isShaderAtomicsF16Supported = false;
    bool isShaderAtomicsF32Supported = false;
    bool isGetSpecialSupported = false;
#if NRI_ENABLE_NVAPI
    NV_D3D11_FEATURE_DATA_RASTERIZER_SUPPORT rasterizerFeatures = {};
    NV_D3D1x_GRAPHICS_CAPS caps = {};

    if (HasNvExt()) {
        REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP16_ATOMIC, &isShaderAtomicsF16Supported));
        REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP32_ATOMIC, &isShaderAtomicsF32Supported));
        REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_GET_SPECIAL, &isGetSpecialSupported));
        REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D11_CheckFeatureSupport(m_Device, NV_D3D11_FEATURE_RASTERIZER, &rasterizerFeatures, sizeof(rasterizerFeatures)));
        REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D1x_GetGraphicsCapabilities(m_Device, NV_D3D1x_GRAPHICS_CAPS_VER, &caps));
    }

    m_Desc.tiers.sampleLocations = rasterizerFeatures.ProgrammableSamplePositions ? 2 : 0;

    m_Desc.tiers.shadingRate = caps.bVariablePixelRateShadingSupported ? 2 : 0;
    m_Desc.other.shadingRateAttachmentTileSize = NV_VARIABLE_PIXEL_SHADING_TILE_WIDTH;
    m_Desc.features.additionalShadingRates = caps.bVariablePixelRateShadingSupported ? 1 : 0;
#endif

    m_Desc.features.getMemoryDesc2 = true;
    m_Desc.features.swapChain = HasOutput();
    m_Desc.features.lowLatency = HasNvExt();

    m_Desc.features.textureFilterMinMax = options1.MinMaxFiltering != 0;
    m_Desc.features.logicOp = options.OutputMergerLogicOp != 0;
    m_Desc.features.lineSmoothing = true;
    m_Desc.features.enhancedBarriers = true;  // don't care, but advertise support
    m_Desc.features.waitableSwapChain = true; // TODO: swap chain version >= 2?
    m_Desc.features.resizableSwapChain = true;
    m_Desc.features.pipelineStatistics = true;
    m_Desc.features.mutableDescriptorType = true;

    m_Desc.shaderFeatures.nativeF64 = options.ExtendedDoublesShaderInstructions;
    m_Desc.shaderFeatures.atomicsF16 = isShaderAtomicsF16Supported;
    m_Desc.shaderFeatures.atomicsF32 = isShaderAtomicsF32Supported;
    m_Desc.shaderFeatures.viewportIndex = options3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer;
    m_Desc.shaderFeatures.layerIndex = options3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer;
    m_Desc.shaderFeatures.clock = isGetSpecialSupported;
    m_Desc.shaderFeatures.rasterizedOrderedView = options2.ROVsSupported != 0;
    m_Desc.shaderFeatures.storageReadWithoutFormat = true; // All desktop GPUs support it since 2014
    m_Desc.shaderFeatures.storageWriteWithoutFormat = true;
}

void DeviceD3D11::InitializeNvExt(bool disableNVAPIInitialization, bool isImported) {
    MaybeUnused(disableNVAPIInitialization, isImported);
#if NRI_ENABLE_NVAPI
    if (GetModuleHandleA("renderdoc.dll") != nullptr) {
        REPORT_WARNING(this, "NVAPI is disabled, because RenderDoc library has been loaded");
        return;
    }

    if (isImported && !disableNVAPIInitialization)
        REPORT_WARNING(this, "NVAPI is disabled, because it's not loaded on the application side");
    else {
        NvAPI_Status status = NvAPI_Initialize();
        if (status != NVAPI_OK)
            REPORT_ERROR(this, "NvAPI_Initialize(): failed, result=%d!", (int32_t)status);
        m_NvExt.available = (status == NVAPI_OK);
    }
#endif
}

void DeviceD3D11::InitializeAmdExt(AGSContext* agsContext, bool isImported) {
    MaybeUnused(agsContext, isImported);
#if NRI_ENABLE_AMDAGS
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
    m_AmdExt.CreateDeviceD3D11 = (AGS_DRIVEREXTENSIONSDX11_CREATEDEVICE)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_CreateDevice");
    m_AmdExt.DestroyDeviceD3D11 = (AGS_DRIVEREXTENSIONSDX11_DESTROYDEVICE)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_DestroyDevice");
    m_AmdExt.BeginUAVOverlap = (AGS_DRIVEREXTENSIONSDX11_BEGINUAVOVERLAP)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_BeginUAVOverlap");
    m_AmdExt.EndUAVOverlap = (AGS_DRIVEREXTENSIONSDX11_ENDUAVOVERLAP)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_EndUAVOverlap");
    m_AmdExt.SetDepthBounds = (AGS_DRIVEREXTENSIONSDX11_SETDEPTHBOUNDS)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_SetDepthBounds");
    m_AmdExt.DrawIndirect = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECT)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_MultiDrawInstancedIndirect");
    m_AmdExt.DrawIndexedIndirect = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECT)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect");
    m_AmdExt.DrawIndirectCount = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect");
    m_AmdExt.DrawIndexedIndirectCount = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect");
    m_AmdExt.SetViewBroadcastMasks = (AGS_DRIVEREXTENSIONSDX11_SETVIEWBROADCASTMASKS)GetSharedLibraryFunction(*agsLibrary, "agsDriverExtensionsDX11_SetViewBroadcastMasks");

    // Verify
    const void** functionArray = (const void**)&m_AmdExt;
    const size_t functionArraySize = 12;
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
        AGSReturnCode result = m_AmdExt.Initialize(AGS_CURRENT_VERSION, &config, &agsContext, &gpuInfo);
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

void DeviceD3D11::GetMemoryDesc(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    const bool isConstantBuffer = (bufferDesc.usage & BufferUsageBits::CONSTANT_BUFFER) == (uint32_t)BufferUsageBits::CONSTANT_BUFFER;

    uint32_t alignment = 65536;
    if (isConstantBuffer)
        alignment = 256;
    else if (bufferDesc.size <= 4096)
        alignment = 4096;

    memoryDesc = {};
    memoryDesc.type = (MemoryType)memoryLocation;
    memoryDesc.size = Align(bufferDesc.size, alignment);
    memoryDesc.alignment = alignment;
}

void DeviceD3D11::GetMemoryDesc(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    bool isMultisampled = textureDesc.sampleNum > 1;
    uint32_t size = TextureD3D11::GetMipmappedSize(textureDesc);

    uint32_t alignment = 65536;
    if (isMultisampled)
        alignment = 4194304;
    else if (size <= 65536)
        alignment = 65536;

    size = Align(size, alignment);

    memoryDesc = {};
    memoryDesc.type = (MemoryType)memoryLocation;
    memoryDesc.size = size;
    memoryDesc.alignment = alignment;
}

void DeviceD3D11::Destruct() {
    Destroy(GetAllocationCallbacks(), this);
}

NRI_INLINE Result DeviceD3D11::GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    const auto& queueFamily = m_QueueFamilies[(uint32_t)queueType];
    if (queueFamily.empty())
        return Result::UNSUPPORTED;

    if (queueIndex < queueFamily.size()) {
        queue = (Queue*)m_QueueFamilies[(uint32_t)queueType].at(queueIndex);
        return Result::SUCCESS;
    }

    return Result::INVALID_ARGUMENT;
}

NRI_INLINE Result DeviceD3D11::WaitIdle() {
    QueueD3D11* anyQueue = nullptr;
    for (auto& queueFamily : m_QueueFamilies) {
        if (!queueFamily.empty()) {
            anyQueue = queueFamily[0];
            break;
        }
    }

    // Can wait only once on any queue, because there are no real queues in D3D11
    if (anyQueue)
        return anyQueue->WaitIdle();

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceD3D11::BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum) {
    for (uint32_t i = 0; i < bindBufferMemoryDescNum; i++) {
        const BindBufferMemoryDesc& desc = bindBufferMemoryDescs[i];
        const MemoryD3D11& memory = *(MemoryD3D11*)desc.memory;
        Result res = ((BufferD3D11*)desc.buffer)->Allocate(memory.GetLocation(), memory.GetPriority());
        if (res != Result::SUCCESS)
            return res;
    }

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceD3D11::BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum) {
    for (uint32_t i = 0; i < bindTextureMemoryDescNum; i++) {
        const BindTextureMemoryDesc& desc = bindTextureMemoryDescs[i];
        const MemoryD3D11& memory = *(MemoryD3D11*)desc.memory;
        Result res = ((TextureD3D11*)desc.texture)->Allocate(memory.GetLocation(), memory.GetPriority());
        if (res != Result::SUCCESS)
            return res;
    }

    return Result::SUCCESS;
}

#define UPDATE_SUPPORT_BITS(required, optional, bit) \
    if ((formatSupport.OutFormatSupport & (required)) == (required) && ((formatSupport.OutFormatSupport & (optional)) != 0 || (optional) == 0)) \
        supportBits |= bit;

#define UPDATE_SUPPORT2_BITS(optional, bit) \
    if ((formatSupport2.OutFormatSupport2 & (optional)) != 0) \
        supportBits |= bit;

NRI_INLINE FormatSupportBits DeviceD3D11::GetFormatSupport(Format format) const {
    DXGI_FORMAT dxgiFormat = GetDxgiFormat(format).typed;
    D3D11_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {dxgiFormat};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));

    FormatSupportBits supportBits = FormatSupportBits::UNSUPPORTED;
    if (SUCCEEDED(hr)) {
        UPDATE_SUPPORT_BITS(0, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE | D3D11_FORMAT_SUPPORT_SHADER_LOAD, FormatSupportBits::TEXTURE);
        UPDATE_SUPPORT_BITS(D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW, 0, FormatSupportBits::STORAGE_TEXTURE);
        UPDATE_SUPPORT_BITS(D3D11_FORMAT_SUPPORT_RENDER_TARGET, 0, FormatSupportBits::COLOR_ATTACHMENT);
        UPDATE_SUPPORT_BITS(D3D11_FORMAT_SUPPORT_DEPTH_STENCIL, 0, FormatSupportBits::DEPTH_STENCIL_ATTACHMENT);
        UPDATE_SUPPORT_BITS(D3D11_FORMAT_SUPPORT_BLENDABLE, 0, FormatSupportBits::BLEND);

        UPDATE_SUPPORT_BITS(D3D11_FORMAT_SUPPORT_BUFFER, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE | D3D11_FORMAT_SUPPORT_SHADER_LOAD, FormatSupportBits::BUFFER);
        UPDATE_SUPPORT_BITS(D3D11_FORMAT_SUPPORT_BUFFER | D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW, 0, FormatSupportBits::STORAGE_BUFFER);
        UPDATE_SUPPORT_BITS(D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER, 0, FormatSupportBits::VERTEX_BUFFER);
    }

    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 formatSupport2 = {dxgiFormat};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &formatSupport2, sizeof(formatSupport2));

    constexpr uint32_t anyAtomics = D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_ADD
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX;

    if (SUCCEEDED(hr)) {
        if (supportBits & FormatSupportBits::STORAGE_TEXTURE)
            UPDATE_SUPPORT2_BITS(anyAtomics, FormatSupportBits::STORAGE_TEXTURE_ATOMICS);

        if (supportBits & FormatSupportBits::STORAGE_BUFFER)
            UPDATE_SUPPORT2_BITS(anyAtomics, FormatSupportBits::STORAGE_BUFFER_ATOMICS);
    }

    for (uint32_t i = 0; i < 3; i++) {
        uint32_t numQualityLevels = 0;
        if (SUCCEEDED(m_Device->CheckMultisampleQualityLevels(dxgiFormat, 2 << i, &numQualityLevels)) && numQualityLevels > 0)
            supportBits |= (FormatSupportBits)((uint32_t)FormatSupportBits::MULTISAMPLE_2X << i);
    }

    // D3D requires only "unorm / snorm" specification, not a fully qualified format like in VK
    if (supportBits & (FormatSupportBits::STORAGE_TEXTURE | FormatSupportBits::STORAGE_BUFFER))
        supportBits |= FormatSupportBits::STORAGE_READ_WITHOUT_FORMAT | FormatSupportBits::STORAGE_WRITE_WITHOUT_FORMAT;

    return supportBits;
}

#undef UPDATE_SUPPORT_BITS
#undef UPDATE_SUPPORT2_BITS
