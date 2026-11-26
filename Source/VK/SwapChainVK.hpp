// © 2021 NVIDIA Corporation

constexpr uint32_t PRESENT_MODE_MAX_NUM = 16;

static constexpr VkPresentGravityFlagBitsKHR GetGravity(Gravity gravity) {
    switch (gravity) {
        case Gravity::CENTERED:
            return VK_PRESENT_GRAVITY_CENTERED_BIT_KHR;

        case Gravity::MAX:
            return VK_PRESENT_GRAVITY_MAX_BIT_KHR;

        default:
            return VK_PRESENT_GRAVITY_MIN_BIT_KHR;
    }
}

SwapChainVK::~SwapChainVK() {
    // TODO: use "vkReleaseSwapchainImagesEXT" to release acquired but not presented images?

    for (size_t i = 0; i < m_Textures.size(); i++)
        Destroy(m_Textures[i]);

    Destroy(m_LatencyFence);

    const auto& vk = m_Device.GetDispatchTable();

    if (m_Handle && m_OwnsNativeObjects)
        vk.DestroySwapchainKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());

    if (m_Surface)
        vk.DestroySurfaceKHR(m_Device, m_Surface, m_Device.GetVkAllocationCallbacks());
}

Result SwapChainVK::Create(const SwapChainDesc& swapChainDesc) {
    const auto& vk = m_Device.GetDispatchTable();

    m_Queue = (QueueVK*)swapChainDesc.queue;
    uint32_t familyIndex = m_Queue->GetFamilyIndex();

    { // Create surface
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (swapChainDesc.window.windows.hwnd) {
            VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            win32SurfaceInfo.hwnd = (HWND)swapChainDesc.window.windows.hwnd;

            VkResult vkResult = vk.CreateWin32SurfaceKHR(m_Device, &win32SurfaceInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
            NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateWin32SurfaceKHR");
        }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (swapChainDesc.window.x11.dpy && swapChainDesc.window.x11.window) {
            VkXlibSurfaceCreateInfoKHR xlibSurfaceInfo = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
            xlibSurfaceInfo.dpy = (::Display*)swapChainDesc.window.x11.dpy;
            xlibSurfaceInfo.window = (::Window)swapChainDesc.window.x11.window;

            VkResult vkResult = vk.CreateXlibSurfaceKHR(m_Device, &xlibSurfaceInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
            NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateXlibSurfaceKHR");
        }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (swapChainDesc.window.wayland.display && swapChainDesc.window.wayland.surface) {
            VkWaylandSurfaceCreateInfoKHR waylandSurfaceInfo = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
            waylandSurfaceInfo.display = (wl_display*)swapChainDesc.window.wayland.display;
            waylandSurfaceInfo.surface = (wl_surface*)swapChainDesc.window.wayland.surface;

            VkResult vkResult = vk.CreateWaylandSurfaceKHR(m_Device, &waylandSurfaceInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
            NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateWaylandSurfaceKHR");
        }
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
        if (swapChainDesc.window.metal.caMetalLayer) {
            VkMetalSurfaceCreateInfoEXT metalSurfaceCreateInfo = {VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
            metalSurfaceCreateInfo.pLayer = (CAMetalLayer*)swapChainDesc.window.metal.caMetalLayer;

            VkResult vkResult = vk.CreateMetalSurfaceEXT(m_Device, &metalSurfaceCreateInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
            NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateMetalSurfaceEXT");
        }
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        {
            VkAndroidSurfaceCreateInfoKHR androidSurfaceInfo = {VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
            androidSurfaceInfo.window = (ANativeWindow*)swapChainDesc.window.android.nativeWindow;

            VkResult vkResult = vk.CreateAndroidSurfaceKHR(m_Device, &androidSurfaceInfo, m_Device.GetVkAllocationCallbacks(), &m_Surface);
            NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateAndroidSurfaceKHR");
        }
#endif
        VkBool32 supported = VK_FALSE;
        VkResult vkResult = vk.GetPhysicalDeviceSurfaceSupportKHR(m_Device, familyIndex, m_Surface, &supported);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "GetPhysicalDeviceSurfaceSupportKHR");

        NRI_RETURN_ON_FAILURE(&m_Device, supported, Result::FAILURE, "Surface is not supported");
    }

    // Low latency
    std::array<VkPresentModeKHR, PRESENT_MODE_MAX_NUM> lowLatencyPresentModes = {};

    VkLatencySurfaceCapabilitiesNV latencySurfaceCaps = {VK_STRUCTURE_TYPE_LATENCY_SURFACE_CAPABILITIES_NV};
    latencySurfaceCaps.presentModeCount = (uint32_t)lowLatencyPresentModes.size();
    latencySurfaceCaps.pPresentModes = lowLatencyPresentModes.data();

    bool allowLowLatency = m_Device.GetDesc().features.lowLatency && (swapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY);
    if (allowLowLatency) {
        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        surfaceInfo.surface = m_Surface;

        VkSurfaceCapabilities2KHR surfaceCaps2 = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};

        PNEXTCHAIN_DECLARE(surfaceCaps2.pNext);
        PNEXTCHAIN_APPEND_STRUCT(latencySurfaceCaps);

        VkResult vkResult = vk.GetPhysicalDeviceSurfaceCapabilities2KHR(m_Device, &surfaceInfo, &surfaceCaps2);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
    }

    // Surface format
    VkSurfaceFormat2KHR surfaceFormat = {VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};
    {
        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        surfaceInfo.surface = m_Surface;

        uint32_t formatNum = 0;
        VkResult vkResult = vk.GetPhysicalDeviceSurfaceFormats2KHR(m_Device, &surfaceInfo, &formatNum, nullptr);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceSurfaceFormats2KHR");

        Scratch<VkSurfaceFormat2KHR> surfaceFormats = NRI_ALLOCATE_SCRATCH(m_Device, VkSurfaceFormat2KHR, formatNum);
        for (uint32_t i = 0; i < formatNum; i++)
            surfaceFormats[i] = {VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};

        vkResult = vk.GetPhysicalDeviceSurfaceFormats2KHR(m_Device, &surfaceInfo, &formatNum, surfaceFormats);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceSurfaceFormats2KHR");

        auto priority_BT709_G22_16BIT = [](const VkSurfaceFormat2KHR& s) -> uint32_t {
            if (s.surfaceFormat.format != VK_FORMAT_R16G16B16A16_SFLOAT)
                return 0;

            // Only scRGB (extended range, linear)
            return s.surfaceFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT ? 1 : 0;
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
            if (s.surfaceFormat.format != VK_FORMAT_A2B10G10R10_UNORM_PACK32)
                return 0;

            // Only HDR10 (BT.2020 primaries, PQ)
            return s.surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT ? 1 : 0;
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
            default:
                NRI_CHECK(false, "Unexpected 'swapChainDesc.format'");
                break;
        }

        // HDR formats don't fall back to another color space: the content would be shown wrong, and the texture format wouldn't tell
        uint32_t priority = 1;
        if (swapChainDesc.format == SwapChainFormat::BT709_G10_16BIT)
            priority = formatNum ? priority_BT709_G22_16BIT(surfaceFormats[0]) : 0;
        else if (swapChainDesc.format == SwapChainFormat::BT2020_G2084_10BIT)
            priority = formatNum ? priority_BT2020_G2084_10BIT(surfaceFormats[0]) : 0;

        if (priority == 0) {
            NRI_REPORT_WARNING(&m_Device, "The surface has no format in the color space of 'swapChainDesc.format=%u'", (uint32_t)swapChainDesc.format);
            return Result::UNSUPPORTED;
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
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceSurfacePresentModesKHR");

        // "LATEST_READY" with "VSYNC": queued like "FIFO", but the newest ready image is taken at vblank
        bool latestReady = (swapChainDesc.flags & SwapChainBits::LATEST_READY) && m_Device.m_IsSupported.fifoLatestReady;
        VkPresentModeKHR cappedMode = (swapChainDesc.flags & SwapChainBits::ALLOW_TEARING) ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_FIFO_KHR;

        const VkPresentModeKHR cappedModes[] = {
            latestReady ? VK_PRESENT_MODE_FIFO_LATEST_READY_EXT : cappedMode,
            cappedMode,
            VK_PRESENT_MODE_FIFO_KHR, // guaranteed to be supported
        };

        const VkPresentModeKHR uncappedModes[] = {
            (swapChainDesc.flags & SwapChainBits::ALLOW_TEARING) ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_MAILBOX_KHR,
            (swapChainDesc.flags & SwapChainBits::ALLOW_TEARING) ? VK_PRESENT_MODE_IMMEDIATE_KHR : (m_Device.m_IsSupported.fifoLatestReady ? VK_PRESENT_MODE_FIFO_LATEST_READY_EXT : VK_PRESENT_MODE_FIFO_KHR),
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
            availableModeNum = latencySurfaceCaps.presentModeCount;
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

    // Scaling mode and caps
    VkExtent2D swapChainExtent = {};
    bool isScalingSupported = false;
    uint32_t textureNum = swapChainDesc.textureNum;
    VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkImageUsageFlags swapchainImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    {
        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        surfaceInfo.surface = m_Surface;

        VkSurfacePresentModeKHR surfacePresentMode = {VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_KHR};
        surfacePresentMode.presentMode = presentMode;

        VkSurfaceCapabilities2KHR surfaceCaps2 = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};

        VkSurfacePresentScalingCapabilitiesKHR surfacePresentScalingCaps = {VK_STRUCTURE_TYPE_SURFACE_PRESENT_SCALING_CAPABILITIES_KHR};

        PNEXTCHAIN_DECLARE(surfaceCaps2.pNext);
        if (m_Device.m_IsSupported.swapChainMaintenance1)
            PNEXTCHAIN_APPEND_STRUCT(surfacePresentScalingCaps);

        PNEXTCHAIN_SET(surfaceInfo.pNext);
        if (m_Device.m_IsSupported.swapChainMaintenance1)
            PNEXTCHAIN_APPEND_STRUCT(surfacePresentMode);

        VkResult vkResult = vk.GetPhysicalDeviceSurfaceCapabilities2KHR(m_Device, &surfaceInfo, &surfaceCaps2);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");

        // TODO: that's the minimal check to detect scaling support
        if (surfacePresentScalingCaps.supportedPresentScaling != 0 && surfacePresentScalingCaps.supportedPresentGravityX != 0 && surfacePresentScalingCaps.supportedPresentGravityY != 0)
            isScalingSupported = true;

        // Caps
        const VkSurfaceCapabilitiesKHR& surfaceCaps = surfaceCaps2.surfaceCapabilities;
        VkExtent2D desiredExtent = { swapChainDesc.width, swapChainDesc.height };
        if (isScalingSupported) {
            swapChainExtent.width = std::max(surfacePresentScalingCaps.minScaledImageExtent.width, std::min(surfacePresentScalingCaps.maxScaledImageExtent.width, desiredExtent.width));
            swapChainExtent.height = std::max(surfacePresentScalingCaps.minScaledImageExtent.height, std::min(surfacePresentScalingCaps.maxScaledImageExtent.height, desiredExtent.height));
        } else if (surfaceCaps.currentExtent.width != UINT32_MAX) {
            swapChainExtent = surfaceCaps.currentExtent;
        } else {
            swapChainExtent.width = std::max(surfaceCaps.minImageExtent.width, std::min(surfaceCaps.maxImageExtent.width, desiredExtent.width));
            swapChainExtent.height = std::max(surfaceCaps.minImageExtent.height, std::min(surfaceCaps.maxImageExtent.height, desiredExtent.height));
        }

        // A minimized surface may have no drawable extent
        if (swapChainExtent.width == 0 || swapChainExtent.height == 0)
            return Result::NOT_READY;

        // Silently clamp "textureNum" to the supported range
        if (textureNum < surfaceCaps.minImageCount)
            textureNum = surfaceCaps.minImageCount;
        if (surfaceCaps.maxImageCount && textureNum > surfaceCaps.maxImageCount) // 0 - unlimited (see spec)
            textureNum = surfaceCaps.maxImageCount;

        if (textureNum != swapChainDesc.textureNum)
            NRI_REPORT_WARNING(&m_Device, "'swapChainDesc.textureNum=%u' clamped to %u", swapChainDesc.textureNum, textureNum);

        if (!(surfaceCaps.supportedTransforms & preTransform))
            preTransform = surfaceCaps.currentTransform;

        if (!(surfaceCaps.supportedCompositeAlpha & compositeAlpha)) {
            if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
                compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
            else if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
                compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
            else
                compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        }

        // VK guarantees only COLOR_ATTACHMENT usage for swap chain images; transfer usages are optional
        swapchainImageUsageFlags |= surfaceCaps.supportedUsageFlags & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    }

    { // Swap chain
        VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchainInfo.flags = 0;
        swapchainInfo.surface = m_Surface;
        swapchainInfo.minImageCount = textureNum;
        swapchainInfo.imageFormat = surfaceFormat.surfaceFormat.format;
        swapchainInfo.imageColorSpace = surfaceFormat.surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = {swapChainExtent.width, swapChainExtent.height};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = swapchainImageUsageFlags;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 1;
        swapchainInfo.pQueueFamilyIndices = &familyIndex;
        swapchainInfo.preTransform = preTransform;
        swapchainInfo.compositeAlpha = compositeAlpha;
        swapchainInfo.presentMode = presentMode;
        PNEXTCHAIN_DECLARE(swapchainInfo.pNext);

        VkSwapchainPresentScalingCreateInfoKHR scalingInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_KHR};
        if (isScalingSupported) {
            scalingInfo.scalingBehavior = swapChainDesc.scaling == Scaling::STRETCH ? VK_PRESENT_SCALING_STRETCH_BIT_KHR : VK_PRESENT_SCALING_ONE_TO_ONE_BIT_KHR;
            scalingInfo.presentGravityX = GetGravity(swapChainDesc.gravityX);
            scalingInfo.presentGravityY = GetGravity(swapChainDesc.gravityY);
            PNEXTCHAIN_APPEND_STRUCT(scalingInfo);
        }

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
            default:
                break;
        }

        VkImageFormatListCreateInfo imageFormatListCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO};
        imageFormatListCreateInfo.pViewFormats = mutableFormats;
        imageFormatListCreateInfo.viewFormatCount = mutableFormatNum;

        if (m_Device.m_IsSupported.swapChainMutableFormat) {
            swapchainInfo.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
            PNEXTCHAIN_APPEND_STRUCT(imageFormatListCreateInfo);
        }

        // Low latency mode
        VkSwapchainLatencyCreateInfoNV latencyCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_LATENCY_CREATE_INFO_NV};
        latencyCreateInfo.latencyModeEnable = allowLowLatency;

        if (allowLowLatency)
            PNEXTCHAIN_APPEND_STRUCT(latencyCreateInfo);

        // Create
        VkResult vkResult = vk.CreateSwapchainKHR(m_Device, &swapchainInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateSwapchainKHR");
    }

#ifndef _WIN32
    // HDR metadata
    const bool isHdr = swapChainDesc.format == SwapChainFormat::BT709_G10_16BIT || swapChainDesc.format == SwapChainFormat::BT2020_G2084_10BIT;
    if (isHdr && swapChainDesc.hdrMetadata && vk.SetHdrMetadataEXT) {
        const HdrMetadata& hdrMetadata = *swapChainDesc.hdrMetadata;

        VkHdrMetadataEXT metadata = {VK_STRUCTURE_TYPE_HDR_METADATA_EXT};
        metadata.displayPrimaryRed = {hdrMetadata.redPrimary.x, hdrMetadata.redPrimary.y};
        metadata.displayPrimaryGreen = {hdrMetadata.greenPrimary.x, hdrMetadata.greenPrimary.y};
        metadata.displayPrimaryBlue = {hdrMetadata.bluePrimary.x, hdrMetadata.bluePrimary.y};
        metadata.whitePoint = {hdrMetadata.whitePoint.x, hdrMetadata.whitePoint.y};
        metadata.maxLuminance = hdrMetadata.maxLuminance;
        metadata.minLuminance = hdrMetadata.minLuminance;
        metadata.maxContentLightLevel = hdrMetadata.maxContentLightLevel;
        metadata.maxFrameAverageLightLevel = hdrMetadata.maxFrameAverageLightLevel;

        vk.SetHdrMetadataEXT(m_Device, 1, &m_Handle, &metadata);
    }
#endif

    // Textures
    Result result = CreateTextures(surfaceFormat.surfaceFormat.format, swapchainImageUsageFlags, (Dim_t)swapChainExtent.width, (Dim_t)swapChainExtent.height, 1);

    if (result != Result::SUCCESS)
        return result;

    // Latency fence
    if (allowLowLatency) {
        FenceDesc fenceDesc = {};
        result = m_Device.CreateImplementation<FenceVK>(m_LatencyFence, fenceDesc);

        if (result != Result::SUCCESS)
            return result;
    }

    // Finalize
    m_Hwnd = swapChainDesc.window.windows.hwnd;
    m_Flags = swapChainDesc.flags;
    if (!allowLowLatency)
        m_Flags &= ~SwapChainBits::ALLOW_LOW_LATENCY;
    if (!m_Device.GetDesc().features.waitableSwapChain)
        m_Flags &= ~SwapChainBits::WAITABLE;

    if (presentMode != VK_PRESENT_MODE_FIFO_LATEST_READY_EXT)
        m_Flags &= ~SwapChainBits::LATEST_READY;

    return Result::SUCCESS;
}

Result SwapChainVK::Create(const SwapChainVKDesc& swapChainVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkSwapchainKHR)swapChainVKDesc.vkSwapChain;
    m_Queue = (QueueVK*)swapChainVKDesc.queue;
    m_Hwnd = swapChainVKDesc.window.hwnd;
    m_Flags = swapChainVKDesc.flags;

    Result result = CreateTextures((VkFormat)swapChainVKDesc.vkFormat, swapChainVKDesc.vkImageUsageFlags, swapChainVKDesc.width, swapChainVKDesc.height, swapChainVKDesc.layerNum);

    if (result != Result::SUCCESS)
        return result;

    if (m_Flags & SwapChainBits::ALLOW_LOW_LATENCY) {
        FenceDesc fenceDesc = {};
        result = m_Device.CreateImplementation<FenceVK>(m_LatencyFence, fenceDesc);

        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

Result SwapChainVK::CreateTextures(VkFormat format, VkImageUsageFlags usage, Dim_t width, Dim_t height, Dim_t layerNum) {
    const auto& vk = m_Device.GetDispatchTable();
    uint32_t imageNum = 0;
    VkResult vkResult = vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, nullptr);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetSwapchainImagesKHR");

    Scratch<VkImage> imageHandles = NRI_ALLOCATE_SCRATCH(m_Device, VkImage, imageNum);
    vkResult = vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, imageHandles);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetSwapchainImagesKHR");

    m_Textures.resize(imageNum);

    for (uint32_t i = 0; i < imageNum; i++) {
        TextureVKDesc desc = {};
        desc.vkImage = (VKNonDispatchableHandle)imageHandles[i];
        desc.vkFormat = format;
        desc.vkImageType = VK_IMAGE_TYPE_2D;
        desc.vkImageUsageFlags = usage;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.mipNum = 1;
        desc.layerNum = layerNum;
        desc.sampleNum = 1;

        Result result = m_Device.CreateImplementation<TextureVK>(m_Textures[i], desc);

        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

NRI_INLINE void SwapChainVK::SetDebugName(const char* name) {
    if (m_Surface)
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SURFACE_KHR, (uint64_t)m_Surface, name);

    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SWAPCHAIN_KHR, (uint64_t)m_Handle, name);
}

NRI_INLINE Texture* const* SwapChainVK::GetTextures(uint32_t& textureNum) const {
    textureNum = (uint32_t)m_Textures.size();

    return (Texture* const*)m_Textures.data();
}

NRI_INLINE Result SwapChainVK::AcquireNextTexture(FenceVK& acquireSemaphore, uint32_t& textureIndex) {
    ExclusiveScope lock(m_Queue->GetLock());

    // Acquire next image (signal)
    VkAcquireNextImageInfoKHR acquireInfo = {VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR};
    acquireInfo.swapchain = m_Handle;
    acquireInfo.timeout = MsToUs(NRI_TIMEOUT_PRESENT);
    acquireInfo.semaphore = acquireSemaphore;
    acquireInfo.deviceMask = NODE_MASK;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.AcquireNextImage2KHR(m_Device, &acquireInfo, &m_TextureIndex);
    if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
        return Result::OUT_OF_DATE;

    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "AcquireNextImage2KHR");

    textureIndex = m_TextureIndex;

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainVK::WaitForPresent(uint64_t presentId) {
    if (!(m_Flags & SwapChainBits::WAITABLE) || presentId == 0)
        return Result::UNSUPPORTED;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.WaitForPresentKHR(m_Device, m_Handle, presentId, MsToUs(NRI_TIMEOUT_PRESENT));
    if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
        return Result::OUT_OF_DATE;

    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "WaitForPresentKHR");

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainVK::Present(FenceVK& releaseSemaphore, uint64_t presentIdValue) {
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
    presentId.pPresentIds = &presentIdValue;

    if (m_Device.m_IsSupported.presentId)
        presentInfo.pNext = &presentId;

    if ((m_Flags & SwapChainBits::ALLOW_LOW_LATENCY) && presentIdValue != 0)
        SetLatencyMarker(presentIdValue, (LatencyMarker)VK_LATENCY_MARKER_PRESENT_START_NV);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.QueuePresentKHR(*m_Queue, &presentInfo);

    if ((m_Flags & SwapChainBits::ALLOW_LOW_LATENCY) && presentIdValue != 0)
        SetLatencyMarker(presentIdValue, (LatencyMarker)VK_LATENCY_MARKER_PRESENT_END_NV);

    if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
        return Result::OUT_OF_DATE;

    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "QueuePresentKHR");

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainVK::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
    VkLatencySleepModeInfoNV sleepModeInfo = {VK_STRUCTURE_TYPE_LATENCY_SLEEP_MODE_INFO_NV};
    sleepModeInfo.lowLatencyMode = latencySleepMode.lowLatencyMode;
    sleepModeInfo.lowLatencyBoost = latencySleepMode.lowLatencyBoost;
    sleepModeInfo.minimumIntervalUs = latencySleepMode.minIntervalUs;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.SetLatencySleepModeNV(m_Device, m_Handle, &sleepModeInfo);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "SetLatencySleepModeNV");

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainVK::SetLatencyMarker(uint64_t presentId, LatencyMarker latencyMarker) {
    VkSetLatencyMarkerInfoNV markerInfo = {VK_STRUCTURE_TYPE_SET_LATENCY_MARKER_INFO_NV};
    markerInfo.presentID = presentId;
    markerInfo.marker = (VkLatencyMarkerNV)latencyMarker;

    const auto& vk = m_Device.GetDispatchTable();
    vk.SetLatencyMarkerNV(m_Device, m_Handle, &markerInfo);

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainVK::LatencySleep(uint64_t presentId) {
    VkLatencySleepInfoNV sleepInfo = {VK_STRUCTURE_TYPE_LATENCY_SLEEP_INFO_NV};
    sleepInfo.signalSemaphore = *m_LatencyFence;
    sleepInfo.value = presentId;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.LatencySleepNV(m_Device, m_Handle, &sleepInfo);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "LatencySleepNV");

    m_LatencyFence->Wait(presentId); // VK_SUCCESS is guaranteed by "NRI_RETURN_ON_BAD_VKRESULT"

    return Result::SUCCESS;
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
