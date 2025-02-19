// © 2021 NVIDIA Corporation

uint8_t QueryLatestDeviceContext(ComPtr<ID3D11DeviceContextBest>& in, ComPtr<ID3D11DeviceContextBest>& out);

static uint8_t QueryLatestDevice(ComPtr<ID3D11DeviceBest>& in, ComPtr<ID3D11DeviceBest>& out) {
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
    : DeviceBase(callbacks, allocationCallbacks) {
    m_Desc.graphicsAPI = GraphicsAPI::D3D11;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;
}

DeviceD3D11::~DeviceD3D11() {
#if NRI_ENABLE_D3D_EXTENSIONS
    if (m_ImmediateContext) {
        if (HasNvExt()) {
            NvAPI_Status status = NvAPI_D3D11_EndUAVOverlap(m_ImmediateContext);
            if (status != NVAPI_OK)
                REPORT_WARNING(this, "NvAPI_D3D11_EndUAVOverlap() failed!");
        } else if (HasAmdExt()) {
            AGSReturnCode res = m_AmdExt.EndUAVOverlap(m_AmdExt.context, m_ImmediateContext);
            if (res != AGS_SUCCESS)
                REPORT_WARNING(this, "agsDriverExtensionsDX11_EndUAVOverlap() failed!");
        }
    }
#endif

    for (auto& queueFamily : m_QueueFamilies) {
        for (uint32_t i = 0; i < queueFamily.size(); i++)
            Destroy<QueueD3D11>(queueFamily[i]);
    }

    DeleteCriticalSection(&m_CriticalSection);

#if NRI_ENABLE_D3D_EXTENSIONS
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
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIDevice::GetAdapter()");
    } else {
        ComPtr<IDXGIFactory4> dxgiFactory;
        HRESULT hr = CreateDXGIFactory2(desc.enableGraphicsAPIValidation ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgiFactory));
        RETURN_ON_BAD_HRESULT(this, hr, "CreateDXGIFactory2()");

        LUID luid = *(LUID*)&m_Desc.adapterDesc.luid;
        hr = dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&m_Adapter));
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIFactory4::EnumAdapterByLuid()");
    }

    // Extensions
    if (m_Desc.adapterDesc.vendor == Vendor::NVIDIA)
        InitializeNvExt(descD3D11.isNVAPILoaded, m_IsWrapped);
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

#if NRI_ENABLE_D3D_EXTENSIONS
        uint32_t shaderExtRegister = desc.shaderExtRegister ? desc.shaderExtRegister : NRI_SHADER_EXT_REGISTER;
        if (HasAmdExt()) {
            AGSDX11DeviceCreationParams deviceCreationParams = {};
            deviceCreationParams.pAdapter = m_Adapter;
            deviceCreationParams.DriverType = D3D_DRIVER_TYPE_UNKNOWN;
            deviceCreationParams.Flags = flags;
            deviceCreationParams.pFeatureLevels = levels;
            deviceCreationParams.FeatureLevels = levelNum;
            deviceCreationParams.SDKVersion = D3D11_SDK_VERSION;

            AGSDX11ExtensionParams extensionsParams = {};
            extensionsParams.uavSlot = shaderExtRegister;

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

            m_Desc.isBarycentricSupported = agsParams.extensionsSupported.intrinsics16;
            m_Desc.viewMaxNum = agsParams.extensionsSupported.multiView ? 4 : 1;
            m_Desc.isViewportBasedMultiviewSupported = agsParams.extensionsSupported.multiView;
        } else {
#endif
            HRESULT hr = D3D11CreateDevice(m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, levels, levelNum, D3D11_SDK_VERSION, (ID3D11Device**)&deviceTemp, nullptr, nullptr);
            if (flags && (uint32_t)hr == 0x887a002d) {
                // If Debug Layer is not available, try without D3D11_CREATE_DEVICE_DEBUG
                hr = D3D11CreateDevice(m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, levels, levelNum, D3D11_SDK_VERSION, (ID3D11Device**)&deviceTemp, nullptr, nullptr);
            }

            RETURN_ON_BAD_HRESULT(this, hr, "D3D11CreateDevice()");

#if NRI_ENABLE_D3D_EXTENSIONS
            if (HasNvExt()) {
                REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D_RegisterDevice(deviceTemp));
                REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D11_SetNvShaderExtnSlot(deviceTemp, shaderExtRegister));
                REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(deviceTemp, NV_EXTN_OP_UINT64_ATOMIC, &isShaderAtomicsI64Supported));
                isDepthBoundsTestSupported = true;
            }
        }
