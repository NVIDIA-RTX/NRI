// © 2021 NVIDIA Corporation

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)

constexpr std::array<DxgiFormat, (size_t)nri::Format::MAX_NUM> DXGI_FORMAT_TABLE = {{
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},                            // UNKNOWN
    {DXGI_FORMAT_R8_TYPELESS, DXGI_FORMAT_R8_UNORM},                       // R8_UNORM
    {DXGI_FORMAT_R8_TYPELESS, DXGI_FORMAT_R8_SNORM},                       // R8_SNORM
    {DXGI_FORMAT_R8_TYPELESS, DXGI_FORMAT_R8_UINT},                        // R8_UINT
    {DXGI_FORMAT_R8_TYPELESS, DXGI_FORMAT_R8_SINT},                        // R8_SINT
    {DXGI_FORMAT_R8G8_TYPELESS, DXGI_FORMAT_R8G8_UNORM},                   // RG8_UNORM
    {DXGI_FORMAT_R8G8_TYPELESS, DXGI_FORMAT_R8G8_SNORM},                   // RG8_SNORM
    {DXGI_FORMAT_R8G8_TYPELESS, DXGI_FORMAT_R8G8_UINT},                    // RG8_UINT
    {DXGI_FORMAT_R8G8_TYPELESS, DXGI_FORMAT_R8G8_SINT},                    // RG8_SINT
    {DXGI_FORMAT_B8G8R8A8_TYPELESS, DXGI_FORMAT_B8G8R8A8_UNORM},           // BGRA8_UNORM
    {DXGI_FORMAT_B8G8R8A8_TYPELESS, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB},      // BGRA8_SRGB
    {DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UNORM},           // RGBA8_UNORM
    {DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB},      // RGBA8_SRGB
    {DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_SNORM},           // RGBA8_SNORM
    {DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UINT},            // RGBA8_UINT
    {DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_SINT},            // RGBA8_SINT
    {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM},                     // R16_UNORM
    {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_SNORM},                     // R16_SNORM
    {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UINT},                      // R16_UINT
    {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_SINT},                      // R16_SINT
    {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_FLOAT},                     // R16_SFLOAT
    {DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R16G16_UNORM},               // RG16_UNORM
    {DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R16G16_SNORM},               // RG16_SNORM
    {DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R16G16_UINT},                // RG16_UINT
    {DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R16G16_SINT},                // RG16_SINT
    {DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R16G16_FLOAT},               // RG16_SFLOAT
    {DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_UNORM},   // RGBA16_UNORM
    {DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_SNORM},   // RGBA16_SNORM
    {DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_UINT},    // RGBA16_UINT
    {DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_SINT},    // RGBA16_SINT
    {DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_FLOAT},   // RGBA16_SFLOAT
    {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_UINT},                      // R32_UINT
    {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_SINT},                      // R32_SINT
    {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT},                     // R32_SFLOAT
    {DXGI_FORMAT_R32G32_TYPELESS, DXGI_FORMAT_R32G32_UINT},                // RG32_UINT
    {DXGI_FORMAT_R32G32_TYPELESS, DXGI_FORMAT_R32G32_SINT},                // RG32_SINT
    {DXGI_FORMAT_R32G32_TYPELESS, DXGI_FORMAT_R32G32_FLOAT},               // RG32_SFLOAT
    {DXGI_FORMAT_R32G32B32_TYPELESS, DXGI_FORMAT_R32G32B32_UINT},          // RGB32_UINT
    {DXGI_FORMAT_R32G32B32_TYPELESS, DXGI_FORMAT_R32G32B32_SINT},          // RGB32_SINT
    {DXGI_FORMAT_R32G32B32_TYPELESS, DXGI_FORMAT_R32G32B32_FLOAT},         // RGB32_SFLOAT
    {DXGI_FORMAT_R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32A32_UINT},    // RGBA32_UINT
    {DXGI_FORMAT_R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32A32_SINT},    // RGBA32_SINT
    {DXGI_FORMAT_R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32A32_FLOAT},   // RGBA32_SFLOAT
    {DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_B5G6R5_UNORM},                  // B5_G6_R5_UNORM
    {DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_B5G5R5A1_UNORM},              // B5_G5_R5_A1_UNORM
    {DXGI_FORMAT_B4G4R4A4_UNORM, DXGI_FORMAT_B4G4R4A4_UNORM},              // B4_G4_R4_A4_UNORM
    {DXGI_FORMAT_R10G10B10A2_TYPELESS, DXGI_FORMAT_R10G10B10A2_UNORM},     // R10_G10_B10_A2_UNORM
    {DXGI_FORMAT_R10G10B10A2_TYPELESS, DXGI_FORMAT_R10G10B10A2_UINT},      // R10_G10_B10_A2_UINT
    {DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT},            // R11_G11_B10_UFLOAT
    {DXGI_FORMAT_R9G9B9E5_SHAREDEXP, DXGI_FORMAT_R9G9B9E5_SHAREDEXP},      // R9_G9_B9_E5_UFLOAT
    {DXGI_FORMAT_BC1_TYPELESS, DXGI_FORMAT_BC1_UNORM},                     // BC1_RGBA_UNORM
    {DXGI_FORMAT_BC1_TYPELESS, DXGI_FORMAT_BC1_UNORM_SRGB},                // BC1_RGBA_SRGB
    {DXGI_FORMAT_BC2_TYPELESS, DXGI_FORMAT_BC2_UNORM},                     // BC2_RGBA_UNORM
    {DXGI_FORMAT_BC2_TYPELESS, DXGI_FORMAT_BC2_UNORM_SRGB},                // BC2_RGBA_SRGB
    {DXGI_FORMAT_BC3_TYPELESS, DXGI_FORMAT_BC3_UNORM},                     // BC3_RGBA_UNORM
    {DXGI_FORMAT_BC3_TYPELESS, DXGI_FORMAT_BC3_UNORM_SRGB},                // BC3_RGBA_SRGB
    {DXGI_FORMAT_BC4_TYPELESS, DXGI_FORMAT_BC4_UNORM},                     // BC4_R_UNORM
    {DXGI_FORMAT_BC4_TYPELESS, DXGI_FORMAT_BC4_SNORM},                     // BC4_R_SNORM
    {DXGI_FORMAT_BC5_TYPELESS, DXGI_FORMAT_BC5_UNORM},                     // BC5_RG_UNORM
    {DXGI_FORMAT_BC5_TYPELESS, DXGI_FORMAT_BC5_SNORM},                     // BC5_RG_SNORM
    {DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16},                    // BC6H_RGB_UFLOAT
    {DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_SF16},                    // BC6H_RGB_SFLOAT
    {DXGI_FORMAT_BC7_TYPELESS, DXGI_FORMAT_BC7_UNORM},                     // BC7_RGBA_UNORM
    {DXGI_FORMAT_BC7_TYPELESS, DXGI_FORMAT_BC7_UNORM_SRGB},                // BC7_RGBA_SRGB
    {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM},                     // D16_UNORM
    {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT},           // D24_UNORM_S8_UINT
    {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT},                     // D32_SFLOAT
    {DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT},     // D32_SFLOAT_S8_UINT_X24
    {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS},       // R24_UNORM_X8
    {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_X24_TYPELESS_G8_UINT},        // X24_G8_UINT
    {DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS}, // R32_SFLOAT_X8_X24
    {DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_X32_TYPELESS_G8X24_UINT},  // X32_G8_UINT_X24
}};

