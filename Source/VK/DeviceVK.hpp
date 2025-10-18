// © 2021 NVIDIA Corporation

constexpr bool VERBOSE = false;

static constexpr VkBufferUsageFlags GetBufferUsageFlags(BufferUsageBits bufferUsageBits, uint32_t structureStride, bool isDeviceAddressSupported) {
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // TODO: ban "the opposite" for UPLOAD/READBACK?

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

    if (bufferUsageBits & BufferUsageBits::MICROMAP_STORAGE)
        flags |= VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT;

    if (bufferUsageBits & BufferUsageBits::MICROMAP_BUILD_INPUT)
        flags |= VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT;

    if (bufferUsageBits & BufferUsageBits::SHADER_RESOURCE)
        flags |= structureStride ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::SHADER_RESOURCE_STORAGE)
        flags |= structureStride ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

    return flags;
}

static constexpr VkImageUsageFlags GetImageUsageFlags(TextureUsageBits textureUsageBits) {
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

static inline bool IsExtensionSupported(const char* ext, const Vector<VkExtensionProperties>& list) {
    for (auto& e : list) {
        if (!strcmp(ext, e.extensionName))
            return true;
    }

    return false;
}

static inline bool IsExtensionSupported(const char* ext, const Vector<const char*>& list) {
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

static VkBool32 VKAPI_PTR MessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    DeviceVK& device = *(DeviceVK*)userData;

    { // TODO: some messages can be muted here
        // Loader info message
        if (callbackData->messageIdNumber == 0)
            return VK_FALSE;
        // Validation Information: [ WARNING-CreateInstance-status-message ] vkCreateInstance(): Khronos Validation Layer Active ...
        if (callbackData->messageIdNumber == 601872502)
            return VK_FALSE;
        // Validation Warning: [ VALIDATION-SETTINGS ] vkCreateInstance(): DebugPrintf logs to the Information message severity, enabling Information level logging otherwise the message will not be seen.
        if (callbackData->messageIdNumber == 2132353751)
            return VK_FALSE;
        // Validation Warning: [ WARNING-DEBUG-PRINTF ] Internal Warning: Setting VkPhysicalDeviceVulkan12Properties::maxUpdateAfterBindDescriptorsInAllPools to 32
        if (callbackData->messageIdNumber == 1985515673)
            return VK_FALSE;
    }

    Message severity = Message::INFO;
    Result result = Result::SUCCESS;
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severity = Message::ERROR;
        result = Result::FAILURE;
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = Message::WARNING;

    device.ReportMessage(severity, result, __FILE__, __LINE__, "[%u] %s", callbackData->messageIdNumber, callbackData->pMessage);

    return VK_FALSE;
}

VkResult DeviceVK::CreateVma() {
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = m_VK.GetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = m_VK.GetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_MAKE_API_VERSION(0, 1, m_MinorVersion, 0);
    allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
    allocatorCreateInfo.device = m_Device;
    allocatorCreateInfo.instance = m_Instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    allocatorCreateInfo.pAllocationCallbacks = m_AllocationCallbackPtr;
    allocatorCreateInfo.preferredLargeHeapBlockSize = VMA_PREFERRED_BLOCK_SIZE;

    allocatorCreateInfo.flags = 0;
    if (m_IsSupported.memoryBudget)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    if (m_IsSupported.deviceAddress)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (m_IsSupported.memoryPriority)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    if (m_IsSupported.maintenance4)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    if (m_IsSupported.maintenance5)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;

    return vmaCreateAllocator(&allocatorCreateInfo, &m_Vma);
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

    if constexpr (VERBOSE) {
        REPORT_INFO(this, "Supported instance extensions:");
        for (const VkExtensionProperties& props : supportedExts)
            REPORT_INFO(this, "    %s (v%u)", props.extensionName, props.specVersion);
    }

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
#ifdef VK_USE_PLATFORM_XLIB_KHR
        desiredInstanceExts.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        desiredInstanceExts.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
        desiredInstanceExts.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif
    }

    if (IsExtensionSupported(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, supportedExts))
        desiredInstanceExts.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);

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

    if constexpr (VERBOSE) {
        REPORT_INFO(this, "Supported device extensions:");
        for (const VkExtensionProperties& props : supportedExts)
            REPORT_INFO(this, "    %s (v%u)", props.extensionName, props.specVersion);
    }