#endif

        // Start filling here to avoid passing additional arguments into "FillDesc"
        m_Desc.isDepthBoundsTestSupported = isDepthBoundsTestSupported;
        m_Desc.isDrawIndirectCountSupported = isDrawIndirectCountSupported;
        m_Desc.isShaderAtomicsI64Supported = isShaderAtomicsI64Supported;
    }

    m_Version = QueryLatestDevice(deviceTemp, m_Device);
    REPORT_INFO(this, "Using ID3D11Device%u", m_Version);

    // Immediate context
    ComPtr<ID3D11DeviceContextBest> immediateContext;
    m_Device->GetImmediateContext((ID3D11DeviceContext**)&immediateContext);

    m_ImmediateContextVersion = QueryLatestDeviceContext(immediateContext, m_ImmediateContext);
    REPORT_INFO(this, "Using ID3D11DeviceContext%u", m_ImmediateContextVersion);

    // Skip UAV barriers by default on the immediate context
#if NRI_ENABLE_D3D_EXTENSIONS
    if (HasNvExt()) {
        NvAPI_Status status = NvAPI_D3D11_BeginUAVOverlap(m_ImmediateContext);
        if (status != NVAPI_OK)
            REPORT_WARNING(this, "NvAPI_D3D11_BeginUAVOverlap() failed!");
    } else if (HasAmdExt()) {
        AGSReturnCode res = m_AmdExt.BeginUAVOverlap(m_AmdExt.context, m_ImmediateContext);
        if (res != AGS_SUCCESS)
            REPORT_WARNING(this, "agsDriverExtensionsDX11_BeginUAVOverlap() failed!");
    }
