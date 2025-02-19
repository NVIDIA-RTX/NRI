// © 2021 NVIDIA Corporation

constexpr VkBufferUsageFlags GetBufferUsageFlags(BufferUsageBits bufferUsageBits, uint32_t structureStride, bool isDeviceAddressSupported) {
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (isDeviceAddressSupported)
        flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (bufferUsageBits & BufferUsageBits::VERTEX_BUFFER)
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::INDEX_BUFFER)
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::CONSTANT_BUFFER)
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::ARGUMENT_BUFFER)
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::SCRATCH_BUFFER)
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::SHADER_BINDING_TABLE)
        flags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;

    if (bufferUsageBits & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE)
        flags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

    if (bufferUsageBits & BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_INPUT)
        flags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    if (bufferUsageBits & BufferUsageBits::SHADER_RESOURCE)
        flags |= structureStride ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::SHADER_RESOURCE_STORAGE)
        flags |= structureStride ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

    return flags;
}

constexpr VkImageUsageFlags GetImageUsageFlags(TextureUsageBits textureUsageBits) {
    VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (textureUsageBits & TextureUsageBits::SHADER_RESOURCE)
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

    if (textureUsageBits & TextureUsageBits::SHADER_RESOURCE_STORAGE)
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;

    if (textureUsageBits & TextureUsageBits::COLOR_ATTACHMENT)
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (textureUsageBits & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT)
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (textureUsageBits & TextureUsageBits::SHADING_RATE_ATTACHMENT)
        flags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;

    return flags;
}

inline bool IsExtensionSupported(const char* ext, const Vector<VkExtensionProperties>& list) {
    for (auto& e : list) {
        if (!strcmp(ext, e.extensionName))
            return true;
    }

    return false;
}

inline bool IsExtensionSupported(const char* ext, const Vector<const char*>& list) {
    for (auto& e : list) {
        if (!strcmp(ext, e))
            return true;
    }

    return false;
}

static void* VKAPI_PTR vkAllocateHostMemory(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pUserData;

    return allocationCallbacks.Allocate(allocationCallbacks.userArg, size, alignment);
}

static void* VKAPI_PTR vkReallocateHostMemory(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pUserData;

    return allocationCallbacks.Reallocate(allocationCallbacks.userArg, pOriginal, size, alignment);
}

static void VKAPI_PTR vkFreeHostMemory(void* pUserData, void* pMemory) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pUserData;

    return allocationCallbacks.Free(allocationCallbacks.userArg, pMemory);
}

void DeviceVK::FilterInstanceLayers(Vector<const char*>& layers) {
    uint32_t layerNum = 0;
    m_VK.EnumerateInstanceLayerProperties(&layerNum, nullptr);

    Vector<VkLayerProperties> supportedLayers(layerNum, GetStdAllocator());
    m_VK.EnumerateInstanceLayerProperties(&layerNum, supportedLayers.data());

    for (size_t i = 0; i < layers.size(); i++) {
        bool found = false;
        for (uint32_t j = 0; j < layerNum && !found; j++) {
            if (strcmp(supportedLayers[j].layerName, layers[i]) == 0)
                found = true;
        }

        if (!found)
            layers.erase(layers.begin() + i--);
    }
}

void DeviceVK::ProcessInstanceExtensions(Vector<const char*>& desiredInstanceExts) {
    // Query extensions
    uint32_t extensionNum = 0;
    m_VK.EnumerateInstanceExtensionProperties(nullptr, &extensionNum, nullptr);

    Vector<VkExtensionProperties> supportedExts(extensionNum, GetStdAllocator());
    m_VK.EnumerateInstanceExtensionProperties(nullptr, &extensionNum, supportedExts.data());

    REPORT_INFO(this, "Supported instance extensions:");
    for (const VkExtensionProperties& props : supportedExts)
        REPORT_INFO(this, "    %s (v%u)", props.extensionName, props.specVersion);

#ifdef __APPLE__
    // Mandatory
    desiredInstanceExts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    desiredInstanceExts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    // Optional
    if (IsExtensionSupported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, supportedExts))
        desiredInstanceExts.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, supportedExts)) {
        desiredInstanceExts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef VK_USE_PLATFORM_WIN32_KHR
        desiredInstanceExts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
        desiredInstanceExts.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        desiredInstanceExts.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        desiredInstanceExts.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
    }

    if (IsExtensionSupported(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME, supportedExts))
        desiredInstanceExts.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, supportedExts))
        desiredInstanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

