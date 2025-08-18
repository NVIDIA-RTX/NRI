// © 2021 NVIDIA Corporation

constexpr std::array<DXGI_FORMAT, (size_t)SwapChainFormat::MAX_NUM> g_swapChainFormat = {
    DXGI_FORMAT_R16G16B16A16_FLOAT, // BT709_G10_16BIT
    DXGI_FORMAT_R8G8B8A8_UNORM,     // BT709_G22_8BIT
    DXGI_FORMAT_R10G10B10A2_UNORM,  // BT709_G22_10BIT
    DXGI_FORMAT_R10G10B10A2_UNORM,  // BT2020_G2084_10BIT
};
VALIDATE_ARRAY(g_swapChainFormat);

constexpr std::array<DXGI_COLOR_SPACE_TYPE, (size_t)SwapChainFormat::MAX_NUM> g_colorSpace = {
    DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,    // BT709_G10_16BIT
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,    // BT709_G22_8BIT
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,    // BT709_G22_10BIT
    DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, // BT2020_G2084_10BIT
};
VALIDATE_ARRAY(g_colorSpace);

static uint8_t QueryLatestInterface(ComPtr<IDXGISwapChainBest>& in, ComPtr<IDXGISwapChainBest>& out) {
    static const IID versions[] = {
        __uuidof(IDXGISwapChain4),
        __uuidof(IDXGISwapChain3),
        __uuidof(IDXGISwapChain2),
        __uuidof(IDXGISwapChain1),
        __uuidof(IDXGISwapChain),
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

SwapChainD3D12::~SwapChainD3D12() {
    if (m_FrameLatencyWaitableObject)
        CloseHandle(m_FrameLatencyWaitableObject);

    for (TextureD3D12* texture : m_Textures)
        Destroy<TextureD3D12>(m_Device.GetAllocationCallbacks(), texture);
}

Result SwapChainD3D12::Create(const SwapChainDesc& swapChainDesc) {
    HWND hwnd = (HWND)swapChainDesc.window.windows.hwnd;
    if (!hwnd)
        return Result::INVALID_ARGUMENT;

    QueueD3D12& queue = *(QueueD3D12*)swapChainDesc.queue;

    // Query DXGIFactory2
    HRESULT hr = m_Device.GetAdapter()->GetParent(IID_PPV_ARGS(&m_DxgiFactory2));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIAdapter::GetParent");

    // Allow tearing?
    bool allowTearing = false;
    if (swapChainDesc.flags & SwapChainBits::ALLOW_TEARING) {
        ComPtr<IDXGIFactory5> dxgiFactory5;
        hr = m_DxgiFactory2->QueryInterface(IID_PPV_ARGS(&dxgiFactory5));
        if (SUCCEEDED(hr)) {
            uint32_t tearingSupport = 0;
            hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupport, sizeof(tearingSupport));
            allowTearing = (SUCCEEDED(hr) && tearingSupport) ? true : false;
        }
    }

    // Create swapchain
    DXGI_FORMAT format = g_swapChainFormat[(uint32_t)swapChainDesc.format];
    DXGI_COLOR_SPACE_TYPE colorSpace = g_colorSpace[(uint32_t)swapChainDesc.format];

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = swapChainDesc.width;
    desc.Height = swapChainDesc.height;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = swapChainDesc.textureNum;
    desc.Scaling = swapChainDesc.scaling == Scaling::STRETCH ? DXGI_SCALING_STRETCH : DXGI_SCALING_NONE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    if (swapChainDesc.flags & SwapChainBits::WAITABLE)
        desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    if (allowTearing)
        desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChainBest> swapChain;
    hr = m_DxgiFactory2->CreateSwapChainForHwnd((ID3D12CommandQueue*)queue, hwnd, &desc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIFactory2::CreateSwapChainForHwnd");

    m_Version = QueryLatestInterface(swapChain, m_SwapChain);

    hr = m_DxgiFactory2->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIFactory::MakeWindowAssociation");

    // Color space
    if (m_Version >= 3) {
        uint32_t colorSpaceSupport = 0;
        hr = m_SwapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);

        if (!(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
            hr = E_FAIL;

        if (SUCCEEDED(hr))
            hr = m_SwapChain->SetColorSpace1(colorSpace);

        if (FAILED(hr))
            REPORT_WARNING(&m_Device, "IDXGISwapChain3::SetColorSpace1() failed!");
    } else
        REPORT_ERROR(&m_Device, "IDXGISwapChain3::SetColorSpace1() is not supported by the OS!");

    // Background color
    if (m_Version >= 1) {
        DXGI_RGBA color = {0.0f, 0.0f, 0.0f, 1.0f};
        hr = m_SwapChain->SetBackgroundColor(&color);
        if (FAILED(hr))
            REPORT_WARNING(&m_Device, "IDXGISwapChain1::SetBackgroundColor() failed!");
    }

    // Maximum frame latency
    uint8_t queuedFrameNum = swapChainDesc.queuedFrameNum;
    if ((swapChainDesc.flags & SwapChainBits::WAITABLE) && m_Version >= 2) {
        if (queuedFrameNum == 0)
            queuedFrameNum = 1;

        // https://docs.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains#step-4-wait-before-rendering-each-frame
        // IMPORTANT: SetMaximumFrameLatency must be called BEFORE GetFrameLatencyWaitableObject!
        hr = m_SwapChain->SetMaximumFrameLatency(queuedFrameNum);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain2::SetMaximumFrameLatency");

        m_FrameLatencyWaitableObject = m_SwapChain->GetFrameLatencyWaitableObject();
    } else {
        if (queuedFrameNum == 0)
            queuedFrameNum = 2;

        ComPtr<IDXGIDevice1> dxgiDevice1;
        hr = m_Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice1));
        if (SUCCEEDED(hr))
            dxgiDevice1->SetMaximumFrameLatency(queuedFrameNum);
    }

    // Textures
    m_Textures.reserve(swapChainDesc.textureNum);
    for (uint32_t i = 0; i < swapChainDesc.textureNum; i++) {
        ComPtr<ID3D12Resource> textureNative;
        hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&textureNative));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain::GetBuffer");

        TextureD3D12Desc textureDesc = {};
        textureDesc.d3d12Resource = textureNative;

        TextureD3D12* texture = Allocate<TextureD3D12>(m_Device.GetAllocationCallbacks(), m_Device);
        const Result res = texture->Create(textureDesc);
        if (res != Result::SUCCESS)
            return res;

        m_Textures.push_back(texture);
    }

    // Finalize
    m_Hwnd = swapChainDesc.window.windows.hwnd;
    m_PresentId = GetSwapChainId();

    m_Flags = swapChainDesc.flags;
    if (!m_Device.HasNvExt())
        m_Flags &= ~SwapChainBits::ALLOW_LOW_LATENCY;
    if (!allowTearing)
        m_Flags &= ~SwapChainBits::ALLOW_TEARING;
    if (!m_FrameLatencyWaitableObject)
        m_Flags &= ~SwapChainBits::WAITABLE;

    return Result::SUCCESS;
}