#endif

    // Threading
    D3D11_FEATURE_DATA_THREADING threadingCaps = {};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingCaps, sizeof(threadingCaps));
    if (FAILED(hr) || !threadingCaps.DriverConcurrentCreates)
        REPORT_WARNING(this, "Concurrent resource creation is not supported by the driver!");

    m_IsDeferredContextEmulated = !HasNvExt() || desc.enableD3D11CommandBufferEmulation;
    if (!threadingCaps.DriverCommandLists) {
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

    m_Desc.viewportMaxNum = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    m_Desc.viewportBoundsRange[0] = D3D11_VIEWPORT_BOUNDS_MIN;
    m_Desc.viewportBoundsRange[1] = D3D11_VIEWPORT_BOUNDS_MAX;

    m_Desc.attachmentMaxDim = D3D11_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH;
    m_Desc.attachmentLayerMaxNum = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.colorAttachmentMaxNum = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;

    m_Desc.colorSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.depthSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.stencilSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.zeroAttachmentsSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureColorSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureIntegerSampleMaxNum = 1;
    m_Desc.textureDepthSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureStencilSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.storageTextureSampleMaxNum = 1;

    m_Desc.texture1DMaxDim = D3D11_REQ_TEXTURE1D_U_DIMENSION;
    m_Desc.texture2DMaxDim = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Desc.texture3DMaxDim = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Desc.textureArrayLayerMaxNum = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.typedBufferMaxDim = 1 << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;

    m_Desc.memoryAllocationMaxNum = (uint32_t)(-1);
    m_Desc.samplerAllocationMaxNum = D3D11_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
    m_Desc.constantBufferMaxRange = D3D11_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    m_Desc.storageBufferMaxRange = 1 << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;
    m_Desc.bufferTextureGranularity = 1;
    m_Desc.bufferMaxSize = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ull * 1024ull;

    m_Desc.uploadBufferTextureRowAlignment = 256;   // D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    m_Desc.uploadBufferTextureSliceAlignment = 512; // D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    m_Desc.bufferShaderResourceOffsetAlignment = D3D11_RAW_UAV_SRV_BYTE_ALIGNMENT;
    m_Desc.constantBufferOffsetAlignment = 256; // D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    m_Desc.pipelineLayoutDescriptorSetMaxNum = ROOT_SIGNATURE_DWORD_NUM / 1;
    m_Desc.pipelineLayoutRootConstantMaxSize = sizeof(uint32_t) * ROOT_SIGNATURE_DWORD_NUM / 1;
    m_Desc.pipelineLayoutRootDescriptorMaxNum = ROOT_SIGNATURE_DWORD_NUM / 2;

    m_Desc.perStageDescriptorSamplerMaxNum = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
    m_Desc.perStageDescriptorConstantBufferMaxNum = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    m_Desc.perStageDescriptorStorageBufferMaxNum = m_Version >= 1 ? D3D11_1_UAV_SLOT_COUNT : D3D11_PS_CS_UAV_REGISTER_COUNT;
    m_Desc.perStageDescriptorTextureMaxNum = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.perStageDescriptorStorageTextureMaxNum = m_Version >= 1 ? D3D11_1_UAV_SLOT_COUNT : D3D11_PS_CS_UAV_REGISTER_COUNT;
    m_Desc.perStageResourceMaxNum = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    m_Desc.descriptorSetSamplerMaxNum = m_Desc.perStageDescriptorSamplerMaxNum;
    m_Desc.descriptorSetConstantBufferMaxNum = m_Desc.perStageDescriptorConstantBufferMaxNum;
    m_Desc.descriptorSetStorageBufferMaxNum = m_Desc.perStageDescriptorStorageBufferMaxNum;
    m_Desc.descriptorSetTextureMaxNum = m_Desc.perStageDescriptorTextureMaxNum;
    m_Desc.descriptorSetStorageTextureMaxNum = m_Desc.perStageDescriptorStorageTextureMaxNum;

    m_Desc.vertexShaderAttributeMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.vertexShaderStreamMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.vertexShaderOutputComponentMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 4;

    m_Desc.tessControlShaderGenerationMaxLevel = D3D11_HS_MAXTESSFACTOR_UPPER_BOUND;
    m_Desc.tessControlShaderPatchPointMaxNum = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
    m_Desc.tessControlShaderPerVertexInputComponentMaxNum = D3D11_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT * D3D11_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessControlShaderPerVertexOutputComponentMaxNum = D3D11_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT * D3D11_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessControlShaderPerPatchOutputComponentMaxNum = D3D11_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS;
    m_Desc.tessControlShaderTotalOutputComponentMaxNum = m_Desc.tessControlShaderPatchPointMaxNum * m_Desc.tessControlShaderPerVertexOutputComponentMaxNum + m_Desc.tessControlShaderPerPatchOutputComponentMaxNum;
    m_Desc.tessEvaluationShaderInputComponentMaxNum = D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessEvaluationShaderOutputComponentMaxNum = D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;

    m_Desc.geometryShaderInvocationMaxNum = D3D11_GS_MAX_INSTANCE_COUNT;
    m_Desc.geometryShaderInputComponentMaxNum = D3D11_GS_INPUT_REGISTER_COUNT * D3D11_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.geometryShaderOutputComponentMaxNum = D3D11_GS_OUTPUT_REGISTER_COUNT * D3D11_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.geometryShaderOutputVertexMaxNum = D3D11_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
    m_Desc.geometryShaderTotalOutputComponentMaxNum = D3D11_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT;

    m_Desc.fragmentShaderInputComponentMaxNum = D3D11_PS_INPUT_REGISTER_COUNT * D3D11_PS_INPUT_REGISTER_COMPONENTS;
    m_Desc.fragmentShaderOutputAttachmentMaxNum = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Desc.fragmentShaderDualSourceAttachmentMaxNum = 1;

    m_Desc.computeShaderSharedMemoryMaxSize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
    m_Desc.computeShaderWorkGroupMaxNum[0] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupMaxNum[1] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupMaxNum[2] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupInvocationMaxNum = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    m_Desc.computeShaderWorkGroupMaxDim[0] = D3D11_CS_THREAD_GROUP_MAX_X;
    m_Desc.computeShaderWorkGroupMaxDim[1] = D3D11_CS_THREAD_GROUP_MAX_Y;
    m_Desc.computeShaderWorkGroupMaxDim[2] = D3D11_CS_THREAD_GROUP_MAX_Z;

    m_Desc.viewportPrecisionBits = D3D11_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.subPixelPrecisionBits = D3D11_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.subTexelPrecisionBits = D3D11_SUBTEXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.mipmapPrecisionBits = D3D11_MIP_LOD_FRACTIONAL_BIT_COUNT;

    m_Desc.timestampFrequencyHz = timestampFrequency;
    m_Desc.drawIndirectMaxNum = (1ull << D3D11_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1;
    m_Desc.samplerLodBiasMin = D3D11_MIP_LOD_BIAS_MIN;
    m_Desc.samplerLodBiasMax = D3D11_MIP_LOD_BIAS_MAX;
    m_Desc.samplerAnisotropyMax = D3D11_DEFAULT_MAX_ANISOTROPY;
    m_Desc.texelOffsetMin = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.texelOffsetMax = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.texelGatherOffsetMin = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.texelGatherOffsetMax = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.clipDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.cullDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.combinedClipAndCullDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.shaderModel = 51;

    m_Desc.conservativeRasterTier = (uint8_t)options2.ConservativeRasterizationTier;

    bool isShaderAtomicsF16Supported = false;
    bool isShaderAtomicsF32Supported = false;
    bool isGetSpecialSupported = false;
#if NRI_ENABLE_D3D_EXTENSIONS
    NV_D3D11_FEATURE_DATA_RASTERIZER_SUPPORT rasterizerFeatures = {};
    NV_D3D1x_GRAPHICS_CAPS caps = {};

    if (HasNvExt()) {
        REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP16_ATOMIC, &isShaderAtomicsF16Supported));
        REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP32_ATOMIC, &isShaderAtomicsF32Supported));
        REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D11_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_GET_SPECIAL, &isGetSpecialSupported));
        REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D11_CheckFeatureSupport(m_Device, NV_D3D11_FEATURE_RASTERIZER, &rasterizerFeatures, sizeof(rasterizerFeatures)));
        REPORT_ERROR_ON_BAD_STATUS(this, NvAPI_D3D1x_GetGraphicsCapabilities(m_Device, NV_D3D1x_GRAPHICS_CAPS_VER, &caps));
    }

    m_Desc.sampleLocationsTier = rasterizerFeatures.ProgrammableSamplePositions ? 2 : 0;

    m_Desc.shadingRateTier = caps.bVariablePixelRateShadingSupported ? 2 : 0;
    m_Desc.shadingRateAttachmentTileSize = NV_VARIABLE_PIXEL_SHADING_TILE_WIDTH;
    m_Desc.isAdditionalShadingRatesSupported = caps.bVariablePixelRateShadingSupported ? 1 : 0;