void DeviceVK::ProcessDeviceExtensions(Vector<const char*>& desiredDeviceExts, bool disableRayTracing) {
    // Query extensions
    uint32_t extensionNum = 0;
    m_VK.EnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionNum, nullptr);

    Vector<VkExtensionProperties> supportedExts(extensionNum, GetStdAllocator());
    m_VK.EnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionNum, supportedExts.data());

    REPORT_INFO(this, "Supported device extensions:");
    for (const VkExtensionProperties& props : supportedExts)
        REPORT_INFO(this, "    %s (v%u)", props.extensionName, props.specVersion);

    // Mandatory
    if (m_MinorVersion < 3) {
        desiredDeviceExts.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        desiredDeviceExts.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        desiredDeviceExts.push_back(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
        desiredDeviceExts.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    }

#ifdef __APPLE__
    if (IsExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    // Optional for Vulkan < 1.3
    if (m_MinorVersion < 3 && IsExtensionSupported(VK_KHR_MAINTENANCE_4_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);

    if (m_MinorVersion < 3 && IsExtensionSupported(VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME);

    // Optional (KHR)
    if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_PRESENT_ID_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_MAINTENANCE_5_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_MAINTENANCE_6_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_MAINTENANCE_6_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_RAY_QUERY_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_SHADER_CLOCK_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);

    // Optional (EXT)
    if (IsExtensionSupported(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_IMAGE_SLICED_VIEW_OF_3D_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_IMAGE_SLICED_VIEW_OF_3D_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);

    // Optional
    if (IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_NV_LOW_LATENCY_2_EXTENSION_NAME);

    if (IsExtensionSupported(VK_NVX_BINARY_IMPORT_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_NVX_BINARY_IMPORT_EXTENSION_NAME);

    if (IsExtensionSupported(VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME);

    // Dependencies
    if (IsExtensionSupported(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
}

DeviceVK::DeviceVK(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks)
    : DeviceBase(callbacks, allocationCallbacks) {
    m_AllocationCallbacks.pUserData = (void*)&GetAllocationCallbacks();
    m_AllocationCallbacks.pfnAllocation = vkAllocateHostMemory;
    m_AllocationCallbacks.pfnReallocation = vkReallocateHostMemory;
    m_AllocationCallbacks.pfnFree = vkFreeHostMemory;

    m_Desc.graphicsAPI = GraphicsAPI::VK;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;
}

DeviceVK::~DeviceVK() {
    DestroyVma();

    for (auto& queueFamily : m_QueueFamilies) {
        for (uint32_t i = 0; i < queueFamily.size(); i++)
            Destroy<QueueVK>(queueFamily[i]);
    }

    if (m_Messenger) {
        typedef PFN_vkDestroyDebugUtilsMessengerEXT Func;
        Func destroyCallback = (Func)m_VK.GetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        destroyCallback(m_Instance, m_Messenger, m_AllocationCallbackPtr);
    }

    if (m_OwnsNativeObjects) {
        if (m_Device)
            m_VK.DestroyDevice(m_Device, m_AllocationCallbackPtr);

        if (m_Instance)
            m_VK.DestroyInstance(m_Instance, m_AllocationCallbackPtr);
    }

    if (m_Loader)
        UnloadSharedLibrary(*m_Loader);
}

Result DeviceVK::Create(const DeviceCreationDesc& desc, const DeviceCreationVKDesc& descVK) {
    bool isWrapper = descVK.vkDevice != nullptr;
    m_OwnsNativeObjects = !isWrapper;
    m_BindingOffsets = desc.vkBindingOffsets;

    if (!isWrapper && !GetAllocationCallbacks().disable3rdPartyAllocationCallbacks)
        m_AllocationCallbackPtr = &m_AllocationCallbacks;

    // Get adapter description as early as possible for meaningful error reporting
    m_Desc.adapterDesc = *desc.adapterDesc;

    { // Loader
        const char* loaderPath = descVK.libraryPath ? descVK.libraryPath : VULKAN_LOADER_NAME;
        m_Loader = LoadSharedLibrary(loaderPath);
        if (!m_Loader) {
            REPORT_ERROR(this, "Failed to load Vulkan loader: '%s'", loaderPath);
            return Result::UNSUPPORTED;
        }
    }

    // Create instance
    Vector<const char*> desiredInstanceExts(GetStdAllocator());
    {
        Result res = ResolvePreInstanceDispatchTable();
        if (res != Result::SUCCESS)
            return res;

        for (uint32_t i = 0; i < desc.vkExtensions.instanceExtensionNum; i++)
            desiredInstanceExts.push_back(desc.vkExtensions.instanceExtensions[i]);

        m_Instance = (VkInstance)descVK.vkInstance;

        if (!isWrapper) {
            ProcessInstanceExtensions(desiredInstanceExts);

            res = CreateInstance(desc.enableGraphicsAPIValidation, desiredInstanceExts);
            if (res != Result::SUCCESS)
                return res;
        }

        res = ResolveInstanceDispatchTable(desiredInstanceExts);
        if (res != Result::SUCCESS)
            return res;
    }

    { // Physical device
        m_MinorVersion = descVK.minorVersion;
        m_PhysicalDevice = (VkPhysicalDevice)descVK.vkPhysicalDevice;

        if (!isWrapper) {
            uint32_t deviceGroupNum = 0;
            VkResult result = m_VK.EnumeratePhysicalDeviceGroups(m_Instance, &deviceGroupNum, nullptr);
            RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkEnumeratePhysicalDeviceGroups returned %d", (int32_t)result);

            Scratch<VkPhysicalDeviceGroupProperties> deviceGroups = AllocateScratch(*this, VkPhysicalDeviceGroupProperties, deviceGroupNum);
            for (uint32_t j = 0; j < deviceGroupNum; j++) {
                deviceGroups[j].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
                deviceGroups[j].pNext = nullptr;
            }

            result = m_VK.EnumeratePhysicalDeviceGroups(m_Instance, &deviceGroupNum, deviceGroups);
            RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkEnumeratePhysicalDeviceGroups returned %d", (int32_t)result);

            uint32_t i = 0;
            for (i = 0; i < deviceGroupNum; i++) {
                VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

                VkPhysicalDeviceIDProperties idProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
                props.pNext = &idProps;

                m_VK.GetPhysicalDeviceProperties2(deviceGroups[i].physicalDevices[0], &props);

                uint32_t majorVersion = VK_VERSION_MAJOR(props.properties.apiVersion);
                m_MinorVersion = VK_VERSION_MINOR(props.properties.apiVersion);

                bool isSupported = (majorVersion * 10 + m_MinorVersion) >= 12;
                if (desc.adapterDesc) {
                    const uint64_t luid = *(uint64_t*)idProps.deviceLUID;
                    if (luid == desc.adapterDesc->luid) {
                        RETURN_ON_FAILURE(this, isSupported, Result::UNSUPPORTED, "Can't create a device: the specified physical device does not support Vulkan 1.2+!");
                        break;
                    }
                } else if (isSupported)
                    break;
            }

            RETURN_ON_FAILURE(this, i != deviceGroupNum, Result::INVALID_ARGUMENT, "Can't create a device: physical device not found");

            if (deviceGroups[i].physicalDeviceCount > 1 && deviceGroups[i].subsetAllocation == VK_FALSE)
                REPORT_WARNING(this, "The device group does not support memory allocation on a subset of the physical devices");

            m_PhysicalDevice = deviceGroups[i].physicalDevices[0];
        }
    }

    // Queue family indices
    std::array<uint32_t, (size_t)QueueType::MAX_NUM> queueFamilyIndices = {};
    queueFamilyIndices.fill(INVALID_FAMILY_INDEX);
    if (isWrapper) {
        for (uint32_t i = 0; i < descVK.queueFamilyNum; i++) {
            const QueueFamilyVKDesc& queueFamily = descVK.queueFamilies[i];
            queueFamilyIndices[(size_t)queueFamily.queueType] = queueFamily.familyIndex;
        }
    } else {
        uint32_t familyNum = 0;
        m_VK.GetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &familyNum, nullptr);

        Scratch<VkQueueFamilyProperties2> familyProps2 = AllocateScratch(*this, VkQueueFamilyProperties2, familyNum);
        for (uint32_t i = 0; i < familyNum; i++)
            familyProps2[i] = {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};

        m_VK.GetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &familyNum, familyProps2);

        std::array<uint32_t, (size_t)QueueType::MAX_NUM> scores = {};
        for (uint32_t i = 0; i < familyNum; i++) { // TODO: same code is used in "Creation.cpp"
            const VkQueueFamilyProperties& familyProps = familyProps2[i].queueFamilyProperties;

            bool graphics = familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            bool compute = familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT;
            bool copy = familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT;
            bool sparse = familyProps.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
            bool videoDecode = familyProps.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;
            bool videoEncode = familyProps.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
            bool protect = familyProps.queueFlags & VK_QUEUE_PROTECTED_BIT;
            bool opticalFlow = familyProps.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV;
            bool taken = false;

            { // Prefer as much features as possible
                size_t index = (size_t)QueueType::GRAPHICS;
                uint32_t score = GRAPHICS_QUEUE_SCORE;

                if (!taken && graphics && score > scores[index]) {
                    queueFamilyIndices[index] = i;
                    scores[index] = score;
                    taken = true;
                }
            }

            { // Prefer compute-only
                size_t index = (size_t)QueueType::COMPUTE;
                uint32_t score = COMPUTE_QUEUE_SCORE;

                if (!taken && compute && score > scores[index]) {
                    queueFamilyIndices[index] = i;
                    scores[index] = score;
                    taken = true;
                }
            }

            { // Prefer copy-only
                size_t index = (size_t)QueueType::COPY;
                uint32_t score = COPY_QUEUE_SCORE;

                if (!taken && copy && score > scores[index]) {
                    queueFamilyIndices[index] = i;
                    scores[index] = score;
                    taken = true;
                }
            }
        }
    }

    { // Memory props
        VkPhysicalDeviceMemoryProperties2 memoryProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
        m_VK.GetPhysicalDeviceMemoryProperties2(m_PhysicalDevice, &memoryProps);

        m_MemoryProps = memoryProps.memoryProperties;
    }

    // Device extensions
    Vector<const char*> desiredDeviceExts(GetStdAllocator());
    if (!isWrapper)
        ProcessDeviceExtensions(desiredDeviceExts, desc.disableVKRayTracing);

    for (uint32_t i = 0; i < desc.vkExtensions.deviceExtensionNum; i++)
        desiredDeviceExts.push_back(desc.vkExtensions.deviceExtensions[i]);

    // Device features
    VkPhysicalDeviceFeatures2 features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    void** tail = &features.pNext;

    VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    APPEND_EXT(features11);

    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    APPEND_EXT(features12);

    VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    if (m_MinorVersion >= 3) {
        APPEND_EXT(features13);
    }

#ifdef __APPLE__
    VkPhysicalDevicePortabilitySubsetFeaturesKHR portabilitySubsetFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(portabilitySubsetFeatures);
    }
#endif

    // Mandatory
    VkPhysicalDeviceSynchronization2Features synchronization2features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
    if (IsExtensionSupported(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(synchronization2features);
    }

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
    if (IsExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(dynamicRenderingFeatures);
    }

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(extendedDynamicStateFeatures);
    }

    // Optional (for Vulkan < 1.2)
    VkPhysicalDeviceMaintenance4Features maintenance4Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES};
    if (IsExtensionSupported(VK_KHR_MAINTENANCE_4_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(maintenance4Features);
    }

    VkPhysicalDeviceImageRobustnessFeatures imageRobustnessFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES};
    if (IsExtensionSupported(VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(imageRobustnessFeatures);
    }

    // Optional (KHR)
    VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_PRESENT_ID_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(presentIdFeatures);
    }

    VkPhysicalDevicePresentWaitFeaturesKHR presentWaitFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(presentWaitFeatures);
    }

    VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_MAINTENANCE_5_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(maintenance5Features);
    }

    VkPhysicalDeviceMaintenance6FeaturesKHR maintenance6Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_MAINTENANCE_6_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(maintenance6Features);
    }

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(shadingRateFeatures);
    }

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(rayTracingPipelineFeatures);
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(accelerationStructureFeatures);
    }

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_RAY_QUERY_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(rayQueryFeatures);
    }

    VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR rayTracingPositionFetchFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(rayTracingPositionFetchFeatures);
    }

    VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR rayTracingMaintenanceFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(rayTracingMaintenanceFeatures);
    }

    VkPhysicalDeviceLineRasterizationFeaturesKHR lineRasterizationFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(lineRasterizationFeatures);
    }

    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(fragmentShaderBarycentricFeatures);
    }

    VkPhysicalDeviceShaderClockFeaturesKHR shaderClockFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_SHADER_CLOCK_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(shaderClockFeatures);
    }

    // Optional (EXT)
    VkPhysicalDeviceOpacityMicromapFeaturesEXT micromapFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(micromapFeatures);
    }

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(meshShaderFeatures);
    }

    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(shaderAtomicFloatFeatures);
    }

    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(shaderAtomicFloat2Features);
    }

    VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(memoryPriorityFeatures);
    }

    VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT slicedViewFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_SLICED_VIEW_OF_3D_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_IMAGE_SLICED_VIEW_OF_3D_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(slicedViewFeatures);
    }

    VkPhysicalDeviceCustomBorderColorFeaturesEXT borderColorFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(borderColorFeatures);
    }

    VkPhysicalDeviceRobustness2FeaturesEXT robustness2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(robustness2Features);
    }

    VkPhysicalDevicePipelineRobustnessFeaturesEXT pipelineRobustnessFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(pipelineRobustnessFeatures);
    }

    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(fragmentShaderInterlockFeatures);
    }

    if (IsExtensionSupported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, desiredDeviceExts))
        m_IsSupported.memoryBudget = true;

    m_VK.GetPhysicalDeviceFeatures2(m_PhysicalDevice, &features);

    m_IsSupported.descriptorIndexing = features12.descriptorIndexing;
    m_IsSupported.deviceAddress = features12.bufferDeviceAddress;
    m_IsSupported.swapChainMutableFormat = IsExtensionSupported(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME, desiredDeviceExts);
    m_IsSupported.presentId = presentIdFeatures.presentId;
    m_IsSupported.lowLatency = presentIdFeatures.presentId != 0 && IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts);
    m_IsSupported.memoryPriority = memoryPriorityFeatures.memoryPriority;
    m_IsSupported.maintenance4 = features13.maintenance4 != 0 || maintenance4Features.maintenance4 != 0;
    m_IsSupported.maintenance5 = maintenance5Features.maintenance5;
    m_IsSupported.maintenance6 = maintenance6Features.maintenance6;
    m_IsSupported.imageSlicedView = slicedViewFeatures.imageSlicedViewOf3D != 0;
    m_IsSupported.customBorderColor = borderColorFeatures.customBorderColors != 0 && borderColorFeatures.customBorderColorWithoutFormat != 0;
    m_IsSupported.robustness = features.features.robustBufferAccess != 0 && (imageRobustnessFeatures.robustImageAccess != 0 || features13.robustImageAccess != 0);
    m_IsSupported.robustness2 = robustness2Features.robustBufferAccess2 != 0 && robustness2Features.robustImageAccess2 != 0;
    m_IsSupported.pipelineRobustness = pipelineRobustnessFeatures.pipelineRobustness;

    { // Check hard requirements
        bool hasDynamicRendering = features13.dynamicRendering != 0 || (dynamicRenderingFeatures.dynamicRendering != 0 && extendedDynamicStateFeatures.extendedDynamicState != 0);
        bool hasSynchronization2 = features13.synchronization2 != 0 || synchronization2features.synchronization2 != 0;

        RETURN_ON_FAILURE(this, hasDynamicRendering && hasSynchronization2, nri::Result::UNSUPPORTED, "'dynamicRendering' and 'synchronization2' are not supported by the device");
    }

    { // Create device
        if (isWrapper)
            m_Device = (VkDevice)descVK.vkDevice;
        else {
            // Disable undesired features
            if (desc.robustness == Robustness::DEFAULT || desc.robustness == Robustness::VK) {
                robustness2Features.robustBufferAccess2 = 0;
                robustness2Features.robustImageAccess2 = 0;
            } else if (desc.robustness == Robustness::OFF) {
                robustness2Features.robustBufferAccess2 = 0;
                robustness2Features.robustImageAccess2 = 0;
                features.features.robustBufferAccess = 0;
                features13.robustImageAccess = 0;
            }

            // Create device
            std::array<VkDeviceQueueCreateInfo, (size_t)QueueType::MAX_NUM> queueCreateInfos = {};

            VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            deviceCreateInfo.pNext = &features;
            deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
            deviceCreateInfo.enabledExtensionCount = (uint32_t)desiredDeviceExts.size();
            deviceCreateInfo.ppEnabledExtensionNames = desiredDeviceExts.data();

            std::array<float, 256> zeroPriorities = {};

            for (uint32_t i = 0; i < desc.queueFamilyNum; i++) {
                const QueueFamilyDesc& queueFamily = desc.queueFamilies[i];

                if (queueFamily.queueNum) {
                    VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[deviceCreateInfo.queueCreateInfoCount++];

                    queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
                    queueCreateInfo.queueCount = queueFamily.queueNum;
                    queueCreateInfo.queueFamilyIndex = queueFamilyIndices[(size_t)queueFamily.queueType];
                    queueCreateInfo.pQueuePriorities = queueFamily.queuePriorities ? queueFamily.queuePriorities : zeroPriorities.data();
                }
            }

            VkResult result = m_VK.CreateDevice(m_PhysicalDevice, &deviceCreateInfo, m_AllocationCallbackPtr, &m_Device);
            RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkCreateDevice returned %d", (int32_t)result);
        }

        Result res = ResolveDispatchTable(desiredDeviceExts);
        if (res != Result::SUCCESS)
            return res;
    }

    // Create queues
    memset(m_Desc.adapterDesc.queueNum, 0, sizeof(m_Desc.adapterDesc.queueNum)); // patch to reflect available queues
    if (isWrapper) {
        for (uint32_t i = 0; i < descVK.queueFamilyNum; i++) {
            const QueueFamilyVKDesc& queueFamilyDesc = descVK.queueFamilies[i];
            auto& queueFamily = m_QueueFamilies[(size_t)queueFamilyDesc.queueType];

            for (uint32_t j = 0; j < queueFamilyDesc.queueNum; j++) {
                VkDeviceQueueInfo2 queueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
                queueInfo.queueFamilyIndex = queueFamilyIndices[(size_t)queueFamilyDesc.queueType];
                queueInfo.queueIndex = j;

                VkQueue handle = VK_NULL_HANDLE;
                m_VK.GetDeviceQueue2(m_Device, &queueInfo, &handle);

                QueueVK* queue;
                Result result = CreateImplementation<QueueVK>(queue, queueFamilyDesc.queueType, queueFamilyDesc.familyIndex, handle);
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
                VkDeviceQueueInfo2 queueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
                queueInfo.queueFamilyIndex = queueFamilyIndices[(size_t)queueFamilyDesc.queueType];
                queueInfo.queueIndex = j;

                VkQueue handle = VK_NULL_HANDLE;
                m_VK.GetDeviceQueue2(m_Device, &queueInfo, &handle);

                QueueVK* queue;
                Result result = CreateImplementation<QueueVK>(queue, queueFamilyDesc.queueType, queueInfo.queueFamilyIndex, handle);
                if (result == Result::SUCCESS)
                    queueFamily.push_back(queue);
            }

            m_Desc.adapterDesc.queueNum[(size_t)queueFamilyDesc.queueType] = queueFamilyDesc.queueNum;
        }
    }

    { // Desc
        // Device properties
        VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        tail = &props.pNext;

        VkPhysicalDeviceVulkan11Properties props11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
        APPEND_EXT(props11);

        VkPhysicalDeviceVulkan12Properties props12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
        APPEND_EXT(props12);

        VkPhysicalDeviceVulkan13Properties props13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
        if (m_MinorVersion >= 3) {
            APPEND_EXT(props13);
        }

        VkPhysicalDeviceMaintenance4PropertiesKHR maintenance4Props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_MAINTENANCE_4_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(maintenance4Props);
        }

        VkPhysicalDeviceMaintenance5PropertiesKHR maintenance5Props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_MAINTENANCE_5_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(maintenance5Props);
        }

        VkPhysicalDeviceMaintenance6PropertiesKHR maintenance6Props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_MAINTENANCE_6_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(maintenance6Props);
        }

        VkPhysicalDeviceLineRasterizationPropertiesKHR lineRasterizationProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(lineRasterizationProps);
        }

        VkPhysicalDeviceFragmentShadingRatePropertiesKHR shadingRateProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(shadingRateProps);
        }

        VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(pushDescriptorProps);
        }

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(rayTracingProps);
        }

        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(accelerationStructureProps);
        }

        VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT};
        if (IsExtensionSupported(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(conservativeRasterProps);
            m_Desc.conservativeRasterTier = 1;
        }

        VkPhysicalDeviceSampleLocationsPropertiesEXT sampleLocationsProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT};
        if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(sampleLocationsProps);
            m_Desc.sampleLocationsTier = 1;
        }

        VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT};
        if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(meshShaderProps);
        }

        m_VK.GetPhysicalDeviceProperties2(m_PhysicalDevice, &props);

        // Fill desc
        const VkPhysicalDeviceLimits& limits = props.properties.limits;

        m_Desc.viewportMaxNum = limits.maxViewports;
        m_Desc.viewportBoundsRange[0] = int32_t(limits.viewportBoundsRange[0]);
        m_Desc.viewportBoundsRange[1] = int32_t(limits.viewportBoundsRange[1]);

        m_Desc.attachmentMaxDim = (Dim_t)std::min(limits.maxFramebufferWidth, limits.maxFramebufferHeight);
        m_Desc.attachmentLayerMaxNum = (Dim_t)limits.maxFramebufferLayers;
        m_Desc.colorAttachmentMaxNum = (Dim_t)limits.maxColorAttachments;

        m_Desc.colorSampleMaxNum = (Sample_t)limits.framebufferColorSampleCounts;
        m_Desc.depthSampleMaxNum = (Sample_t)limits.framebufferDepthSampleCounts;
        m_Desc.stencilSampleMaxNum = (Sample_t)limits.framebufferStencilSampleCounts;
        m_Desc.zeroAttachmentsSampleMaxNum = (Sample_t)limits.framebufferNoAttachmentsSampleCounts;
        m_Desc.textureColorSampleMaxNum = (Sample_t)limits.sampledImageColorSampleCounts;
        m_Desc.textureIntegerSampleMaxNum = (Sample_t)limits.sampledImageIntegerSampleCounts;
        m_Desc.textureDepthSampleMaxNum = (Sample_t)limits.sampledImageDepthSampleCounts;
        m_Desc.textureStencilSampleMaxNum = (Sample_t)limits.sampledImageStencilSampleCounts;
        m_Desc.storageTextureSampleMaxNum = (Sample_t)limits.storageImageSampleCounts;

        m_Desc.texture1DMaxDim = (Dim_t)limits.maxImageDimension1D;
        m_Desc.texture2DMaxDim = (Dim_t)limits.maxImageDimension2D;
        m_Desc.texture3DMaxDim = (Dim_t)limits.maxImageDimension3D;
        m_Desc.textureArrayLayerMaxNum = (Dim_t)limits.maxImageArrayLayers;
        m_Desc.typedBufferMaxDim = limits.maxTexelBufferElements;

        const VkMemoryPropertyFlags neededFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
            const VkMemoryType& memoryType = m_MemoryProps.memoryTypes[i];
            if ((memoryType.propertyFlags & neededFlags) == neededFlags)
                m_Desc.deviceUploadHeapSize += m_MemoryProps.memoryHeaps[memoryType.heapIndex].size;
        }

        m_Desc.memoryAllocationMaxNum = limits.maxMemoryAllocationCount;
        m_Desc.samplerAllocationMaxNum = limits.maxSamplerAllocationCount;
        m_Desc.constantBufferMaxRange = limits.maxUniformBufferRange;
        m_Desc.storageBufferMaxRange = limits.maxStorageBufferRange;
        m_Desc.bufferTextureGranularity = (uint32_t)limits.bufferImageGranularity;
        m_Desc.bufferMaxSize = m_MinorVersion >= 3 ? props13.maxBufferSize : maintenance4Props.maxBufferSize;

        m_Desc.uploadBufferTextureRowAlignment = (uint32_t)limits.optimalBufferCopyRowPitchAlignment;
        m_Desc.uploadBufferTextureSliceAlignment = (uint32_t)limits.optimalBufferCopyOffsetAlignment; // TODO: ?
        m_Desc.bufferShaderResourceOffsetAlignment = (uint32_t)std::max(limits.minTexelBufferOffsetAlignment, limits.minStorageBufferOffsetAlignment);
        m_Desc.constantBufferOffsetAlignment = (uint32_t)limits.minUniformBufferOffsetAlignment;
        m_Desc.scratchBufferOffsetAlignment = accelerationStructureProps.minAccelerationStructureScratchOffsetAlignment;
        m_Desc.shaderBindingTableAlignment = rayTracingProps.shaderGroupBaseAlignment;

        m_Desc.pipelineLayoutDescriptorSetMaxNum = limits.maxBoundDescriptorSets;
        m_Desc.pipelineLayoutRootConstantMaxSize = limits.maxPushConstantsSize;
        m_Desc.pipelineLayoutRootDescriptorMaxNum = pushDescriptorProps.maxPushDescriptors;

        m_Desc.perStageDescriptorSamplerMaxNum = limits.maxPerStageDescriptorSamplers;
        m_Desc.perStageDescriptorConstantBufferMaxNum = limits.maxPerStageDescriptorUniformBuffers;
        m_Desc.perStageDescriptorStorageBufferMaxNum = limits.maxPerStageDescriptorStorageBuffers;
        m_Desc.perStageDescriptorTextureMaxNum = limits.maxPerStageDescriptorSampledImages;
        m_Desc.perStageDescriptorStorageTextureMaxNum = limits.maxPerStageDescriptorStorageImages;
        m_Desc.perStageResourceMaxNum = limits.maxPerStageResources;

        m_Desc.descriptorSetSamplerMaxNum = limits.maxDescriptorSetSamplers;
        m_Desc.descriptorSetConstantBufferMaxNum = limits.maxDescriptorSetUniformBuffers;
        m_Desc.descriptorSetStorageBufferMaxNum = limits.maxDescriptorSetStorageBuffers;
        m_Desc.descriptorSetTextureMaxNum = limits.maxDescriptorSetSampledImages;
        m_Desc.descriptorSetStorageTextureMaxNum = limits.maxDescriptorSetStorageImages;

        m_Desc.vertexShaderAttributeMaxNum = limits.maxVertexInputAttributes;
        m_Desc.vertexShaderStreamMaxNum = limits.maxVertexInputBindings;
        m_Desc.vertexShaderOutputComponentMaxNum = limits.maxVertexOutputComponents;

        m_Desc.tessControlShaderGenerationMaxLevel = (float)limits.maxTessellationGenerationLevel;
        m_Desc.tessControlShaderPatchPointMaxNum = limits.maxTessellationPatchSize;
        m_Desc.tessControlShaderPerVertexInputComponentMaxNum = limits.maxTessellationControlPerVertexInputComponents;
        m_Desc.tessControlShaderPerVertexOutputComponentMaxNum = limits.maxTessellationControlPerVertexOutputComponents;
        m_Desc.tessControlShaderPerPatchOutputComponentMaxNum = limits.maxTessellationControlPerPatchOutputComponents;
        m_Desc.tessControlShaderTotalOutputComponentMaxNum = limits.maxTessellationControlTotalOutputComponents;
        m_Desc.tessEvaluationShaderInputComponentMaxNum = limits.maxTessellationEvaluationInputComponents;
        m_Desc.tessEvaluationShaderOutputComponentMaxNum = limits.maxTessellationEvaluationOutputComponents;

        m_Desc.geometryShaderInvocationMaxNum = limits.maxGeometryShaderInvocations;
        m_Desc.geometryShaderInputComponentMaxNum = limits.maxGeometryInputComponents;
        m_Desc.geometryShaderOutputComponentMaxNum = limits.maxGeometryOutputComponents;
        m_Desc.geometryShaderOutputVertexMaxNum = limits.maxGeometryOutputVertices;
        m_Desc.geometryShaderTotalOutputComponentMaxNum = limits.maxGeometryTotalOutputComponents;

        m_Desc.fragmentShaderInputComponentMaxNum = limits.maxFragmentInputComponents;
        m_Desc.fragmentShaderOutputAttachmentMaxNum = limits.maxFragmentOutputAttachments;
        m_Desc.fragmentShaderDualSourceAttachmentMaxNum = limits.maxFragmentDualSrcAttachments;

        m_Desc.computeShaderSharedMemoryMaxSize = limits.maxComputeSharedMemorySize;
        m_Desc.computeShaderWorkGroupMaxNum[0] = limits.maxComputeWorkGroupCount[0];
        m_Desc.computeShaderWorkGroupMaxNum[1] = limits.maxComputeWorkGroupCount[1];
        m_Desc.computeShaderWorkGroupMaxNum[2] = limits.maxComputeWorkGroupCount[2];
        m_Desc.computeShaderWorkGroupInvocationMaxNum = limits.maxComputeWorkGroupInvocations;
        m_Desc.computeShaderWorkGroupMaxDim[0] = limits.maxComputeWorkGroupSize[0];
        m_Desc.computeShaderWorkGroupMaxDim[1] = limits.maxComputeWorkGroupSize[1];
        m_Desc.computeShaderWorkGroupMaxDim[2] = limits.maxComputeWorkGroupSize[2];

        m_Desc.rayTracingShaderGroupIdentifierSize = rayTracingProps.shaderGroupHandleSize;
        m_Desc.rayTracingShaderTableMaxStride = rayTracingProps.maxShaderGroupStride;
        m_Desc.rayTracingShaderRecursionMaxDepth = rayTracingProps.maxRayRecursionDepth;
        m_Desc.rayTracingGeometryObjectMaxNum = (uint32_t)accelerationStructureProps.maxGeometryCount;

        m_Desc.meshControlSharedMemoryMaxSize = meshShaderProps.maxTaskSharedMemorySize;
        m_Desc.meshControlWorkGroupInvocationMaxNum = meshShaderProps.maxTaskWorkGroupInvocations;
        m_Desc.meshControlPayloadMaxSize = meshShaderProps.maxTaskPayloadSize;
        m_Desc.meshEvaluationOutputVerticesMaxNum = meshShaderProps.maxMeshOutputVertices;
        m_Desc.meshEvaluationOutputPrimitiveMaxNum = meshShaderProps.maxMeshOutputPrimitives;
        m_Desc.meshEvaluationOutputComponentMaxNum = meshShaderProps.maxMeshOutputComponents;
        m_Desc.meshEvaluationSharedMemoryMaxSize = meshShaderProps.maxMeshSharedMemorySize;
        m_Desc.meshEvaluationWorkGroupInvocationMaxNum = meshShaderProps.maxMeshWorkGroupInvocations;

        m_Desc.viewportPrecisionBits = limits.viewportSubPixelBits;
        m_Desc.subPixelPrecisionBits = limits.subPixelPrecisionBits;
        m_Desc.subTexelPrecisionBits = limits.subTexelPrecisionBits;
        m_Desc.mipmapPrecisionBits = limits.mipmapPrecisionBits;

        m_Desc.timestampFrequencyHz = uint64_t(1e9 / double(limits.timestampPeriod) + 0.5);
        m_Desc.drawIndirectMaxNum = limits.maxDrawIndirectCount;
        m_Desc.samplerLodBiasMin = -limits.maxSamplerLodBias;
        m_Desc.samplerLodBiasMax = limits.maxSamplerLodBias;
        m_Desc.samplerAnisotropyMax = limits.maxSamplerAnisotropy;
        m_Desc.texelOffsetMin = limits.minTexelOffset;
        m_Desc.texelOffsetMax = limits.maxTexelOffset;
        m_Desc.texelGatherOffsetMin = limits.minTexelGatherOffset;
        m_Desc.texelGatherOffsetMax = limits.maxTexelGatherOffset;
        m_Desc.clipDistanceMaxNum = limits.maxClipDistances;
        m_Desc.cullDistanceMaxNum = limits.maxCullDistances;
        m_Desc.combinedClipAndCullDistanceMaxNum = limits.maxCombinedClipAndCullDistances;
        m_Desc.viewMaxNum = features11.multiview ? props11.maxMultiviewViewCount : 1;
        m_Desc.shadingRateAttachmentTileSize = (uint8_t)shadingRateProps.minFragmentShadingRateAttachmentTexelSize.width;

        if (m_Desc.conservativeRasterTier) {
            if (conservativeRasterProps.primitiveOverestimationSize < 1.0f / 2.0f && conservativeRasterProps.degenerateTrianglesRasterized)
                m_Desc.conservativeRasterTier = 2;
            if (conservativeRasterProps.primitiveOverestimationSize <= 1.0 / 256.0f && conservativeRasterProps.degenerateTrianglesRasterized)
                m_Desc.conservativeRasterTier = 3;
        }

        if (m_Desc.sampleLocationsTier) {
            if (sampleLocationsProps.variableSampleLocations) // TODO: it's weird...
                m_Desc.sampleLocationsTier = 2;
        }

        m_Desc.rayTracingTier = accelerationStructureFeatures.accelerationStructure != 0;
        if (m_Desc.rayTracingTier) {
            if (rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect && rayQueryFeatures.rayQuery)
                m_Desc.rayTracingTier = 2;
        }

        m_Desc.shadingRateTier = shadingRateFeatures.pipelineFragmentShadingRate != 0;
        if (m_Desc.shadingRateTier) {
            if (shadingRateFeatures.primitiveFragmentShadingRate && shadingRateFeatures.attachmentFragmentShadingRate)
                m_Desc.shadingRateTier = 2;

            m_Desc.isAdditionalShadingRatesSupported = shadingRateProps.maxFragmentSize.height > 2 || shadingRateProps.maxFragmentSize.width > 2;
        }

        m_Desc.bindlessTier = m_IsSupported.descriptorIndexing ? 1 : 0;

        m_Desc.isGetMemoryDesc2Supported = m_IsSupported.maintenance4;
        m_Desc.isEnchancedBarrierSupported = true;
        m_Desc.isMemoryTier2Supported = true; // TODO: seems to be the best match

        m_Desc.isIndependentFrontAndBackStencilReferenceAndMasksSupported = true;
        m_Desc.isTextureFilterMinMaxSupported = features12.samplerFilterMinmax;
        m_Desc.isLogicFuncSupported = features.features.logicOp;
        m_Desc.isDepthBoundsTestSupported = features.features.depthBounds;
        m_Desc.isDrawIndirectCountSupported = features12.drawIndirectCount;
        m_Desc.isLineSmoothingSupported = lineRasterizationFeatures.smoothLines;
        m_Desc.isCopyQueueTimestampSupported = limits.timestampComputeAndGraphics;
        m_Desc.isMeshShaderPipelineStatsSupported = meshShaderFeatures.meshShaderQueries == VK_TRUE;
        m_Desc.isDynamicDepthBiasSupported = true;
        m_Desc.isViewportOriginBottomLeftSupported = true;
        m_Desc.isRegionResolveSupported = true;
        m_Desc.isLayerBasedMultiviewSupported = features11.multiview;
        m_Desc.isPresentFromComputeSupported = true;
        m_Desc.isWaitableSwapChainSupported = presentIdFeatures.presentId != 0 && presentWaitFeatures.presentWait != 0;;

        m_Desc.isShaderNativeI16Supported = features.features.shaderInt16;
        m_Desc.isShaderNativeF16Supported = features12.shaderFloat16;
        m_Desc.isShaderNativeI64Supported = features.features.shaderInt64;
        m_Desc.isShaderNativeF64Supported = features.features.shaderFloat64;
        m_Desc.isShaderAtomicsF16Supported = (shaderAtomicFloat2Features.shaderBufferFloat16Atomics || shaderAtomicFloat2Features.shaderSharedFloat16Atomics) ? true : false;
        m_Desc.isShaderAtomicsF32Supported = (shaderAtomicFloatFeatures.shaderBufferFloat32Atomics || shaderAtomicFloatFeatures.shaderSharedFloat32Atomics) ? true : false;
        m_Desc.isShaderAtomicsI64Supported = (features12.shaderBufferInt64Atomics || features12.shaderSharedInt64Atomics) ? true : false;
        m_Desc.isShaderAtomicsF64Supported = (shaderAtomicFloatFeatures.shaderBufferFloat64Atomics || shaderAtomicFloatFeatures.shaderSharedFloat64Atomics) ? true : false;
        m_Desc.isShaderViewportIndexSupported = features12.shaderOutputViewportIndex;
        m_Desc.isShaderLayerSupported = features12.shaderOutputLayer;
        m_Desc.isShaderClockSupported = (shaderClockFeatures.shaderDeviceClock || shaderClockFeatures.shaderSubgroupClock) ? true : false;
        m_Desc.isRasterizedOrderedViewSupported = fragmentShaderInterlockFeatures.fragmentShaderPixelInterlock != 0 && fragmentShaderInterlockFeatures.fragmentShaderSampleInterlock != 0;
        m_Desc.isBarycentricSupported = fragmentShaderBarycentricFeatures.fragmentShaderBarycentric;
        m_Desc.isRayTracingPositionFetchSupported = rayTracingPositionFetchFeatures.rayTracingPositionFetch;

        m_Desc.isSwapChainSupported = IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, desiredDeviceExts);
        m_Desc.isRayTracingSupported = m_Desc.rayTracingTier != 0;
        m_Desc.isMeshShaderSupported = meshShaderFeatures.meshShader != 0 && meshShaderFeatures.taskShader != 0;
        m_Desc.isLowLatencySupported = IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts);

        // Estimate shader model last since it depends on many "m_Desc" fields
        // Based on https://docs.vulkan.org/guide/latest/hlsl.html#_shader_model_coverage // TODO: code below needs to be improved
        m_Desc.shaderModel = 51;
        if (m_Desc.isShaderNativeI64Supported)
            m_Desc.shaderModel = 60;
        if (m_Desc.viewMaxNum > 1 || m_Desc.isBarycentricSupported)
            m_Desc.shaderModel = 61;
        if (m_Desc.isShaderNativeF16Supported || m_Desc.isShaderNativeI16Supported)
            m_Desc.shaderModel = 62;
        if (m_Desc.isRayTracingSupported)
            m_Desc.shaderModel = 63;
        if (m_Desc.shadingRateTier >= 2)
            m_Desc.shaderModel = 64;
        if (m_Desc.isMeshShaderSupported || m_Desc.rayTracingTier >= 2)
            m_Desc.shaderModel = 65;
        if (m_Desc.isShaderAtomicsI64Supported)
            m_Desc.shaderModel = 66;
        if (features.features.shaderStorageImageMultisample)
            m_Desc.shaderModel = 67;
    }

    ReportDeviceGroupInfo();

    return FillFunctionTable(m_iCore);
}