#ifdef __APPLE__
    APPEND_EXT(true, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    APPEND_EXT(m_MinorVersion < 3, VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 3, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 3, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 3, VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 3, VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 3, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME); // TODO: there are 2 and 3 versions...
    APPEND_EXT(m_MinorVersion < 3, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);

    APPEND_EXT(m_MinorVersion < 4, VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 4, VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 4, VK_KHR_MAINTENANCE_6_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 4, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    APPEND_EXT(m_MinorVersion < 4, VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME);

    APPEND_EXT(!disableRayTracing, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    APPEND_EXT(!disableRayTracing, VK_KHR_RAY_QUERY_EXTENSION_NAME);
    APPEND_EXT(!disableRayTracing, VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
    APPEND_EXT(!disableRayTracing, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    APPEND_EXT(!disableRayTracing, VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);
    APPEND_EXT(!disableRayTracing, VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME);

    APPEND_EXT(true, VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_MAINTENANCE_7_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_MAINTENANCE_8_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_MAINTENANCE_9_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_PRESENT_ID_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_SHADER_CLOCK_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    APPEND_EXT(true, VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_IMAGE_SLICED_VIEW_OF_3D_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_MESH_SHADER_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_ROBUSTNESS_2_EXTENSION_NAME); // TODO: use KHR
    APPEND_EXT(true, VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_ZERO_INITIALIZE_DEVICE_MEMORY_EXTENSION_NAME);
    APPEND_EXT(true, VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME);
    APPEND_EXT(true, VK_NVX_BINARY_IMPORT_EXTENSION_NAME);
    APPEND_EXT(true, VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME);
    APPEND_EXT(true, VK_NV_LOW_LATENCY_2_EXTENSION_NAME);
}

DeviceVK::DeviceVK(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks)
    : DeviceBase(callbacks, allocationCallbacks)
    , m_QueueFamilies{
          Vector<QueueVK*>(GetStdAllocator()),
          Vector<QueueVK*>(GetStdAllocator()),
          Vector<QueueVK*>(GetStdAllocator()),
      } {
    m_AllocationCallbacks.pUserData = (void*)&GetAllocationCallbacks();
    m_AllocationCallbacks.pfnAllocation = vkAllocateHostMemory;
    m_AllocationCallbacks.pfnReallocation = vkReallocateHostMemory;
    m_AllocationCallbacks.pfnFree = vkFreeHostMemory;

    m_Desc.graphicsAPI = GraphicsAPI::VK;
    m_Desc.nriVersion = NRI_VERSION;
}

DeviceVK::~DeviceVK() {
    if (m_Vma)
        vmaDestroyAllocator(m_Vma);

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

    { // Create instance
        Vector<const char*> desiredInstanceExts(GetStdAllocator());
        for (uint32_t i = 0; i < desc.vkExtensions.instanceExtensionNum; i++)
            desiredInstanceExts.push_back(desc.vkExtensions.instanceExtensions[i]);

        Result res = ResolvePreInstanceDispatchTable();
        if (res != Result::SUCCESS)
            return res;

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
            VkResult vkResult = m_VK.EnumeratePhysicalDeviceGroups(m_Instance, &deviceGroupNum, nullptr);
            RETURN_ON_BAD_VKRESULT(this, vkResult, "vkEnumeratePhysicalDeviceGroups");

            Scratch<VkPhysicalDeviceGroupProperties> deviceGroups = AllocateScratch(*this, VkPhysicalDeviceGroupProperties, deviceGroupNum);
            for (uint32_t j = 0; j < deviceGroupNum; j++) {
                deviceGroups[j].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
                deviceGroups[j].pNext = nullptr;
            }

            vkResult = m_VK.EnumeratePhysicalDeviceGroups(m_Instance, &deviceGroupNum, deviceGroups);
            RETURN_ON_BAD_VKRESULT(this, vkResult, "vkEnumeratePhysicalDeviceGroups");

            uint32_t i = 0;
            for (i = 0; i < deviceGroupNum; i++) {
                VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

                VkPhysicalDeviceIDProperties idProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
                props.pNext = &idProps;

                m_VK.GetPhysicalDeviceProperties2(deviceGroups[i].physicalDevices[0], &props);

                uint32_t majorVersion = VK_API_VERSION_MAJOR(props.properties.apiVersion);
                m_MinorVersion = VK_API_VERSION_MINOR(props.properties.apiVersion);

                bool isSupported = (majorVersion * 10 + m_MinorVersion) >= 12;
                if (desc.adapterDesc) {
                    Uid_t uid = ConstructUid(idProps.deviceLUID, idProps.deviceUUID, idProps.deviceLUIDValid);
                    if (CompareUid(uid, desc.adapterDesc->uid)) {
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
            const QueueFamilyVKDesc& queueFamilyVKDesc = descVK.queueFamilies[i];
            queueFamilyIndices[(size_t)queueFamilyVKDesc.queueType] = queueFamilyVKDesc.familyIndex;
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
    for (uint32_t i = 0; i < desc.vkExtensions.deviceExtensionNum; i++)
        desiredDeviceExts.push_back(desc.vkExtensions.deviceExtensions[i]);

    if (!isWrapper)
        ProcessDeviceExtensions(desiredDeviceExts, desc.disableVKRayTracing);

    REPORT_INFO(this, "Using Vulkan v1.%u (%u device extensions initialized)", m_MinorVersion, (uint32_t)desiredDeviceExts.size());

    // Device features
    VkPhysicalDeviceFeatures2 features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    void** tail = &features.pNext;

    VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    APPEND_STRUCT(features11);

    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    APPEND_STRUCT(features12);

    VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    if (m_MinorVersion >= 3) {
        APPEND_STRUCT(features13);
    }

    VkPhysicalDeviceVulkan14Features features14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};
    if (m_MinorVersion >= 4) {
        APPEND_STRUCT(features14);
    }

    APPEND_FEATURES(m_MinorVersion < 3, KHR, DynamicRendering, DYNAMIC_RENDERING);
    APPEND_FEATURES(m_MinorVersion < 3, KHR, Maintenance4, MAINTENANCE_4);
    APPEND_FEATURES(m_MinorVersion < 3, KHR, Synchronization2, SYNCHRONIZATION_2);
    APPEND_FEATURES(m_MinorVersion < 3, EXT, ExtendedDynamicState, EXTENDED_DYNAMIC_STATE);
    APPEND_FEATURES(m_MinorVersion < 3, EXT, ImageRobustness, IMAGE_ROBUSTNESS);
    APPEND_FEATURES(m_MinorVersion < 3, EXT, SubgroupSizeControl, SUBGROUP_SIZE_CONTROL);

    APPEND_FEATURES(m_MinorVersion < 4, KHR, LineRasterization, LINE_RASTERIZATION);
    APPEND_FEATURES(m_MinorVersion < 4, KHR, Maintenance5, MAINTENANCE_5);
    APPEND_FEATURES(m_MinorVersion < 4, KHR, Maintenance6, MAINTENANCE_6);
    APPEND_FEATURES(m_MinorVersion < 4, EXT, PipelineRobustness, PIPELINE_ROBUSTNESS);

    APPEND_FEATURES(true, KHR, AccelerationStructure, ACCELERATION_STRUCTURE);
    APPEND_FEATURES(true, KHR, ComputeShaderDerivatives, COMPUTE_SHADER_DERIVATIVES);
    APPEND_FEATURES(true, KHR, FragmentShaderBarycentric, FRAGMENT_SHADER_BARYCENTRIC);
    APPEND_FEATURES(true, KHR, FragmentShadingRate, FRAGMENT_SHADING_RATE);
    APPEND_FEATURES(true, KHR, Maintenance7, MAINTENANCE_7);
    APPEND_FEATURES(true, KHR, Maintenance8, MAINTENANCE_8);
    APPEND_FEATURES(true, KHR, Maintenance9, MAINTENANCE_9);
    APPEND_FEATURES(true, KHR, PresentId, PRESENT_ID);
    APPEND_FEATURES(true, KHR, PresentWait, PRESENT_WAIT);
    APPEND_FEATURES(true, KHR, RayQuery, RAY_QUERY);
    APPEND_FEATURES(true, KHR, RayTracingMaintenance1, RAY_TRACING_MAINTENANCE_1);
    APPEND_FEATURES(true, KHR, RayTracingPipeline, RAY_TRACING_PIPELINE);
    APPEND_FEATURES(true, KHR, RayTracingPositionFetch, RAY_TRACING_POSITION_FETCH);
    APPEND_FEATURES(true, KHR, ShaderClock, SHADER_CLOCK);
    APPEND_FEATURES(true, EXT, CustomBorderColor, CUSTOM_BORDER_COLOR);
    APPEND_FEATURES(true, EXT, FragmentShaderInterlock, FRAGMENT_SHADER_INTERLOCK);
    APPEND_FEATURES(true, EXT, ImageSlicedViewOf3D, IMAGE_SLICED_VIEW_OF_3D);
    APPEND_FEATURES(true, EXT, MemoryPriority, MEMORY_PRIORITY);
    APPEND_FEATURES(true, EXT, MeshShader, MESH_SHADER);
    APPEND_FEATURES(true, EXT, OpacityMicromap, OPACITY_MICROMAP);
    APPEND_FEATURES(true, EXT, PresentModeFifoLatestReady, PRESENT_MODE_FIFO_LATEST_READY);
    APPEND_FEATURES(true, EXT, Robustness2, ROBUSTNESS_2);
    APPEND_FEATURES(true, EXT, ShaderAtomicFloat, SHADER_ATOMIC_FLOAT);
    APPEND_FEATURES(true, EXT, ShaderAtomicFloat2, SHADER_ATOMIC_FLOAT_2);
    APPEND_FEATURES(true, EXT, SwapchainMaintenance1, SWAPCHAIN_MAINTENANCE_1);
    APPEND_FEATURES(true, EXT, ZeroInitializeDeviceMemory, ZERO_INITIALIZE_DEVICE_MEMORY);
    APPEND_FEATURES(true, EXT, MutableDescriptorType, MUTABLE_DESCRIPTOR_TYPE);

#ifdef __APPLE__
    APPEND_FEATURES(true, KHR, PortabilitySubset, PORTABILITY_SUBSET);
#endif

    m_VK.GetPhysicalDeviceFeatures2(m_PhysicalDevice, &features);

    if (m_MinorVersion < 3) {
        features13.dynamicRendering = DynamicRenderingFeatures.dynamicRendering;
        features13.synchronization2 = Synchronization2Features.synchronization2;
        features13.maintenance4 = Maintenance4Features.maintenance4;
        features13.robustImageAccess = ImageRobustnessFeatures.robustImageAccess;
    }

    if (m_MinorVersion < 4) {
        features14.maintenance5 = Maintenance5Features.maintenance5;
        features14.maintenance6 = Maintenance6Features.maintenance6;
        features14.pipelineRobustness = PipelineRobustnessFeatures.pipelineRobustness;
        features14.rectangularLines = LineRasterizationFeatures.rectangularLines;
        features14.bresenhamLines = LineRasterizationFeatures.bresenhamLines;
        features14.smoothLines = LineRasterizationFeatures.smoothLines;
        features14.stippledRectangularLines = LineRasterizationFeatures.stippledRectangularLines;
        features14.stippledBresenhamLines = LineRasterizationFeatures.stippledBresenhamLines;
        features14.stippledSmoothLines = LineRasterizationFeatures.stippledSmoothLines;
    }

    if (m_MinorVersion > 2)
        ExtendedDynamicStateFeatures.extendedDynamicState = true;

    m_IsSupported.maintenance4 = features13.maintenance4;
    m_IsSupported.maintenance5 = features14.maintenance5;
    m_IsSupported.maintenance6 = features14.maintenance6;
    m_IsSupported.deviceAddress = features12.bufferDeviceAddress;
    m_IsSupported.swapChainMutableFormat = IsExtensionSupported(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME, desiredDeviceExts);
    m_IsSupported.presentId = PresentIdFeatures.presentId;
    m_IsSupported.memoryPriority = MemoryPriorityFeatures.memoryPriority;
    m_IsSupported.memoryBudget = IsExtensionSupported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, desiredDeviceExts);
    m_IsSupported.imageSlicedView = ImageSlicedViewOf3DFeatures.imageSlicedViewOf3D != 0;
    m_IsSupported.customBorderColor = CustomBorderColorFeatures.customBorderColors != 0 && CustomBorderColorFeatures.customBorderColorWithoutFormat != 0;
    m_IsSupported.robustness = features.features.robustBufferAccess != 0 && features13.robustImageAccess != 0;
    m_IsSupported.robustness2 = Robustness2Features.robustBufferAccess2 != 0 && Robustness2Features.robustImageAccess2 != 0;
    m_IsSupported.pipelineRobustness = features14.pipelineRobustness;
    m_IsSupported.swapChainMaintenance1 = SwapchainMaintenance1Features.swapchainMaintenance1;
    m_IsSupported.fifoLatestReady = PresentModeFifoLatestReadyFeatures.presentModeFifoLatestReady;

    m_IsMemoryZeroInitializationEnabled = desc.enableMemoryZeroInitialization && ZeroInitializeDeviceMemoryFeatures.zeroInitializeDeviceMemory;

    // Check hard requirements
    RETURN_ON_FAILURE(this, ExtendedDynamicStateFeatures.extendedDynamicState != 0, Result::UNSUPPORTED, "'extendedDynamicState' is not supported by the device");
    RETURN_ON_FAILURE(this, features13.dynamicRendering, Result::UNSUPPORTED, "'dynamicRendering' is not supported by the device");
    RETURN_ON_FAILURE(this, features13.synchronization2, Result::UNSUPPORTED, "'synchronization2' is not supported by the device");

    { // Create device
        if (isWrapper)
            m_Device = (VkDevice)descVK.vkDevice;
        else {
            // Disable undesired features
            if (desc.robustness == Robustness::DEFAULT || desc.robustness == Robustness::VK) {
                Robustness2Features.robustBufferAccess2 = 0;
                Robustness2Features.robustImageAccess2 = 0;
            } else if (desc.robustness == Robustness::OFF) {
                Robustness2Features.robustBufferAccess2 = 0;
                Robustness2Features.robustImageAccess2 = 0;
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
                uint32_t queueFamilyIndex = queueFamilyIndices[(size_t)queueFamily.queueType];

                if (queueFamily.queueNum && queueFamilyIndex != INVALID_FAMILY_INDEX) {
                    VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[deviceCreateInfo.queueCreateInfoCount++];

                    queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
                    queueCreateInfo.queueCount = queueFamily.queueNum;
                    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
                    queueCreateInfo.pQueuePriorities = queueFamily.queuePriorities ? queueFamily.queuePriorities : zeroPriorities.data();
                }
            }

            VkResult vkResult = m_VK.CreateDevice(m_PhysicalDevice, &deviceCreateInfo, m_AllocationCallbackPtr, &m_Device);
            RETURN_ON_BAD_VKRESULT(this, vkResult, "vkCreateDevice");
        }

        Result res = ResolveDispatchTable(desiredDeviceExts);
        if (res != Result::SUCCESS)
            return res;
    }

    // Create queues
    memset(m_Desc.adapterDesc.queueNum, 0, sizeof(m_Desc.adapterDesc.queueNum)); // patch to reflect available queues
    if (isWrapper) {
        for (uint32_t i = 0; i < descVK.queueFamilyNum; i++) {
            const QueueFamilyVKDesc& queueFamilyVKDesc = descVK.queueFamilies[i];
            auto& queueFamily = m_QueueFamilies[(size_t)queueFamilyVKDesc.queueType];
            uint32_t queueFamilyIndex = queueFamilyIndices[(size_t)queueFamilyVKDesc.queueType];

            if (queueFamilyIndex != INVALID_FAMILY_INDEX) {
                for (uint32_t j = 0; j < queueFamilyVKDesc.queueNum; j++) {
                    VkDeviceQueueInfo2 queueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
                    queueInfo.queueFamilyIndex = queueFamilyIndex;
                    queueInfo.queueIndex = j;

                    VkQueue handle = VK_NULL_HANDLE;
                    m_VK.GetDeviceQueue2(m_Device, &queueInfo, &handle);

                    QueueVK* queue;
                    Result result = CreateImplementation<QueueVK>(queue, queueFamilyVKDesc.queueType, queueFamilyVKDesc.familyIndex, handle);
                    if (result == Result::SUCCESS)
                        queueFamily.push_back(queue);
                }

                m_Desc.adapterDesc.queueNum[(size_t)queueFamilyVKDesc.queueType] = queueFamilyVKDesc.queueNum;
            } else
                m_Desc.adapterDesc.queueNum[(size_t)queueFamilyVKDesc.queueType] = 0;
        }
    } else {
        for (uint32_t i = 0; i < desc.queueFamilyNum; i++) {
            const QueueFamilyDesc& queueFamilyDesc = desc.queueFamilies[i];
            auto& queueFamily = m_QueueFamilies[(size_t)queueFamilyDesc.queueType];
            uint32_t queueFamilyIndex = queueFamilyIndices[(size_t)queueFamilyDesc.queueType];

            if (queueFamilyIndex != INVALID_FAMILY_INDEX) {
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
            } else
                m_Desc.adapterDesc.queueNum[(size_t)queueFamilyDesc.queueType] = 0;
        }
    }

    { // Desc
        // Device properties
        VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        tail = &props.pNext;

        VkPhysicalDeviceVulkan11Properties props11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
        APPEND_STRUCT(props11);

        VkPhysicalDeviceVulkan12Properties props12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
        APPEND_STRUCT(props12);

        VkPhysicalDeviceVulkan13Properties props13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
        if (m_MinorVersion >= 3) {
            APPEND_STRUCT(props13);
        }

        VkPhysicalDeviceVulkan14Properties props14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES};
        if (m_MinorVersion >= 4) {
            APPEND_STRUCT(props14);
        }

        APPEND_PROPS(m_MinorVersion < 3, KHR, Maintenance4, MAINTENANCE_4);
        APPEND_PROPS(m_MinorVersion < 3, EXT, SubgroupSizeControl, SUBGROUP_SIZE_CONTROL);
        APPEND_PROPS(m_MinorVersion < 4, KHR, PushDescriptor, PUSH_DESCRIPTOR);

        APPEND_PROPS(true, KHR, AccelerationStructure, ACCELERATION_STRUCTURE);
        APPEND_PROPS(true, KHR, ComputeShaderDerivatives, COMPUTE_SHADER_DERIVATIVES);
        APPEND_PROPS(true, KHR, FragmentShadingRate, FRAGMENT_SHADING_RATE);
        APPEND_PROPS(true, KHR, LineRasterization, LINE_RASTERIZATION);
        APPEND_PROPS(true, KHR, Maintenance5, MAINTENANCE_5);
        APPEND_PROPS(true, KHR, Maintenance6, MAINTENANCE_6);
        APPEND_PROPS(true, KHR, Maintenance7, MAINTENANCE_7);
        APPEND_PROPS(true, KHR, Maintenance9, MAINTENANCE_9);
        APPEND_PROPS(true, KHR, RayTracingPipeline, RAY_TRACING_PIPELINE);
        APPEND_PROPS(true, EXT, ConservativeRasterization, CONSERVATIVE_RASTERIZATION);
        APPEND_PROPS(true, EXT, MeshShader, MESH_SHADER);
        APPEND_PROPS(true, EXT, OpacityMicromap, OPACITY_MICROMAP);
        APPEND_PROPS(true, EXT, SampleLocations, SAMPLE_LOCATIONS);

        m_VK.GetPhysicalDeviceProperties2(m_PhysicalDevice, &props);

        if (m_MinorVersion < 3) {
            props13.maxBufferSize = Maintenance4Props.maxBufferSize;
            props13.minSubgroupSize = SubgroupSizeControlProps.minSubgroupSize;
            props13.maxSubgroupSize = SubgroupSizeControlProps.minSubgroupSize;
        }

        if (m_MinorVersion < 4) {
            props14.maxPushDescriptors = PushDescriptorProps.maxPushDescriptors;
        }

        // Fill desc
        const VkPhysicalDeviceLimits& limits = props.properties.limits;

        m_Desc.viewport.maxNum = limits.maxViewports;
        m_Desc.viewport.boundsMin = (int16_t)limits.viewportBoundsRange[0];
        m_Desc.viewport.boundsMax = (int16_t)limits.viewportBoundsRange[1];

        m_Desc.dimensions.attachmentMaxDim = (Dim_t)std::min(limits.maxFramebufferWidth, limits.maxFramebufferHeight);
        m_Desc.dimensions.attachmentLayerMaxNum = (Dim_t)limits.maxFramebufferLayers;
        m_Desc.dimensions.texture1DMaxDim = (Dim_t)limits.maxImageDimension1D;
        m_Desc.dimensions.texture2DMaxDim = (Dim_t)limits.maxImageDimension2D;
        m_Desc.dimensions.texture3DMaxDim = (Dim_t)limits.maxImageDimension3D;
        m_Desc.dimensions.textureLayerMaxNum = (Dim_t)limits.maxImageArrayLayers;
        m_Desc.dimensions.typedBufferMaxDim = limits.maxTexelBufferElements;

        m_Desc.precision.viewportBits = limits.viewportSubPixelBits;
        m_Desc.precision.subPixelBits = limits.subPixelPrecisionBits;
        m_Desc.precision.subTexelBits = limits.subTexelPrecisionBits;
        m_Desc.precision.mipmapBits = limits.mipmapPrecisionBits;

        const VkMemoryPropertyFlags neededFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
            const VkMemoryType& memoryType = m_MemoryProps.memoryTypes[i];
            if ((memoryType.propertyFlags & neededFlags) == neededFlags)
                m_Desc.memory.deviceUploadHeapSize += m_MemoryProps.memoryHeaps[memoryType.heapIndex].size;
        }

        m_Desc.memory.allocationMaxNum = limits.maxMemoryAllocationCount;
        m_Desc.memory.samplerAllocationMaxNum = limits.maxSamplerAllocationCount;
        m_Desc.memory.constantBufferMaxRange = limits.maxUniformBufferRange;
        m_Desc.memory.storageBufferMaxRange = limits.maxStorageBufferRange;
        m_Desc.memory.bufferTextureGranularity = (uint32_t)limits.bufferImageGranularity;
        m_Desc.memory.bufferMaxSize = props13.maxBufferSize;

        // VUID-VkCopyBufferToImageInfo2-dstImage-07975: If "dstImage" does not have either a depth/stencil format or a multi-planar format,
        //      "bufferOffset" must be a multiple of the texel block size
        // VUID-VkCopyBufferToImageInfo2-dstImage-07978: If "dstImage" has a depth/stencil format,
        //      "bufferOffset" must be a multiple of 4
        // Least Common Multiple stride across all formats: 1, 2, 4, 8, 16 // TODO: rarely used "12" fucks up the beauty of power-of-2 numbers, such formats must be avoided!
        constexpr uint32_t leastCommonMultipleStrideAccrossAllFormats = 16;

        m_Desc.memoryAlignment.uploadBufferTextureRow = (uint32_t)limits.optimalBufferCopyRowPitchAlignment;
        m_Desc.memoryAlignment.uploadBufferTextureSlice = std::lcm((uint32_t)limits.optimalBufferCopyOffsetAlignment, leastCommonMultipleStrideAccrossAllFormats);
        m_Desc.memoryAlignment.shaderBindingTable = RayTracingPipelineProps.shaderGroupBaseAlignment;
        m_Desc.memoryAlignment.bufferShaderResourceOffset = std::lcm((uint32_t)limits.minTexelBufferOffsetAlignment, (uint32_t)limits.minStorageBufferOffsetAlignment);
        m_Desc.memoryAlignment.constantBufferOffset = (uint32_t)limits.minUniformBufferOffsetAlignment;
        m_Desc.memoryAlignment.scratchBufferOffset = AccelerationStructureProps.minAccelerationStructureScratchOffsetAlignment;
        m_Desc.memoryAlignment.accelerationStructureOffset = 256; // see the spec
        m_Desc.memoryAlignment.micromapOffset = 256;              // see the spec

        m_Desc.pipelineLayout.descriptorSetMaxNum = limits.maxBoundDescriptorSets;
        m_Desc.pipelineLayout.rootConstantMaxSize = limits.maxPushConstantsSize;
        m_Desc.pipelineLayout.rootDescriptorMaxNum = props14.maxPushDescriptors;

        m_Desc.descriptorSet.samplerMaxNum = limits.maxDescriptorSetSamplers;
        m_Desc.descriptorSet.constantBufferMaxNum = limits.maxDescriptorSetUniformBuffers;
        m_Desc.descriptorSet.storageBufferMaxNum = limits.maxDescriptorSetStorageBuffers;
        m_Desc.descriptorSet.textureMaxNum = limits.maxDescriptorSetSampledImages;
        m_Desc.descriptorSet.storageTextureMaxNum = limits.maxDescriptorSetStorageImages;

        m_Desc.descriptorSet.updateAfterSet.samplerMaxNum = props12.maxDescriptorSetUpdateAfterBindSamplers;
        m_Desc.descriptorSet.updateAfterSet.constantBufferMaxNum = props12.maxDescriptorSetUpdateAfterBindUniformBuffers;
        m_Desc.descriptorSet.updateAfterSet.storageBufferMaxNum = props12.maxDescriptorSetUpdateAfterBindStorageBuffers;
        m_Desc.descriptorSet.updateAfterSet.textureMaxNum = props12.maxDescriptorSetUpdateAfterBindSampledImages;
        m_Desc.descriptorSet.updateAfterSet.storageTextureMaxNum = props12.maxDescriptorSetUpdateAfterBindStorageImages;

        m_Desc.shaderStage.descriptorSamplerMaxNum = limits.maxPerStageDescriptorSamplers;
        m_Desc.shaderStage.descriptorConstantBufferMaxNum = limits.maxPerStageDescriptorUniformBuffers;
        m_Desc.shaderStage.descriptorStorageBufferMaxNum = limits.maxPerStageDescriptorStorageBuffers;
        m_Desc.shaderStage.descriptorTextureMaxNum = limits.maxPerStageDescriptorSampledImages;
        m_Desc.shaderStage.descriptorStorageTextureMaxNum = limits.maxPerStageDescriptorStorageImages;
        m_Desc.shaderStage.resourceMaxNum = limits.maxPerStageResources;

        m_Desc.shaderStage.updateAfterSet.descriptorSamplerMaxNum = props12.maxPerStageDescriptorUpdateAfterBindSamplers;
        m_Desc.shaderStage.updateAfterSet.descriptorConstantBufferMaxNum = props12.maxPerStageDescriptorUpdateAfterBindUniformBuffers;
        m_Desc.shaderStage.updateAfterSet.descriptorStorageBufferMaxNum = props12.maxPerStageDescriptorUpdateAfterBindStorageBuffers;
        m_Desc.shaderStage.updateAfterSet.descriptorTextureMaxNum = props12.maxPerStageDescriptorUpdateAfterBindSampledImages;
        m_Desc.shaderStage.updateAfterSet.descriptorStorageTextureMaxNum = props12.maxPerStageDescriptorUpdateAfterBindStorageImages;
        m_Desc.shaderStage.updateAfterSet.resourceMaxNum = props12.maxPerStageUpdateAfterBindResources;

        m_Desc.shaderStage.vertex.attributeMaxNum = limits.maxVertexInputAttributes;
        m_Desc.shaderStage.vertex.streamMaxNum = limits.maxVertexInputBindings;
        m_Desc.shaderStage.vertex.outputComponentMaxNum = limits.maxVertexOutputComponents;

        m_Desc.shaderStage.tesselationControl.generationMaxLevel = (float)limits.maxTessellationGenerationLevel;
        m_Desc.shaderStage.tesselationControl.patchPointMaxNum = limits.maxTessellationPatchSize;
        m_Desc.shaderStage.tesselationControl.perVertexInputComponentMaxNum = limits.maxTessellationControlPerVertexInputComponents;
        m_Desc.shaderStage.tesselationControl.perVertexOutputComponentMaxNum = limits.maxTessellationControlPerVertexOutputComponents;
        m_Desc.shaderStage.tesselationControl.perPatchOutputComponentMaxNum = limits.maxTessellationControlPerPatchOutputComponents;
        m_Desc.shaderStage.tesselationControl.totalOutputComponentMaxNum = limits.maxTessellationControlTotalOutputComponents;

        m_Desc.shaderStage.tesselationEvaluation.inputComponentMaxNum = limits.maxTessellationEvaluationInputComponents;
        m_Desc.shaderStage.tesselationEvaluation.outputComponentMaxNum = limits.maxTessellationEvaluationOutputComponents;

        m_Desc.shaderStage.geometry.invocationMaxNum = limits.maxGeometryShaderInvocations;
        m_Desc.shaderStage.geometry.inputComponentMaxNum = limits.maxGeometryInputComponents;
        m_Desc.shaderStage.geometry.outputComponentMaxNum = limits.maxGeometryOutputComponents;
        m_Desc.shaderStage.geometry.outputVertexMaxNum = limits.maxGeometryOutputVertices;
        m_Desc.shaderStage.geometry.totalOutputComponentMaxNum = limits.maxGeometryTotalOutputComponents;

        m_Desc.shaderStage.fragment.inputComponentMaxNum = limits.maxFragmentInputComponents;
        m_Desc.shaderStage.fragment.attachmentMaxNum = limits.maxFragmentOutputAttachments;
        m_Desc.shaderStage.fragment.dualSourceAttachmentMaxNum = limits.maxFragmentDualSrcAttachments;

        m_Desc.shaderStage.compute.workGroupMaxNum[0] = limits.maxComputeWorkGroupCount[0];
        m_Desc.shaderStage.compute.workGroupMaxNum[1] = limits.maxComputeWorkGroupCount[1];
        m_Desc.shaderStage.compute.workGroupMaxNum[2] = limits.maxComputeWorkGroupCount[2];
        m_Desc.shaderStage.compute.workGroupMaxDim[0] = limits.maxComputeWorkGroupSize[0];
        m_Desc.shaderStage.compute.workGroupMaxDim[1] = limits.maxComputeWorkGroupSize[1];
        m_Desc.shaderStage.compute.workGroupMaxDim[2] = limits.maxComputeWorkGroupSize[2];
        m_Desc.shaderStage.compute.workGroupInvocationMaxNum = limits.maxComputeWorkGroupInvocations;
        m_Desc.shaderStage.compute.sharedMemoryMaxSize = limits.maxComputeSharedMemorySize;

        m_Desc.shaderStage.rayTracing.shaderGroupIdentifierSize = RayTracingPipelineProps.shaderGroupHandleSize;
        m_Desc.shaderStage.rayTracing.tableMaxStride = RayTracingPipelineProps.maxShaderGroupStride;
        m_Desc.shaderStage.rayTracing.recursionMaxDepth = RayTracingPipelineProps.maxRayRecursionDepth;

        m_Desc.shaderStage.meshControl.sharedMemoryMaxSize = MeshShaderProps.maxTaskSharedMemorySize;
        m_Desc.shaderStage.meshControl.workGroupInvocationMaxNum = MeshShaderProps.maxTaskWorkGroupInvocations;
        m_Desc.shaderStage.meshControl.payloadMaxSize = MeshShaderProps.maxTaskPayloadSize;

        m_Desc.shaderStage.meshEvaluation.outputVerticesMaxNum = MeshShaderProps.maxMeshOutputVertices;
        m_Desc.shaderStage.meshEvaluation.outputPrimitiveMaxNum = MeshShaderProps.maxMeshOutputPrimitives;
        m_Desc.shaderStage.meshEvaluation.outputComponentMaxNum = MeshShaderProps.maxMeshOutputComponents;
        m_Desc.shaderStage.meshEvaluation.sharedMemoryMaxSize = MeshShaderProps.maxMeshSharedMemorySize;
        m_Desc.shaderStage.meshEvaluation.workGroupInvocationMaxNum = MeshShaderProps.maxMeshWorkGroupInvocations;

        m_Desc.wave.laneMinNum = props13.minSubgroupSize;
        m_Desc.wave.laneMaxNum = props13.maxSubgroupSize;

        m_Desc.wave.derivativeOpsStages = StageBits::FRAGMENT_SHADER;
        if (ComputeShaderDerivativesFeatures.computeDerivativeGroupQuads || ComputeShaderDerivativesFeatures.computeDerivativeGroupLinear)
            m_Desc.wave.derivativeOpsStages |= StageBits::COMPUTE_SHADER;
        if (ComputeShaderDerivativesProps.meshAndTaskShaderDerivatives)
            m_Desc.wave.derivativeOpsStages |= StageBits::TASK_SHADER | StageBits::MESH_SHADER;

        if (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT)
            m_Desc.wave.quadOpsStages = props11.subgroupQuadOperationsInAllStages ? StageBits::ALL_SHADERS : (StageBits::FRAGMENT_SHADER | StageBits::COMPUTE_SHADER);

        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_VERTEX_BIT)
            m_Desc.wave.waveOpsStages |= StageBits::VERTEX_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
            m_Desc.wave.waveOpsStages |= StageBits::TESS_CONTROL_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
            m_Desc.wave.waveOpsStages |= StageBits::TESS_EVALUATION_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_GEOMETRY_BIT)
            m_Desc.wave.waveOpsStages |= StageBits::GEOMETRY_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_FRAGMENT_BIT)
            m_Desc.wave.waveOpsStages |= StageBits::FRAGMENT_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_COMPUTE_BIT)
            m_Desc.wave.waveOpsStages |= StageBits::COMPUTE_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            m_Desc.wave.waveOpsStages |= StageBits::RAYGEN_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
            m_Desc.wave.waveOpsStages |= StageBits::ANY_HIT_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            m_Desc.wave.waveOpsStages |= StageBits::CLOSEST_HIT_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_MISS_BIT_KHR)
            m_Desc.wave.waveOpsStages |= StageBits::MISS_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_INTERSECTION_BIT_KHR)
            m_Desc.wave.waveOpsStages |= StageBits::INTERSECTION_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_CALLABLE_BIT_KHR)
            m_Desc.wave.waveOpsStages |= StageBits::CALLABLE_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_TASK_BIT_EXT)
            m_Desc.wave.waveOpsStages |= StageBits::TASK_SHADER;
        if (props11.subgroupSupportedStages & VK_SHADER_STAGE_MESH_BIT_EXT)
            m_Desc.wave.waveOpsStages |= StageBits::MESH_SHADER;

        m_Desc.other.timestampFrequencyHz = uint64_t(1e9 / double(limits.timestampPeriod) + 0.5);
        m_Desc.other.micromapSubdivisionMaxLevel = OpacityMicromapProps.maxOpacity2StateSubdivisionLevel;
        m_Desc.other.drawIndirectMaxNum = limits.maxDrawIndirectCount;
        m_Desc.other.samplerLodBiasMax = limits.maxSamplerLodBias;
        m_Desc.other.samplerAnisotropyMax = limits.maxSamplerAnisotropy;
        m_Desc.other.texelOffsetMin = (int8_t)limits.minTexelOffset;
        m_Desc.other.texelOffsetMax = (uint8_t)limits.maxTexelOffset;
        m_Desc.other.texelGatherOffsetMin = (int8_t)limits.minTexelGatherOffset;
        m_Desc.other.texelGatherOffsetMax = (uint8_t)limits.maxTexelGatherOffset;
        m_Desc.other.clipDistanceMaxNum = (uint8_t)limits.maxClipDistances;
        m_Desc.other.cullDistanceMaxNum = (uint8_t)limits.maxCullDistances;
        m_Desc.other.combinedClipAndCullDistanceMaxNum = (uint8_t)limits.maxCombinedClipAndCullDistances;
        m_Desc.other.viewMaxNum = features11.multiview ? (uint8_t)props11.maxMultiviewViewCount : 1;
        m_Desc.other.shadingRateAttachmentTileSize = (uint8_t)FragmentShadingRateProps.minFragmentShadingRateAttachmentTexelSize.width;

        if (IsExtensionSupported(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, desiredDeviceExts)) {
            m_Desc.tiers.conservativeRaster = 1;
            if (ConservativeRasterizationProps.primitiveOverestimationSize < 1.0f / 2.0f && ConservativeRasterizationProps.degenerateTrianglesRasterized)
                m_Desc.tiers.conservativeRaster = 2;
            if (ConservativeRasterizationProps.primitiveOverestimationSize <= 1.0 / 256.0f && ConservativeRasterizationProps.degenerateTrianglesRasterized)
                m_Desc.tiers.conservativeRaster = 3;
        }

        if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, desiredDeviceExts)) {
            constexpr VkSampleCountFlags allSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT;
            if (SampleLocationsProps.sampleLocationSampleCounts == allSampleCounts) // like in D3D12 spec
                m_Desc.tiers.sampleLocations = 2;
        }

        m_Desc.tiers.rayTracing = AccelerationStructureFeatures.accelerationStructure != 0;
        if (m_Desc.tiers.rayTracing) {
            if (RayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect && RayQueryFeatures.rayQuery)
                m_Desc.tiers.rayTracing++;
            if (OpacityMicromapFeatures.micromap)
                m_Desc.tiers.rayTracing++;
        }

        m_Desc.tiers.shadingRate = FragmentShadingRateFeatures.pipelineFragmentShadingRate != 0;
        if (m_Desc.tiers.shadingRate) {
            if (FragmentShadingRateFeatures.primitiveFragmentShadingRate && FragmentShadingRateFeatures.attachmentFragmentShadingRate)
                m_Desc.tiers.shadingRate = 2;

            m_Desc.features.additionalShadingRates = FragmentShadingRateProps.maxFragmentSize.height > 2 || FragmentShadingRateProps.maxFragmentSize.width > 2;
        }

        m_Desc.tiers.bindless = features12.descriptorIndexing ? 1 : 0;
        m_Desc.tiers.resourceBinding = 2; // TODO: seems to be the best match
        m_Desc.tiers.memory = 1;          // TODO: seems to be the best match

        m_Desc.features.getMemoryDesc2 = m_IsSupported.maintenance4;
        m_Desc.features.enhancedBarriers = true;
        m_Desc.features.swapChain = IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, desiredDeviceExts);
        m_Desc.features.rayTracing = m_Desc.tiers.rayTracing != 0;
        m_Desc.features.meshShader = MeshShaderFeatures.meshShader != 0 && MeshShaderFeatures.taskShader != 0;
        m_Desc.features.lowLatency = m_IsSupported.presentId != 0 && IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts);
        m_Desc.features.micromap = OpacityMicromapFeatures.micromap != 0;

        m_Desc.features.independentFrontAndBackStencilReferenceAndMasks = true;
        m_Desc.features.textureFilterMinMax = features12.samplerFilterMinmax;
        m_Desc.features.logicOp = features.features.logicOp;
        m_Desc.features.depthBoundsTest = features.features.depthBounds;
        m_Desc.features.drawIndirectCount = features12.drawIndirectCount;
        m_Desc.features.lineSmoothing = features14.smoothLines;
        m_Desc.features.copyQueueTimestamp = limits.timestampComputeAndGraphics;
        m_Desc.features.meshShaderPipelineStats = MeshShaderFeatures.meshShaderQueries == VK_TRUE;
        m_Desc.features.dynamicDepthBias = true;
        m_Desc.features.viewportOriginBottomLeft = true;
        m_Desc.features.regionResolve = true;
        m_Desc.features.layerBasedMultiview = features11.multiview;
        m_Desc.features.presentFromCompute = true;
        m_Desc.features.waitableSwapChain = PresentIdFeatures.presentId != 0 && PresentWaitFeatures.presentWait != 0;
        m_Desc.features.resizableSwapChain = m_IsSupported.swapChainMaintenance1;
        m_Desc.features.pipelineStatistics = features.features.pipelineStatisticsQuery;
        m_Desc.features.rootConstantsOffset = true;
        m_Desc.features.nonConstantBufferRootDescriptorOffset = true;
        m_Desc.features.mutableDescriptorType = MutableDescriptorTypeFeatures.mutableDescriptorType;

        m_Desc.shaderFeatures.nativeI16 = features.features.shaderInt16;
        m_Desc.shaderFeatures.nativeF16 = features12.shaderFloat16;
        m_Desc.shaderFeatures.nativeI64 = features.features.shaderInt64;
        m_Desc.shaderFeatures.nativeF64 = features.features.shaderFloat64;
        m_Desc.shaderFeatures.atomicsF16 = (ShaderAtomicFloat2Features.shaderBufferFloat16Atomics || ShaderAtomicFloat2Features.shaderSharedFloat16Atomics) ? true : false;
        m_Desc.shaderFeatures.atomicsF32 = (ShaderAtomicFloatFeatures.shaderBufferFloat32Atomics || ShaderAtomicFloatFeatures.shaderSharedFloat32Atomics) ? true : false;
        m_Desc.shaderFeatures.atomicsI64 = (features12.shaderBufferInt64Atomics || features12.shaderSharedInt64Atomics) ? true : false;
        m_Desc.shaderFeatures.atomicsF64 = (ShaderAtomicFloatFeatures.shaderBufferFloat64Atomics || ShaderAtomicFloatFeatures.shaderSharedFloat64Atomics) ? true : false;
        m_Desc.shaderFeatures.viewportIndex = features12.shaderOutputViewportIndex;
        m_Desc.shaderFeatures.layerIndex = features12.shaderOutputLayer;
        m_Desc.shaderFeatures.unnormalizedCoordinates = true;
        m_Desc.shaderFeatures.clock = (ShaderClockFeatures.shaderDeviceClock || ShaderClockFeatures.shaderSubgroupClock) ? true : false;
        m_Desc.shaderFeatures.rasterizedOrderedView = FragmentShaderInterlockFeatures.fragmentShaderPixelInterlock != 0 && FragmentShaderInterlockFeatures.fragmentShaderSampleInterlock != 0;
        m_Desc.shaderFeatures.barycentric = FragmentShaderBarycentricFeatures.fragmentShaderBarycentric;
        m_Desc.shaderFeatures.rayTracingPositionFetch = RayTracingPositionFetchFeatures.rayTracingPositionFetch;
        m_Desc.shaderFeatures.storageReadWithoutFormat = features.features.shaderStorageImageReadWithoutFormat;
        m_Desc.shaderFeatures.storageWriteWithoutFormat = features.features.shaderStorageImageWriteWithoutFormat;
        m_Desc.shaderFeatures.waveQuery = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_BASIC_BIT) ? true : false;
        m_Desc.shaderFeatures.waveVote = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT) ? true : false;
        m_Desc.shaderFeatures.waveShuffle = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT) ? true : false;
        m_Desc.shaderFeatures.waveArithmetic = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT) ? true : false;
        m_Desc.shaderFeatures.waveReduction = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT) ? true : false;
        m_Desc.shaderFeatures.waveQuad = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT) ? true : false;

        // Estimate shader model last since it depends on many "m_Desc" fields
        // Based on https://docs.vulkan.org/guide/latest/hlsl.html#_shader_model_coverage // TODO: code below needs to be improved
        m_Desc.shaderModel = 51;
        if (m_Desc.shaderFeatures.nativeI64)
            m_Desc.shaderModel = 60;
        if (m_Desc.other.viewMaxNum > 1 || m_Desc.shaderFeatures.barycentric)
            m_Desc.shaderModel = 61;
        if (m_Desc.shaderFeatures.nativeF16 || m_Desc.shaderFeatures.nativeI16)
            m_Desc.shaderModel = 62;
        if (m_Desc.features.rayTracing)
            m_Desc.shaderModel = 63;
        if (m_Desc.tiers.shadingRate >= 2)
            m_Desc.shaderModel = 64;
        if (m_Desc.features.meshShader || m_Desc.tiers.rayTracing >= 2)
            m_Desc.shaderModel = 65;
        // TODO: "m_Desc.features.mutableDescriptorType" is an optional feature, despite that it's needed to emulate SM 6.6 "ultimate" bindless
        if (m_Desc.shaderFeatures.atomicsI64)
            m_Desc.shaderModel = 66;
        if (features.features.shaderStorageImageMultisample)
            m_Desc.shaderModel = 67;
        // TODO: add SM 6.8 and 6.9 detection
    }

    // Create VMA
    VkResult vkResult = CreateVma();
    RETURN_ON_BAD_VKRESULT(this, vkResult, "vmaCreateAllocator");

    if constexpr (VERBOSE)
        ReportMemoryTypes();

    return FillFunctionTable(m_iCore);
}

