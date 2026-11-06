// © 2021 NVIDIA Corporation

// Goal: device creation

#pragma once

#define NRI_DEVICE_CREATION_H 1

NriNamespaceBegin

NriEnum(Message, uint8_t,
    INFO,
    WARNING,
    ERROR // "wingdi.h" must not be included after
);

NriEnum(DeviceLostInfoLevel, uint8_t,
    NONE,
    BASIC,   // backend-supported basic device lost reporting
    VERBOSE  // D3D12: additionally enables DRED breadcrumb contexts
);

NriStruct(DeviceLostDump) {
    NriOptional const void* data; // opaque backend-specific binary data, NRI-owned and valid until device destruction
    uint64_t size;                // bytes, 0 if "data" is NULL
};

// Callbacks must be thread safe
NriStruct(AllocationCallbacks) {
    void* (NRI_CALL *Allocate)(void* userArg, size_t size, size_t alignment);
    void* (NRI_CALL *Reallocate)(void* userArg, void* memory, size_t size, size_t alignment);
    void (NRI_CALL *Free)(void* userArg, void* memory);
    void* userArg;
    bool disable3rdPartyAllocationCallbacks; // to use "AllocationCallbacks" only for NRI needs
};

NriStruct(CallbackInterface) {
    void (NRI_CALL *MessageCallback)(Nri(Message) messageType, const char* file, uint32_t line, const char* message, void* userArg);
    NriOptional void (NRI_CALL *AbortExecution)(void* userArg); // break on "Message::ERROR" if provided
    NriOptional void* userArg;
};

// Use largest offset for the resource type planned to be used as an unbounded array
NriStruct(VKBindingOffsets) {
    uint32_t sRegister; // samplers
    uint32_t tRegister; // shader resources, including acceleration structures (SRVs)
    uint32_t bRegister; // constant buffers
    uint32_t uRegister; // storage shader resources (UAVs)
};

NriStruct(VKExtensions) {
    const char* const* instanceExtensions;
    uint32_t instanceExtensionNum;
    const char* const* deviceExtensions;
    uint32_t deviceExtensionNum;
};

// A collection of queues of the same type
NriStruct(QueueFamilyDesc) {
    NriOptional const float* queuePriorities;   // [-1; 1]: low < 0, normal = 0, high > 0 ("queueNum" entries expected)
    uint32_t queueNum;
    Nri(QueueType) queueType;
};

// D3D12: process-global settings affect all subsequently created D3D12 devices; later NRI device creation does not disable them
NriStruct(DeviceCreationDesc) {
    Nri(GraphicsAPI) graphicsAPI;
    NriOptional Nri(Robustness) robustness;
    NriOptional Nri(DeviceLostInfoLevel) deviceLostInfoLevel; // D3D12: process-global
    NriOptional const NriPtr(AdapterDesc) adapterDesc;
    NriOptional Nri(CallbackInterface) callbackInterface;
    NriOptional Nri(AllocationCallbacks) allocationCallbacks;

    // One "GRAPHICS" queue is created by default
    NriOptional const NriPtr(QueueFamilyDesc) queueFamilies;
    NriOptional uint32_t queueFamilyNum;        // put "GRAPHICS" queue at the beginning of the list

    // D3D specific
    NriOptional uint32_t d3dShaderExtRegister;  // vendor specific shader extensions (default is "NRI_SHADER_EXT_REGISTER", space is always "0")
    NriOptional uint32_t d3dZeroBufferSize;     // no "memset" functionality in D3D, "CmdZeroBuffer" implemented via a bunch of copies (4 Mb by default)

    // Directory holding "D3D12Core.dll" of the Agility SDK NRI was built against:
    // - useful for hosts that cannot export "D3D12SDKVersion" / "D3D12SDKPath" (managed runtimes, plugins)
    // - process-global: NULL keeps the current SDK selection, including a previously supplied custom path
    // - existing global configuration is preserved; switching to a different runtime fails while incompatible devices remain alive
    // - may be silently ignored if not supported
    NriOptional const char* d3d12AgilitySdkPath;

    // Vulkan specific
    Nri(VKBindingOffsets) vkBindingOffsets;
    NriOptional Nri(VKExtensions) vkExtensions; // to enable
    NriOptional const void* vkInstanceCreateInfoNext; // appended to "VkInstanceCreateInfo::pNext" after NRI's "VkDebugUtilsMessengerCreateInfoEXT" and "VkValidationFeaturesEXT" (i.e. "VkLayerSettingsCreateInfoEXT")

    // Switches (disabled by default)
    bool enableNRIValidation;                   // embedded validation layer, checks for NRI specifics
    bool enableGraphicsAPIValidation;           // GAPI-provided validation layer (D3D12: process-global; METAL: ignored, set "MTL_DEBUG_LAYER=1" instead)
    bool enableD3D11CommandBufferEmulation;     // enable? but why? (auto-enabled if deferred contexts are not supported)
    bool enableD3D12RayTracingValidation;       // slow but useful, can only be enabled if envvar "NV_ALLOW_RAYTRACING_VALIDATION" is set to "1"
    bool enableD3D12GpuBasedValidation;         // D3D12: "ID3D12Debug1::SetEnableGPUBasedValidation" (slow, process-global, requires "enableGraphicsAPIValidation")
    bool enableMemoryZeroInitialization;        // page-clears are fast, but memory is not cleared by default in VK

    // Switches (enabled by default)
    bool disableVKRayTracing;                   // to save CPU memory in some implementations
    bool disableD3D12EnhancedBarriers;          // even if AgilitySDK is in use, some apps still use legacy barriers. It can be important for integrations
    bool disableD3D12BreakOnError;              // D3D12: don't "SetBreakOnSeverity" for "ERROR" and "CORRUPTION" (messages still go through "CallbackInterface", if supported)
};

// if "adapterDescs == NULL", then "adapterDescNum" is set to the number of adapters
// else "adapterDescNum" must be set to number of elements in "adapterDescs"
// "desiredGraphicsAPIs" selects backends to probe, not adapters to return; "ALL" probes every enabled backend
// Adapter properties can differ depending on the probed backends
NRI_API Nri(Result) NRI_CALL nriEnumerateAdapters(NriPtr(AdapterDesc) adapterDescs, NonNriRef(uint32_t) adapterDescNum, Nri(GraphicsAPI) desiredGraphicsAPIs NriDefault(GraphicsAPI::ALL));

NRI_API Nri(Result) NRI_CALL nriCreateDevice(const NriRef(DeviceCreationDesc) deviceCreationDesc, NriOut NriRef(Device*) device);
NRI_API void NRI_CALL nriDestroyDevice(NriPtr(Device) device);

// It's global state for D3D, not needed for VK because validation is tied to the logical device
NRI_API void NRI_CALL nriReportLiveObjects();

// Reports backend-specific device lost diagnostics and returns a binary dump (if available), "device" must be in a lost state (see "DEVICE_LOST")
NRI_API Nri(Result) NRI_CALL nriReportDeviceLostInfo(NriRef(Device) device, NriOut NriRef(DeviceLostDump) deviceLostDump);

NriNamespaceEnd