void DeviceVK::FillCreateInfo(const BufferDesc& bufferDesc, VkBufferCreateInfo& info) const {
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; // should be already set
    info.size = bufferDesc.size;
    info.usage = GetBufferUsageFlags(bufferDesc.usage, bufferDesc.structureStride, m_IsSupported.deviceAddress);
    info.sharingMode = m_NumActiveFamilyIndices > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE; // TODO: still no DCC on AMD with concurrent?
    info.queueFamilyIndexCount = m_NumActiveFamilyIndices;
    info.pQueueFamilyIndices = m_ActiveQueueFamilyIndices.data();
}

void DeviceVK::FillCreateInfo(const TextureDesc& textureDesc, VkImageCreateInfo& info) const {
    VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT; // typeless
    const FormatProps& formatProps = GetFormatProps(textureDesc.format);
    if (formatProps.blockWidth > 1)
        flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
    if (textureDesc.layerNum >= 6 && textureDesc.width == textureDesc.height)
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
    if (textureDesc.type == nri::TextureType::TEXTURE_3D)
        flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT; // allow 3D demotion to a set of layers // TODO: hook up "VK_EXT_image_2d_view_of_3d"?
    if (m_Desc.sampleLocationsTier && formatProps.isDepth)
        flags |= VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT;

    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; // should be already set
    info.flags = flags;
    info.imageType = ::GetImageType(textureDesc.type);
    info.format = ::GetVkFormat(textureDesc.format, true);
    info.extent.width = textureDesc.width;
    info.extent.height = std::max(textureDesc.height, (Dim_t)1);
    info.extent.depth = std::max(textureDesc.depth, (Dim_t)1);
    info.mipLevels = std::max(textureDesc.mipNum, (Mip_t)1);
    info.arrayLayers = std::max(textureDesc.layerNum, (Dim_t)1);
    info.samples = (VkSampleCountFlagBits)std::max(textureDesc.sampleNum, (Sample_t)1);
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = GetImageUsageFlags(textureDesc.usage);
    info.sharingMode = m_NumActiveFamilyIndices > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE; // TODO: still no DCC on AMD with concurrent?
    info.queueFamilyIndexCount = m_NumActiveFamilyIndices;
    info.pQueueFamilyIndices = m_ActiveQueueFamilyIndices.data();
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void DeviceVK::GetMemoryDesc2(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    FillCreateInfo(bufferDesc, createInfo);

    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    requirements.pNext = &dedicatedRequirements;

    VkDeviceBufferMemoryRequirements bufferMemoryRequirements = {VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS};
    bufferMemoryRequirements.pCreateInfo = &createInfo;

    const auto& vk = GetDispatchTable();
    vk.GetDeviceBufferMemoryRequirements(m_Device, &bufferMemoryRequirements, &requirements);

    MemoryTypeInfo memoryTypeInfo = {};
    memoryTypeInfo.mustBeDedicated = dedicatedRequirements.prefersDedicatedAllocation;

    memoryDesc = {};
    if (GetMemoryTypeInfo(memoryLocation, requirements.memoryRequirements.memoryTypeBits, memoryTypeInfo)) {
        memoryDesc.size = requirements.memoryRequirements.size;
        memoryDesc.alignment = (uint32_t)requirements.memoryRequirements.alignment;
        memoryDesc.type = Pack(memoryTypeInfo);
        memoryDesc.mustBeDedicated = memoryTypeInfo.mustBeDedicated;
    }
}

void DeviceVK::GetMemoryDesc2(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkImageCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    FillCreateInfo(textureDesc, createInfo);

    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    requirements.pNext = &dedicatedRequirements;

    VkDeviceImageMemoryRequirements imageMemoryRequirements = {VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS};
    imageMemoryRequirements.pCreateInfo = &createInfo;

    const auto& vk = GetDispatchTable();
    vk.GetDeviceImageMemoryRequirements(m_Device, &imageMemoryRequirements, &requirements);

    MemoryTypeInfo memoryTypeInfo = {};
    memoryTypeInfo.mustBeDedicated = dedicatedRequirements.prefersDedicatedAllocation;

    memoryDesc = {};
    if (GetMemoryTypeInfo(memoryLocation, requirements.memoryRequirements.memoryTypeBits, memoryTypeInfo)) {
        memoryDesc.size = requirements.memoryRequirements.size;
        memoryDesc.alignment = (uint32_t)requirements.memoryRequirements.alignment;
        memoryDesc.type = Pack(memoryTypeInfo);
        memoryDesc.mustBeDedicated = memoryTypeInfo.mustBeDedicated;
    }
}

void DeviceVK::GetMemoryDesc2(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    VkAccelerationStructureBuildSizesInfoKHR sizesInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    GetAccelerationStructureBuildSizesInfo(accelerationStructureDesc, sizesInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = sizesInfo.accelerationStructureSize;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    GetMemoryDesc2(bufferDesc, memoryLocation, memoryDesc);
}

bool DeviceVK::GetMemoryTypeInfo(MemoryLocation memoryLocation, uint32_t memoryTypeMask, MemoryTypeInfo& memoryTypeInfo) const {
    VkMemoryPropertyFlags neededFlags = 0;    // must have
    VkMemoryPropertyFlags undesiredFlags = 0; // have higher priority than desired
    VkMemoryPropertyFlags desiredFlags = 0;   // nice to have

    if (memoryLocation == MemoryLocation::DEVICE) {
        neededFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        undesiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    } else if (memoryLocation == MemoryLocation::DEVICE_UPLOAD) {
        neededFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        undesiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        desiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    } else {
        neededFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        undesiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desiredFlags = (memoryLocation == MemoryLocation::HOST_READBACK ? VK_MEMORY_PROPERTY_HOST_CACHED_BIT : 0) | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    // Phase 1: needed, undesired and desired
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;
        bool hasUndesiredFlags = undesiredFlags == 0 ? false : (m_MemoryProps.memoryTypes[i].propertyFlags & undesiredFlags) == undesiredFlags;
        bool hasDesiredFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & desiredFlags) == desiredFlags;

        if (isSupported && hasNeededFlags && !hasUndesiredFlags && hasDesiredFlags) {
            memoryTypeInfo.index = (MemoryTypeIndex)i;
            memoryTypeInfo.location = memoryLocation;

            return true;
        }
    }

    // Phase 2: needed and undesired
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;
        bool hasUndesiredFlags = undesiredFlags == 0 ? false : (m_MemoryProps.memoryTypes[i].propertyFlags & undesiredFlags) == undesiredFlags;

        if (isSupported && hasNeededFlags && !hasUndesiredFlags) {
            memoryTypeInfo.index = (MemoryTypeIndex)i;
            memoryTypeInfo.location = memoryLocation;

            return true;
        }
    }

    // Phase 3: needed and desired
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;
        bool hasDesiredFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & desiredFlags) == desiredFlags;

        if (isSupported && hasNeededFlags && hasDesiredFlags) {
            memoryTypeInfo.index = (MemoryTypeIndex)i;
            memoryTypeInfo.location = memoryLocation;

            return true;
        }
    }

    // Phase 4: only needed
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;

        if (isSupported && hasNeededFlags) {
            memoryTypeInfo.index = (MemoryTypeIndex)i;
            memoryTypeInfo.location = memoryLocation;

            return true;
        }
    }

    CHECK(false, "Can't find suitable memory type");

    return false;
}