void DeviceVK::FillCreateInfo(const BufferDesc& bufferDesc, VkBufferCreateInfo& info) const {
    info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO}; // should be already set
    info.size = bufferDesc.size;
    info.usage = GetBufferUsageFlags(bufferDesc.usage, bufferDesc.structureStride, m_IsSupported.deviceAddress);
    info.sharingMode = m_NumActiveFamilyIndices <= 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = m_NumActiveFamilyIndices;
    info.pQueueFamilyIndices = m_ActiveQueueFamilyIndices.data();
}

void DeviceVK::FillCreateInfo(const TextureDesc& textureDesc, VkImageCreateInfo& info) const {
    const FormatProps& formatProps = GetFormatProps(textureDesc.format);

    VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT // typeless (basic)
        | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT                      // typeless (advanced)
        | VK_IMAGE_CREATE_ALIAS_BIT;                              // matches https://learn.microsoft.com/en-us/windows/win32/direct3d12/memory-aliasing-and-data-inheritance#data-inheritance

    if (formatProps.blockWidth > 1 && (textureDesc.usage & TextureUsageBits::SHADER_RESOURCE_STORAGE))
        flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
    if (textureDesc.layerNum >= 6 && textureDesc.width == textureDesc.height)
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
    if (textureDesc.type == TextureType::TEXTURE_3D)
        flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT; // allow 3D demotion to a set of layers // TODO: hook up "VK_EXT_image_2d_view_of_3d"?
    if (m_Desc.tiers.sampleLocations && formatProps.isDepth)
        flags |= VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT;

    info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; // should be already set
    info.flags = flags;
    info.imageType = ::GetImageType(textureDesc.type);
    info.format = ::GetVkFormat(textureDesc.format, true);
    info.extent.width = textureDesc.width;
    info.extent.height = std::max(textureDesc.height, (Dim_t)1);
    info.extent.depth = std::max(textureDesc.depth, (Dim_t)1);
    info.mipLevels = std::max(textureDesc.mipNum, (Dim_t)1);
    info.arrayLayers = std::max(textureDesc.layerNum, (Dim_t)1);
    info.samples = (VkSampleCountFlagBits)std::max(textureDesc.sampleNum, (Sample_t)1);
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = GetImageUsageFlags(textureDesc.usage);
    info.sharingMode = (m_NumActiveFamilyIndices <= 1 || textureDesc.sharingMode == SharingMode::EXCLUSIVE) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = m_NumActiveFamilyIndices;
    info.pQueueFamilyIndices = m_ActiveQueueFamilyIndices.data();
    info.initialLayout = IsMemoryZeroInitializationEnabled() ? VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT : VK_IMAGE_LAYOUT_UNDEFINED;
}