const DxgiFormat& GetDxgiFormat(nri::Format format) {
    return DXGI_FORMAT_TABLE[(size_t)format];
}

nri::Result GetResultFromHRESULT(long result) {
    if (SUCCEEDED(result))
        return nri::Result::SUCCESS;

    if (result == E_INVALIDARG || result == E_POINTER || result == E_HANDLE)
        return nri::Result::INVALID_ARGUMENT;

    if (result == DXGI_ERROR_UNSUPPORTED)
        return nri::Result::UNSUPPORTED;

    if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
        return nri::Result::DEVICE_LOST;

    if (result == E_OUTOFMEMORY)
        return nri::Result::OUT_OF_MEMORY;

    return nri::Result::FAILURE;
}

uint32_t NRIFormatToDXGIFormat(nri::Format format) {
    return DXGI_FORMAT_TABLE[(size_t)format].typed;
}

// Returns true if this is an integrated display panel e.g. the screen attached to tablets or laptops
static bool IsInternalVideoOutput(const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY VideoOutputTechnologyType) {
    switch (VideoOutputTechnologyType) {
        case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL:
        case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
        case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED:
            return TRUE;

        default:
            return FALSE;
    }
}

// Since an HMONITOR can represent multiple monitors while in clone, this function as written will return
// the value for the internal monitor if one exists, and otherwise the highest clone-path priority
static HRESULT GetPathInfo(_In_ PCWSTR pszDeviceName, _Out_ DISPLAYCONFIG_PATH_INFO* pPathInfo) {
    HRESULT hr = S_OK;
    UINT32 NumPathArrayElements = 0;
    UINT32 NumModeInfoArrayElements = 0;
    DISPLAYCONFIG_PATH_INFO* PathInfoArray = nullptr;
    DISPLAYCONFIG_MODE_INFO* ModeInfoArray = nullptr;

    do {
        // In case this isn't the first time through the loop, delete the buffers allocated
        delete[] PathInfoArray;
        PathInfoArray = nullptr;

        delete[] ModeInfoArray;
        ModeInfoArray = nullptr;

        hr = HRESULT_FROM_WIN32(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements, &NumModeInfoArrayElements));
        if (FAILED(hr))
            break;

        PathInfoArray = new (std::nothrow) DISPLAYCONFIG_PATH_INFO[NumPathArrayElements];
        if (!PathInfoArray) {
            hr = E_OUTOFMEMORY;
            break;
        }

        ModeInfoArray = new (std::nothrow) DISPLAYCONFIG_MODE_INFO[NumModeInfoArrayElements];
        if (!ModeInfoArray) {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = HRESULT_FROM_WIN32(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements, PathInfoArray, &NumModeInfoArrayElements, ModeInfoArray, nullptr));
    } while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    INT DesiredPathIdx = -1;

    if (SUCCEEDED(hr)) {
        // Loop through all sources until the one which matches the 'monitor' is found.
        for (uint32_t PathIdx = 0; PathIdx < NumPathArrayElements; ++PathIdx) {
            DISPLAYCONFIG_SOURCE_DEVICE_NAME SourceName = {};
            SourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            SourceName.header.size = sizeof(SourceName);
            SourceName.header.adapterId = PathInfoArray[PathIdx].sourceInfo.adapterId;
            SourceName.header.id = PathInfoArray[PathIdx].sourceInfo.id;

            hr = HRESULT_FROM_WIN32(DisplayConfigGetDeviceInfo(&SourceName.header));
            if (SUCCEEDED(hr)) {
                if (wcscmp(pszDeviceName, SourceName.viewGdiDeviceName) == 0) {
                    // Found the source which matches this hmonitor. The paths are given in path-priority order
                    // so the first found is the most desired, unless we later find an internal.
                    if (DesiredPathIdx == -1 || IsInternalVideoOutput(PathInfoArray[PathIdx].targetInfo.outputTechnology)) {
                        DesiredPathIdx = PathIdx;
                    }
                }
            }
        }
    }

    if (DesiredPathIdx != -1)
        *pPathInfo = PathInfoArray[DesiredPathIdx];
    else
        hr = E_INVALIDARG;

    delete[] PathInfoArray;
    PathInfoArray = nullptr;

    delete[] ModeInfoArray;
    ModeInfoArray = nullptr;

    return hr;
}

// Overloaded function accepts an HMONITOR and converts to DeviceName
static HRESULT GetPathInfo(HMONITOR hMonitor, DISPLAYCONFIG_PATH_INFO* pPathInfo) {
    HRESULT hr = S_OK;

    // Get the name of the 'monitor' being requested
    MONITORINFOEXW ViewInfo;
    RtlZeroMemory(&ViewInfo, sizeof(ViewInfo));
    ViewInfo.cbSize = sizeof(ViewInfo);
    if (!GetMonitorInfoW(hMonitor, &ViewInfo))
        hr = HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr))
        hr = GetPathInfo(ViewInfo.szDevice, pPathInfo);

    return hr;
}

static float GetSdrLuminance(void* hMonitor) {
    float nits = 80.0f;

    DISPLAYCONFIG_PATH_INFO info;
    if (SUCCEEDED(GetPathInfo((HMONITOR)hMonitor, &info))) {
        const DISPLAYCONFIG_PATH_TARGET_INFO& targetInfo = info.targetInfo;

        DISPLAYCONFIG_SDR_WHITE_LEVEL level;
        DISPLAYCONFIG_DEVICE_INFO_HEADER& header = level.header;
        header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
        header.size = sizeof(level);
        header.adapterId = targetInfo.adapterId;
        header.id = targetInfo.id;

        if (DisplayConfigGetDeviceInfo(&header) == ERROR_SUCCESS)
            nits = (level.SDRWhiteLevel * 80.0f) / 1000.0f;
    }

    return nits;
}

// Compute the overlay area of two rectangles, A and B:
//  (ax1, ay1) = left-top coordinates of A; (ax2, ay2) = right-bottom coordinates of A
//  (bx1, by1) = left-top coordinates of B; (bx2, by2) = right-bottom coordinates of B

static inline int32_t ComputeIntersectionArea(int32_t ax1, int32_t ay1, int32_t ax2, int32_t ay2, int32_t bx1, int32_t by1, int32_t bx2, int32_t by2) {
    return std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
}

