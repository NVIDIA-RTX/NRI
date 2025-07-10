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

// Callbacks must be thread safe
NriStruct(AllocationCallbacks) {
    void* (*Allocate)(void* userArg, size_t size, size_t alignment);
    void* (*Reallocate)(void* userArg, void* memory, size_t size, size_t alignment);
    void (*Free)(void* userArg, void* memory);
    void* userArg;
    bool disable3rdPartyAllocationCallbacks; // to use "AllocationCallbacks" only for NRI needs
};

NriStruct(CallbackInterface) {
    void (*MessageCallback)(Nri(Message) messageType, const char* file, uint32_t line, const char* message, void* userArg);
    NriOptional void (*AbortExecution)(void* userArg); // break on "Message::ERROR" if provided
    NriOptional void* userArg;
};

// Use largest offset for the resource type planned to be used as an unbounded array
NriStruct(VKBindingOffsets) {
    uint32_t samplerOffset;
    uint32_t textureOffset;
    uint32_t constantBufferOffset;
    uint32_t storageTextureAndBufferOffset;
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

NriStruct(DeviceCreationDesc) {
    Nri(GraphicsAPI) graphicsAPI;
    NriOptional Nri(Robustness) robustness;
    NriOptional const NriPtr(AdapterDesc) adapterDesc;
    NriOptional Nri(CallbackInterface) callbackInterface;
    NriOptional Nri(AllocationCallbacks) allocationCallbacks;

    // One "GRAPHICS" queue is created by default
    NriOptional const NriPtr(QueueFamilyDesc) queueFamilies;
    NriOptional uint32_t queueFamilyNum;        // put "GRAPHICS" queue at the beginning of the list

    // D3D specific
    NriOptional uint32_t d3dShaderExtRegister;  // vendor specific shader extensions (default is "NRI_SHADER_EXT_REGISTER", space is always "0")
    NriOptional uint32_t d3dZeroBufferSize;     // no "memset" functionality in D3D, "CmdZeroBuffer" implemented via a bunch of copies (4 Mb by default)

    // Vulkan specific
    Nri(VKBindingOffsets) vkBindingOffsets;
    NriOptional Nri(VKExtensions) vkExtensions; // to enable

    // Switches (disabled by default)
    bool enableNRIValidation;                   // embedded validation layer, checks for NRI specifics
    bool enableGraphicsAPIValidation;           // GAPI-provided validation layer
    bool enableD3D11CommandBufferEmulation;     // enable? but why? (auto-enabled if deferred contexts are not supported)
    bool enableD3D12RayTracingValidation;       // slow but useful, can only be enabled if envvar "NV_ALLOW_RAYTRACING_VALIDATION" is set to "1"

    // Switches (enabled by default)
    bool disableVKRayTracing;                   // to save CPU memory in some implementations
    bool disableD3D12EnhancedBarriers;          // even if AgilitySDK is in use, some apps still use legacy barriers. It can be important for integrations
};

// if "adapterDescs == NULL", then "adapterDescNum" is set to the number of adapters
// else "adapterDescNum" must be set to number of elements in "adapterDescs"
NRI_API Nri(Result) NRI_CALL nriEnumerateAdapters(NriPtr(AdapterDesc) adapterDescs, NonNriRef(uint32_t) adapterDescNum);

NRI_API Nri(Result) NRI_CALL nriCreateDevice(const NriRef(DeviceCreationDesc) deviceCreationDesc, NriOut NriRef(Device*) device);
NRI_API void NRI_CALL nriDestroyDevice(NriRef(Device) device);

// It's global state for D3D, not needed for VK because validation is tied to the logical device
NRI_API void NRI_CALL nriReportLiveObjects();

NriNamespaceEnd