void DeviceVK::FillCreateInfo(const SamplerDesc& samplerDesc, VkSamplerCreateInfo& info, VkSamplerReductionModeCreateInfo& reductionModeInfo, VkSamplerCustomBorderColorCreateInfoEXT& borderColorInfo) const {
    info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO}; // should be already set
    info.magFilter = GetFilter(samplerDesc.filters.mag);
    info.minFilter = GetFilter(samplerDesc.filters.min);
    info.mipmapMode = GetSamplerMipmapMode(samplerDesc.filters.mip);
    info.addressModeU = GetSamplerAddressMode(samplerDesc.addressModes.u);
    info.addressModeV = GetSamplerAddressMode(samplerDesc.addressModes.v);
    info.addressModeW = GetSamplerAddressMode(samplerDesc.addressModes.w);
    info.mipLodBias = samplerDesc.mipBias;
    info.anisotropyEnable = VkBool32(samplerDesc.anisotropy > 1.0f);
    info.maxAnisotropy = (float)samplerDesc.anisotropy;
    info.compareEnable = VkBool32(samplerDesc.compareOp != CompareOp::NONE);
    info.compareOp = GetCompareOp(samplerDesc.compareOp);
    info.minLod = samplerDesc.mipMin;
    info.maxLod = samplerDesc.mipMax;
    info.unnormalizedCoordinates = samplerDesc.unnormalizedCoordinates ? VK_TRUE : VK_FALSE;

    const void** tail = &info.pNext;

    reductionModeInfo = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO}; // should be already set
    if (m_Desc.features.textureFilterMinMax) {
        reductionModeInfo.reductionMode = GetFilterExt(samplerDesc.filters.ext);

        APPEND_STRUCT(reductionModeInfo);
    }

    borderColorInfo = {VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT}; // should be already set
    const Color& borderColor = samplerDesc.borderColor;
    if (borderColor.ui.x == 0 && borderColor.ui.y == 0 && borderColor.ui.z == 0 && borderColor.ui.w == 0)
        info.borderColor = samplerDesc.isInteger ? VK_BORDER_COLOR_INT_TRANSPARENT_BLACK : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    else if (samplerDesc.isInteger && borderColor.ui.x == 1 && borderColor.ui.y == 1 && borderColor.ui.z == 1 && borderColor.ui.w == 1)
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    else if (samplerDesc.isInteger && borderColor.ui.x == 0 && borderColor.ui.y == 0 && borderColor.ui.z == 0 && borderColor.ui.w == 1)
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    else if (!samplerDesc.isInteger && borderColor.f.x == 1.0f && borderColor.f.y == 1.0f && borderColor.f.z == 1.0f && borderColor.f.w == 1.0f)
        info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    else if (!samplerDesc.isInteger && borderColor.f.x == 0.0f && borderColor.f.y == 0.0f && borderColor.f.z == 0.0f && borderColor.f.w == 1.0f)
        info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    else if (m_IsSupported.customBorderColor) {
        info.borderColor = samplerDesc.isInteger ? VK_BORDER_COLOR_INT_CUSTOM_EXT : VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;

        static_assert(sizeof(VkClearColorValue) == sizeof(samplerDesc.borderColor), "Unexpected sizeof");
        memcpy(&borderColorInfo.customBorderColor, &samplerDesc.borderColor, sizeof(borderColorInfo.customBorderColor));

        APPEND_STRUCT(borderColorInfo);
    }
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

    memoryDesc = {};
    GetMemoryDesc(memoryLocation, requirements.memoryRequirements, dedicatedRequirements, memoryDesc);
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

    memoryDesc = {};
    GetMemoryDesc(memoryLocation, requirements.memoryRequirements, dedicatedRequirements, memoryDesc);
}

