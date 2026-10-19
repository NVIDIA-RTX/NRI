// © 2026 NVIDIA Corporation

static inline WGPUTextureFormat GetSurfaceFormat(const WGPUSurfaceCapabilities& capabilities, SwapChainFormat format) {
    WGPUTextureFormat requested = GetSwapChainTextureFormat(format);

    for (size_t i = 0; i < capabilities.formatCount; i++) {
        if (capabilities.formats[i] == requested)
            return requested;
    }

    if (format == SwapChainFormat::BT709_G22_8BIT) {
        for (size_t i = 0; i < capabilities.formatCount; i++) {
            if (capabilities.formats[i] == WGPUTextureFormat_BGRA8Unorm)
                return WGPUTextureFormat_BGRA8Unorm;
        }
    }

    return capabilities.formatCount ? capabilities.formats[0] : requested;
}

static inline WGPUPresentMode GetSurfacePresentMode(const WGPUSurfaceCapabilities& capabilities, SwapChainBits flags) {
    WGPUPresentMode requested = (flags & SwapChainBits::VSYNC) ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;

    for (size_t i = 0; i < capabilities.presentModeCount; i++) {
        if (capabilities.presentModes[i] == requested)
            return requested;
    }

    if (!(flags & SwapChainBits::VSYNC)) {
        for (size_t i = 0; i < capabilities.presentModeCount; i++) {
            if (capabilities.presentModes[i] == WGPUPresentMode_Mailbox)
                return WGPUPresentMode_Mailbox;
        }
    }

    return WGPUPresentMode_Fifo;
}

static inline WGPUCompositeAlphaMode GetSurfaceAlphaMode(const WGPUSurfaceCapabilities& capabilities) {
    for (size_t i = 0; i < capabilities.alphaModeCount; i++) {
        if (capabilities.alphaModes[i] == WGPUCompositeAlphaMode_Opaque)
            return WGPUCompositeAlphaMode_Opaque;
    }

    return WGPUCompositeAlphaMode_Auto;
}

SwapChainWGPU::~SwapChainWGPU() {
    for (TextureWGPU* texture : m_Textures)
        Destroy(m_Device.GetAllocationCallbacks(), texture);

    if (m_Surface) {
        wgpuSurfaceUnconfigure(m_Surface);
        wgpuSurfaceRelease(m_Surface);
    }
}