bool DeviceVK::GetMemoryTypeByIndex(uint32_t index, MemoryTypeInfo& memoryTypeInfo) const {
    if (index >= m_MemoryProps.memoryTypeCount)
        return false;

    const VkMemoryType& memoryType = m_MemoryProps.memoryTypes[index];
    bool isHostVisible = memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    bool isDevice = memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    memoryTypeInfo.index = (MemoryTypeIndex)index;
    if (isDevice)
        memoryTypeInfo.location = isHostVisible ? MemoryLocation::DEVICE_UPLOAD : MemoryLocation::DEVICE;
    else
        memoryTypeInfo.location = MemoryLocation::HOST_UPLOAD;

    return true;
}

void DeviceVK::GetAccelerationStructureBuildSizesInfo(const AccelerationStructureDesc& accelerationStructureDesc, VkAccelerationStructureBuildSizesInfoKHR& sizesInfo) {
    uint32_t geometryCount = accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL ? accelerationStructureDesc.instanceOrGeometryObjectNum : 1;
    Scratch<uint32_t> primitiveMaxNums = AllocateScratch(*this, uint32_t, geometryCount);
    Scratch<VkAccelerationStructureGeometryKHR> geometries = AllocateScratch(*this, VkAccelerationStructureGeometryKHR, geometryCount);

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL)
        ConvertGeometryObjectSizesVK(geometries, primitiveMaxNums, accelerationStructureDesc.geometryObjects, geometryCount);
    else {
        geometries[0] = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometries[0].geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometries[0].geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometries[0].geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        geometries[0].geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;

        primitiveMaxNums[0] = accelerationStructureDesc.instanceOrGeometryObjectNum;
    }

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = GetAccelerationStructureType(accelerationStructureDesc.type);
    buildInfo.flags = GetAccelerationStructureBuildFlags(accelerationStructureDesc.flags);
    buildInfo.geometryCount = geometryCount;
    buildInfo.pGeometries = geometries;

    const auto& vk = GetDispatchTable();
    vk.GetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveMaxNums, &sizesInfo);
}