void DeviceVK::GetMemoryDesc2(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    VkAccelerationStructureBuildSizesInfoKHR sizesInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    GetAccelerationStructureBuildSizesInfo(accelerationStructureDesc, sizesInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = sizesInfo.accelerationStructureSize;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    GetMemoryDesc2(bufferDesc, memoryLocation, memoryDesc);
}

void DeviceVK::GetMemoryDesc2(const MicromapDesc& micromapDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    VkMicromapBuildSizesInfoEXT sizesInfo = {VK_STRUCTURE_TYPE_MICROMAP_BUILD_SIZES_INFO_EXT};
    GetMicromapBuildSizesInfo(micromapDesc, sizesInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = sizesInfo.micromapSize;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    GetMemoryDesc2(bufferDesc, memoryLocation, memoryDesc);
}

bool DeviceVK::GetMemoryDesc(MemoryLocation memoryLocation, const VkMemoryRequirements& memoryRequirements, const VkMemoryDedicatedRequirements& memoryDedicatedRequirements, MemoryDesc& memoryDesc) const {
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

    memoryDesc = {};

    for (uint32_t phase = 0; phase < 4; phase++) {
        for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
            bool isSupported = memoryRequirements.memoryTypeBits & (1 << i);
            bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;
            bool hasUndesiredFlags = undesiredFlags == 0 ? false : (m_MemoryProps.memoryTypes[i].propertyFlags & undesiredFlags) == undesiredFlags;
            bool hasDesiredFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & desiredFlags) == desiredFlags;

            bool isOK = isSupported && hasNeededFlags; // phase 3 - only needed
            if (phase == 0)
                isOK = isOK && !hasUndesiredFlags && hasDesiredFlags; // phase 0 - needed, undesired and desired
            else if (phase == 1)
                isOK = isOK && !hasUndesiredFlags; // phase 1 - needed, undesired
            else if (phase == 2)
                isOK = isOK && hasDesiredFlags; // phase 2 - needed and desired

            if (isOK) {
                MemoryTypeInfo memoryTypeInfo = {};
                memoryTypeInfo.index = (MemoryTypeIndex)i;
                memoryTypeInfo.location = memoryLocation;

                // "prefersDedicatedAllocation" seems to be "too soft" making more allocations "dedicated", "requiresDedicatedAllocation" better matches D3D12
                memoryTypeInfo.mustBeDedicated = memoryDedicatedRequirements.requiresDedicatedAllocation;

                memoryDesc.size = memoryRequirements.size;
                memoryDesc.alignment = (uint32_t)memoryRequirements.alignment;
                memoryDesc.type = Pack(memoryTypeInfo);
                memoryDesc.mustBeDedicated = memoryTypeInfo.mustBeDedicated;

                return true;
            }
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
    // Allocate scratch
    uint32_t geometryNum = 0;
    uint32_t micromapNum = 0;

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        geometryNum = accelerationStructureDesc.geometryOrInstanceNum;

        for (uint32_t i = 0; i < geometryNum; i++) {
            const BottomLevelGeometryDesc& geometryDesc = accelerationStructureDesc.geometries[i];

            if (geometryDesc.type == BottomLevelGeometryType::TRIANGLES && geometryDesc.triangles.micromap)
                micromapNum++;
        }
    } else
        geometryNum = 1;

    Scratch<uint32_t> primitiveNums = AllocateScratch(*this, uint32_t, geometryNum);
    Scratch<VkAccelerationStructureGeometryKHR> geometries = AllocateScratch(*this, VkAccelerationStructureGeometryKHR, geometryNum);
    Scratch<VkAccelerationStructureTrianglesOpacityMicromapEXT> trianglesMicromaps = AllocateScratch(*this, VkAccelerationStructureTrianglesOpacityMicromapEXT, micromapNum);

    // Convert geometries
    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        micromapNum = ConvertBotomLevelGeometries(nullptr, geometries, trianglesMicromaps, accelerationStructureDesc.geometries, geometryNum);

        for (uint32_t i = 0; i < geometryNum; i++) {
            const BottomLevelGeometryDesc& in = accelerationStructureDesc.geometries[i];

            if (in.type == BottomLevelGeometryType::TRIANGLES) {
                uint32_t triangleNum = (in.triangles.indexNum ? in.triangles.indexNum : in.triangles.vertexNum) / 3;
                primitiveNums[i] = triangleNum;
            } else if (in.type == BottomLevelGeometryType::AABBS)
                primitiveNums[i] = in.aabbs.num;
        }
    } else {
        geometries[0] = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometries[0].geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometries[0].geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;

        primitiveNums[0] = accelerationStructureDesc.geometryOrInstanceNum;
    }

    // Get sizes
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = GetAccelerationStructureType(accelerationStructureDesc.type);
    buildInfo.flags = GetBuildAccelerationStructureFlags(accelerationStructureDesc.flags);
    buildInfo.geometryCount = geometryNum;
    buildInfo.pGeometries = geometries;

    const auto& vk = GetDispatchTable();
    vk.GetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveNums, &sizesInfo);
}

void DeviceVK::GetMicromapBuildSizesInfo(const MicromapDesc& micromapDesc, VkMicromapBuildSizesInfoEXT& sizesInfo) {
    static_assert((uint32_t)MicromapFormat::OPACITY_2_STATE == VK_OPACITY_MICROMAP_FORMAT_2_STATE_EXT, "Format doesn't match");
    static_assert((uint32_t)MicromapFormat::OPACITY_4_STATE == VK_OPACITY_MICROMAP_FORMAT_4_STATE_EXT, "Format doesn't match");

    Scratch<VkMicromapUsageEXT> usages = AllocateScratch(*this, VkMicromapUsageEXT, micromapDesc.usageNum);
    for (uint32_t i = 0; i < micromapDesc.usageNum; i++) {
        const MicromapUsageDesc& in = micromapDesc.usages[i];

        VkMicromapUsageEXT& out = usages[i];
        out = {};
        out.count = in.triangleNum;
        out.subdivisionLevel = in.subdivisionLevel;
        out.format = (VkOpacityMicromapFormatEXT)in.format;
    }

    VkMicromapBuildInfoEXT buildInfo = {VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT};
    buildInfo.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
    buildInfo.flags = GetBuildMicromapFlags(micromapDesc.flags);
    buildInfo.usageCountsCount = micromapDesc.usageNum;
    buildInfo.pUsageCounts = usages;

    const auto& vk = GetDispatchTable();
    vk.GetMicromapBuildSizesEXT(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &sizesInfo);
}