Result SwapChainWGPU::Create(const SwapChainDesc& swapChainDesc) {
    WGPUChainedStruct* surfaceSource = nullptr;

#if defined(_WIN32)
    WGPUSurfaceSourceWindowsHWND surfaceSourceWindows = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
    if (swapChainDesc.window.windows.hwnd) {
        surfaceSourceWindows.hinstance = GetModuleHandleW(nullptr);
        surfaceSourceWindows.hwnd = swapChainDesc.window.windows.hwnd;
        surfaceSource = &surfaceSourceWindows.chain;
    }
#endif
#if defined(NRI_ENABLE_XLIB_SUPPORT)
    WGPUSurfaceSourceXlibWindow surfaceSourceXlib = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
    if (swapChainDesc.window.x11.dpy && swapChainDesc.window.x11.window) {
        surfaceSourceXlib.display = swapChainDesc.window.x11.dpy;
        surfaceSourceXlib.window = swapChainDesc.window.x11.window;
        surfaceSource = &surfaceSourceXlib.chain;
    }
#endif
#if defined(NRI_ENABLE_WAYLAND_SUPPORT)
    WGPUSurfaceSourceWaylandSurface surfaceSourceWayland = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
    if (swapChainDesc.window.wayland.display && swapChainDesc.window.wayland.surface) {
        surfaceSourceWayland.display = swapChainDesc.window.wayland.display;
        surfaceSourceWayland.surface = swapChainDesc.window.wayland.surface;
        surfaceSource = &surfaceSourceWayland.chain;
    }
#endif
#if defined(__APPLE__)
    WGPUSurfaceSourceMetalLayer surfaceSourceMetal = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
    if (swapChainDesc.window.metal.caMetalLayer) {
        surfaceSourceMetal.layer = swapChainDesc.window.metal.caMetalLayer;
        surfaceSource = &surfaceSourceMetal.chain;
    }
#endif

    if (!surfaceSource)
        return Result::UNSUPPORTED;

    WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surfaceDesc.nextInChain = surfaceSource;

    m_Surface = wgpuInstanceCreateSurface(m_Device.GetInstance(), &surfaceDesc);
    if (!m_Surface)
        return Result::FAILURE;

    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    WGPUStatus status = wgpuSurfaceGetCapabilities(m_Surface, m_Device.GetAdapter(), &capabilities);
    if (status != WGPUStatus_Success)
        return Result::FAILURE;

    WGPUTextureFormat format = GetSurfaceFormat(capabilities, swapChainDesc.format);
    WGPUPresentMode presentMode = GetSurfacePresentMode(capabilities, swapChainDesc.flags);
    WGPUCompositeAlphaMode alphaMode = GetSurfaceAlphaMode(capabilities);

    WGPUTextureUsage usage = (WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst) & capabilities.usages;

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);

    // TODO: WGPU surface configuration does not implement NRI present scaling/gravity, queued frame count, waitable swapchain, or low-latency flags.
    WGPUSurfaceConfiguration desc = WGPU_SURFACE_CONFIGURATION_INIT;
    desc.device = m_Device;
    desc.format = format;
    desc.usage = usage;
    desc.width = swapChainDesc.width;
    desc.height = swapChainDesc.height;
    desc.alphaMode = alphaMode;
    desc.presentMode = presentMode;

    wgpuSurfaceConfigure(m_Surface, &desc);

    m_Format = GetNRIFormat(format);
    m_Width = swapChainDesc.width;
    m_Height = swapChainDesc.height;
    m_Hwnd = swapChainDesc.window.windows.hwnd;

    uint32_t textureNum = std::max<uint32_t>(swapChainDesc.textureNum, 1);
    m_Textures.reserve(textureNum);

    for (uint32_t i = 0; i < textureNum; i++) {
        TextureWGPU* texture = Allocate<TextureWGPU>(m_Device.GetAllocationCallbacks(), m_Device);
        if (!texture)
            return Result::OUT_OF_MEMORY;

        texture->SetSurfaceTexture(nullptr, m_Format, m_Width, m_Height);
        m_Textures.push_back(texture);
    }

    return Result::SUCCESS;
}

Texture* const* SwapChainWGPU::GetTextures(uint32_t& textureNum) const {
    textureNum = (uint32_t)m_Textures.size();

    return (Texture**)m_Textures.data();
}

Result SwapChainWGPU::GetDisplayDesc(DisplayDesc& displayDesc) {
    // TODO: DisplayDescHelper is HWND-based. Add X11/Wayland/macOS display queries for non-Windows swapchains.
    return DisplayDescHelper::GetDisplayDesc(m_Hwnd, displayDesc);
}

Result SwapChainWGPU::AcquireNextTexture(uint32_t& textureIndex) {
    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(m_Surface, &surfaceTexture);

    if ((uint32_t)surfaceTexture.status == (uint32_t)WGPUSurfaceGetCurrentTextureStatus_Occluded || surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Timeout)
        return Result::NOT_READY;
    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated || surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Lost)
        return Result::OUT_OF_DATE;
    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal && surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
        return Result::FAILURE;

    textureIndex = m_TextureIndex++ % (uint32_t)m_Textures.size();
    m_Textures[textureIndex]->SetSurfaceTexture(surfaceTexture.texture, m_Format, m_Width, m_Height);
    m_CurrentTextureIndex = textureIndex;

    return Result::SUCCESS;
}

Result SwapChainWGPU::WaitForPresent() {
    // TODO: WGPU has no waitable-swapchain equivalent. Keep "features.waitableSwapChain = false".
    return Result::UNSUPPORTED;
}

Result SwapChainWGPU::Present() {
    WGPUStatus status = wgpuSurfacePresent(m_Surface);
    if (m_CurrentTextureIndex != uint32_t(-1)) {
        m_Textures[m_CurrentTextureIndex]->DetachSurfaceTexture();
        m_CurrentTextureIndex = uint32_t(-1);
    }

    return status == WGPUStatus_Success ? Result::SUCCESS : Result::FAILURE;
}