const char* GetObjectTypeName(VkObjectType objectType) {
    switch (objectType) {
        case VK_OBJECT_TYPE_INSTANCE:
            return "VkInstance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "VkPhysicalDevice";
        case VK_OBJECT_TYPE_DEVICE:
            return "VkDevice";
        case VK_OBJECT_TYPE_QUEUE:
            return "VkQueue";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "VkSemaphore";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "VkCommandBuffer";
        case VK_OBJECT_TYPE_FENCE:
            return "VkFence";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "VkDeviceMemory";
        case VK_OBJECT_TYPE_BUFFER:
            return "VkBuffer";
        case VK_OBJECT_TYPE_IMAGE:
            return "VkImage";
        case VK_OBJECT_TYPE_EVENT:
            return "VkEvent";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "VkQueryPool";
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return "VkBufferView";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "VkImageView";
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return "VkShaderModule";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return "VkPipelineCache";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "VkPipelineLayout";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "VkRenderPass";
        case VK_OBJECT_TYPE_PIPELINE:
            return "VkPipeline";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "VkDescriptorSetLayout";
        case VK_OBJECT_TYPE_SAMPLER:
            return "VkSampler";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "VkDescriptorPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "VkDescriptorSet";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "VkFramebuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return "VkCommandPool";
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
            return "VkSamplerYcbcrConversion";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
            return "VkDescriptorUpdateTemplate";
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return "VkSurfaceKHR";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "VkSwapchainKHR";
        case VK_OBJECT_TYPE_DISPLAY_KHR:
            return "VkDisplayKHR";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
            return "VkDisplayModeKHR";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
            return "VkDebugReportCallbackEXT";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
            return "VkDebugUtilsMessengerEXT";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "VkAccelerationStructureKHR";
        case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
            return "VkValidationCacheEXT";
        case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
            return "VkDeferredOperationKHR";
        default:
            return "unknown";
    }
}