Result DeviceVK::CreateInstance(bool enableGraphicsAPIValidation, const Vector<const char*>& desiredInstanceExts) {
    Vector<const char*> layers(GetStdAllocator());
    if (enableGraphicsAPIValidation)
        layers.push_back("VK_LAYER_KHRONOS_validation");

    FilterInstanceLayers(layers);

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_4;

    const VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT, // TODO: add VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT?
    };

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
#ifdef __APPLE__
    instanceCreateInfo.flags = (VkInstanceCreateFlags)VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = (uint32_t)layers.size();
    instanceCreateInfo.ppEnabledLayerNames = layers.data();
    instanceCreateInfo.enabledExtensionCount = (uint32_t)desiredInstanceExts.size();
    instanceCreateInfo.ppEnabledExtensionNames = desiredInstanceExts.data();

    const void** tail = &instanceCreateInfo.pNext;

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    messengerCreateInfo.pUserData = this;
    messengerCreateInfo.pfnUserCallback = MessageCallback;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    APPEND_STRUCT(messengerCreateInfo);

    VkValidationFeaturesEXT validationFeatures = {VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    validationFeatures.enabledValidationFeatureCount = GetCountOf(enabledValidationFeatures);
    validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

    if (enableGraphicsAPIValidation) {
        APPEND_STRUCT(validationFeatures);
    }

    VkResult vkResult = m_VK.CreateInstance(&instanceCreateInfo, m_AllocationCallbackPtr, &m_Instance);
    RETURN_ON_BAD_VKRESULT(this, vkResult, "vkCreateInstance");

    if (enableGraphicsAPIValidation) {
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)m_VK.GetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        vkResult = vkCreateDebugUtilsMessengerEXT(m_Instance, &messengerCreateInfo, m_AllocationCallbackPtr, &m_Messenger);

        RETURN_ON_BAD_VKRESULT(this, vkResult, "vkCreateDebugUtilsMessengerEXT");
    }

    return Result::SUCCESS;
}

void DeviceVK::SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name) {
    if (!m_VK.SetDebugUtilsObjectNameEXT)
        return;

    VkDebugUtilsObjectNameInfoEXT objectNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, objectType, (uint64_t)handle, name};

    VkResult vkResult = m_VK.SetDebugUtilsObjectNameEXT(m_Device, &objectNameInfo);
    RETURN_VOID_ON_BAD_VKRESULT(this, vkResult, "vkSetDebugUtilsObjectNameEXT");
}

void DeviceVK::ReportMemoryTypes() {
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

#define MERGE_TOKENS2(a, b)    a##b
#define MERGE_TOKENS3(a, b, c) a##b##c

#define GET_DEVICE_OPTIONAL_CORE_FUNC(name) \
    { \
        /* Core */ \
        m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        /* KHR */ \
        if (!m_VK.name) \
            m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS3(vk, name, KHR))); \
        /* EXT (some extensions were promoted to core from EXT bypassing KHR status) */ \
        if (!m_VK.name) \
            m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS3(vk, name, EXT))); \
    }

#define GET_DEVICE_CORE_FUNC(name) \
    { \
        GET_DEVICE_OPTIONAL_CORE_FUNC(name); \
        if (!m_VK.name) { \
            REPORT_ERROR(this, "Failed to get device function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
            return Result::UNSUPPORTED; \
        } \
    }

#define GET_DEVICE_FUNC(name) \
    { \
        m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        if (!m_VK.name) { \
            REPORT_ERROR(this, "Failed to get device function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
            return Result::UNSUPPORTED; \
        } \
    }

#define GET_INSTANCE_FUNC(name) \
    { \
        m_VK.name = (PFN_vk##name)m_VK.GetInstanceProcAddr(m_Instance, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        if (!m_VK.name) { \
            REPORT_ERROR(this, "Failed to get instance function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
            return Result::UNSUPPORTED; \
        } \
    }

Result DeviceVK::ResolvePreInstanceDispatchTable() {
    m_VK = {};

    m_VK.GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*m_Loader, "vkGetInstanceProcAddr");
    if (!m_VK.GetInstanceProcAddr) {
        REPORT_ERROR(this, "Failed to get 'vkGetInstanceProcAddr'");
        return Result::UNSUPPORTED;
    }

    GET_INSTANCE_FUNC(CreateInstance);
    GET_INSTANCE_FUNC(EnumerateInstanceExtensionProperties);
    GET_INSTANCE_FUNC(EnumerateInstanceLayerProperties);

    return Result::SUCCESS;
}

Result DeviceVK::ResolveInstanceDispatchTable(const Vector<const char*>& desiredInstanceExts) {
    GET_INSTANCE_FUNC(GetDeviceProcAddr);
    GET_INSTANCE_FUNC(DestroyInstance);
    GET_INSTANCE_FUNC(DestroyDevice);
    GET_INSTANCE_FUNC(GetPhysicalDeviceMemoryProperties2);
    GET_INSTANCE_FUNC(GetDeviceGroupPeerMemoryFeatures);
    GET_INSTANCE_FUNC(GetPhysicalDeviceFormatProperties2);
    GET_INSTANCE_FUNC(GetPhysicalDeviceImageFormatProperties2);
    GET_INSTANCE_FUNC(CreateDevice);
    GET_INSTANCE_FUNC(GetDeviceQueue2);
    GET_INSTANCE_FUNC(EnumeratePhysicalDeviceGroups);
    GET_INSTANCE_FUNC(GetPhysicalDeviceProperties2);
    GET_INSTANCE_FUNC(GetPhysicalDeviceFeatures2);
    GET_INSTANCE_FUNC(GetPhysicalDeviceQueueFamilyProperties2);
    GET_INSTANCE_FUNC(EnumerateDeviceExtensionProperties);

    if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_FUNC(SetDebugUtilsObjectNameEXT);
        GET_INSTANCE_FUNC(CmdBeginDebugUtilsLabelEXT);
        GET_INSTANCE_FUNC(CmdEndDebugUtilsLabelEXT);
        GET_INSTANCE_FUNC(CmdInsertDebugUtilsLabelEXT);
        GET_INSTANCE_FUNC(QueueBeginDebugUtilsLabelEXT);
        GET_INSTANCE_FUNC(QueueEndDebugUtilsLabelEXT);
        GET_INSTANCE_FUNC(QueueInsertDebugUtilsLabelEXT);
    }

    if (IsExtensionSupported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_FUNC(GetPhysicalDeviceSurfaceFormats2KHR);
        GET_INSTANCE_FUNC(GetPhysicalDeviceSurfaceCapabilities2KHR);
    }

    if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_FUNC(GetPhysicalDeviceSurfaceSupportKHR);
        GET_INSTANCE_FUNC(GetPhysicalDeviceSurfacePresentModesKHR);
        GET_INSTANCE_FUNC(DestroySurfaceKHR);

#ifdef VK_USE_PLATFORM_WIN32_KHR
        GET_INSTANCE_FUNC(CreateWin32SurfaceKHR);
        GET_INSTANCE_FUNC(GetMemoryWin32HandlePropertiesKHR);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        GET_INSTANCE_FUNC(CreateXlibSurfaceKHR);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        GET_INSTANCE_FUNC(CreateWaylandSurfaceKHR);
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
        GET_INSTANCE_FUNC(CreateMetalSurfaceEXT);
#endif
    }

    return Result::SUCCESS;
}

Result DeviceVK::ResolveDispatchTable(const Vector<const char*>& desiredDeviceExts) {
    GET_DEVICE_CORE_FUNC(CreateBuffer);
    GET_DEVICE_CORE_FUNC(CreateImage);
    GET_DEVICE_CORE_FUNC(CreateBufferView);
    GET_DEVICE_CORE_FUNC(CreateImageView);
    GET_DEVICE_CORE_FUNC(CreateSampler);
    GET_DEVICE_CORE_FUNC(CreateQueryPool);
    GET_DEVICE_CORE_FUNC(CreateCommandPool);
    GET_DEVICE_CORE_FUNC(CreateSemaphore);
    GET_DEVICE_CORE_FUNC(CreateDescriptorPool);
    GET_DEVICE_CORE_FUNC(CreatePipelineLayout);
    GET_DEVICE_CORE_FUNC(CreateDescriptorSetLayout);
    GET_DEVICE_CORE_FUNC(CreateShaderModule);
    GET_DEVICE_CORE_FUNC(CreateGraphicsPipelines);
    GET_DEVICE_CORE_FUNC(CreateComputePipelines);
    GET_DEVICE_CORE_FUNC(AllocateMemory);
    GET_DEVICE_CORE_FUNC(DestroyBuffer);
    GET_DEVICE_CORE_FUNC(DestroyImage);
    GET_DEVICE_CORE_FUNC(DestroyBufferView);
    GET_DEVICE_CORE_FUNC(DestroyImageView);
    GET_DEVICE_CORE_FUNC(DestroySampler);
    GET_DEVICE_CORE_FUNC(DestroyFramebuffer);
    GET_DEVICE_CORE_FUNC(DestroyQueryPool);
    GET_DEVICE_CORE_FUNC(DestroyCommandPool);
    GET_DEVICE_CORE_FUNC(DestroySemaphore);
    GET_DEVICE_CORE_FUNC(DestroyDescriptorPool);
    GET_DEVICE_CORE_FUNC(DestroyPipelineLayout);
    GET_DEVICE_CORE_FUNC(DestroyDescriptorSetLayout);
    GET_DEVICE_CORE_FUNC(DestroyShaderModule);
    GET_DEVICE_CORE_FUNC(DestroyPipeline);
    GET_DEVICE_CORE_FUNC(FreeMemory);
    GET_DEVICE_CORE_FUNC(FreeCommandBuffers);
    GET_DEVICE_CORE_FUNC(MapMemory);
    GET_DEVICE_CORE_FUNC(FlushMappedMemoryRanges);
    GET_DEVICE_CORE_FUNC(QueueWaitIdle);
    GET_DEVICE_CORE_FUNC(QueueSubmit2);
    GET_DEVICE_CORE_FUNC(GetSemaphoreCounterValue);
    GET_DEVICE_CORE_FUNC(WaitSemaphores);
    GET_DEVICE_CORE_FUNC(ResetCommandPool);
    GET_DEVICE_CORE_FUNC(ResetDescriptorPool);
    GET_DEVICE_CORE_FUNC(AllocateCommandBuffers);
    GET_DEVICE_CORE_FUNC(AllocateDescriptorSets);
    GET_DEVICE_CORE_FUNC(UpdateDescriptorSets);
    GET_DEVICE_CORE_FUNC(BindBufferMemory2);
    GET_DEVICE_CORE_FUNC(BindImageMemory2);
    GET_DEVICE_CORE_FUNC(GetBufferMemoryRequirements2);
    GET_DEVICE_CORE_FUNC(GetImageMemoryRequirements2);
    GET_DEVICE_CORE_FUNC(ResetQueryPool);
    GET_DEVICE_CORE_FUNC(GetBufferDeviceAddress);
    GET_DEVICE_CORE_FUNC(BeginCommandBuffer);
    GET_DEVICE_CORE_FUNC(CmdSetViewportWithCount);
    GET_DEVICE_CORE_FUNC(CmdSetScissorWithCount);
    GET_DEVICE_CORE_FUNC(CmdSetDepthBounds);
    GET_DEVICE_CORE_FUNC(CmdSetStencilReference);
    GET_DEVICE_CORE_FUNC(CmdSetBlendConstants);
    GET_DEVICE_CORE_FUNC(CmdSetDepthBias);
    GET_DEVICE_CORE_FUNC(CmdClearAttachments);
    GET_DEVICE_CORE_FUNC(CmdClearColorImage);
    GET_DEVICE_CORE_FUNC(CmdBindVertexBuffers2);
    GET_DEVICE_CORE_FUNC(CmdBindIndexBuffer);
    GET_DEVICE_CORE_FUNC(CmdBindPipeline);
    GET_DEVICE_CORE_FUNC(CmdBindDescriptorSets);
    GET_DEVICE_CORE_FUNC(CmdPushConstants);
    GET_DEVICE_CORE_FUNC(CmdDispatch);
    GET_DEVICE_CORE_FUNC(CmdDispatchIndirect);
    GET_DEVICE_CORE_FUNC(CmdDraw);
    GET_DEVICE_CORE_FUNC(CmdDrawIndexed);
    GET_DEVICE_CORE_FUNC(CmdDrawIndirect);
    GET_DEVICE_CORE_FUNC(CmdDrawIndirectCount);
    GET_DEVICE_CORE_FUNC(CmdDrawIndexedIndirect);
    GET_DEVICE_CORE_FUNC(CmdDrawIndexedIndirectCount);
    GET_DEVICE_CORE_FUNC(CmdCopyBuffer2);
    GET_DEVICE_CORE_FUNC(CmdCopyImage2);
    GET_DEVICE_CORE_FUNC(CmdResolveImage2);
    GET_DEVICE_CORE_FUNC(CmdCopyBufferToImage2);
    GET_DEVICE_CORE_FUNC(CmdCopyImageToBuffer2);
    GET_DEVICE_CORE_FUNC(CmdPipelineBarrier2);
    GET_DEVICE_CORE_FUNC(CmdBeginQuery);
    GET_DEVICE_CORE_FUNC(CmdEndQuery);
    GET_DEVICE_CORE_FUNC(CmdWriteTimestamp2);
    GET_DEVICE_CORE_FUNC(CmdCopyQueryPoolResults);
    GET_DEVICE_CORE_FUNC(CmdResetQueryPool);
    GET_DEVICE_CORE_FUNC(CmdFillBuffer);
    GET_DEVICE_CORE_FUNC(CmdBeginRendering);
    GET_DEVICE_CORE_FUNC(CmdEndRendering);
    GET_DEVICE_CORE_FUNC(CmdPushDescriptorSet);
    GET_DEVICE_CORE_FUNC(EndCommandBuffer);

    GET_DEVICE_OPTIONAL_CORE_FUNC(GetDeviceBufferMemoryRequirements);
    GET_DEVICE_OPTIONAL_CORE_FUNC(GetDeviceImageMemoryRequirements);
    GET_DEVICE_OPTIONAL_CORE_FUNC(CmdBindIndexBuffer2);
    GET_DEVICE_OPTIONAL_CORE_FUNC(CmdBindDescriptorSets2);
    GET_DEVICE_OPTIONAL_CORE_FUNC(CmdPushConstants2);

    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, desiredDeviceExts))
        GET_DEVICE_FUNC(CmdSetFragmentShadingRateKHR);

    if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_FUNC(AcquireNextImage2KHR);
        GET_DEVICE_FUNC(QueuePresentKHR);
        GET_DEVICE_FUNC(CreateSwapchainKHR);
        GET_DEVICE_FUNC(DestroySwapchainKHR);
        GET_DEVICE_FUNC(GetSwapchainImagesKHR);
    }

    if (IsExtensionSupported(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, desiredDeviceExts))
        GET_DEVICE_FUNC(WaitForPresentKHR);

    if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_FUNC(CreateAccelerationStructureKHR);
        GET_DEVICE_FUNC(DestroyAccelerationStructureKHR);
        GET_DEVICE_FUNC(GetAccelerationStructureDeviceAddressKHR);
        GET_DEVICE_FUNC(GetAccelerationStructureBuildSizesKHR);
        GET_DEVICE_FUNC(CmdBuildAccelerationStructuresKHR);
        GET_DEVICE_FUNC(CmdCopyAccelerationStructureKHR);
        GET_DEVICE_FUNC(CmdWriteAccelerationStructuresPropertiesKHR);
    }

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_FUNC(CreateRayTracingPipelinesKHR);
        GET_DEVICE_FUNC(GetRayTracingShaderGroupHandlesKHR);
        GET_DEVICE_FUNC(CmdTraceRaysKHR);
        GET_DEVICE_FUNC(CmdTraceRaysIndirect2KHR);
    }

    if (IsExtensionSupported(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_FUNC(CreateMicromapEXT);
        GET_DEVICE_FUNC(DestroyMicromapEXT);
        GET_DEVICE_FUNC(GetMicromapBuildSizesEXT);
        GET_DEVICE_FUNC(CmdBuildMicromapsEXT);
        GET_DEVICE_FUNC(CmdCopyMicromapEXT);
        GET_DEVICE_FUNC(CmdWriteMicromapsPropertiesEXT);
    }

    if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_FUNC(CmdSetSampleLocationsEXT);
    }

    if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_FUNC(CmdDrawMeshTasksEXT);
        GET_DEVICE_FUNC(CmdDrawMeshTasksIndirectEXT);
        GET_DEVICE_FUNC(CmdDrawMeshTasksIndirectCountEXT);
    }

    if (IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_FUNC(GetLatencyTimingsNV);
        GET_DEVICE_FUNC(LatencySleepNV);
        GET_DEVICE_FUNC(SetLatencyMarkerNV);
        GET_DEVICE_FUNC(SetLatencySleepModeNV);
    }

    return Result::SUCCESS;
}