nri::Result DisplayDescHelper::GetDisplayDesc(void* hwnd, nri::DisplayDesc& displayDesc) {
    // To detect HDR support, we will need to check the color space in the primary DXGI output associated with the app at
    // this point in time (using window/display intersection).

    // If the display's advanced color state has changed (e.g. HDR display plug/unplug, or OS HDR setting on/off),
    // then this app's DXGI factory is invalidated and must be created anew in order to retrieve up-to-date display information.

    if (!m_DxgiFactory2 || !m_DxgiFactory2->IsCurrent()) {
        m_HasDisplayDesc = false;
        ComPtr<IDXGIFactory2> newDxgiFactory2;
        HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&newDxgiFactory2));
        if (FAILED(hr))
            return GetResultFromHRESULT(hr);
        m_DxgiFactory2 = newDxgiFactory2;
    } else if (m_HasDisplayDesc) {
        displayDesc = m_DisplayDesc;
        return nri::Result::SUCCESS;
    }

    // Get the retangle bounds of the app window

    RECT windowRect = {};
    GetWindowRect((HWND)hwnd, &windowRect);

    // First, the method must determine the app's current display.
    // We don't recommend using IDXGISwapChain::GetContainingOutput method to do that because of two reasons:
    //    1. Swap chains created with CreateSwapChainForComposition do not support this method.
    //    2. Swap chains will return a stale dxgi output once DXGIFactory::IsCurrent() is false. In addition,
    //       we don't recommend re-creating swapchain to resolve the stale dxgi output because it will cause a short
    //       period of black screen.
    // Instead, we suggest enumerating through the bounds of all dxgi outputs and determine which one has the greatest
    // intersection with the app window bounds. Then, use the DXGI output found in previous step to determine if the
    // app is on a HDR capable display.

    // Retrieve the current default adapter.

    ComPtr<IDXGIAdapter1> dxgiAdapter;
    HRESULT hr = m_DxgiFactory2->EnumAdapters1(0, &dxgiAdapter);
    if (FAILED(hr))
        return GetResultFromHRESULT(hr);

    // Iterate through the DXGI outputs associated with the DXGI adapter, and find the output whose bounds have the greatest
    // overlap with the app window (i.e. the output for which the intersection area is the greatest).

    ComPtr<IDXGIOutput> bestOutput;
    int32_t bestIntersectArea = 0;
    uint32_t i = 0;

    while (true) {
        ComPtr<IDXGIOutput> currentOutput;
        hr = dxgiAdapter->EnumOutputs(i, &currentOutput);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        // Get the rectangle bounds of current output

        DXGI_OUTPUT_DESC desc;
        hr = currentOutput->GetDesc(&desc);
        if (FAILED(hr))
            return GetResultFromHRESULT(hr);

        const RECT& outputRect = desc.DesktopCoordinates;

        // Compute the intersection

        int32_t intersectArea = ComputeIntersectionArea(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom, outputRect.left, outputRect.top, outputRect.right, outputRect.bottom);

        if (intersectArea > bestIntersectArea) {
            bestOutput = currentOutput;
            bestIntersectArea = intersectArea;
        }

        i++;
    }

    if (!bestOutput)
        return nri::Result::FAILURE;

    // Having determined the output (display) upon which the app is primarily being
    // rendered, retrieve the HDR capabilities of that display by checking the color space.

    ComPtr<IDXGIOutput6> output6;
    hr = bestOutput->QueryInterface(IID_PPV_ARGS(&output6));
    if (FAILED(hr))
        return GetResultFromHRESULT(hr);

    DXGI_OUTPUT_DESC1 desc = {};
    hr = output6->GetDesc1(&desc);
    if (FAILED(hr))
        return GetResultFromHRESULT(hr);

    displayDesc = {};
    displayDesc.redPrimary = {desc.RedPrimary[0], desc.RedPrimary[1]};
    displayDesc.greenPrimary = {desc.GreenPrimary[0], desc.GreenPrimary[1]};
    displayDesc.bluePrimary = {desc.BluePrimary[0], desc.BluePrimary[1]};
    displayDesc.whitePoint = {desc.WhitePoint[0], desc.WhitePoint[1]};
    displayDesc.minLuminance = desc.MinLuminance;
    displayDesc.maxLuminance = desc.MaxLuminance;
    displayDesc.maxFullFrameLuminance = desc.MaxFullFrameLuminance;
    displayDesc.sdrLuminance = GetSdrLuminance(desc.Monitor);
    displayDesc.isHDR = desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

    m_DisplayDesc = displayDesc;
    m_HasDisplayDesc = true;

    return nri::Result::SUCCESS;
}

bool HasOutput() {
    ComPtr<IDXGIFactory> factory;
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));
    if (FAILED(hr))
        return false;

    uint32_t i = 0;
    while (true) {
        // Get adapter
        ComPtr<IDXGIAdapter> adapter;
        hr = factory->EnumAdapters(i++, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        // Check if there is an output
        ComPtr<IDXGIOutput> output;
        hr = adapter->EnumOutputs(0, &output);
        if (hr != DXGI_ERROR_NOT_FOUND)
            return true;
    }

    return false;
}

nri::Result QueryVideoMemoryInfoDXGI(uint64_t luid, nri::MemoryLocation memoryLocation, nri::VideoMemoryInfo& videoMemoryInfo) {
    videoMemoryInfo = {};

    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return nri::Result::FAILURE;

    ComPtr<IDXGIAdapter3> adapter;
    if (FAILED(dxgifactory->EnumAdapterByLuid(*(LUID*)&luid, IID_PPV_ARGS(&adapter))))
        return nri::Result::FAILURE;

    bool isLocal = memoryLocation == nri::MemoryLocation::DEVICE || memoryLocation == nri::MemoryLocation::DEVICE_UPLOAD;

    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (FAILED(adapter->QueryVideoMemoryInfo(0, isLocal ? DXGI_MEMORY_SEGMENT_GROUP_LOCAL : DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info)))
        return nri::Result::FAILURE;

    videoMemoryInfo.budgetSize = info.Budget;
    videoMemoryInfo.usageSize = info.CurrentUsage;

    return nri::Result::SUCCESS;
}

#else

uint32_t NRIFormatToDXGIFormat(nri::Format) {
    return 0;
}

#endif

// clang-format off

#define _ 0
#define X 1