VkBool32 VKAPI_PTR DebugUtilsMessenger(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    // TODO: some messages can be muted here
    if (callbackData->messageIdNumber == 0) // loader info message
        return VK_FALSE;
    if (callbackData->messageIdNumber == 0x76589099) // Validation Warning: [ WARNING-DEBUG-PRINTF ] Internal Warning: Setting VkPhysicalDeviceVulkan12Properties::maxUpdateAfterBindDescriptorsInAllPools to 32
        return VK_FALSE;

    Message severity = Message::INFO;
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severity = Message::ERROR;
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = Message::WARNING;

    DeviceVK& device = *(DeviceVK*)userData;
    device.ReportMessage(severity, __FILE__, __LINE__, "%s", callbackData->pMessage);

    return VK_FALSE;
}

Result DeviceVK::CreateInstance(bool enableGraphicsAPIValidation, const Vector<const char*>& desiredInstanceExts) {
    Vector<const char*> layers(GetStdAllocator());
    if (enableGraphicsAPIValidation)
        layers.push_back("VK_LAYER_KHRONOS_validation");

    FilterInstanceLayers(layers);

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    };

    VkValidationFeaturesEXT validationFeatures = {VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    validationFeatures.enabledValidationFeatureCount = GetCountOf(enabledValidationFeatures);
    validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

    VkInstanceCreateInfo info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pNext = enableGraphicsAPIValidation ? &validationFeatures : nullptr;
#ifdef __APPLE__
    info.flags = (VkInstanceCreateFlags)VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
    info.pApplicationInfo = &appInfo;
    info.enabledLayerCount = (uint32_t)layers.size();
    info.ppEnabledLayerNames = layers.data();
    info.enabledExtensionCount = (uint32_t)desiredInstanceExts.size();
    info.ppEnabledExtensionNames = desiredInstanceExts.data();

    VkResult result = m_VK.CreateInstance(&info, m_AllocationCallbackPtr, &m_Instance);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkCreateInstance returned %d", (int32_t)result);

    if (enableGraphicsAPIValidation) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        createInfo.pUserData = this;
        createInfo.pfnUserCallback = DebugUtilsMessenger;

        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)m_VK.GetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        result = vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, m_AllocationCallbackPtr, &m_Messenger);

        RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkCreateDebugUtilsMessengerEXT returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

void DeviceVK::SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name) {
    if (!m_VK.SetDebugUtilsObjectNameEXT)
        return;

    VkDebugUtilsObjectNameInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, objectType, (uint64_t)handle, name};

    VkResult result = m_VK.SetDebugUtilsObjectNameEXT(m_Device, &info);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, ReturnVoid(), "vkSetDebugUtilsObjectNameEXT returned %d", (int32_t)result);
}

void DeviceVK::ReportDeviceGroupInfo() {
    String text(GetStdAllocator());

    REPORT_INFO(this, "Memory heaps:");
    for (uint32_t i = 0; i < m_MemoryProps.memoryHeapCount; i++) {
        text.clear();

        if (m_MemoryProps.memoryHeaps[i].flags == 0)
            text += "*SYSMEM* ";
        if (m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            text += "DEVICE_LOCAL_BIT ";
        if (m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
            text += "MULTI_INSTANCE_BIT ";

        double size = double(m_MemoryProps.memoryHeaps[i].size) / (1024.0 * 1024.0);
        REPORT_INFO(this, "  Heap #%u: %.f Mb - %s", i, size, text.c_str());
    }

    REPORT_INFO(this, "Memory types:");
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        text.clear();

        REPORT_INFO(this, "  Memory type #%u", i);
        REPORT_INFO(this, "    Heap #%u", m_MemoryProps.memoryTypes[i].heapIndex);

        VkMemoryPropertyFlags flags = m_MemoryProps.memoryTypes[i].propertyFlags;
        if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            text += "DEVICE_LOCAL_BIT ";
        if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            text += "HOST_VISIBLE_BIT ";
        if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            text += "HOST_COHERENT_BIT ";
        if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
            text += "HOST_CACHED_BIT ";
        if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            text += "LAZILY_ALLOCATED_BIT ";
        if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
            text += "PROTECTED_BIT ";
        if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
            text += "DEVICE_COHERENT_BIT_AMD ";
        if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
            text += "DEVICE_UNCACHED_BIT_AMD ";
        if (flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
            text += "RDMA_CAPABLE_BIT_NV ";

        if (!text.empty())
            REPORT_INFO(this, "    %s", text.c_str());
    }
}

#define MERGE_TOKENS2(a, b) a##b
#define MERGE_TOKENS3(a, b, c) a##b##c

#define GET_DEVICE_OPTIONAL_CORE_PROC(name) \
    /* Core */ \
    m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
    /* KHR */ \
    if (!m_VK.name) \
        m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS3(vk, name, KHR))); \
    /* EXT (some extensions were promoted to core from EXT bypassing KHR status) */ \
    if (!m_VK.name) \
    m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS3(vk, name, EXT)))

