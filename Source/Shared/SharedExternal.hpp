// © 2021 NVIDIA Corporation

#include <cstdarg> // va_start, va_end

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)

constexpr std::array<DxgiFormat, (size_t)Format::MAX_NUM> g_dxgiFormats = {{
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
VALIDATE_ARRAY_BY_FILED(g_dxgiFormats, typeless);

const DxgiFormat& nri::GetDxgiFormat(Format format) {
    return g_dxgiFormats[(size_t)format];
}

Result nri::GetResultFromHRESULT(long result) {
    if (SUCCEEDED(result))
        return Result::SUCCESS;

    switch (result) {
        case E_INVALIDARG:
        case E_POINTER:
        case E_HANDLE:
            return Result::INVALID_ARGUMENT;

        case DXGI_ERROR_UNSUPPORTED:
            return Result::UNSUPPORTED;

        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        case DXGI_ERROR_DEVICE_HUNG:
            return Result::DEVICE_LOST;

        case E_NOINTERFACE:
        case D3D12_ERROR_INVALID_REDIST:
        case DXGI_ERROR_SDK_COMPONENT_MISSING:
            return Result::INVALID_SDK;

        case E_OUTOFMEMORY:
        case DXGI_ERROR_REMOTE_OUTOFMEMORY:
        case DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY:
            return Result::OUT_OF_MEMORY;

        default:
            return Result::FAILURE;
    }
}

uint32_t nri::NRIFormatToDXGIFormat(Format format) {
    return g_dxgiFormats[(size_t)format].typed;
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

Result DisplayDescHelper::GetDisplayDesc(void* hwnd, DisplayDesc& displayDesc) {
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
        return Result::SUCCESS;
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
        return Result::FAILURE;

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

    return Result::SUCCESS;
}

bool nri::HasOutput() {
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

Result nri::QueryVideoMemoryInfoDXGI(uint64_t luid, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    videoMemoryInfo = {};

    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return Result::FAILURE;

    ComPtr<IDXGIAdapter3> adapter;
    if (FAILED(dxgifactory->EnumAdapterByLuid(*(LUID*)&luid, IID_PPV_ARGS(&adapter))))
        return Result::FAILURE;

    bool isLocal = memoryLocation == MemoryLocation::DEVICE || memoryLocation == MemoryLocation::DEVICE_UPLOAD;

    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (FAILED(adapter->QueryVideoMemoryInfo(0, isLocal ? DXGI_MEMORY_SEGMENT_GROUP_LOCAL : DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info)))
        return Result::FAILURE;

    videoMemoryInfo.budgetSize = info.Budget;
    videoMemoryInfo.usageSize = info.CurrentUsage;

    return Result::SUCCESS;
}

#else

uint32_t NRIFormatToDXGIFormat(Format) {
    return 0;
}

#endif

// clang-format off

#define _ 0
#define X 1

constexpr std::array<FormatProps, (size_t)Format::MAX_NUM> g_formatProps = {{
    //                                                                                                             isStencil
    //                                                                                                             isSrgb  |
    //                                                                                                        isSigned  |  |
    //                                                                                                       isNorm  |  |  |
    //                                                                                                 isInteger  |  |  |  |
    //                                                                                               isPacked  |  |  |  |  |
    //                                                                                             isFloat  |  |  |  |  |  |
    //                                                                                      isExpShared  |  |  |  |  |  |  |
    //                                                                                       isDepth  |  |  |  |  |  |  |  |
    //                                                                               isCompressed  |  |  |  |  |  |  |  |  |
    //                                                                                   isBgr  |  |  |  |  |  |  |  |  |  |
    //                                                                          blockHeight  |  |  |  |  |  |  |  |  |  |  |
    //                                                                        blockWidth  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                                        stride   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                                    A bits   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                                B bits   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                            G bits   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                                                        R bits   |   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    //                          self                               |   |   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    // format name              |                                  |   |   |   |   |   |  |  |  |  |  |  |  |  |  |  |  |  |
    {"UNKNOWN",                 Format::UNKNOWN,                   0,  0,  0,  0,  1,  0, 0, _, _, _, _, _, _, _, _, _, _, _}, // UNKNOWN
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"R8_UNORM",                Format::R8_UNORM,                  8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // R8_UNORM
    {"R8_SNORM",                Format::R8_SNORM,                  8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, _, X, X, _, _}, // R8_SNORM
    {"R8_UINT",                 Format::R8_UINT,                   8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // R8_UINT
    {"R8_SINT",                 Format::R8_SINT,                   8,  0,  0,  0,  1,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // R8_SINT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RG8_UNORM",               Format::RG8_UNORM,                 8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RG8_UNORM
    {"RG8_SNORM",               Format::RG8_SNORM,                 8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, X, _, _}, // RG8_SNORM
    {"RG8_UINT",                Format::RG8_UINT,                  8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RG8_UINT
    {"RG8_SINT",                Format::RG8_SINT,                  8,  8,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RG8_SINT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"BGRA8_UNORM",             Format::BGRA8_UNORM,               8,  8,  8,  8,  4,  1, 1, X, _, _, _, _, _, _, X, _, _, _}, // BGRA8_UNORM
    {"BGRA8_SRGB",              Format::BGRA8_SRGB,                8,  8,  8,  8,  4,  1, 1, X, _, _, _, _, _, _, _, _, X, _}, // BGRA8_SRGB
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGBA8_UNORM",             Format::RGBA8_UNORM,               8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RGBA8_UNORM
    {"RGBA8_SRGB",              Format::RGBA8_SRGB,                8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, _, _, _, X, _}, // RGBA8_SRGB
    {"RGBA8_SNORM",             Format::RGBA8_SNORM,               8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, _, X, X, _, _}, // RGBA8_SNORM
    {"RGBA8_UINT",              Format::RGBA8_UINT,                8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGBA8_UINT
    {"RGBA8_SINT",              Format::RGBA8_SINT,                8,  8,  8,  8,  4,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGBA8_SINT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"R16_UNORM",               Format::R16_UNORM,                 16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // R16_UNORM
    {"R16_SNORM",               Format::R16_SNORM,                 16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, _, X, X, _, _}, // R16_SNORM
    {"R16_UINT",                Format::R16_UINT,                  16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // R16_UINT
    {"R16_SINT",                Format::R16_SINT,                  16, 0,  0,  0,  2,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // R16_SINT
    {"R16_SFLOAT",              Format::R16_SFLOAT,                16, 0,  0,  0,  2,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // R16_SFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RG16_UNORM",              Format::RG16_UNORM,                16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RG16_UNORM
    {"RG16_SNORM",              Format::RG16_SNORM,                16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, _, X, X, _, _}, // RG16_SNORM
    {"RG16_UINT",               Format::RG16_UINT,                 16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RG16_UINT
    {"RG16_SINT",               Format::RG16_SINT,                 16, 16, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RG16_SINT
    {"RG16_SFLOAT",             Format::RG16_SFLOAT,               16, 16, 0,  0,  4,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RG16_SFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGBA16_UNORM",            Format::RGBA16_UNORM,              16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, _, X, _, _, _}, // RGBA16_UNORM
    {"RGBA16_SNORM",            Format::RGBA16_SNORM,              16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, _, X, X, _, _}, // RGBA16_SNORM
    {"RGBA16_UINT",             Format::RGBA16_UINT,               16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGBA16_UINT
    {"RGBA16_SINT",             Format::RGBA16_SINT,               16, 16, 16, 16, 8,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGBA16_SINT
    {"RGBA16_SFLOAT",           Format::RGBA16_SFLOAT,             16, 16, 16, 16, 8,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RGBA16_SFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"R32_UINT",                Format::R32_UINT,                  32, 32, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // R32_UINT
    {"R32_SINT",                Format::R32_SINT,                  32, 32, 0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // R32_SINT
    {"R32_SFLOAT",              Format::R32_SFLOAT,                32, 32, 0,  0,  4,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // R32_SFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RG32_UINT",               Format::RG32_UINT,                 32, 32, 0,  0,  8,  1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RG32_UINT
    {"RG32_SINT",               Format::RG32_SINT,                 32, 32, 0,  0,  8,  1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RG32_SINT
    {"RG32_SFLOAT",             Format::RG32_SFLOAT,               32, 32, 0,  0,  8,  1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RG32_SFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGB32_UINT",              Format::RGB32_UINT,                32, 32, 32, 0,  12, 1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGB32_UINT
    {"RGB32_SINT",              Format::RGB32_SINT,                32, 32, 32, 0,  12, 1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGB32_SINT
    {"RGB32_SFLOAT",            Format::RGB32_SFLOAT,              32, 32, 32, 0,  12, 1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RGB32_SFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"RGBA32_UINT",             Format::RGBA32_UINT,               32, 32, 32, 32, 16, 1, 1, _, _, _, _, _, _, X, _, _, _, _}, // RGBA32_UINT
    {"RGBA32_SINT",             Format::RGBA32_SINT,               32, 32, 32, 32, 16, 1, 1, _, _, _, _, _, _, X, _, X, _, _}, // RGBA32_SINT
    {"RGBA32_SFLOAT",           Format::RGBA32_SFLOAT,             32, 32, 32, 32, 16, 1, 1, _, _, _, _, X, _, _, _, X, _, _}, // RGBA32_SFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"B5_G6_R5_UNORM",          Format::B5_G6_R5_UNORM,            5,  6,  5,  0,  2,  1, 1, X, _, _, _, _, X, _, X, _, _, _}, // B5_G6_R5_UNORM
    {"B5_G5_R5_A1_UNORM",       Format::B5_G5_R5_A1_UNORM,         5,  5,  5,  1,  2,  1, 1, X, _, _, _, _, X, _, X, _, _, _}, // B5_G5_R5_A1_UNORM
    {"B4_G4_R4_A4_UNORM",       Format::B4_G4_R4_A4_UNORM,         4,  4,  4,  4,  2,  1, 1, X, _, _, _, _, X, _, X, _, _, _}, // B4_G4_R4_A4_UNORM
    {"R10_G10_B10_A2_UNORM",    Format::R10_G10_B10_A2_UNORM,      10, 10, 10, 2,  4,  1, 1, _, _, _, _, _, X, _, X, _, _, _}, // R10_G10_B10_A2_UNORM
    {"R10_G10_B10_A2_UINT",     Format::R10_G10_B10_A2_UINT,       10, 10, 10, 2,  4,  1, 1, _, _, _, _, _, X, X, _, _, _, _}, // R10_G10_B10_A2_UINT
    {"R11_G11_B10_UFLOAT",      Format::R11_G11_B10_UFLOAT,        11, 11, 10, 0,  4,  1, 1, _, _, _, _, X, X, _, _, _, _, _}, // R11_G11_B10_UFLOAT
    {"R9_G9_B9_E5_UFLOAT",      Format::R9_G9_B9_E5_UFLOAT,        9,  9,  9,  5,  4,  1, 1, _, _, _, X, X, X, _, _, _, _, _}, // R9_G9_B9_E5_UFLOAT
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"BC1_RGBA_UNORM",          Format::BC1_RGBA_UNORM,            5,  6,  5,  1,  8,  4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC1_RGBA_UNORM
    {"BC1_RGBA_SRGB",           Format::BC1_RGBA_SRGB,             5,  6,  5,  1,  8,  4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC1_RGBA_SRGB
    {"BC2_RGBA_UNORM",          Format::BC2_RGBA_UNORM,            5,  6,  5,  4,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC2_RGBA_UNORM
    {"BC2_RGBA_SRGB",           Format::BC2_RGBA_SRGB,             5,  6,  5,  4,  16, 4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC2_RGBA_SRGB
    {"BC3_RGBA_UNORM",          Format::BC3_RGBA_UNORM,            5,  6,  5,  8,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC3_RGBA_UNORM
    {"BC3_RGBA_SRGB",           Format::BC3_RGBA_SRGB,             5,  6,  5,  8,  16, 4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC3_RGBA_SRGB
    {"BC4_R_UNORM",             Format::BC4_R_UNORM,               8,  0,  0,  0,  8,  4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC4_R_UNORM
    {"BC4_R_SNORM",             Format::BC4_R_SNORM,               8,  0,  0,  0,  8,  4, 4, _, X, _, _, _, _, _, X, X, _, _}, // BC4_R_SNORM
    {"BC5_RG_UNORM",            Format::BC5_RG_UNORM,              8,  8,  0,  0,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC5_RG_UNORM
    {"BC5_RG_SNORM",            Format::BC5_RG_SNORM,              8,  8,  0,  0,  16, 4, 4, _, X, _, _, _, _, _, X, X, _, _}, // BC5_RG_SNORM
    {"BC6H_RGB_UFLOAT",         Format::BC6H_RGB_UFLOAT,           16, 16, 16, 0,  16, 4, 4, _, X, _, _, X, _, _, _, _, _, _}, // BC6H_RGB_UFLOAT
    {"BC6H_RGB_SFLOAT",         Format::BC6H_RGB_SFLOAT,           16, 16, 16, 0,  16, 4, 4, _, X, _, _, X, _, _, _, X, _, _}, // BC6H_RGB_SFLOAT
    {"BC7_RGBA_UNORM",          Format::BC7_RGBA_UNORM,            8,  8,  8,  8,  16, 4, 4, _, X, _, _, _, _, _, X, _, _, _}, // BC7_RGBA_UNORM
    {"BC7_RGBA_SRGB",           Format::BC7_RGBA_SRGB,             8,  8,  8,  8,  16, 4, 4, _, X, _, _, _, _, _, _, _, X, _}, // BC7_RGBA_SRGB
    //                                                             r   g   b   a   s   w  h  b  c  d  e  f  p  i  n  s  s  s
    {"D16_UNORM",               Format::D16_UNORM,                 16, 0,  0,  0,  2,  1, 1, _, _, X, _, _, _, _, X, _, _, _}, // D16_UNORM
    {"D24_UNORM_S8_UINT",       Format::D24_UNORM_S8_UINT,         24, 8,  0,  0,  4,  1, 1, _, _, X, _, _, _, X, X, _, _, X}, // D24_UNORM_S8_UINT
    {"D32_SFLOAT",              Format::D32_SFLOAT,                32, 0,  0,  0,  4,  1, 1, _, _, X, _, X, _, _, _, X, _, _}, // D32_SFLOAT
    {"D32_SFLOAT_S8_UINT_X24",  Format::D32_SFLOAT_S8_UINT_X24,    32, 8,  0,  0,  8,  1, 1, _, _, X, _, X, _, X, _, X, _, X}, // D32_SFLOAT_S8_UINT_X24
    {"R24_UNORM_X8",            Format::R24_UNORM_X8,              24, 8,  0,  0,  4,  1, 1, _, _, X, _, _, _, _, X, _, _, _}, // R24_UNORM_X8
    {"X24_G8_UINT",             Format::X24_G8_UINT,               24, 8,  0,  0,  4,  1, 1, _, _, _, _, _, _, X, _, _, _, X}, // X24_G8_UINT
    {"R32_SFLOAT_X8_X24",       Format::R32_SFLOAT_X8_X24,         32, 8,  0,  0,  8,  1, 1, _, _, X, _, X, _, _, _, X, _, _}, // R32_SFLOAT_X8_X24
    {"X32_G8_UINT_X24",         Format::X32_G8_UINT_X24,           32, 8,  0,  0,  8,  1, 1, _, _, _, _, _, _, X, _, _, _, X}, // X32_G8_UINT_X24
}};
VALIDATE_ARRAY_BY_FILED(g_formatProps, name);

#undef _
#undef X

// clang-format on

const FormatProps& nri::GetFormatProps(Format format) {
    return g_formatProps[(size_t)format];
}

constexpr std::array<Format, 116> NRI_FORMAT_TABLE = {
    Format::UNKNOWN,                // DXGI_FORMAT_UNKNOWN = 0
    Format::RGBA32_SFLOAT,          // DXGI_FORMAT_R32G32B32A32_TYPELESS = 1
    Format::RGBA32_SFLOAT,          // DXGI_FORMAT_R32G32B32A32_FLOAT = 2
    Format::RGBA32_UINT,            // DXGI_FORMAT_R32G32B32A32_UINT = 3
    Format::RGBA32_SINT,            // DXGI_FORMAT_R32G32B32A32_SINT = 4
    Format::RGB32_SFLOAT,           // DXGI_FORMAT_R32G32B32_TYPELESS = 5
    Format::RGB32_SFLOAT,           // DXGI_FORMAT_R32G32B32_FLOAT = 6
    Format::RGB32_UINT,             // DXGI_FORMAT_R32G32B32_UINT = 7
    Format::RGB32_SINT,             // DXGI_FORMAT_R32G32B32_SINT = 8
    Format::RGBA16_SFLOAT,          // DXGI_FORMAT_R16G16B16A16_TYPELESS = 9
    Format::RGBA16_SFLOAT,          // DXGI_FORMAT_R16G16B16A16_FLOAT = 10
    Format::RGBA16_UNORM,           // DXGI_FORMAT_R16G16B16A16_UNORM = 11
    Format::RGBA16_UINT,            // DXGI_FORMAT_R16G16B16A16_UINT = 12
    Format::RGBA16_SNORM,           // DXGI_FORMAT_R16G16B16A16_SNORM = 13
    Format::RGBA16_SINT,            // DXGI_FORMAT_R16G16B16A16_SINT = 14
    Format::RG32_SFLOAT,            // DXGI_FORMAT_R32G32_TYPELESS = 15
    Format::RG32_SFLOAT,            // DXGI_FORMAT_R32G32_FLOAT = 16
    Format::RG32_UINT,              // DXGI_FORMAT_R32G32_UINT = 17
    Format::RGB32_SINT,             // DXGI_FORMAT_R32G32_SINT = 18
    Format::D32_SFLOAT_S8_UINT_X24, // DXGI_FORMAT_R32G8X24_TYPELESS = 19
    Format::D32_SFLOAT_S8_UINT_X24, // DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20
    Format::R32_SFLOAT_X8_X24,      // DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21
    Format::X32_G8_UINT_X24,        // DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22
    Format::R10_G10_B10_A2_UNORM,   // DXGI_FORMAT_R10G10B10A2_TYPELESS = 23
    Format::R10_G10_B10_A2_UNORM,   // DXGI_FORMAT_R10G10B10A2_UNORM = 24
    Format::R10_G10_B10_A2_UINT,    // DXGI_FORMAT_R10G10B10A2_UINT = 25
    Format::R11_G11_B10_UFLOAT,     // DXGI_FORMAT_R11G11B10_FLOAT = 26
    Format::RGBA8_UNORM,            // DXGI_FORMAT_R8G8B8A8_TYPELESS = 27
    Format::RGBA8_UNORM,            // DXGI_FORMAT_R8G8B8A8_UNORM = 28
    Format::RGBA8_SRGB,             // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29
    Format::RGBA8_UINT,             // DXGI_FORMAT_R8G8B8A8_UINT = 30
    Format::RGBA8_SNORM,            // DXGI_FORMAT_R8G8B8A8_SNORM = 31
    Format::RGBA8_SINT,             // DXGI_FORMAT_R8G8B8A8_SINT = 32
    Format::RG16_SFLOAT,            // DXGI_FORMAT_R16G16_TYPELESS = 33
    Format::RG16_SFLOAT,            // DXGI_FORMAT_R16G16_FLOAT = 34
    Format::RG16_UNORM,             // DXGI_FORMAT_R16G16_UNORM = 35
    Format::RG16_UINT,              // DXGI_FORMAT_R16G16_UINT = 36
    Format::RG16_SNORM,             // DXGI_FORMAT_R16G16_SNORM = 37
    Format::RG16_SINT,              // DXGI_FORMAT_R16G16_SINT = 38
    Format::R32_SFLOAT,             // DXGI_FORMAT_R32_TYPELESS = 39
    Format::D32_SFLOAT,             // DXGI_FORMAT_D32_FLOAT = 40
    Format::R32_SFLOAT,             // DXGI_FORMAT_R32_FLOAT = 41
    Format::R32_UINT,               // DXGI_FORMAT_R32_UINT = 42
    Format::R32_SINT,               // DXGI_FORMAT_R32_SINT = 43
    Format::D24_UNORM_S8_UINT,      // DXGI_FORMAT_R24G8_TYPELESS = 44
    Format::D24_UNORM_S8_UINT,      // DXGI_FORMAT_D24_UNORM_S8_UINT = 45
    Format::R24_UNORM_X8,           // DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46
    Format::X24_G8_UINT,            // DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47
    Format::RG8_UNORM,              // DXGI_FORMAT_R8G8_TYPELESS = 48
    Format::RG8_UNORM,              // DXGI_FORMAT_R8G8_UNORM = 49
    Format::RG8_UINT,               // DXGI_FORMAT_R8G8_UINT = 50
    Format::RG8_SNORM,              // DXGI_FORMAT_R8G8_SNORM = 51
    Format::RG8_SINT,               // DXGI_FORMAT_R8G8_SINT = 52
    Format::R16_SFLOAT,             // DXGI_FORMAT_R16_TYPELESS = 53
    Format::R16_SFLOAT,             // DXGI_FORMAT_R16_FLOAT = 54
    Format::D16_UNORM,              // DXGI_FORMAT_D16_UNORM = 55
    Format::R16_UNORM,              // DXGI_FORMAT_R16_UNORM = 56
    Format::R16_UINT,               // DXGI_FORMAT_R16_UINT = 57
    Format::R16_SNORM,              // DXGI_FORMAT_R16_SNORM = 58
    Format::R16_SINT,               // DXGI_FORMAT_R16_SINT = 59
    Format::R8_UNORM,               // DXGI_FORMAT_R8_TYPELESS = 60
    Format::R8_UNORM,               // DXGI_FORMAT_R8_UNORM = 61
    Format::R8_UINT,                // DXGI_FORMAT_R8_UINT = 62
    Format::R8_SNORM,               // DXGI_FORMAT_R8_SNORM = 63
    Format::R8_SINT,                // DXGI_FORMAT_R8_SINT = 64
    Format::UNKNOWN,                // DXGI_FORMAT_A8_UNORM = 65
    Format::UNKNOWN,                // DXGI_FORMAT_R1_UNORM = 66
    Format::R9_G9_B9_E5_UFLOAT,     // DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67
    Format::UNKNOWN,                // DXGI_FORMAT_R8G8_B8G8_UNORM = 68
    Format::UNKNOWN,                // DXGI_FORMAT_G8R8_G8B8_UNORM = 69
    Format::BC1_RGBA_UNORM,         // DXGI_FORMAT_BC1_TYPELESS = 70
    Format::BC1_RGBA_UNORM,         // DXGI_FORMAT_BC1_UNORM = 71
    Format::BC1_RGBA_SRGB,          // DXGI_FORMAT_BC1_UNORM_SRGB = 72
    Format::BC2_RGBA_UNORM,         // DXGI_FORMAT_BC2_TYPELESS = 73
    Format::BC2_RGBA_UNORM,         // DXGI_FORMAT_BC2_UNORM = 74
    Format::BC2_RGBA_SRGB,          // DXGI_FORMAT_BC2_UNORM_SRGB = 75
    Format::BC3_RGBA_UNORM,         // DXGI_FORMAT_BC3_TYPELESS = 76
    Format::BC3_RGBA_UNORM,         // DXGI_FORMAT_BC3_UNORM = 77
    Format::BC3_RGBA_SRGB,          // DXGI_FORMAT_BC3_UNORM_SRGB = 78
    Format::BC4_R_UNORM,            // DXGI_FORMAT_BC4_TYPELESS = 79
    Format::BC4_R_UNORM,            // DXGI_FORMAT_BC4_UNORM = 80
    Format::BC4_R_SNORM,            // DXGI_FORMAT_BC4_SNORM = 81
    Format::BC5_RG_UNORM,           // DXGI_FORMAT_BC5_TYPELESS = 82
    Format::BC5_RG_UNORM,           // DXGI_FORMAT_BC5_UNORM = 83
    Format::BC5_RG_SNORM,           // DXGI_FORMAT_BC5_SNORM = 84
    Format::B5_G6_R5_UNORM,         // DXGI_FORMAT_B5G6R5_UNORM = 85
    Format::B5_G5_R5_A1_UNORM,      // DXGI_FORMAT_B5G5R5A1_UNORM = 86
    Format::BGRA8_UNORM,            // DXGI_FORMAT_B8G8R8A8_UNORM = 87
    Format::UNKNOWN,                // DXGI_FORMAT_B8G8R8X8_UNORM = 88
    Format::UNKNOWN,                // DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89
    Format::BGRA8_UNORM,            // DXGI_FORMAT_B8G8R8A8_TYPELESS = 90
    Format::BGRA8_SRGB,             // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91
    Format::BGRA8_UNORM,            // DXGI_FORMAT_B8G8R8X8_TYPELESS = 92
    Format::BGRA8_SRGB,             // DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93
    Format::BC6H_RGB_UFLOAT,        // DXGI_FORMAT_BC6H_TYPELESS = 94
    Format::BC6H_RGB_UFLOAT,        // DXGI_FORMAT_BC6H_UF16 = 95
    Format::BC6H_RGB_SFLOAT,        // DXGI_FORMAT_BC6H_SF16 = 96
    Format::BC7_RGBA_UNORM,         // DXGI_FORMAT_BC7_TYPELESS = 97
    Format::BC7_RGBA_UNORM,         // DXGI_FORMAT_BC7_UNORM = 98
    Format::BC7_RGBA_SRGB,          // DXGI_FORMAT_BC7_UNORM_SRGB = 99
    Format::UNKNOWN,                // DXGI_FORMAT_AYUV = 100
    Format::UNKNOWN,                // DXGI_FORMAT_Y410 = 101
    Format::UNKNOWN,                // DXGI_FORMAT_Y416 = 102
    Format::UNKNOWN,                // DXGI_FORMAT_NV12 = 103
    Format::UNKNOWN,                // DXGI_FORMAT_P010 = 104
    Format::UNKNOWN,                // DXGI_FORMAT_P016 = 105
    Format::UNKNOWN,                // DXGI_FORMAT_420_OPAQUE = 106
    Format::UNKNOWN,                // DXGI_FORMAT_YUY2 = 107
    Format::UNKNOWN,                // DXGI_FORMAT_Y210 = 108
    Format::UNKNOWN,                // DXGI_FORMAT_Y216 = 109
    Format::UNKNOWN,                // DXGI_FORMAT_NV11 = 110
    Format::UNKNOWN,                // DXGI_FORMAT_AI44 = 111
    Format::UNKNOWN,                // DXGI_FORMAT_IA44 = 112
    Format::UNKNOWN,                // DXGI_FORMAT_P8 = 113
    Format::UNKNOWN,                // DXGI_FORMAT_A8P8 = 114
    Format::B4_G4_R4_A4_UNORM,      // DXGI_FORMAT_B4G4R4A4_UNORM = 115
};

Format nri::DXGIFormatToNRIFormat(uint32_t dxgiFormat) {
    MaybeUnused(dxgiFormat);

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    if (dxgiFormat < NRI_FORMAT_TABLE.size())
        return NRI_FORMAT_TABLE[dxgiFormat];
#endif

    return Format::UNKNOWN;
}

constexpr std::array<Format, 131> VK_FORMAT_TABLE = {
    Format::UNKNOWN,                // VK_FORMAT_UNDEFINED = 0
    Format::UNKNOWN,                // VK_FORMAT_R4G4_UNORM_PACK8 = 1
    Format::UNKNOWN,                // VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2
    Format::UNKNOWN,                // VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3
    Format::B5_G6_R5_UNORM,         // VK_FORMAT_R5G6B5_UNORM_PACK16 = 4
    Format::UNKNOWN,                // VK_FORMAT_B5G6R5_UNORM_PACK16 = 5
    Format::UNKNOWN,                // VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6
    Format::UNKNOWN,                // VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7
    Format::B5_G5_R5_A1_UNORM,      // VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8
    Format::R8_UNORM,               // VK_FORMAT_R8_UNORM = 9
    Format::R8_SNORM,               // VK_FORMAT_R8_SNORM = 10
    Format::UNKNOWN,                // VK_FORMAT_R8_USCALED = 11
    Format::UNKNOWN,                // VK_FORMAT_R8_SSCALED = 12
    Format::R8_UINT,                // VK_FORMAT_R8_UINT = 13
    Format::R8_SINT,                // VK_FORMAT_R8_SINT = 14
    Format::UNKNOWN,                // VK_FORMAT_R8_SRGB = 15
    Format::RG8_UNORM,              // VK_FORMAT_R8G8_UNORM = 16
    Format::RG8_SNORM,              // VK_FORMAT_R8G8_SNORM = 17
    Format::UNKNOWN,                // VK_FORMAT_R8G8_USCALED = 18
    Format::UNKNOWN,                // VK_FORMAT_R8G8_SSCALED = 19
    Format::RG8_UINT,               // VK_FORMAT_R8G8_UINT = 20
    Format::RG8_SINT,               // VK_FORMAT_R8G8_SINT = 21
    Format::UNKNOWN,                // VK_FORMAT_R8G8_SRGB = 22
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8_UNORM = 23
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SNORM = 24
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8_USCALED = 25
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SSCALED = 26
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8_UINT = 27
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SINT = 28
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8_SRGB = 29
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8_UNORM = 30
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SNORM = 31
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8_USCALED = 32
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SSCALED = 33
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8_UINT = 34
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SINT = 35
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8_SRGB = 36
    Format::RGBA8_UNORM,            // VK_FORMAT_R8G8B8A8_UNORM = 37
    Format::RGBA8_SNORM,            // VK_FORMAT_R8G8B8A8_SNORM = 38
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8A8_USCALED = 39
    Format::UNKNOWN,                // VK_FORMAT_R8G8B8A8_SSCALED = 40
    Format::RGBA8_UINT,             // VK_FORMAT_R8G8B8A8_UINT = 41
    Format::RGBA8_SINT,             // VK_FORMAT_R8G8B8A8_SINT = 42
    Format::RGBA8_SRGB,             // VK_FORMAT_R8G8B8A8_SRGB = 43
    Format::BGRA8_UNORM,            // VK_FORMAT_B8G8R8A8_UNORM = 44
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_SNORM = 45
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_USCALED = 46
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_SSCALED = 47
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_UINT = 48
    Format::UNKNOWN,                // VK_FORMAT_B8G8R8A8_SINT = 49
    Format::BGRA8_SRGB,             // VK_FORMAT_B8G8R8A8_SRGB = 50
    Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51
    Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52
    Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53
    Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54
    Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55
    Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56
    Format::UNKNOWN,                // VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57
    Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58
    Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59
    Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60
    Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61
    Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62
    Format::UNKNOWN,                // VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63
    Format::R10_G10_B10_A2_UNORM,   // VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64
    Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65
    Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66
    Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67
    Format::R10_G10_B10_A2_UINT,    // VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68
    Format::UNKNOWN,                // VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69
    Format::R16_UNORM,              // VK_FORMAT_R16_UNORM = 70
    Format::R16_SNORM,              // VK_FORMAT_R16_SNORM = 71
    Format::UNKNOWN,                // VK_FORMAT_R16_USCALED = 72
    Format::UNKNOWN,                // VK_FORMAT_R16_SSCALED = 73
    Format::R16_UINT,               // VK_FORMAT_R16_UINT = 74
    Format::R16_SINT,               // VK_FORMAT_R16_SINT = 75
    Format::R16_SFLOAT,             // VK_FORMAT_R16_SFLOAT = 76
    Format::RG16_UNORM,             // VK_FORMAT_R16G16_UNORM = 77
    Format::RG16_SNORM,             // VK_FORMAT_R16G16_SNORM = 78
    Format::UNKNOWN,                // VK_FORMAT_R16G16_USCALED = 79
    Format::UNKNOWN,                // VK_FORMAT_R16G16_SSCALED = 80
    Format::RG16_UINT,              // VK_FORMAT_R16G16_UINT = 81
    Format::RG16_SINT,              // VK_FORMAT_R16G16_SINT = 82
    Format::RG16_SFLOAT,            // VK_FORMAT_R16G16_SFLOAT = 83
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16_UNORM = 84
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SNORM = 85
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16_USCALED = 86
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SSCALED = 87
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16_UINT = 88
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SINT = 89
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16_SFLOAT = 90
    Format::RGBA16_UNORM,           // VK_FORMAT_R16G16B16A16_UNORM = 91
    Format::RGBA16_SNORM,           // VK_FORMAT_R16G16B16A16_SNORM = 92
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16A16_USCALED = 93
    Format::UNKNOWN,                // VK_FORMAT_R16G16B16A16_SSCALED = 94
    Format::RGBA16_UINT,            // VK_FORMAT_R16G16B16A16_UINT = 95
    Format::RGBA16_SINT,            // VK_FORMAT_R16G16B16A16_SINT = 96
    Format::RGBA16_SFLOAT,          // VK_FORMAT_R16G16B16A16_SFLOAT = 97
    Format::R32_UINT,               // VK_FORMAT_R32_UINT = 98
    Format::R32_SINT,               // VK_FORMAT_R32_SINT = 99
    Format::R32_SFLOAT,             // VK_FORMAT_R32_SFLOAT = 100
    Format::RG32_UINT,              // VK_FORMAT_R32G32_UINT = 101
    Format::RG32_SINT,              // VK_FORMAT_R32G32_SINT = 102
    Format::RG32_SFLOAT,            // VK_FORMAT_R32G32_SFLOAT = 103
    Format::RGB32_UINT,             // VK_FORMAT_R32G32B32_UINT = 104
    Format::RGB32_SINT,             // VK_FORMAT_R32G32B32_SINT = 105
    Format::RGB32_SFLOAT,           // VK_FORMAT_R32G32B32_SFLOAT = 106
    Format::RGBA32_UINT,            // VK_FORMAT_R32G32B32A32_UINT = 107
    Format::RGBA32_SINT,            // VK_FORMAT_R32G32B32A32_SINT = 108
    Format::RGBA32_SFLOAT,          // VK_FORMAT_R32G32B32A32_SFLOAT = 109
    Format::UNKNOWN,                // VK_FORMAT_R64_UINT = 110
    Format::UNKNOWN,                // VK_FORMAT_R64_SINT = 111
    Format::UNKNOWN,                // VK_FORMAT_R64_SFLOAT = 112
    Format::UNKNOWN,                // VK_FORMAT_R64G64_UINT = 113
    Format::UNKNOWN,                // VK_FORMAT_R64G64_SINT = 114
    Format::UNKNOWN,                // VK_FORMAT_R64G64_SFLOAT = 115
    Format::UNKNOWN,                // VK_FORMAT_R64G64B64_UINT = 116
    Format::UNKNOWN,                // VK_FORMAT_R64G64B64_SINT = 117
    Format::UNKNOWN,                // VK_FORMAT_R64G64B64_SFLOAT = 118
    Format::UNKNOWN,                // VK_FORMAT_R64G64B64A64_UINT = 119
    Format::UNKNOWN,                // VK_FORMAT_R64G64B64A64_SINT = 120
    Format::UNKNOWN,                // VK_FORMAT_R64G64B64A64_SFLOAT = 121
    Format::R11_G11_B10_UFLOAT,     // VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122
    Format::R9_G9_B9_E5_UFLOAT,     // VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123
    Format::D16_UNORM,              // VK_FORMAT_D16_UNORM = 124
    Format::D24_UNORM_S8_UINT,      // VK_FORMAT_X8_D24_UNORM_PACK32 = 125
    Format::D32_SFLOAT,             // VK_FORMAT_D32_SFLOAT = 126
    Format::UNKNOWN,                // VK_FORMAT_S8_UINT = 127
    Format::UNKNOWN,                // VK_FORMAT_D16_UNORM_S8_UINT = 128
    Format::D24_UNORM_S8_UINT,      // VK_FORMAT_D24_UNORM_S8_UINT = 129
    Format::D32_SFLOAT_S8_UINT_X24, // VK_FORMAT_D32_SFLOAT_S8_UINT = 130
};

Format nri::VKFormatToNRIFormat(uint32_t format) {
    MaybeUnused(format);

#if NRI_ENABLE_VK_SUPPORT
    if (format < VK_FORMAT_TABLE.size())
        return VK_FORMAT_TABLE[format];
    else if (format == VK_FORMAT_A4R4G4B4_UNORM_PACK16)
        return Format::B4_G4_R4_A4_UNORM;
#endif

    return Format::UNKNOWN;
}

void DeviceBase::ReportMessage(Message messageType, Result result, const char* file, uint32_t line, const char* format, ...) const {
    // Report message
    if (m_CallbackInterface.MessageCallback) { // TODO: "MessageCallback" actually can't be "NULL"
        const DeviceDesc& desc = GetDesc();
        const char* graphicsAPIName = nriGetGraphicsAPIString(desc.graphicsAPI);

        const char* temp = strrchr(file, FILE_SEPARATOR);
        file = temp ? temp + 1 : file;

        char buf[MAX_MESSAGE_LENGTH];
        int32_t written = snprintf(buf, sizeof(buf), "%s::%s - ", graphicsAPIName, *desc.adapterDesc.name == '\0' ? "Unknown" : desc.adapterDesc.name);

        va_list argptr;
        va_start(argptr, format);
        written += vsnprintf(buf + written, sizeof(buf) - written, format, argptr);
        va_end(argptr);

        m_CallbackInterface.MessageCallback(messageType, file, line, buf, m_CallbackInterface.userArg);
    }

    // Abort execution
    if (m_CallbackInterface.AbortExecution && (int8_t)result > 0)
        m_CallbackInterface.AbortExecution(m_CallbackInterface.userArg);
}

void nri::ConvertCharToWchar(const char* in, wchar_t* out, size_t outLength) {
    if (outLength == 0)
        return;

    for (size_t i = 0; i < outLength - 1 && *in; i++)
        *out++ = *in++;

    *out = 0;
}

void nri::ConvertWcharToChar(const wchar_t* in, char* out, size_t outLength) {
    if (outLength == 0)
        return;

    for (size_t i = 0; i < outLength - 1 && *in; i++)
        *out++ = char(*in++);

    *out = 0;
}

uint64_t nri::GetSwapChainId() {
    static uint64_t id = 0;
    return id++ << PRESENT_INDEX_BIT_NUM;
}