#undef MERGE_TOKENS2
#undef MERGE_TOKENS3
#undef GET_DEVICE_OPTIONAL_CORE_FUNC
#undef GET_DEVICE_CORE_FUNC
#undef GET_DEVICE_FUNC
#undef GET_INSTANCE_FUNC

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

NRI_INLINE Result DeviceVK::WaitIdle() {
    // Don't use "vkDeviceWaitIdle" because it requires host access synchronization to all queues, better do it one by one instead
    for (auto& queueFamily : m_QueueFamilies) {
        for (auto queue : queueFamily) {
            Result result = queue->WaitIdle();
            if (result != Result::SUCCESS)
                return result;
        }
    }

    return Result::SUCCESS;
}

NRI_INLINE void DeviceVK::CopyDescriptorRanges(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum) {
    Scratch<VkCopyDescriptorSet> copies = AllocateScratch(*this, VkCopyDescriptorSet, copyDescriptorRangeDescNum);
    for (uint32_t i = 0; i < copyDescriptorRangeDescNum; i++) {
        const CopyDescriptorRangeDesc& copyDescriptorSetDesc = copyDescriptorRangeDescs[i];

        const DescriptorSetVK& dst = *(DescriptorSetVK*)copyDescriptorSetDesc.dstDescriptorSet;
        const DescriptorSetVK& src = *(DescriptorSetVK*)copyDescriptorSetDesc.srcDescriptorSet;

        const DescriptorRangeDesc& dstRangeDesc = dst.GetDesc()->ranges[copyDescriptorSetDesc.dstRangeIndex];
        const DescriptorRangeDesc& srcRangeDesc = src.GetDesc()->ranges[copyDescriptorSetDesc.srcRangeIndex];

        uint32_t descriptorNum = copyDescriptorSetDesc.descriptorNum;
        if (!descriptorNum)
            descriptorNum = srcRangeDesc.descriptorNum;

        VkCopyDescriptorSet& copy = copies[i];
        copy = {VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
        copy.srcSet = src.GetHandle();
        copy.dstSet = dst.GetHandle();
        copy.descriptorCount = descriptorNum;

        bool isDstArray = dstRangeDesc.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        if (isDstArray) {
            copy.dstBinding = dstRangeDesc.baseRegisterIndex;
            copy.dstArrayElement = copyDescriptorSetDesc.dstBaseDescriptor;
        } else
            copy.dstBinding = dstRangeDesc.baseRegisterIndex + copyDescriptorSetDesc.dstBaseDescriptor;
        bool isSrcArray = srcRangeDesc.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        if (isSrcArray) {
            copy.srcBinding = srcRangeDesc.baseRegisterIndex;
            copy.srcArrayElement = copyDescriptorSetDesc.srcBaseDescriptor;
        } else
            copy.srcBinding = srcRangeDesc.baseRegisterIndex + copyDescriptorSetDesc.srcBaseDescriptor;
    }

    m_VK.UpdateDescriptorSets(m_Device, 0, nullptr, copyDescriptorRangeDescNum, copies);
}

static void WriteSamplers(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const UpdateDescriptorRangeDesc& rangeUpdateDesc) {
    VkDescriptorImageInfo* imageInfos = (VkDescriptorImageInfo*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkDescriptorImageInfo);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        imageInfos[i].imageView = VK_NULL_HANDLE;
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfos[i].sampler = descriptorVK.GetSampler();
    }

    writeDescriptorSet.pImageInfo = imageInfos;
}

static void WriteTextures(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const UpdateDescriptorRangeDesc& rangeUpdateDesc) {
    VkDescriptorImageInfo* imageInfos = (VkDescriptorImageInfo*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkDescriptorImageInfo);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];

        imageInfos[i].imageView = descriptorVK.GetImageView();
        imageInfos[i].imageLayout = descriptorVK.GetTexDesc().layout;
        imageInfos[i].sampler = VK_NULL_HANDLE;
    }

    writeDescriptorSet.pImageInfo = imageInfos;
}

static void WriteBuffers(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const UpdateDescriptorRangeDesc& rangeUpdateDesc) {
    VkDescriptorBufferInfo* bufferInfos = (VkDescriptorBufferInfo*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkDescriptorBufferInfo);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptor = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        bufferInfos[i] = descriptor.GetBufferInfo();
    }

    writeDescriptorSet.pBufferInfo = bufferInfos;
}

static void WriteTypedBuffers(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const UpdateDescriptorRangeDesc& rangeUpdateDesc) {
    VkBufferView* bufferViews = (VkBufferView*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkBufferView);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        bufferViews[i] = descriptorVK.GetBufferView();
    }

    writeDescriptorSet.pTexelBufferView = bufferViews;
}

static void WriteAccelerationStructures(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const UpdateDescriptorRangeDesc& rangeUpdateDesc) {
    VkAccelerationStructureKHR* accelerationStructures = (VkAccelerationStructureKHR*)(scratch + scratchOffset);
    scratchOffset += rangeUpdateDesc.descriptorNum * sizeof(VkAccelerationStructureKHR);

    for (uint32_t i = 0; i < rangeUpdateDesc.descriptorNum; i++) {
        const DescriptorVK& descriptorVK = *(DescriptorVK*)rangeUpdateDesc.descriptors[i];
        accelerationStructures[i] = descriptorVK.GetAccelerationStructure();
    }

    VkWriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo = (VkWriteDescriptorSetAccelerationStructureKHR*)(scratch + scratchOffset);
    scratchOffset += sizeof(VkWriteDescriptorSetAccelerationStructureKHR);

    accelerationStructureInfo->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureInfo->pNext = nullptr;
    accelerationStructureInfo->accelerationStructureCount = rangeUpdateDesc.descriptorNum;
    accelerationStructureInfo->pAccelerationStructures = accelerationStructures;

    writeDescriptorSet.pNext = accelerationStructureInfo;
}

typedef void (*WriteDescriptorsFunc)(VkWriteDescriptorSet& writeDescriptorSet, size_t& scratchOffset, uint8_t* scratch, const UpdateDescriptorRangeDesc& rangeUpdateDesc);

constexpr std::array<WriteDescriptorsFunc, (size_t)DescriptorType::MAX_NUM> g_WriteFuncs = {
    WriteSamplers,               // SAMPLER
    WriteBuffers,                // CONSTANT_BUFFER
    WriteTextures,               // TEXTURE
    WriteTextures,               // STORAGE_TEXTURE
    WriteTypedBuffers,           // BUFFER
    WriteTypedBuffers,           // STORAGE_BUFFER
    WriteBuffers,                // STRUCTURED_BUFFER
    WriteBuffers,                // STORAGE_STRUCTURED_BUFFER
    WriteAccelerationStructures, // ACCELERATION_STRUCTURE
};
VALIDATE_ARRAY_BY_PTR(g_WriteFuncs);

NRI_INLINE void DeviceVK::UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum) {
    // Count and allocate scratch memory
    size_t scratchOffset = updateDescriptorRangeDescNum * sizeof(VkWriteDescriptorSet);
    size_t scratchSize = scratchOffset;
    for (uint32_t i = 0; i < updateDescriptorRangeDescNum; i++) {
        const UpdateDescriptorRangeDesc& updateDescriptorRangeDesc = updateDescriptorRangeDescs[i];
        const DescriptorSetVK& dst = *(DescriptorSetVK*)updateDescriptorRangeDesc.descriptorSet;
        const DescriptorRangeDesc& rangeDesc = dst.GetDesc()->ranges[updateDescriptorRangeDesc.rangeIndex];

        switch (rangeDesc.descriptorType) {
            case DescriptorType::SAMPLER:
            case DescriptorType::TEXTURE:
            case DescriptorType::STORAGE_TEXTURE:
                scratchSize += sizeof(VkDescriptorImageInfo) * updateDescriptorRangeDesc.descriptorNum;
                break;
            case DescriptorType::CONSTANT_BUFFER:
            case DescriptorType::STRUCTURED_BUFFER:
            case DescriptorType::STORAGE_STRUCTURED_BUFFER:
                scratchSize += sizeof(VkDescriptorBufferInfo) * updateDescriptorRangeDesc.descriptorNum;
                break;
            case DescriptorType::BUFFER:
            case DescriptorType::STORAGE_BUFFER:
                scratchSize += sizeof(VkBufferView) * updateDescriptorRangeDesc.descriptorNum;
                break;

            case DescriptorType::ACCELERATION_STRUCTURE:
                scratchSize += sizeof(VkAccelerationStructureKHR) * updateDescriptorRangeDesc.descriptorNum + sizeof(VkWriteDescriptorSetAccelerationStructureKHR);
                break;

            default:
                CHECK(false, "Unexpected");
                break;
        }
    }

    Scratch<uint8_t> writes = AllocateScratch(*this, uint8_t, scratchSize);

    // Update ranges
    for (uint32_t i = 0; i < updateDescriptorRangeDescNum; i++) {
        const UpdateDescriptorRangeDesc& updateDescriptorRangeDesc = updateDescriptorRangeDescs[i];
        const DescriptorSetVK& dst = *(DescriptorSetVK*)updateDescriptorRangeDesc.descriptorSet;
        const DescriptorRangeDesc& rangeDesc = dst.GetDesc()->ranges[updateDescriptorRangeDesc.rangeIndex];

        VkWriteDescriptorSet& write = *(VkWriteDescriptorSet*)(writes + i * sizeof(VkWriteDescriptorSet)); // must be first and consecutive in "scratch"
        write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = dst.GetHandle();
        write.descriptorCount = updateDescriptorRangeDesc.descriptorNum;
        write.descriptorType = GetDescriptorType(rangeDesc.descriptorType);

        bool isArray = rangeDesc.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        if (isArray) {
            write.dstBinding = rangeDesc.baseRegisterIndex;
            write.dstArrayElement = updateDescriptorRangeDesc.baseDescriptor;
        } else
            write.dstBinding = rangeDesc.baseRegisterIndex + updateDescriptorRangeDesc.baseDescriptor;

        g_WriteFuncs[(uint32_t)rangeDesc.descriptorType](write, scratchOffset, writes, updateDescriptorRangeDesc);
    }

    m_VK.UpdateDescriptorSets(m_Device, updateDescriptorRangeDescNum, (VkWriteDescriptorSet*)(writes + 0), 0, nullptr);
}

