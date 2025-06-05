// © 2021 NVIDIA Corporation

constexpr uint32_t PRESENT_MODE_MAX_NUM = 16;

SwapChainVK::~SwapChainVK() {
    // TODO: use "vkReleaseSwapchainImagesEXT" to release acquired but not presented images?

    for (size_t i = 0; i < m_Textures.size(); i++)
        Destroy(m_Textures[i]);

    Destroy(m_LatencyFence);

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle)
        vk.DestroySwapchainKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());

    if (m_Surface)
        vk.DestroySurfaceKHR(m_Device, m_Surface, m_Device.GetVkAllocationCallbacks());
}

Result SwapChainVK::Create(const SwapChainDesc& swapChainDesc) {
    const auto& vk = m_Device.GetDispatchTable();

    m_Queue = (QueueVK*)swapChainDesc.queue;
    uint32_t familyIndex = m_Queue->GetFamilyIndex();

    // Create surface
#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (swapChainDesc.window.windows.hwnd) {
        VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        win32SurfaceInfo.hwnd = (HWND)swapChainDesc.window.windows.hwnd;

        VkResult vkResult = vk.CreateWin32SurfaceKHR(m_Device, &win32SurfaceInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateWin32SurfaceKHR returned %d", (int32_t)vkResult);
    }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if (swapChainDesc.window.x11.dpy && swapChainDesc.window.x11.window) {
        VkXlibSurfaceCreateInfoKHR xlibSurfaceInfo = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
        xlibSurfaceInfo.dpy = (::Display*)swapChainDesc.window.x11.dpy;
        xlibSurfaceInfo.window = (::Window)swapChainDesc.window.x11.window;

        VkResult vkResult = vk.CreateXlibSurfaceKHR(m_Device, &xlibSurfaceInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateXlibSurfaceKHR returned %d", (int32_t)vkResult);
    }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if (swapChainDesc.window.wayland.display && swapChainDesc.window.wayland.surface) {
        VkWaylandSurfaceCreateInfoKHR waylandSurfaceInfo = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
        waylandSurfaceInfo.display = (wl_display*)swapChainDesc.window.wayland.display;
        waylandSurfaceInfo.surface = (wl_surface*)swapChainDesc.window.wayland.surface;

        VkResult vkResult = vk.CreateWaylandSurfaceKHR(m_Device, &waylandSurfaceInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateWaylandSurfaceKHR returned %d", (int32_t)vkResult);
    }
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    if (swapChainDesc.window.metal.caMetalLayer) {
        VkMetalSurfaceCreateInfoEXT metalSurfaceCreateInfo = {VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
        metalSurfaceCreateInfo.pLayer = (CAMetalLayer*)swapChainDesc.window.metal.caMetalLayer;

        VkResult vkResult = vk.CreateMetalSurfaceEXT(m_Device, &metalSurfaceCreateInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateMetalSurfaceEXT returned %d", (int32_t)vkResult);
    }
#endif

    // Surface caps
    std::array<VkPresentModeKHR, PRESENT_MODE_MAX_NUM> lowLatencyPresentModes = {};

    VkLatencySurfaceCapabilitiesNV latencySurfaceCapabilities = {VK_STRUCTURE_TYPE_LATENCY_SURFACE_CAPABILITIES_NV};
    latencySurfaceCapabilities.presentModeCount = (uint32_t)lowLatencyPresentModes.size();
    latencySurfaceCapabilities.pPresentModes = lowLatencyPresentModes.data();

    bool allowLowLatency = m_Device.GetDesc().features.lowLatency && (swapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY);

    uint32_t textureNum = swapChainDesc.textureNum;
    {
        VkBool32 supported = VK_FALSE;
        VkResult vkResult = vk.GetPhysicalDeviceSurfaceSupportKHR(m_Device, familyIndex, m_Surface, &supported);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS && supported, GetReturnCode(vkResult), "Surface is not supported");

        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        surfaceInfo.surface = m_Surface;

        VkSurfaceCapabilities2KHR caps2 = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
        const VkSurfaceCapabilitiesKHR& surfaceCaps = caps2.surfaceCapabilities;

        if (allowLowLatency)
            caps2.pNext = &latencySurfaceCapabilities;

        vkResult = vk.GetPhysicalDeviceSurfaceCapabilities2KHR(m_Device, &surfaceInfo, &caps2);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkGetPhysicalDeviceSurfaceCapabilities2KHR returned %d", (int32_t)vkResult);

        bool isWidthValid = swapChainDesc.width >= surfaceCaps.minImageExtent.width && swapChainDesc.width <= surfaceCaps.maxImageExtent.width;
        RETURN_ON_FAILURE(&m_Device, isWidthValid, Result::INVALID_ARGUMENT, "swapChainDesc.width is out of [%u, %u] range", surfaceCaps.minImageExtent.width,
            surfaceCaps.maxImageExtent.width);

        bool isHeightValid = swapChainDesc.height >= surfaceCaps.minImageExtent.height && swapChainDesc.height <= surfaceCaps.maxImageExtent.height;
        RETURN_ON_FAILURE(&m_Device, isHeightValid, Result::INVALID_ARGUMENT, "swapChainDesc.height is out of [%u, %u] range", surfaceCaps.minImageExtent.height,
            surfaceCaps.maxImageExtent.height);

        // Silently clamp "textureNum" to the supported range
        if (textureNum < surfaceCaps.minImageCount)
            textureNum = surfaceCaps.minImageCount;
        if (surfaceCaps.maxImageCount && textureNum > surfaceCaps.maxImageCount) // 0 - unlimited (see spec)
            textureNum = surfaceCaps.maxImageCount;

        if (textureNum != swapChainDesc.textureNum)
            REPORT_WARNING(&m_Device, "'swapChainDesc.textureNum=%u' clamped to %u", swapChainDesc.textureNum, textureNum);
    }

    // Surface format
    VkSurfaceFormat2KHR surfaceFormat = {VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};
    {
        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        surfaceInfo.surface = m_Surface;

        uint32_t formatNum = 0;
        VkResult vkResult = vk.GetPhysicalDeviceSurfaceFormats2KHR(m_Device, &surfaceInfo, &formatNum, nullptr);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkGetPhysicalDeviceSurfaceFormats2KHR returned %d", (int32_t)vkResult);

        Scratch<VkSurfaceFormat2KHR> surfaceFormats = AllocateScratch(m_Device, VkSurfaceFormat2KHR, formatNum);
        for (uint32_t i = 0; i < formatNum; i++)
            surfaceFormats[i] = {VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};

        vkResult = vk.GetPhysicalDeviceSurfaceFormats2KHR(m_Device, &surfaceInfo, &formatNum, surfaceFormats);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkGetPhysicalDeviceSurfaceFormats2KHR returned %d", (int32_t)vkResult);

        auto priority_BT709_G22_16BIT = [](const VkSurfaceFormat2KHR& s) -> uint32_t {
            if (s.surfaceFormat.format != VK_FORMAT_R16G16B16A16_SFLOAT)
                return 0;

            uint32_t priority = s.surfaceFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT ? 20 : 10;

            return priority;
        };

        auto priority_BT709_G22_8BIT = [](const VkSurfaceFormat2KHR& s) -> uint32_t {
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
            // There is always a corresponding UNORM, SRGB just need to consider UNORM
            uint32_t priority = 0;
            if (s.surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
                priority = 4;
            else if (s.surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
                priority = 3;
            else if (s.surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB)
                priority = 2;
            else if (s.surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
                priority = 1;
            else
                return 0;

            priority += s.surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ? 10 : 0;

            return priority;
        };

        auto priority_BT709_G22_10BIT = [](const VkSurfaceFormat2KHR& s) -> uint32_t {
            uint32_t priority = 0;
            if (s.surfaceFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
                priority = 1;
            else
                return 0;

            priority += s.surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ? 10 : 0;

            return priority;
        };

        auto priority_BT2020_G2084_10BIT = [](const VkSurfaceFormat2KHR& s) -> uint32_t {
            uint32_t priority = 0;
            if (s.surfaceFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
                priority = 1;
            else
                return 0;

            priority += s.surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT ? 10 : 0;

            return priority;
        };

        switch (swapChainDesc.format) {
            case SwapChainFormat::BT709_G10_16BIT:
                std::sort(surfaceFormats + 0, surfaceFormats + formatNum, [&](VkSurfaceFormat2KHR& a1, VkSurfaceFormat2KHR& b1) {
                    return priority_BT709_G22_16BIT(a1) > priority_BT709_G22_16BIT(b1);
                });
                break;
            case SwapChainFormat::BT709_G22_8BIT:
                std::sort(surfaceFormats + 0, surfaceFormats + formatNum, [&](VkSurfaceFormat2KHR& a1, VkSurfaceFormat2KHR& b1) {
                    return priority_BT709_G22_8BIT(a1) > priority_BT709_G22_8BIT(b1);
                });
                break;
            case SwapChainFormat::BT709_G22_10BIT:
                std::sort(surfaceFormats + 0, surfaceFormats + formatNum, [&](VkSurfaceFormat2KHR& a1, VkSurfaceFormat2KHR& b1) {
                    return priority_BT709_G22_10BIT(a1) > priority_BT709_G22_10BIT(b1);
                });
                break;
            case SwapChainFormat::BT2020_G2084_10BIT:
                std::sort(surfaceFormats + 0, surfaceFormats + formatNum, [&](VkSurfaceFormat2KHR& a1, VkSurfaceFormat2KHR& b1) {
                    return priority_BT2020_G2084_10BIT(a1) > priority_BT2020_G2084_10BIT(b1);
                });
                break;
        }

        surfaceFormat = surfaceFormats[0];
    }

    /* Present modes:
        Capped frame rate            Vsync  Comments
            FIFO                       Y       Classic VSYNC, always supported
            FIFO_RELAXED              Y/N      FIFO with fallback to IMMEDIATE if framerate < monitor refresh rate

        Uncapped frame rate          Vsync  Comments
            IMMEDIATE                  N       Classic tearing, most likely supported
            MAILBOX                    Y       No tearing, but almost uncapped FPS
            FIFO_LATEST_READY          Y       Similar to MAILBOX, but offers lower latency
    */

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // the only one 100% supported (see spec)
    {
        uint32_t surfacePresentModeNum = PRESENT_MODE_MAX_NUM;
        std::array<VkPresentModeKHR, PRESENT_MODE_MAX_NUM> surfacePresentModes = {};

        VkResult vkResult = vk.GetPhysicalDeviceSurfacePresentModesKHR(m_Device, m_Surface, &surfacePresentModeNum, surfacePresentModes.data());
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkGetPhysicalDeviceSurfacePresentModesKHR returned %d", (int32_t)vkResult);

        const VkPresentModeKHR cappedModes[] = {
            (swapChainDesc.flags & SwapChainBits::ALLOW_TEARING) ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_FIFO_KHR,
            VK_PRESENT_MODE_FIFO_KHR, // guaranteed to be supported
        };
        
        const VkPresentModeKHR uncappedModes[] = {
            m_Device.m_IsSupported.fifoLatestReady ? VK_PRESENT_MODE_FIFO_LATEST_READY_EXT : VK_PRESENT_MODE_MAILBOX_KHR,
            (swapChainDesc.flags & SwapChainBits::ALLOW_TEARING) ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR,
            VK_PRESENT_MODE_FIFO_KHR, // guaranteed to be supported
        };

        const VkPresentModeKHR* wantedModes = uncappedModes;
        uint32_t wantedModeNum = GetCountOf(uncappedModes);

        if (swapChainDesc.flags & SwapChainBits::VSYNC) {
            wantedModes = cappedModes;
            wantedModeNum = GetCountOf(cappedModes);
        }

        const VkPresentModeKHR* availableModes = surfacePresentModes.data();
        uint32_t availableModeNum = surfacePresentModeNum;
        if (allowLowLatency) {
            availableModes = lowLatencyPresentModes.data();
            availableModeNum = latencySurfaceCapabilities.presentModeCount;
        }

        bool isFound = false;
        for (uint32_t j = 0; j < wantedModeNum && !isFound; j++) {
            for (uint32_t i = 0; i < availableModeNum && !isFound; i++) {
                if (wantedModes[j] == availableModes[i]) {
                    presentMode = wantedModes[j];
                    isFound = true;
                }
            }
        }
    }

    { // Swap chain
        VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchainInfo.flags = 0;
        swapchainInfo.surface = m_Surface;
        swapchainInfo.minImageCount = textureNum;
        swapchainInfo.imageFormat = surfaceFormat.surfaceFormat.format;
        swapchainInfo.imageColorSpace = surfaceFormat.surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = {swapChainDesc.width, swapChainDesc.height};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 1;
        swapchainInfo.pQueueFamilyIndices = &familyIndex;
        swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentMode;
        const void** tail = &swapchainInfo.pNext;

        // Mutable formats
        VkFormat mutableFormats[2];
        uint32_t mutableFormatNum = 0;
        mutableFormats[mutableFormatNum++] = surfaceFormat.surfaceFormat.format;
        switch (surfaceFormat.surfaceFormat.format) {
            case VK_FORMAT_R8G8B8A8_UNORM:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_R8G8B8A8_SRGB;
                break;
            case VK_FORMAT_R8G8B8A8_SRGB:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_R8G8B8A8_UNORM;
                break;
            case VK_FORMAT_B8G8R8A8_UNORM:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_B8G8R8A8_SRGB;
                break;
            case VK_FORMAT_B8G8R8A8_SRGB:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_B8G8R8A8_UNORM;
                break;
        }

        VkImageFormatListCreateInfo imageFormatListCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO};
        imageFormatListCreateInfo.pViewFormats = mutableFormats;
        imageFormatListCreateInfo.viewFormatCount = mutableFormatNum;

        if (m_Device.m_IsSupported.swapChainMutableFormat) {
            swapchainInfo.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
            APPEND_EXT(imageFormatListCreateInfo);
        }

        // Low latency mode
        VkSwapchainLatencyCreateInfoNV latencyCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_LATENCY_CREATE_INFO_NV};
        latencyCreateInfo.latencyModeEnable = allowLowLatency;

        if (allowLowLatency) {
            APPEND_EXT(latencyCreateInfo);
        }

        // Create
        VkResult vkResult = vk.CreateSwapchainKHR(m_Device, &swapchainInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
        RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateSwapchainKHR returned %d", (int32_t)vkResult);
    }

    { // Textures
        uint32_t imageNum = 0;
        vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, nullptr);

        Scratch<VkImage> imageHandles = AllocateScratch(m_Device, VkImage, imageNum);
        vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, imageHandles);

        m_Textures.resize(imageNum);
        for (uint32_t i = 0; i < imageNum; i++) {
            TextureVKDesc desc = {};
            desc.vkImage = (VKNonDispatchableHandle)imageHandles[i];
            desc.vkFormat = surfaceFormat.surfaceFormat.format;
            desc.vkImageType = VK_IMAGE_TYPE_2D;
            desc.width = swapChainDesc.width;
            desc.height = swapChainDesc.height;
            desc.depth = 1;
            desc.mipNum = 1;
            desc.layerNum = 1;
            desc.sampleNum = 1;

            TextureVK* texture = Allocate<TextureVK>(m_Device.GetAllocationCallbacks(), m_Device);
            texture->Create(desc);

            m_Textures[i] = texture;
        }
    }

    // Latency fence
    if (allowLowLatency) {
        m_LatencyFence = Allocate<FenceVK>(m_Device.GetAllocationCallbacks(), m_Device);
        m_LatencyFence->Create(0);
    }

    // Finalize
    m_Hwnd = swapChainDesc.window.windows.hwnd;
    m_PresentId = GetSwapChainId();

    m_Flags = swapChainDesc.flags;
    if (!allowLowLatency)
        m_Flags &= ~SwapChainBits::ALLOW_LOW_LATENCY;
    if (!m_Device.GetDesc().features.waitableSwapChain)
        m_Flags &= ~SwapChainBits::WAITABLE;

    return Result::SUCCESS;
}

NRI_INLINE void SwapChainVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SURFACE_KHR, (uint64_t)m_Surface, name);
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SWAPCHAIN_KHR, (uint64_t)m_Handle, name);
}

NRI_INLINE Texture* const* SwapChainVK::GetTextures(uint32_t& textureNum) const {
    textureNum = (uint32_t)m_Textures.size();

    return (Texture* const*)m_Textures.data();
}

NRI_INLINE Result SwapChainVK::AcquireNextTexture(FenceVK& acquireSemaphore, uint32_t& textureIndex) {
    ExclusiveScope lock(m_Queue->GetLock());
    const auto& vk = m_Device.GetDispatchTable();

    // Acquire next image (signal)
    VkAcquireNextImageInfoKHR acquireInfo = {VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR};
    acquireInfo.swapchain = m_Handle;
    acquireInfo.timeout = MsToUs(TIMEOUT_PRESENT);
    acquireInfo.semaphore = acquireSemaphore;
    acquireInfo.deviceMask = NODE_MASK;

    VkResult vkResult = vk.AcquireNextImage2KHR(m_Device, &acquireInfo, &m_TextureIndex);

    Result result = GetReturnCode(vkResult);
    if (result != Result::OUT_OF_DATE && result != Result::SUCCESS)
        REPORT_ERROR(&m_Device, "vkAcquireNextImage2KHR returned %d", (int32_t)vkResult);

    textureIndex = m_TextureIndex;

    return result;
}

NRI_INLINE Result SwapChainVK::WaitForPresent() {
    if (!(m_Flags & SwapChainBits::WAITABLE) || GetPresentIndex(m_PresentId) == 0)
        return Result::UNSUPPORTED;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.WaitForPresentKHR(m_Device, m_Handle, m_PresentId - 1, MsToUs(TIMEOUT_PRESENT));

    return GetReturnCode(vkResult);
}

NRI_INLINE Result SwapChainVK::Present(FenceVK& releaseSemaphore) {
    ExclusiveScope lock(m_Queue->GetLock());

    // Present (wait)
    VkSemaphore vkReleaseSemaphore = releaseSemaphore;

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &vkReleaseSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Handle;
    presentInfo.pImageIndices = &m_TextureIndex;

    VkPresentIdKHR presentId = {VK_STRUCTURE_TYPE_PRESENT_ID_KHR};
    presentId.swapchainCount = 1;
    presentId.pPresentIds = &m_PresentId;

    if (m_Device.m_IsSupported.presentId)
        presentInfo.pNext = &presentId;

    if (m_Flags & SwapChainBits::ALLOW_LOW_LATENCY)
        SetLatencyMarker((LatencyMarker)VK_LATENCY_MARKER_PRESENT_START_NV);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.QueuePresentKHR(*m_Queue, &presentInfo);

    if (m_Flags & SwapChainBits::ALLOW_LOW_LATENCY)
        SetLatencyMarker((LatencyMarker)VK_LATENCY_MARKER_PRESENT_END_NV);

    m_PresentId++;

    return GetReturnCode(vkResult);
}

NRI_INLINE Result SwapChainVK::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
    VkLatencySleepModeInfoNV sleepModeInfo = {VK_STRUCTURE_TYPE_LATENCY_SLEEP_MODE_INFO_NV};
    sleepModeInfo.lowLatencyMode = latencySleepMode.lowLatencyMode;
    sleepModeInfo.lowLatencyBoost = latencySleepMode.lowLatencyBoost;
    sleepModeInfo.minimumIntervalUs = latencySleepMode.minIntervalUs;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.SetLatencySleepModeNV(m_Device, m_Handle, &sleepModeInfo);

    return GetReturnCode(vkResult);
}

NRI_INLINE Result SwapChainVK::SetLatencyMarker(LatencyMarker latencyMarker) {
    VkSetLatencyMarkerInfoNV markerInfo = {VK_STRUCTURE_TYPE_SET_LATENCY_MARKER_INFO_NV};
    markerInfo.presentID = m_PresentId;
    markerInfo.marker = (VkLatencyMarkerNV)latencyMarker;

    const auto& vk = m_Device.GetDispatchTable();
    vk.SetLatencyMarkerNV(m_Device, m_Handle, &markerInfo);

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainVK::LatencySleep() {
    VkLatencySleepInfoNV sleepInfo = {VK_STRUCTURE_TYPE_LATENCY_SLEEP_INFO_NV};
    sleepInfo.signalSemaphore = *m_LatencyFence;
    sleepInfo.value = m_PresentId;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.LatencySleepNV(m_Device, m_Handle, &sleepInfo);

    if (vkResult == VK_SUCCESS)
        m_LatencyFence->Wait(m_PresentId);

    return GetReturnCode(vkResult);
}

NRI_INLINE Result SwapChainVK::GetLatencyReport(LatencyReport& latencyReport) {
    VkLatencyTimingsFrameReportNV timingsInfo[64] = {};
    for (uint32_t i = 0; i < GetCountOf(timingsInfo); i++)
        timingsInfo[i].sType = VK_STRUCTURE_TYPE_LATENCY_TIMINGS_FRAME_REPORT_NV;

    VkGetLatencyMarkerInfoNV getTimingsInfo = {VK_STRUCTURE_TYPE_GET_LATENCY_MARKER_INFO_NV};
    getTimingsInfo.pTimings = timingsInfo;
    getTimingsInfo.timingCount = GetCountOf(timingsInfo);

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetLatencyTimingsNV(m_Device, m_Handle, &getTimingsInfo);

    latencyReport = {};
    if (getTimingsInfo.timingCount >= 64) {
        const uint32_t i = 63;
        latencyReport.inputSampleTimeUs = timingsInfo[i].inputSampleTimeUs;
        latencyReport.simulationStartTimeUs = timingsInfo[i].simStartTimeUs;
        latencyReport.simulationEndTimeUs = timingsInfo[i].simEndTimeUs;
        latencyReport.renderSubmitStartTimeUs = timingsInfo[i].renderSubmitStartTimeUs;
        latencyReport.renderSubmitEndTimeUs = timingsInfo[i].renderSubmitEndTimeUs;
        latencyReport.presentStartTimeUs = timingsInfo[i].presentStartTimeUs;
        latencyReport.presentEndTimeUs = timingsInfo[i].presentEndTimeUs;
        latencyReport.driverStartTimeUs = timingsInfo[i].driverStartTimeUs;
        latencyReport.driverEndTimeUs = timingsInfo[i].driverEndTimeUs;
        latencyReport.osRenderQueueStartTimeUs = timingsInfo[i].osRenderQueueStartTimeUs;
        latencyReport.osRenderQueueEndTimeUs = timingsInfo[i].osRenderQueueEndTimeUs;
        latencyReport.gpuRenderStartTimeUs = timingsInfo[i].gpuRenderStartTimeUs;
        latencyReport.gpuRenderEndTimeUs = timingsInfo[i].gpuRenderEndTimeUs;
    }

    return Result::SUCCESS;
}