#define GET_DEVICE_CORE_PROC(name) \
    GET_DEVICE_OPTIONAL_CORE_PROC(name); \
    if (!m_VK.name) { \
        REPORT_ERROR(this, "Failed to get device function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        return Result::UNSUPPORTED; \
    }

#define GET_DEVICE_PROC(name) \
    m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
    if (!m_VK.name) { \
        REPORT_ERROR(this, "Failed to get device function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        return Result::UNSUPPORTED; \
    }

#define GET_INSTANCE_PROC(name) \
    m_VK.name = (PFN_vk##name)m_VK.GetInstanceProcAddr(m_Instance, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
    if (!m_VK.name) { \
        REPORT_ERROR(this, "Failed to get instance function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        return Result::UNSUPPORTED; \
    }

Result DeviceVK::ResolvePreInstanceDispatchTable() {
    m_VK = {};

    m_VK.GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*m_Loader, "vkGetInstanceProcAddr");
    if (!m_VK.GetInstanceProcAddr) {
        REPORT_ERROR(this, "Failed to get 'vkGetInstanceProcAddr'");
        return Result::UNSUPPORTED;
    }

    GET_INSTANCE_PROC(CreateInstance);
    GET_INSTANCE_PROC(EnumerateInstanceExtensionProperties);
    GET_INSTANCE_PROC(EnumerateInstanceLayerProperties);

    return Result::SUCCESS;
}

Result DeviceVK::ResolveInstanceDispatchTable(const Vector<const char*>& desiredInstanceExts) {
    GET_INSTANCE_PROC(GetDeviceProcAddr);
    GET_INSTANCE_PROC(DestroyInstance);
    GET_INSTANCE_PROC(DestroyDevice);
    GET_INSTANCE_PROC(GetPhysicalDeviceMemoryProperties2);
    GET_INSTANCE_PROC(GetDeviceGroupPeerMemoryFeatures);
    GET_INSTANCE_PROC(GetPhysicalDeviceFormatProperties2);
    GET_INSTANCE_PROC(CreateDevice);
    GET_INSTANCE_PROC(GetDeviceQueue2);
    GET_INSTANCE_PROC(EnumeratePhysicalDeviceGroups);
    GET_INSTANCE_PROC(GetPhysicalDeviceProperties2);
    GET_INSTANCE_PROC(GetPhysicalDeviceFeatures2);
    GET_INSTANCE_PROC(GetPhysicalDeviceQueueFamilyProperties2);
    GET_INSTANCE_PROC(EnumerateDeviceExtensionProperties);

    if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_PROC(SetDebugUtilsObjectNameEXT);
        GET_INSTANCE_PROC(CmdBeginDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(CmdEndDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(CmdInsertDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(QueueBeginDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(QueueEndDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(QueueInsertDebugUtilsLabelEXT);
    }

    if (IsExtensionSupported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceFormats2KHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceCapabilities2KHR);
    }

    if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceSupportKHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfacePresentModesKHR);
        GET_INSTANCE_PROC(DestroySurfaceKHR);

#if VK_USE_PLATFORM_WIN32_KHR
        GET_INSTANCE_PROC(CreateWin32SurfaceKHR);
        GET_INSTANCE_PROC(GetMemoryWin32HandlePropertiesKHR);
#endif
#if VK_USE_PLATFORM_METAL_EXT
        GET_INSTANCE_PROC(CreateMetalSurfaceEXT);
#endif
#if VK_USE_PLATFORM_XLIB_KHR
        GET_INSTANCE_PROC(CreateXlibSurfaceKHR);
#endif
#if VK_USE_PLATFORM_WAYLAND_KHR
        GET_INSTANCE_PROC(CreateWaylandSurfaceKHR);
#endif
    }

    return Result::SUCCESS;
}

Result DeviceVK::ResolveDispatchTable(const Vector<const char*>& desiredDeviceExts) {
    GET_DEVICE_CORE_PROC(CreateBuffer);
    GET_DEVICE_CORE_PROC(CreateImage);
    GET_DEVICE_CORE_PROC(CreateBufferView);
    GET_DEVICE_CORE_PROC(CreateImageView);
    GET_DEVICE_CORE_PROC(CreateSampler);
    GET_DEVICE_CORE_PROC(CreateQueryPool);
    GET_DEVICE_CORE_PROC(CreateCommandPool);
    GET_DEVICE_CORE_PROC(CreateSemaphore);
    GET_DEVICE_CORE_PROC(CreateDescriptorPool);
    GET_DEVICE_CORE_PROC(CreatePipelineLayout);
    GET_DEVICE_CORE_PROC(CreateDescriptorSetLayout);
    GET_DEVICE_CORE_PROC(CreateShaderModule);
    GET_DEVICE_CORE_PROC(CreateGraphicsPipelines);
    GET_DEVICE_CORE_PROC(CreateComputePipelines);
    GET_DEVICE_CORE_PROC(DestroyBuffer);
    GET_DEVICE_CORE_PROC(DestroyImage);
    GET_DEVICE_CORE_PROC(DestroyBufferView);
    GET_DEVICE_CORE_PROC(DestroyImageView);
    GET_DEVICE_CORE_PROC(DestroySampler);
    GET_DEVICE_CORE_PROC(DestroyFramebuffer);
    GET_DEVICE_CORE_PROC(DestroyQueryPool);
    GET_DEVICE_CORE_PROC(DestroyCommandPool);
    GET_DEVICE_CORE_PROC(DestroySemaphore);
    GET_DEVICE_CORE_PROC(DestroyDescriptorPool);
    GET_DEVICE_CORE_PROC(DestroyPipelineLayout);
    GET_DEVICE_CORE_PROC(DestroyDescriptorSetLayout);
    GET_DEVICE_CORE_PROC(DestroyShaderModule);
    GET_DEVICE_CORE_PROC(DestroyPipeline);
    GET_DEVICE_CORE_PROC(AllocateMemory);
    GET_DEVICE_CORE_PROC(MapMemory);
    GET_DEVICE_CORE_PROC(UnmapMemory);
    GET_DEVICE_CORE_PROC(FreeMemory);
    GET_DEVICE_CORE_PROC(FlushMappedMemoryRanges);
    GET_DEVICE_CORE_PROC(QueueWaitIdle);
    GET_DEVICE_CORE_PROC(QueueSubmit2);
    GET_DEVICE_CORE_PROC(GetSemaphoreCounterValue);
    GET_DEVICE_CORE_PROC(WaitSemaphores);
    GET_DEVICE_CORE_PROC(ResetCommandPool);
    GET_DEVICE_CORE_PROC(ResetDescriptorPool);
    GET_DEVICE_CORE_PROC(AllocateCommandBuffers);
    GET_DEVICE_CORE_PROC(AllocateDescriptorSets);
    GET_DEVICE_CORE_PROC(FreeCommandBuffers);
    GET_DEVICE_CORE_PROC(FreeDescriptorSets);
    GET_DEVICE_CORE_PROC(UpdateDescriptorSets);
    GET_DEVICE_CORE_PROC(BindBufferMemory2);
    GET_DEVICE_CORE_PROC(BindImageMemory2);
    GET_DEVICE_CORE_PROC(GetBufferMemoryRequirements2);
    GET_DEVICE_CORE_PROC(GetImageMemoryRequirements2);
    GET_DEVICE_CORE_PROC(ResetQueryPool);
    GET_DEVICE_CORE_PROC(GetBufferDeviceAddress);
    GET_DEVICE_CORE_PROC(BeginCommandBuffer);
    GET_DEVICE_CORE_PROC(CmdSetViewportWithCount);
    GET_DEVICE_CORE_PROC(CmdSetScissorWithCount);
    GET_DEVICE_CORE_PROC(CmdSetDepthBounds);
    GET_DEVICE_CORE_PROC(CmdSetStencilReference);
    GET_DEVICE_CORE_PROC(CmdSetBlendConstants);
    GET_DEVICE_CORE_PROC(CmdSetDepthBias);
    GET_DEVICE_CORE_PROC(CmdClearAttachments);
    GET_DEVICE_CORE_PROC(CmdClearColorImage);
    GET_DEVICE_CORE_PROC(CmdBindVertexBuffers2);
    GET_DEVICE_CORE_PROC(CmdBindIndexBuffer);
    GET_DEVICE_CORE_PROC(CmdBindPipeline);
    GET_DEVICE_CORE_PROC(CmdBindDescriptorSets);
    GET_DEVICE_CORE_PROC(CmdPushConstants);
    GET_DEVICE_CORE_PROC(CmdDispatch);
    GET_DEVICE_CORE_PROC(CmdDispatchIndirect);
    GET_DEVICE_CORE_PROC(CmdDraw);
    GET_DEVICE_CORE_PROC(CmdDrawIndexed);
    GET_DEVICE_CORE_PROC(CmdDrawIndirect);
    GET_DEVICE_CORE_PROC(CmdDrawIndirectCount);
    GET_DEVICE_CORE_PROC(CmdDrawIndexedIndirect);
    GET_DEVICE_CORE_PROC(CmdDrawIndexedIndirectCount);
    GET_DEVICE_CORE_PROC(CmdCopyBuffer2);
    GET_DEVICE_CORE_PROC(CmdCopyImage2);
    GET_DEVICE_CORE_PROC(CmdResolveImage2);
    GET_DEVICE_CORE_PROC(CmdCopyBufferToImage2);
    GET_DEVICE_CORE_PROC(CmdCopyImageToBuffer2);
    GET_DEVICE_CORE_PROC(CmdPipelineBarrier2);
    GET_DEVICE_CORE_PROC(CmdBeginQuery);
    GET_DEVICE_CORE_PROC(CmdEndQuery);
    GET_DEVICE_CORE_PROC(CmdWriteTimestamp2);
    GET_DEVICE_CORE_PROC(CmdCopyQueryPoolResults);
    GET_DEVICE_CORE_PROC(CmdResetQueryPool);
    GET_DEVICE_CORE_PROC(CmdFillBuffer);
    GET_DEVICE_CORE_PROC(CmdBeginRendering);
    GET_DEVICE_CORE_PROC(CmdEndRendering);
    GET_DEVICE_CORE_PROC(EndCommandBuffer);

    // IMPORTANT: { } are mandatory here!

    if (m_MinorVersion >= 3 || IsExtensionSupported(VK_KHR_MAINTENANCE_4_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_CORE_PROC(GetDeviceBufferMemoryRequirements);
        GET_DEVICE_CORE_PROC(GetDeviceImageMemoryRequirements);
    }

    if (IsExtensionSupported(VK_KHR_MAINTENANCE_5_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CmdBindIndexBuffer2KHR);
    }

    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CmdSetFragmentShadingRateKHR);
    }

    if (IsExtensionSupported(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CmdPushDescriptorSetKHR);
    }

    if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(AcquireNextImageKHR);
        GET_DEVICE_PROC(QueuePresentKHR);
        GET_DEVICE_PROC(CreateSwapchainKHR);
        GET_DEVICE_PROC(DestroySwapchainKHR);
        GET_DEVICE_PROC(GetSwapchainImagesKHR);
    }

    if (IsExtensionSupported(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(WaitForPresentKHR);
    }

    if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CreateAccelerationStructureKHR);
        GET_DEVICE_PROC(DestroyAccelerationStructureKHR);
        GET_DEVICE_PROC(GetAccelerationStructureDeviceAddressKHR);
        GET_DEVICE_PROC(GetAccelerationStructureBuildSizesKHR);
        GET_DEVICE_PROC(CmdBuildAccelerationStructuresKHR);
        GET_DEVICE_PROC(CmdCopyAccelerationStructureKHR);
        GET_DEVICE_PROC(CmdWriteAccelerationStructuresPropertiesKHR);
    }

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CreateRayTracingPipelinesKHR);
        GET_DEVICE_PROC(GetRayTracingShaderGroupHandlesKHR);
        GET_DEVICE_PROC(CmdTraceRaysKHR);
        GET_DEVICE_PROC(CmdTraceRaysIndirect2KHR);
    }

    if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CmdSetSampleLocationsEXT);
    }

    if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CmdDrawMeshTasksEXT);
        GET_DEVICE_PROC(CmdDrawMeshTasksIndirectEXT);
        GET_DEVICE_PROC(CmdDrawMeshTasksIndirectCountEXT);
    }

    if (IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(GetLatencyTimingsNV);
        GET_DEVICE_PROC(LatencySleepNV);
        GET_DEVICE_PROC(SetLatencyMarkerNV);
        GET_DEVICE_PROC(SetLatencySleepModeNV);
    }

    return Result::SUCCESS;
}