NRI_INLINE Result DeviceVK::BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum) {
    Scratch<VkBindBufferMemoryInfo> infos = AllocateScratch(*this, VkBindBufferMemoryInfo, bindBufferMemoryDescNum);
    for (uint32_t i = 0; i < bindBufferMemoryDescNum; i++) {
        const BindBufferMemoryDesc& bindBufferMemoryDesc = bindBufferMemoryDescs[i];

        BufferVK& bufferVK = *(BufferVK*)bindBufferMemoryDesc.buffer;
        MemoryVK& memoryVK = *(MemoryVK*)bindBufferMemoryDesc.memory;

        MemoryTypeInfo memoryTypeInfo = Unpack(memoryVK.GetType());
        if (memoryTypeInfo.mustBeDedicated)
            memoryVK.CreateDedicated(&bufferVK, nullptr);

        VkBindBufferMemoryInfo& info = infos[i];
        info = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
        info.buffer = bufferVK.GetHandle();
        info.memory = memoryVK.GetHandle();
        info.memoryOffset = memoryVK.GetOffset() + bindBufferMemoryDesc.offset;
    }

    VkResult vkResult = m_VK.BindBufferMemory2(m_Device, bindBufferMemoryDescNum, infos);
    RETURN_ON_BAD_VKRESULT(this, vkResult, "vkBindBufferMemory2");

    for (uint32_t i = 0; i < bindBufferMemoryDescNum; i++) {
        const BindBufferMemoryDesc& bindBufferMemoryDesc = bindBufferMemoryDescs[i];

        BufferVK& bufferVK = *(BufferVK*)bindBufferMemoryDesc.buffer;
        MemoryVK& memoryVK = *(MemoryVK*)bindBufferMemoryDesc.memory;

        bufferVK.BindMemory(memoryVK, bindBufferMemoryDesc.offset, false);
    }

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceVK::BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum) {
    Scratch<VkBindImageMemoryInfo> infos = AllocateScratch(*this, VkBindImageMemoryInfo, bindTextureMemoryDescNum);
    for (uint32_t i = 0; i < bindTextureMemoryDescNum; i++) {
        const BindTextureMemoryDesc& bindTextureMemoryDesc = bindTextureMemoryDescs[i];

        MemoryVK& memoryVK = *(MemoryVK*)bindTextureMemoryDesc.memory;
        TextureVK& textureVK = *(TextureVK*)bindTextureMemoryDesc.texture;

        MemoryTypeInfo memoryTypeInfo = Unpack(memoryVK.GetType());
        if (memoryTypeInfo.mustBeDedicated)
            memoryVK.CreateDedicated(nullptr, &textureVK);

        VkBindImageMemoryInfo& info = infos[i];
        info = {VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
        info.image = textureVK.GetHandle();
        info.memory = memoryVK.GetHandle();
        info.memoryOffset = memoryVK.GetOffset() + bindTextureMemoryDesc.offset;
    }

    VkResult vkResult = m_VK.BindImageMemory2(m_Device, bindTextureMemoryDescNum, infos);
    RETURN_ON_BAD_VKRESULT(this, vkResult, "vkBindImageMemory2");

    return Result::SUCCESS;
}

NRI_INLINE Result DeviceVK::BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum) {
    Scratch<BindBufferMemoryDesc> bufferMemoryBindingDescs = AllocateScratch(*this, BindBufferMemoryDesc, bindAccelerationStructureMemoryDescNum);
    for (uint32_t i = 0; i < bindAccelerationStructureMemoryDescNum; i++) {
        const BindAccelerationStructureMemoryDesc& bindAccelerationStructureMemoryDesc = bindAccelerationStructureMemoryDescs[i];
        AccelerationStructureVK& accelerationStructureVK = *(AccelerationStructureVK*)bindAccelerationStructureMemoryDesc.accelerationStructure;

        BindBufferMemoryDesc& desc = bufferMemoryBindingDescs[i];

        desc = {};
        desc.buffer = (Buffer*)accelerationStructureVK.GetBuffer();
        desc.memory = bindAccelerationStructureMemoryDesc.memory;
        desc.offset = bindAccelerationStructureMemoryDesc.offset;
    }

    Result result = BindBufferMemory(bufferMemoryBindingDescs, bindAccelerationStructureMemoryDescNum);

    for (uint32_t i = 0; i < bindAccelerationStructureMemoryDescNum && result == Result::SUCCESS; i++) {
        const BindAccelerationStructureMemoryDesc& bindAccelerationStructureMemoryDesc = bindAccelerationStructureMemoryDescs[i];
        AccelerationStructureVK& accelerationStructureVK = *(AccelerationStructureVK*)bindAccelerationStructureMemoryDesc.accelerationStructure;

        result = accelerationStructureVK.BindMemory(nullptr, 0);
    }

    return result;
}

NRI_INLINE Result DeviceVK::BindMicromapMemory(const BindMicromapMemoryDesc* bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum) {
    Scratch<BindBufferMemoryDesc> bindBufferMemoryDescs = AllocateScratch(*this, BindBufferMemoryDesc, bindMicromapMemoryDescNum);
    for (uint32_t i = 0; i < bindMicromapMemoryDescNum; i++) {
        const BindMicromapMemoryDesc& bindMicromapMemoryDesc = bindMicromapMemoryDescs[i];
        MicromapVK& micromapVK = *(MicromapVK*)bindMicromapMemoryDesc.micromap;

        BindBufferMemoryDesc& desc = bindBufferMemoryDescs[i];

        desc = {};
        desc.buffer = (Buffer*)micromapVK.GetBuffer();
        desc.memory = bindMicromapMemoryDesc.memory;
        desc.offset = bindMicromapMemoryDesc.offset;
    }

    Result result = BindBufferMemory(bindBufferMemoryDescs, bindMicromapMemoryDescNum);

    for (uint32_t i = 0; i < bindMicromapMemoryDescNum && result == Result::SUCCESS; i++) {
        const BindMicromapMemoryDesc& bindMicromapMemoryDesc = bindMicromapMemoryDescs[i];
        MicromapVK& micromap = *(MicromapVK*)bindMicromapMemoryDesc.micromap;

        result = micromap.BindMemory(nullptr, 0);
    }

    return result;
}

#define UPDATE_TEXTURE_SUPPORT_BITS(required, bit) \
    if ((props3.optimalTilingFeatures & (required)) == (required)) \
        supportBits |= bit;

#define UPDATE_BUFFER_SUPPORT_BITS(required, bit) \
    if ((props3.bufferFeatures & (required)) == (required)) \
        supportBits |= bit;

NRI_INLINE FormatSupportBits DeviceVK::GetFormatSupport(Format format) const {
    FormatSupportBits supportBits = FormatSupportBits::UNSUPPORTED;
    VkFormat vkFormat = GetVkFormat(format);

    VkFormatProperties3 props3 = {VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3};
    VkFormatProperties2 props2 = {VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2, &props3};
    m_VK.GetPhysicalDeviceFormatProperties2(m_PhysicalDevice, vkFormat, &props2);

    UPDATE_TEXTURE_SUPPORT_BITS(VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT, FormatSupportBits::TEXTURE);
    UPDATE_TEXTURE_SUPPORT_BITS(VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT, FormatSupportBits::STORAGE_TEXTURE);
    UPDATE_TEXTURE_SUPPORT_BITS(VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT, FormatSupportBits::COLOR_ATTACHMENT);
    UPDATE_TEXTURE_SUPPORT_BITS(VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT, FormatSupportBits::DEPTH_STENCIL_ATTACHMENT);
    UPDATE_TEXTURE_SUPPORT_BITS(VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BLEND_BIT, FormatSupportBits::BLEND);
    UPDATE_TEXTURE_SUPPORT_BITS(VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT, FormatSupportBits::STORAGE_TEXTURE_ATOMICS);

    UPDATE_BUFFER_SUPPORT_BITS(VK_FORMAT_FEATURE_2_UNIFORM_TEXEL_BUFFER_BIT, FormatSupportBits::BUFFER);
    UPDATE_BUFFER_SUPPORT_BITS(VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT, FormatSupportBits::STORAGE_BUFFER);
    UPDATE_BUFFER_SUPPORT_BITS(VK_FORMAT_FEATURE_2_VERTEX_BUFFER_BIT, FormatSupportBits::VERTEX_BUFFER);
    UPDATE_BUFFER_SUPPORT_BITS(VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_ATOMIC_BIT, FormatSupportBits::STORAGE_BUFFER_ATOMICS);

    if ((props3.optimalTilingFeatures | props3.bufferFeatures) & VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT)
        supportBits |= FormatSupportBits::STORAGE_READ_WITHOUT_FORMAT;

    if ((props3.optimalTilingFeatures | props3.bufferFeatures) & VK_FORMAT_FEATURE_2_STORAGE_WRITE_WITHOUT_FORMAT_BIT)
        supportBits |= FormatSupportBits::STORAGE_WRITE_WITHOUT_FORMAT;

    VkPhysicalDeviceImageFormatInfo2 imageInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2};
    imageInfo.format = vkFormat;
    imageInfo.type = VK_IMAGE_TYPE_2D;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.flags = 0; // TODO: kinda needed, but unknown here

    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (supportBits & FormatSupportBits::TEXTURE)
        imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (supportBits & FormatSupportBits::DEPTH_STENCIL_ATTACHMENT)
        imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (supportBits & FormatSupportBits::COLOR_ATTACHMENT)
        imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageFormatProperties2 imageProps = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2};
    m_VK.GetPhysicalDeviceImageFormatProperties2(m_PhysicalDevice, &imageInfo, &imageProps);

    if (imageProps.imageFormatProperties.sampleCounts & VK_SAMPLE_COUNT_2_BIT)
        supportBits |= FormatSupportBits::MULTISAMPLE_2X;
    if (imageProps.imageFormatProperties.sampleCounts & VK_SAMPLE_COUNT_4_BIT)
        supportBits |= FormatSupportBits::MULTISAMPLE_4X;
    if (imageProps.imageFormatProperties.sampleCounts & VK_SAMPLE_COUNT_8_BIT)
        supportBits |= FormatSupportBits::MULTISAMPLE_8X;

    return supportBits;
}

#undef UPDATE_TEXTURE_SUPPORT_BITS
#undef UPDATE_BUFFER_SUPPORT_BITS

NRI_INLINE Result DeviceVK::QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const {
    videoMemoryInfo = {};

    if (!m_IsSupported.memoryBudget)
        return Result::UNSUPPORTED;

    VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};

    VkPhysicalDeviceMemoryProperties2 memoryProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    memoryProps.pNext = &budgetProps;

    const auto& vk = GetDispatchTable();
    vk.GetPhysicalDeviceMemoryProperties2(m_PhysicalDevice, &memoryProps);

    bool isLocal = memoryLocation == MemoryLocation::DEVICE || memoryLocation == MemoryLocation::DEVICE_UPLOAD;

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