NRI_INLINE Texture* const* SwapChainD3D12::GetTextures(uint32_t& textureNum) const {
    textureNum = (uint32_t)m_Textures.size();

    return (Texture**)m_Textures.data();
}

NRI_INLINE Result SwapChainD3D12::AcquireNextTexture(uint32_t& textureIndex) {
    textureIndex = m_SwapChain->GetCurrentBackBufferIndex();

    // Is device lost?
    HRESULT hr = m_Device->GetDeviceRemovedReason() == S_OK ? S_OK : DXGI_ERROR_DEVICE_REMOVED;
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "AcquireNextTexture");

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainD3D12::WaitForPresent() {
    if (m_FrameLatencyWaitableObject) {
        // Is device lost?
        HRESULT hr = m_Device->GetDeviceRemovedReason() == S_OK ? S_OK : DXGI_ERROR_DEVICE_REMOVED;
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "WaitForPresent");

        uint32_t result = WaitForSingleObjectEx(m_FrameLatencyWaitableObject, TIMEOUT_PRESENT, TRUE);

        return result == WAIT_OBJECT_0 ? Result::SUCCESS : Result::FAILURE;
    }

    return Result::UNSUPPORTED;
}

NRI_INLINE Result SwapChainD3D12::Present() {
#if NRI_ENABLE_NVAPI
    if (m_Flags & SwapChainBits::ALLOW_LOW_LATENCY)
        SetLatencyMarker((LatencyMarker)PRESENT_START);
#endif

    bool vsync = (m_Flags & SwapChainBits::VSYNC) != 0;
    bool allowTearing = (m_Flags & SwapChainBits::ALLOW_TEARING) != 0;
    uint32_t flags = (!vsync && allowTearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    HRESULT hr = m_SwapChain->Present(vsync ? 1 : 0, flags);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain::Present");

#if NRI_ENABLE_NVAPI
    if (m_Flags & SwapChainBits::ALLOW_LOW_LATENCY)
        SetLatencyMarker((LatencyMarker)PRESENT_END);
#endif

    m_PresentId++;

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainD3D12::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
#if NRI_ENABLE_NVAPI
    NV_SET_SLEEP_MODE_PARAMS params = {NV_SET_SLEEP_MODE_PARAMS_VER};
    params.bLowLatencyMode = latencySleepMode.lowLatencyMode;
    params.bLowLatencyBoost = latencySleepMode.lowLatencyBoost;
    params.minimumIntervalUs = latencySleepMode.minIntervalUs;
    params.bUseMarkersToOptimize = true;

    NvAPI_Status status = NvAPI_D3D_SetSleepMode(m_Device.GetNativeObject(), &params);

    return status == NVAPI_OK ? Result::SUCCESS : Result::FAILURE;
#else
    MaybeUnused(latencySleepMode);

    return Result::UNSUPPORTED;
#endif
}

NRI_INLINE Result SwapChainD3D12::SetLatencyMarker(LatencyMarker latencyMarker) {
#if NRI_ENABLE_NVAPI
    NV_LATENCY_MARKER_PARAMS params = {NV_LATENCY_MARKER_PARAMS_VER};
    params.frameID = m_PresentId;
    params.markerType = (NV_LATENCY_MARKER_TYPE)latencyMarker;

    NvAPI_Status status = NvAPI_D3D_SetLatencyMarker(m_Device.GetNativeObject(), &params);

    return status == NVAPI_OK ? Result::SUCCESS : Result::FAILURE;
#else
    MaybeUnused(latencyMarker);

    return Result::UNSUPPORTED;
#endif
}

NRI_INLINE Result SwapChainD3D12::LatencySleep() {
#if NRI_ENABLE_NVAPI
    NvAPI_Status status = NvAPI_D3D_Sleep(m_Device.GetNativeObject());

    return status == NVAPI_OK ? Result::SUCCESS : Result::FAILURE;
#else
    return Result::UNSUPPORTED;
#endif
}

NRI_INLINE Result SwapChainD3D12::GetLatencyReport(LatencyReport& latencyReport) {
    latencyReport = {};
#if NRI_ENABLE_NVAPI
    NV_LATENCY_RESULT_PARAMS params = {NV_LATENCY_RESULT_PARAMS_VER};
    NvAPI_Status status = NvAPI_D3D_GetLatency(m_Device.GetNativeObject(), &params);

    if (status == NVAPI_OK) {
        const uint32_t i = 63; // the most recent frame
        latencyReport.inputSampleTimeUs = params.frameReport[i].inputSampleTime;
        latencyReport.simulationStartTimeUs = params.frameReport[i].simStartTime;
        latencyReport.simulationEndTimeUs = params.frameReport[i].simEndTime;
        latencyReport.renderSubmitStartTimeUs = params.frameReport[i].renderSubmitStartTime;
        latencyReport.renderSubmitEndTimeUs = params.frameReport[i].renderSubmitEndTime;
        latencyReport.presentStartTimeUs = params.frameReport[i].presentStartTime;
        latencyReport.presentEndTimeUs = params.frameReport[i].presentEndTime;
        latencyReport.driverStartTimeUs = params.frameReport[i].driverStartTime;
        latencyReport.driverEndTimeUs = params.frameReport[i].driverEndTime;
        latencyReport.osRenderQueueStartTimeUs = params.frameReport[i].osRenderQueueStartTime;
        latencyReport.osRenderQueueEndTimeUs = params.frameReport[i].osRenderQueueEndTime;
        latencyReport.gpuRenderStartTimeUs = params.frameReport[i].gpuRenderStartTime;
        latencyReport.gpuRenderEndTimeUs = params.frameReport[i].gpuRenderEndTime;

        return Result::SUCCESS;
    }

    return Result::FAILURE;
#else
    return Result::UNSUPPORTED;
#endif
}