void DeviceVK::Destruct() {
    Destroy(GetAllocationCallbacks(), this);
}

NRI_INLINE void DeviceVK::SetDebugName(const char* name) {
    SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DEVICE, (uint64_t)m_Device, name);
}

NRI_INLINE Result DeviceVK::GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    const auto& queueFamily = m_QueueFamilies[(uint32_t)queueType];
    if (queueFamily.empty())
        return Result::UNSUPPORTED;

    if (queueIndex < queueFamily.size()) {
        QueueVK* queueVK = m_QueueFamilies[(uint32_t)queueType].at(queueIndex);
        queue = (Queue*)queueVK;

        { // Update active family indices
            ExclusiveScope lock(m_Lock);

            uint32_t i = 0;
            for (; i < m_NumActiveFamilyIndices; i++) {
                if (m_ActiveQueueFamilyIndices[i] == queueVK->GetFamilyIndex())
                    break;
            }

            if (i == m_NumActiveFamilyIndices)
                m_ActiveQueueFamilyIndices[m_NumActiveFamilyIndices++] = queueVK->GetFamilyIndex();
        }

        return Result::SUCCESS;
    }

    return Result::FAILURE;
}

NRI_INLINE Result DeviceVK::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (!memoryBindingDescNum)
        return Result::SUCCESS;

    Scratch<VkBindBufferMemoryInfo> infos = AllocateScratch(*this, VkBindBufferMemoryInfo, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const BufferMemoryBindingDesc& memoryBindingDesc = memoryBindingDescs[i];

        BufferVK& bufferImpl = *(BufferVK*)memoryBindingDesc.buffer;
        MemoryVK& memoryImpl = *(MemoryVK*)memoryBindingDesc.memory;

        MemoryTypeInfo memoryTypeInfo = Unpack(memoryImpl.GetType());
        if (memoryTypeInfo.mustBeDedicated)
            memoryImpl.CreateDedicated(bufferImpl);

        VkBindBufferMemoryInfo& info = infos[i];
        info = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
        info.buffer = bufferImpl.GetHandle();
        info.memory = memoryImpl.GetHandle();
        info.memoryOffset = memoryBindingDesc.offset;
    }

    VkResult result = m_VK.BindBufferMemory2(m_Device, memoryBindingDescNum, infos);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkBindBufferMemory2 returned %d", (int32_t)result);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const BufferMemoryBindingDesc& memoryBindingDesc = memoryBindingDescs[i];

        BufferVK& bufferImpl = *(BufferVK*)memoryBindingDesc.buffer;
        MemoryVK& memoryImpl = *(MemoryVK*)memoryBindingDesc.memory;

        bufferImpl.FinishMemoryBinding(memoryImpl, memoryBindingDesc.offset);
    }

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceVK::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (!memoryBindingDescNum)
        return Result::SUCCESS;

    Scratch<VkBindImageMemoryInfo> infos = AllocateScratch(*this, VkBindImageMemoryInfo, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const TextureMemoryBindingDesc& memoryBindingDesc = memoryBindingDescs[i];

        MemoryVK& memoryImpl = *(MemoryVK*)memoryBindingDesc.memory;
        TextureVK& textureImpl = *(TextureVK*)memoryBindingDesc.texture;

        MemoryTypeInfo memoryTypeInfo = Unpack(memoryImpl.GetType());
        if (memoryTypeInfo.mustBeDedicated)
            memoryImpl.CreateDedicated(textureImpl);

        VkBindImageMemoryInfo& info = infos[i];
        info = {VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
        info.image = textureImpl.GetHandle();
        info.memory = memoryImpl.GetHandle();
        info.memoryOffset = memoryBindingDesc.offset;
    }

    VkResult result = m_VK.BindImageMemory2(m_Device, memoryBindingDescNum, infos);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkBindImageMemory2 returned %d", (int32_t)result);

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceVK::BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (!memoryBindingDescNum)
        return Result::SUCCESS;

    Scratch<BufferMemoryBindingDesc> infos = AllocateScratch(*this, BufferMemoryBindingDesc, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const AccelerationStructureMemoryBindingDesc& memoryBindingDesc = memoryBindingDescs[i];
        AccelerationStructureVK& accelerationStructure = *(AccelerationStructureVK*)memoryBindingDesc.accelerationStructure;

        BufferMemoryBindingDesc& bufferMemoryBinding = infos[i];
        bufferMemoryBinding = {};
        bufferMemoryBinding.buffer = (Buffer*)accelerationStructure.GetBuffer();
        bufferMemoryBinding.memory = memoryBindingDesc.memory;
        bufferMemoryBinding.offset = memoryBindingDesc.offset;
    }

    Result result = BindBufferMemory(infos, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum && result == Result::SUCCESS; i++) {
        AccelerationStructureVK& accelerationStructure = *(AccelerationStructureVK*)memoryBindingDescs[i].accelerationStructure;
        result = accelerationStructure.FinishCreation();
    }

    return result;
}

NRI_INLINE FormatSupportBits DeviceVK::GetFormatSupport(Format format) const {
    FormatSupportBits mask = FormatSupportBits::UNSUPPORTED;

    const VkFormat vkFormat = GetVkFormat(format);

    VkFormatProperties2 props = {VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
    m_VK.GetPhysicalDeviceFormatProperties2(m_PhysicalDevice, vkFormat, &props);

#define UPDATE_SUPPORT_BITS(required, bit) \
    if ((props.formatProperties.optimalTilingFeatures & (required)) == (required)) \
        mask |= bit;

    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, FormatSupportBits::TEXTURE);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT, FormatSupportBits::STORAGE_TEXTURE);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, FormatSupportBits::COLOR_ATTACHMENT);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, FormatSupportBits::DEPTH_STENCIL_ATTACHMENT);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT, FormatSupportBits::BLEND);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT, FormatSupportBits::STORAGE_TEXTURE_ATOMICS);

#undef UPDATE_SUPPORT_BITS

#define UPDATE_SUPPORT_BITS(required, bit) \
    if ((props.formatProperties.bufferFeatures & (required)) == (required)) \
        mask |= bit;

    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT, FormatSupportBits::BUFFER);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT, FormatSupportBits::STORAGE_BUFFER);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, FormatSupportBits::VERTEX_BUFFER);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT, FormatSupportBits::STORAGE_BUFFER_ATOMICS);

#undef UPDATE_SUPPORT_BITS

    return mask;
}

NRI_INLINE Result DeviceVK::QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const {
    videoMemoryInfo = {};

    if (!m_IsSupported.memoryBudget)
        return Result::UNSUPPORTED;

    VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};

    VkPhysicalDeviceMemoryProperties2 memoryProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    memoryProps.pNext = &budgetProps;

    const auto& vk = GetDispatchTable();
    vk.GetPhysicalDeviceMemoryProperties2(m_PhysicalDevice, &memoryProps);

    bool isLocal = memoryLocation == nri::MemoryLocation::DEVICE || memoryLocation == nri::MemoryLocation::DEVICE_UPLOAD;

    for (uint32_t i = 0; i < GetCountOf(budgetProps.heapBudget); i++) {
        VkDeviceSize size = budgetProps.heapBudget[i];
        bool state = m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;

        if (size && state == isLocal)
            videoMemoryInfo.budgetSize += size;
    }

    for (uint32_t i = 0; i < GetCountOf(budgetProps.heapUsage); i++) {
        VkDeviceSize size = budgetProps.heapUsage[i];
        bool state = m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;

        if (size && state == isLocal)
            videoMemoryInfo.usageSize += size;
    }

    return Result::SUCCESS;
}