#endif

    m_Desc.isGetMemoryDesc2Supported = true;

    m_Desc.isTextureFilterMinMaxSupported = options1.MinMaxFiltering != 0;
    m_Desc.isLogicFuncSupported = options.OutputMergerLogicOp != 0;
    m_Desc.isLineSmoothingSupported = true;
    m_Desc.isEnchancedBarrierSupported = true; // don't care, but advertise support
    m_Desc.isWaitableSwapChainSupported = true; // TODO: swap chain version >= 2?

    m_Desc.isShaderNativeF64Supported = options.ExtendedDoublesShaderInstructions;
    m_Desc.isShaderAtomicsF16Supported = isShaderAtomicsF16Supported;
    m_Desc.isShaderAtomicsF32Supported = isShaderAtomicsF32Supported;
    m_Desc.isShaderViewportIndexSupported = options3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer;
    m_Desc.isShaderLayerSupported = options3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer;
    m_Desc.isShaderClockSupported = isGetSpecialSupported;
    m_Desc.isRasterizedOrderedViewSupported = options2.ROVsSupported != 0;

    m_Desc.isSwapChainSupported = HasOutput();
    m_Desc.isLowLatencySupported = HasNvExt();
}

void DeviceD3D11::InitializeNvExt(bool isNVAPILoadedInApp, bool isImported) {
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
            REPORT_ERROR(this, "Failed to initialize NVAPI: %d", (int32_t)status);
        m_NvExt.available = (status == NVAPI_OK);
    }