constexpr std::array<nri::FormatProps, (size_t)nri::Format::MAX_NUM> FORMAT_PROPS = {{
    //                                                                                                                  isStencil
    //                                                                                                                  isSrgb  |
    //                                                                                                             isSigned  |  |
    //                                                                                                            isNorm  |  |  |
    //                                                                                                      isInteger  |  |  |  |
    //                                                                                                    isPacked  |  |  |  |  |
    //                                                                                                  isFloat  |  |  |  |  |  |
    //                                                                                           isExpShared  |  |  |  |  |  |  |
    //                                                                                            isDepth  |  |  |  |  |  |  |  |
    //                                                                                    isCompressed  |  |  |  |  |  |  |  |  |
    //                                                                                        isBgr  |  |  |  |  |  |  |  |  |  |
    //                                                                               blockHeight  |  |  |  |  |  |  |  |  |  |  |
    //                                                                             blockWidth  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                                             stride   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                                         A bits   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                                     B bits   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                                 G bits   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                             R bits   |   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                          self                                    |   |   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    // format name              |                                       |   |   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    {"UNKNOWN",                 nri::Format::UNKNOWN,                   0,  0,  0,  0,  1,  0, 0, _, _, _, _, _, _, _, _, _, _, _}, // UNKNOWN
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"R8_UNORM",                nri::Format::R8_UNORM,                  8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // R8_UNORM
    {"R8_SNORM",                nri::Format::R8_SNORM,                  8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // R8_SNORM
    {"R8_UINT",                 nri::Format::R8_UINT,                   8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // R8_UINT
    {"R8_SINT",                 nri::Format::R8_SINT,                   8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // R8_SINT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RG8_UNORM",               nri::Format::RG8_UNORM,                 8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RG8_UNORM
    {"RG8_SNORM",               nri::Format::RG8_SNORM,                 8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RG8_SNORM
    {"RG8_UINT",                nri::Format::RG8_UINT,                  8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RG8_UINT
    {"RG8_SINT",                nri::Format::RG8_SINT,                  8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RG8_SINT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"BGRA8_UNORM",             nri::Format::BGRA8_UNORM,               8,  8,  8,  8,  4,  1, 1, X, _, _, _, _, _, _, X, _, _, _}, // BGRA8_UNORM
    {"BGRA8_SRGB",              nri::Format::BGRA8_SRGB,                8,  8,  8,  8,  4,  1, 1, X, _, _, _, _, _, _, _, _, X, _}, // BGRA8_SRGB
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGBA8_UNORM",             nri::Format::RGBA8_UNORM,               8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RGBA8_UNORM
    {"RGBA8_SRGB",              nri::Format::RGBA8_SRGB,                8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, _, _, _, X, _}, // RGBA8_SRGB
    {"RGBA8_SNORM",             nri::Format::RGBA8_SNORM,               8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RGBA8_SNORM
    {"RGBA8_UINT",              nri::Format::RGBA8_UINT,                8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGBA8_UINT
    {"RGBA8_SINT",              nri::Format::RGBA8_SINT,                8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGBA8_SINT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"R16_UNORM",               nri::Format::R16_UNORM,                 16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // R16_UNORM
    {"R16_SNORM",               nri::Format::R16_SNORM,                 16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // R16_SNORM
    {"R16_UINT",                nri::Format::R16_UINT,                  16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // R16_UINT
    {"R16_SINT",                nri::Format::R16_SINT,                  16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // R16_SINT
    {"R16_SFLOAT",              nri::Format::R16_SFLOAT,                16, 0,  0,  0,  2,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // R16_SFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RG16_UNORM",              nri::Format::RG16_UNORM,                16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RG16_UNORM
    {"RG16_SNORM",              nri::Format::RG16_SNORM,                16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RG16_SNORM
    {"RG16_UINT",               nri::Format::RG16_UINT,                 16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RG16_UINT
    {"RG16_SINT",               nri::Format::RG16_SINT,                 16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RG16_SINT
    {"RG16_SFLOAT",             nri::Format::RG16_SFLOAT,               16, 16, 0,  0,  4,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RG16_SFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGBA16_UNORM",            nri::Format::RGBA16_UNORM,              16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RGBA16_UNORM
    {"RGBA16_SNORM",            nri::Format::RGBA16_SNORM,              16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RGBA16_SNORM
    {"RGBA16_UINT",             nri::Format::RGBA16_UINT,               16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGBA16_UINT
    {"RGBA16_SINT",             nri::Format::RGBA16_SINT,               16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGBA16_SINT
    {"RGBA16_SFLOAT",           nri::Format::RGBA16_SFLOAT,             16, 16, 16, 16, 8,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RGBA16_SFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"R32_UINT",                nri::Format::R32_UINT,                  32, 32, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // R32_UINT
    {"R32_SINT",                nri::Format::R32_SINT,                  32, 32, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // R32_SINT
    {"R32_SFLOAT",              nri::Format::R32_SFLOAT,                32, 32, 0,  0,  4,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // R32_SFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RG32_UINT",               nri::Format::RG32_UINT,                 32, 32, 0,  0,  8,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RG32_UINT
    {"RG32_SINT",               nri::Format::RG32_SINT,                 32, 32, 0,  0,  8,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RG32_SINT
    {"RG32_SFLOAT",             nri::Format::RG32_SFLOAT,               32, 32, 0,  0,  8,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RG32_SFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGB32_UINT",              nri::Format::RGB32_UINT,                32, 32, 32, 0,  12, 1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGB32_UINT
    {"RGB32_SINT",              nri::Format::RGB32_SINT,                32, 32, 32, 0,  12, 1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGB32_SINT
    {"RGB32_SFLOAT",            nri::Format::RGB32_SFLOAT,              32, 32, 32, 0,  12, 1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RGB32_SFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGBA32_UINT",             nri::Format::RGBA32_UINT,               32, 32, 32, 32, 16, 1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGBA32_UINT
    {"RGBA32_SINT",             nri::Format::RGBA32_SINT,               32, 32, 32, 32, 16, 1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGBA32_SINT
    {"RGBA32_SFLOAT",           nri::Format::RGBA32_SFLOAT,             32, 32, 32, 32, 16, 1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RGBA32_SFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"B5_G6_R5_UNORM",          nri::Format::B5_G6_R5_UNORM,            5,  6,  5,  0,  2,  1, 1, X, _, _, _, _, X, _, X, _, _, _}, // B5_G6_R5_UNORM
    {"B5_G5_R5_A1_UNORM",       nri::Format::B5_G5_R5_A1_UNORM,         5,  5,  5,  1,  2,  1, 1, X, _, _, _, _, X, _, X, _, _, _}, // B5_G5_R5_A1_UNORM
    {"B4_G4_R4_A4_UNORM",       nri::Format::B4_G4_R4_A4_UNORM,         4,  4,  4,  4,  2,  1, 1, X, _, _, _, _, X, _, X, _, _, _}, // B4_G4_R4_A4_UNORM
    {"R10_G10_B10_A2_UNORM",    nri::Format::R10_G10_B10_A2_UNORM,      10, 10, 10, 2,  4,  1, 1, _, _, _, _, _, X, _, X, _, _, _}, // R10_G10_B10_A2_UNORM
    {"R10_G10_B10_A2_UINT",     nri::Format::R10_G10_B10_A2_UINT,       10, 10, 10, 2,  4,  1, 1, _, _, _, _, _, X, X, _, _, _, _}, // R10_G10_B10_A2_UINT
    {"R11_G11_B10_UFLOAT",      nri::Format::R11_G11_B10_UFLOAT,        11, 11, 10, 0,  4,  1, 1, _, _, _, _, X, X, _, _, _, _, _}, // R11_G11_B10_UFLOAT
    {"R9_G9_B9_E5_UFLOAT",      nri::Format::R9_G9_B9_E5_UFLOAT,        9,  9,  9,  5,  4,  1, 1, _, _, _, X, X, X, _, _, _, _, _}, // R9_G9_B9_E5_UFLOAT
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"BC1_RGBA_UNORM",          nri::Format::BC1_RGBA_UNORM,            5,  6,  5,  1,  8,  4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC1_RGBA_UNORM
    {"BC1_RGBA_SRGB",           nri::Format::BC1_RGBA_SRGB,             5,  6,  5,  1,  8,  4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC1_RGBA_SRGB
    {"BC2_RGBA_UNORM",          nri::Format::BC2_RGBA_UNORM,            5,  6,  5,  4,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC2_RGBA_UNORM
    {"BC2_RGBA_SRGB",           nri::Format::BC2_RGBA_SRGB,             5,  6,  5,  4,  16, 4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC2_RGBA_SRGB
    {"BC3_RGBA_UNORM",          nri::Format::BC3_RGBA_UNORM,            5,  6,  5,  8,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC3_RGBA_UNORM
    {"BC3_RGBA_SRGB",           nri::Format::BC3_RGBA_SRGB,             5,  6,  5,  8,  16, 4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC3_RGBA_SRGB
    {"BC4_R_UNORM",             nri::Format::BC4_R_UNORM,               8,  0,  0,  0,  8,  4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC4_R_UNORM
    {"BC4_R_SNORM",             nri::Format::BC4_R_SNORM,               8,  0,  0,  0,  8,  4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC4_R_SNORM
    {"BC5_RG_UNORM",            nri::Format::BC5_RG_UNORM,              8,  8,  0,  0,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC5_RG_UNORM
    {"BC5_RG_SNORM",            nri::Format::BC5_RG_SNORM,              8,  8,  0,  0,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC5_RG_SNORM
    {"BC6H_RGB_UFLOAT",         nri::Format::BC6H_RGB_UFLOAT,           16, 16, 16, 0,  16, 4, 4, _, X, _, _, X, _, _, _, _, _, _}, // BC6H_RGB_UFLOAT
    {"BC6H_RGB_SFLOAT",         nri::Format::BC6H_RGB_SFLOAT,           16, 16, 16, 0,  16, 4, 4, _, X, _, _, X, _, _, _, X, _, _}, // BC6H_RGB_SFLOAT
    {"BC7_RGBA_UNORM",          nri::Format::BC7_RGBA_UNORM,            8,  8,  8,  8,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC7_RGBA_UNORM
    {"BC7_RGBA_SRGB",           nri::Format::BC7_RGBA_SRGB,             8,  8,  8,  8,  16, 4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC7_RGBA_SRGB
    //                                                                  r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"D16_UNORM",               nri::Format::D16_UNORM,                 16, 0,  0,  0,  2,  1, 1, _, _, X, _, _, _, _, X, _, _, _}, // D16_UNORM
    {"D24_UNORM_S8_UINT",       nri::Format::D24_UNORM_S8_UINT,         24, 8,  0,  0,  4,  1, 1, _, _, X, _, _, _, X, X, _, _, X}, // D24_UNORM_S8_UINT
    {"D32_SFLOAT",              nri::Format::D32_SFLOAT,                32, 0,  0,  0,  4,  1, 1, _, _, X, _, X, _, _, _, X, _, _}, // D32_SFLOAT
    {"D32_SFLOAT_S8_UINT_X24",  nri::Format::D32_SFLOAT_S8_UINT_X24,    32, 8,  0,  0,  8,  1, 1, _, _, X, _, X, _, X, _, X, _, X}, // D32_SFLOAT_S8_UINT_X24
    {"R24_UNORM_X8",            nri::Format::R24_UNORM_X8,              24, 8,  0,  0,  4,  1, 1, _, _, X, _, _, _, _, X, _, _, _}, // R24_UNORM_X8
    {"X24_G8_UINT",             nri::Format::X24_G8_UINT,               24, 8,  0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, X}, // X24_G8_UINT
    {"R32_SFLOAT_X8_X24",       nri::Format::R32_SFLOAT_X8_X24,         32, 8,  0,  0,  8,  1, 1, _, _, X, _, X, _, _, _, X, _, _}, // R32_SFLOAT_X8_X24
    {"X32_G8_UINT_X24",         nri::Format::X32_G8_UINT_X24,           32, 8,  0,  0,  8,  1, 1, _, _, _, _, _, _, X, _, _, _, X}, // X32_G8_UINT_X24
}};

#undef _
#undef X

// clang-format on

const nri::FormatProps& GetFormatProps(nri::Format format) {
    return FORMAT_PROPS[(size_t)format];
}

constexpr std::array<nri::Format, 116> NRI_FORMAT_TABLE = {
    nri::Format::UNKNOWN,                // DXGI_FORMAT_UNKNOWN = 0
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R32G32B32A32_TYPELESS = 1
    nri::Format::RGBA32_SFLOAT,          // DXGI_FORMAT_R32G32B32A32_FLOAT = 2
    nri::Format::RGBA32_UINT,            // DXGI_FORMAT_R32G32B32A32_UINT = 3
    nri::Format::RGBA32_SINT,            // DXGI_FORMAT_R32G32B32A32_SINT = 4
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R32G32B32_TYPELESS = 5
    nri::Format::RGB32_SFLOAT,           // DXGI_FORMAT_R32G32B32_FLOAT = 6
    nri::Format::RGB32_UINT,             // DXGI_FORMAT_R32G32B32_UINT = 7
    nri::Format::RGB32_SINT,             // DXGI_FORMAT_R32G32B32_SINT = 8
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R16G16B16A16_TYPELESS = 9
    nri::Format::RGBA16_SFLOAT,          // DXGI_FORMAT_R16G16B16A16_FLOAT = 10
    nri::Format::RGBA16_UNORM,           // DXGI_FORMAT_R16G16B16A16_UNORM = 11
    nri::Format::RGBA16_UINT,            // DXGI_FORMAT_R16G16B16A16_UINT = 12
    nri::Format::RGBA16_SNORM,           // DXGI_FORMAT_R16G16B16A16_SNORM = 13
    nri::Format::RGBA16_SINT,            // DXGI_FORMAT_R16G16B16A16_SINT = 14
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R32G32_TYPELESS = 15
    nri::Format::RG32_SFLOAT,            // DXGI_FORMAT_R32G32_FLOAT = 16
    nri::Format::RG32_UINT,              // DXGI_FORMAT_R32G32_UINT = 17
    nri::Format::RGB32_SINT,             // DXGI_FORMAT_R32G32_SINT = 18
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R32G8X24_TYPELESS = 19
    nri::Format::D32_SFLOAT_S8_UINT_X24, // DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20
    nri::Format::R32_SFLOAT_X8_X24,      // DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21
    nri::Format::X32_G8_UINT_X24,        // DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R10G10B10A2_TYPELESS = 23
    nri::Format::R10_G10_B10_A2_UNORM,   // DXGI_FORMAT_R10G10B10A2_UNORM = 24
    nri::Format::R10_G10_B10_A2_UINT,    // DXGI_FORMAT_R10G10B10A2_UINT = 25
    nri::Format::R11_G11_B10_UFLOAT,     // DXGI_FORMAT_R11G11B10_FLOAT = 26
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R8G8B8A8_TYPELESS = 27
    nri::Format::RGBA8_UNORM,            // DXGI_FORMAT_R8G8B8A8_UNORM = 28
    nri::Format::RGBA8_SRGB,             // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29
    nri::Format::RGBA8_UINT,             // DXGI_FORMAT_R8G8B8A8_UINT = 30
    nri::Format::RGBA8_SNORM,            // DXGI_FORMAT_R8G8B8A8_SNORM = 31
    nri::Format::RGBA8_SINT,             // DXGI_FORMAT_R8G8B8A8_SINT = 32
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R16G16_TYPELESS = 33
    nri::Format::RG16_SFLOAT,            // DXGI_FORMAT_R16G16_FLOAT = 34
    nri::Format::RG16_UNORM,             // DXGI_FORMAT_R16G16_UNORM = 35
    nri::Format::RG16_UINT,              // DXGI_FORMAT_R16G16_UINT = 36
    nri::Format::RG16_SNORM,             // DXGI_FORMAT_R16G16_SNORM = 37
    nri::Format::RG16_SINT,              // DXGI_FORMAT_R16G16_SINT = 38
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R32_TYPELESS = 39
    nri::Format::D32_SFLOAT,             // DXGI_FORMAT_D32_FLOAT = 40
    nri::Format::R32_SFLOAT,             // DXGI_FORMAT_R32_FLOAT = 41
    nri::Format::R32_UINT,               // DXGI_FORMAT_R32_UINT = 42
    nri::Format::R32_SINT,               // DXGI_FORMAT_R32_SINT = 43
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R24G8_TYPELESS = 44
    nri::Format::D24_UNORM_S8_UINT,      // DXGI_FORMAT_D24_UNORM_S8_UINT = 45
    nri::Format::R24_UNORM_X8,           // DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46
    nri::Format::X24_G8_UINT,            // DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R8G8_TYPELESS = 48
    nri::Format::RG8_UNORM,              // DXGI_FORMAT_R8G8_UNORM = 49
    nri::Format::RG8_UINT,               // DXGI_FORMAT_R8G8_UINT = 50
    nri::Format::RG8_SNORM,              // DXGI_FORMAT_R8G8_SNORM = 51
    nri::Format::RG8_SINT,               // DXGI_FORMAT_R8G8_SINT = 52
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R16_TYPELESS = 53
    nri::Format::R16_SFLOAT,             // DXGI_FORMAT_R16_FLOAT = 54
    nri::Format::D16_UNORM,              // DXGI_FORMAT_D16_UNORM = 55
    nri::Format::R16_UNORM,              // DXGI_FORMAT_R16_UNORM = 56
    nri::Format::R16_UINT,               // DXGI_FORMAT_R16_UINT = 57
    nri::Format::R16_SNORM,              // DXGI_FORMAT_R16_SNORM = 58
    nri::Format::R16_SINT,               // DXGI_FORMAT_R16_SINT = 59
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R8_TYPELESS = 60
    nri::Format::R8_UNORM,               // DXGI_FORMAT_R8_UNORM = 61
    nri::Format::R8_UINT,                // DXGI_FORMAT_R8_UINT = 62
    nri::Format::R8_SNORM,               // DXGI_FORMAT_R8_SNORM = 63
    nri::Format::R8_SINT,                // DXGI_FORMAT_R8_SINT = 64
    nri::Format::UNKNOWN,                // DXGI_FORMAT_A8_UNORM = 65
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R1_UNORM = 66
    nri::Format::R9_G9_B9_E5_UFLOAT,     // DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R8G8_B8G8_UNORM = 68
    nri::Format::UNKNOWN,                // DXGI_FORMAT_G8R8_G8B8_UNORM = 69
    nri::Format::UNKNOWN,                // DXGI_FORMAT_BC1_TYPELESS = 70
    nri::Format::BC1_RGBA_UNORM,         // DXGI_FORMAT_BC1_UNORM = 71
    nri::Format::BC1_RGBA_SRGB,          // DXGI_FORMAT_BC1_UNORM_SRGB = 72
    nri::Format::UNKNOWN,                // DXGI_FORMAT_BC2_TYPELESS = 73
    nri::Format::BC2_RGBA_UNORM,         // DXGI_FORMAT_BC2_UNORM = 74
    nri::Format::BC2_RGBA_SRGB,          // DXGI_FORMAT_BC2_UNORM_SRGB = 75
    nri::Format::UNKNOWN,                // DXGI_FORMAT_BC3_TYPELESS = 76
    nri::Format::BC3_RGBA_UNORM,         // DXGI_FORMAT_BC3_UNORM = 77
    nri::Format::BC3_RGBA_SRGB,          // DXGI_FORMAT_BC3_UNORM_SRGB = 78
    nri::Format::UNKNOWN,                // DXGI_FORMAT_BC4_TYPELESS = 79
    nri::Format::BC4_R_UNORM,            // DXGI_FORMAT_BC4_UNORM = 80
    nri::Format::BC4_R_SNORM,            // DXGI_FORMAT_BC4_SNORM = 81
    nri::Format::UNKNOWN,                // DXGI_FORMAT_BC5_TYPELESS = 82
    nri::Format::BC5_RG_UNORM,           // DXGI_FORMAT_BC5_UNORM = 83
    nri::Format::BC5_RG_SNORM,           // DXGI_FORMAT_BC5_SNORM = 84
    nri::Format::B5_G6_R5_UNORM,         // DXGI_FORMAT_B5G6R5_UNORM = 85
    nri::Format::B5_G5_R5_A1_UNORM,      // DXGI_FORMAT_B5G5R5A1_UNORM = 86
    nri::Format::BGRA8_UNORM,            // DXGI_FORMAT_B8G8R8A8_UNORM = 87
    nri::Format::UNKNOWN,                // DXGI_FORMAT_B8G8R8X8_UNORM = 88
    nri::Format::UNKNOWN,                // DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89
    nri::Format::UNKNOWN,                // DXGI_FORMAT_B8G8R8A8_TYPELESS = 90
    nri::Format::BGRA8_SRGB,             // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91
    nri::Format::UNKNOWN,                // DXGI_FORMAT_B8G8R8X8_TYPELESS = 92
    nri::Format::UNKNOWN,                // DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93
    nri::Format::UNKNOWN,                // DXGI_FORMAT_BC6H_TYPELESS = 94
    nri::Format::BC6H_RGB_UFLOAT,        // DXGI_FORMAT_BC6H_UF16 = 95
    nri::Format::BC6H_RGB_SFLOAT,        // DXGI_FORMAT_BC6H_SF16 = 96
    nri::Format::UNKNOWN,                // DXGI_FORMAT_BC7_TYPELESS = 97
    nri::Format::BC7_RGBA_UNORM,         // DXGI_FORMAT_BC7_UNORM = 98
    nri::Format::BC7_RGBA_SRGB,          // DXGI_FORMAT_BC7_UNORM_SRGB = 99
    nri::Format::UNKNOWN,                // DXGI_FORMAT_AYUV = 100
    nri::Format::UNKNOWN,                // DXGI_FORMAT_Y410 = 101
    nri::Format::UNKNOWN,                // DXGI_FORMAT_Y416 = 102
    nri::Format::UNKNOWN,                // DXGI_FORMAT_NV12 = 103
    nri::Format::UNKNOWN,                // DXGI_FORMAT_P010 = 104
    nri::Format::UNKNOWN,                // DXGI_FORMAT_P016 = 105
    nri::Format::UNKNOWN,                // DXGI_FORMAT_420_OPAQUE = 106
    nri::Format::UNKNOWN,                // DXGI_FORMAT_YUY2 = 107
    nri::Format::UNKNOWN,                // DXGI_FORMAT_Y210 = 108
    nri::Format::UNKNOWN,                // DXGI_FORMAT_Y216 = 109
    nri::Format::UNKNOWN,                // DXGI_FORMAT_NV11 = 110
    nri::Format::UNKNOWN,                // DXGI_FORMAT_AI44 = 111
    nri::Format::UNKNOWN,                // DXGI_FORMAT_IA44 = 112
    nri::Format::UNKNOWN,                // DXGI_FORMAT_P8 = 113
    nri::Format::UNKNOWN,                // DXGI_FORMAT_A8P8 = 114
    nri::Format::B4_G4_R4_A4_UNORM,      // DXGI_FORMAT_B4G4R4A4_UNORM = 115
};

nri::Format DXGIFormatToNRIFormat(uint32_t dxgiFormat) {
    if (dxgiFormat < NRI_FORMAT_TABLE.size())
        return NRI_FORMAT_TABLE[dxgiFormat];

    return nri::Format::UNKNOWN;
}

constexpr std::array<nri::Format, 131> VK_FORMAT_TABLE = {
    nri::Format::UNKNOWN,                // VK_FORMAT_UNDEFINED = 0
    nri::Format::UNKNOWN,                // VK_FORMAT_R4G4_UNORM_PACK8 = 1
    nri::Format::UNKNOWN,                // VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2
    nri::Format::UNKNOWN,                // VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3
    nri::Format::B5_G6_R5_UNORM,         // VK_FORMAT_R5G6B5_UNORM_PACK16 = 4
    nri::Format::UNKNOWN,                // VK_FORMAT_B5G6R5_UNORM_PACK16 = 5
    nri::Format::UNKNOWN,                // VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6
    nri::Format::UNKNOWN,                // VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7
    nri::Format::B5_G5_R5_A1_UNORM,      // VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8
    nri::Format::R8_UNORM,               // VK_FORMAT_R8_UNORM = 9
    nri::Format::R8_SNORM,               // VK_FORMAT_R8_SNORM = 10
    nri::Format::UNKNOWN,                // VK_FORMAT_R8_USCALED = 11
    nri::Format::UNKNOWN,                // VK_FORMAT_R8_SSCALED = 12
    nri::Format::R8_UINT,                // VK_FORMAT_R8_UINT = 13
    nri::Format::R8_SINT,                // VK_FORMAT_R8_SINT = 14
    nri::Format::UNKNOWN,                // VK_FORMAT_R8_SRGB = 15
    nri::Format::RG8_UNORM,              // VK_FORMAT_R8G8_UNORM = 16
    nri::Format::RG8_SNORM,              // VK_FORMAT_R8G8_SNORM = 17
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8_USCALED = 18
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8_SSCALED = 19
    nri::Format::RG8_UINT,               // VK_FORMAT_R8G8_UINT = 20
    nri::Format::RG8_SINT,               // VK_FORMAT_R8G8_SINT = 21
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8_SRGB = 22
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8_UNORM = 23
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SNORM = 24
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8_USCALED = 25
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SSCALED = 26
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8_UINT = 27
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SINT = 28
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SRGB = 29
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8_UNORM = 30
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SNORM = 31
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8_USCALED = 32
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SSCALED = 33
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8_UINT = 34
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SINT = 35
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SRGB = 36
    nri::Format::RGBA8_UNORM,            // VK_FORMAT_R8G8B8A8_UNORM = 37
    nri::Format::RGBA8_SNORM,            // VK_FORMAT_R8G8B8A8_SNORM = 38
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8A8_USCALED = 39
    nri::Format::UNKNOWN,                // VK_FORMAT_R8G8B8A8_SSCALED = 40
    nri::Format::RGBA8_UINT,             // VK_FORMAT_R8G8B8A8_UINT = 41
    nri::Format::RGBA8_SINT,             // VK_FORMAT_R8G8B8A8_SINT = 42
    nri::Format::RGBA8_SRGB,             // VK_FORMAT_R8G8B8A8_SRGB = 43
    nri::Format::BGRA8_UNORM,            // VK_FORMAT_B8G8R8A8_UNORM = 44
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_SNORM = 45
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_USCALED = 46
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_SSCALED = 47
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_UINT = 48
    nri::Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_SINT = 49
    nri::Format::BGRA8_SRGB,             // VK_FORMAT_B8G8R8A8_SRGB = 50
    nri::Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51
    nri::Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52
    nri::Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53
    nri::Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54
    nri::Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55
    nri::Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56
    nri::Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57
    nri::Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58
    nri::Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59
    nri::Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60
    nri::Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61
    nri::Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62
    nri::Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63
    nri::Format::R10_G10_B10_A2_UNORM,   // VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64
    nri::Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65
    nri::Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66
    nri::Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67
    nri::Format::R10_G10_B10_A2_UINT,    // VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68
    nri::Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69
    nri::Format::R16_UNORM,              // VK_FORMAT_R16_UNORM = 70
    nri::Format::R16_SNORM,              // VK_FORMAT_R16_SNORM = 71
    nri::Format::UNKNOWN,                // VK_FORMAT_R16_USCALED = 72
    nri::Format::UNKNOWN,                // VK_FORMAT_R16_SSCALED = 73
    nri::Format::R16_UINT,               // VK_FORMAT_R16_UINT = 74
    nri::Format::R16_SINT,               // VK_FORMAT_R16_SINT = 75
    nri::Format::R16_SFLOAT,             // VK_FORMAT_R16_SFLOAT = 76
    nri::Format::RG16_UNORM,             // VK_FORMAT_R16G16_UNORM = 77
    nri::Format::RG16_SNORM,             // VK_FORMAT_R16G16_SNORM = 78
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16_USCALED = 79
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16_SSCALED = 80
    nri::Format::RG16_UINT,              // VK_FORMAT_R16G16_UINT = 81
    nri::Format::RG16_SINT,              // VK_FORMAT_R16G16_SINT = 82
    nri::Format::RG16_SFLOAT,            // VK_FORMAT_R16G16_SFLOAT = 83
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16_UNORM = 84
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SNORM = 85
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16_USCALED = 86
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SSCALED = 87
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16_UINT = 88
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SINT = 89
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SFLOAT = 90
    nri::Format::RGBA16_UNORM,           // VK_FORMAT_R16G16B16A16_UNORM = 91
    nri::Format::RGBA16_SNORM,           // VK_FORMAT_R16G16B16A16_SNORM = 92
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16A16_USCALED = 93
    nri::Format::UNKNOWN,                // VK_FORMAT_R16G16B16A16_SSCALED = 94
    nri::Format::RGBA16_UINT,            // VK_FORMAT_R16G16B16A16_UINT = 95
    nri::Format::RGBA16_SINT,            // VK_FORMAT_R16G16B16A16_SINT = 96
    nri::Format::RGBA16_SFLOAT,          // VK_FORMAT_R16G16B16A16_SFLOAT = 97
    nri::Format::R32_UINT,               // VK_FORMAT_R32_UINT = 98
    nri::Format::R32_SINT,               // VK_FORMAT_R32_SINT = 99
    nri::Format::R32_SFLOAT,             // VK_FORMAT_R32_SFLOAT = 100
    nri::Format::RG32_UINT,              // VK_FORMAT_R32G32_UINT = 101
    nri::Format::RG32_SINT,              // VK_FORMAT_R32G32_SINT = 102
    nri::Format::RG32_SFLOAT,            // VK_FORMAT_R32G32_SFLOAT = 103
    nri::Format::RGB32_UINT,             // VK_FORMAT_R32G32B32_UINT = 104
    nri::Format::RGB32_SINT,             // VK_FORMAT_R32G32B32_SINT = 105
    nri::Format::RGB32_SFLOAT,           // VK_FORMAT_R32G32B32_SFLOAT = 106
    nri::Format::RGBA32_UINT,            // VK_FORMAT_R32G32B32A32_UINT = 107
    nri::Format::RGBA32_SINT,            // VK_FORMAT_R32G32B32A32_SINT = 108
    nri::Format::RGBA32_SFLOAT,          // VK_FORMAT_R32G32B32A32_SFLOAT = 109
    nri::Format::UNKNOWN,                // VK_FORMAT_R64_UINT = 110
    nri::Format::UNKNOWN,                // VK_FORMAT_R64_SINT = 111
    nri::Format::UNKNOWN,                // VK_FORMAT_R64_SFLOAT = 112
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64_UINT = 113
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64_SINT = 114
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64_SFLOAT = 115
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64B64_UINT = 116
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64B64_SINT = 117
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64B64_SFLOAT = 118
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64B64A64_UINT = 119
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64B64A64_SINT = 120
    nri::Format::UNKNOWN,                // VK_FORMAT_R64G64B64A64_SFLOAT = 121
    nri::Format::R11_G11_B10_UFLOAT,     // VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122
    nri::Format::R9_G9_B9_E5_UFLOAT,     // VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123
    nri::Format::D16_UNORM,              // VK_FORMAT_D16_UNORM = 124
    nri::Format::D24_UNORM_S8_UINT,      // VK_FORMAT_X8_D24_UNORM_PACK32 = 125
    nri::Format::D32_SFLOAT,             // VK_FORMAT_D32_SFLOAT = 126
    nri::Format::UNKNOWN,                // VK_FORMAT_S8_UINT = 127
    nri::Format::UNKNOWN,                // VK_FORMAT_D16_UNORM_S8_UINT = 128
    nri::Format::D24_UNORM_S8_UINT,      // VK_FORMAT_D24_UNORM_S8_UINT = 129
    nri::Format::D32_SFLOAT_S8_UINT_X24, // VK_FORMAT_D32_SFLOAT_S8_UINT = 130
};

nri::Format VKFormatToNRIFormat(uint32_t format) {
    if (format < VK_FORMAT_TABLE.size())
        return VK_FORMAT_TABLE[format];
    else if (format == 1000340000) // VK_FORMAT_A4R4G4B4_UNORM_PACK16
        return nri::Format::B4_G4_R4_A4_UNORM;

    return nri::Format::UNKNOWN;
}

constexpr std::array<const char*, (size_t)nri::Message::MAX_NUM> MESSAGE_TYPE_NAME = {
    "INFO",    // INFO,
    "WARNING", // WARNING,
    "ERROR",   // ERROR
};

static void MessageCallback(nri::Message messageType, const char* file, uint32_t line, const char* message, void* userArg) {
    MaybeUnused(userArg);

    const char* messageTypeName = MESSAGE_TYPE_NAME[(size_t)messageType];

    char buf[MAX_MESSAGE_LENGTH];
    snprintf(buf, sizeof(buf), "nri::%s (%s:%u) - %s\n", messageTypeName, file, line, message);

    fprintf(stderr, "%s", buf);
#ifdef _WIN32
    OutputDebugStringA(buf);
#endif
}

static void AbortExecution(void* userArg) {
    MaybeUnused(userArg);

#ifdef _WIN32
    DebugBreak();
#else
    raise(SIGTRAP);
#endif
}

void CheckAndSetDefaultCallbacks(nri::CallbackInterface& callbackInterface) {
    if (!callbackInterface.MessageCallback)
        callbackInterface.MessageCallback = MessageCallback;

    if (!callbackInterface.AbortExecution)
        callbackInterface.AbortExecution = AbortExecution;
}

void nri::DeviceBase::ReportMessage(nri::Message messageType, const char* file, uint32_t line, const char* format, ...) const {
    const nri::DeviceDesc& desc = GetDesc();
    const char* graphicsAPIName = nriGetGraphicsAPIString(desc.graphicsAPI);

    const char* temp = strrchr(file, FILE_SEPARATOR);
    file = temp ? temp + 1 : file;

    char buf[MAX_MESSAGE_LENGTH];
    int32_t written = snprintf(buf, sizeof(buf), "%s::%s - ", graphicsAPIName, *desc.adapterDesc.name == '\0' ? "Unknown" : desc.adapterDesc.name);

    va_list argptr;
    va_start(argptr, format);
    written += vsnprintf(buf + written, sizeof(buf) - written, format, argptr);
    va_end(argptr);

    if (m_CallbackInterface.MessageCallback)
        m_CallbackInterface.MessageCallback(messageType, file, line, buf, m_CallbackInterface.userArg);

    if (messageType == nri::Message::ERROR && m_CallbackInterface.AbortExecution != nullptr)
        m_CallbackInterface.AbortExecution(m_CallbackInterface.userArg);
}

void ConvertCharToWchar(const char* in, wchar_t* out, size_t outLength) {
    if (outLength == 0)
        return;

    for (size_t i = 0; i < outLength - 1 && *in; i++)
        *out++ = *in++;

    *out = 0;
}

void ConvertWcharToChar(const wchar_t* in, char* out, size_t outLength) {
    if (outLength == 0)
        return;

    for (size_t i = 0; i < outLength - 1 && *in; i++)
        *out++ = char(*in++);

    *out = 0;
}

uint64_t GetSwapChainId() {
    static uint64_t id = 0;
    return id++ << PRESENT_INDEX_BIT_NUM;
}