#endif
}

void DeviceD3D11::InitializeAmdExt(AGSContext* agsContext, bool isImported) {
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

NRI_INLINE Result DeviceD3D11::CreateCommandAllocator(const Queue&, CommandAllocator*& commandAllocator) {
    commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorD3D11>(GetAllocationCallbacks(), *this);

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceD3D11::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const BufferMemoryBindingDesc& desc = memoryBindingDescs[i];
        const MemoryD3D11& memory = *(MemoryD3D11*)desc.memory;
        Result res = ((BufferD3D11*)desc.buffer)->Create(memory.GetLocation(), memory.GetPriority());
        if (res != Result::SUCCESS)
            return res;
    }

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceD3D11::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const TextureMemoryBindingDesc& desc = memoryBindingDescs[i];
        const MemoryD3D11& memory = *(MemoryD3D11*)desc.memory;
        Result res = ((TextureD3D11*)desc.texture)->Create(memory.GetLocation(), memory.GetPriority());
        if (res != Result::SUCCESS)
            return res;
    }

    return Result::SUCCESS;
}

NRI_INLINE FormatSupportBits DeviceD3D11::GetFormatSupport(Format format) const {
    FormatSupportBits mask = FormatSupportBits::UNSUPPORTED;

    D3D11_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {GetDxgiFormat(format).typed};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));

#define UPDATE_SUPPORT_BITS(required, optional, bit) \
    if ((formatSupport.OutFormatSupport & (required)) == (required) && ((formatSupport.OutFormatSupport & (optional)) != 0 || (optional) == 0)) \
        mask |= bit;

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

#undef UPDATE_SUPPORT_BITS

    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 formatSupport2 = {GetDxgiFormat(format).typed};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &formatSupport2, sizeof(formatSupport2));

#define UPDATE_SUPPORT_BITS(optional, bit) \
    if ((formatSupport2.OutFormatSupport2 & (optional)) != 0) \
        mask |= bit;

    const uint32_t anyAtomics = D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_ADD
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX
        | D3D11_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX;

    if (SUCCEEDED(hr)) {
        if (mask & FormatSupportBits::STORAGE_TEXTURE)
            UPDATE_SUPPORT_BITS(anyAtomics, FormatSupportBits::STORAGE_TEXTURE_ATOMICS);

        if (mask & FormatSupportBits::STORAGE_BUFFER)
            UPDATE_SUPPORT_BITS(anyAtomics, FormatSupportBits::STORAGE_BUFFER_ATOMICS);
    }

#undef UPDATE_SUPPORT_BITS

    return mask;
}
