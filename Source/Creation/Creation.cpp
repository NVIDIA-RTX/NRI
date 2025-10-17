// © 2021 NVIDIA Corporation

#ifdef _WIN32
#    include <windows.h> // OutputDebugStringA
#else
#    include <csignal> // raise
#    include <cstdlib> // malloc
#endif

#if NRI_ENABLE_NVTX_SUPPORT
#    include "nvtx3/nvToolsExt.h"
#endif

#if (NRI_ENABLE_D3D11_SUPPORT)
#    include <d3d11.h>
#    include <dxgidebug.h>
#endif

#if NRI_ENABLE_D3D12_SUPPORT
#    include <d3d12.h>
#    include <dxgidebug.h>
#endif

#if NRI_ENABLE_VK_SUPPORT
#    include <vulkan/vulkan.h>
#endif

#include "SharedExternal.h"

#define ADAPTER_MAX_NUM 32

using namespace nri;

Result CreateDeviceNONE(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D11(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D11Desc& deviceCreationDescD3D11, DeviceBase*& device);
Result CreateDeviceD3D12(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationDescD3D12, DeviceBase*& device);
Result CreateDeviceVK(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationVKDesc& deviceCreationDescVK, DeviceBase*& device);
DeviceBase* CreateDeviceValidation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& device);

constexpr uint64_t Hash(const char* name) {
    return *name != 0 ? *name ^ (33 * Hash(name + 1)) : 5381;
}

constexpr std::array<const char*, (size_t)Message::MAX_NUM> g_messageTypes = {
    "INFO",    // INFO,
    "WARNING", // WARNING,
    "ERROR",   // ERROR
};
VALIDATE_ARRAY_BY_PTR(g_messageTypes);

static void NRI_CALL MessageCallback(Message messageType, const char* file, uint32_t line, const char* message, void*) {
    const char* messageTypeName = g_messageTypes[(size_t)messageType];

    char buf[MAX_MESSAGE_LENGTH];
    snprintf(buf, sizeof(buf), "%s (%s:%u) - %s\n", messageTypeName, file, line, message);

    fprintf(stderr, "%s", buf);
#ifdef _WIN32
    OutputDebugStringA(buf);
#endif
}

static void NRI_CALL AbortExecution(void*) {
#ifdef _WIN32
    DebugBreak();
#else
    raise(SIGTRAP);
#endif
}

#ifdef _WIN32

static void* NRI_CALL AlignedMalloc(void*, size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

static void* NRI_CALL AlignedRealloc(void*, void* memory, size_t size, size_t alignment) {
    return _aligned_realloc(memory, size, alignment);
}

static void NRI_CALL AlignedFree(void*, void* memory) {
    _aligned_free(memory);
}

#else

static void* NRI_CALL AlignedMalloc(void*, size_t size, size_t alignment) {
    uint8_t* memory = (uint8_t*)malloc(size + sizeof(uint8_t*) + alignment - 1);
    if (!memory)
        return nullptr;

    uint8_t* alignedMemory = Align(memory + sizeof(uint8_t*), alignment);
    uint8_t** memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = memory;

    return alignedMemory;
}

static void* NRI_CALL AlignedRealloc(void* userArg, void* memory, size_t size, size_t alignment) {
    if (!memory)
        return AlignedMalloc(userArg, size, alignment);

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;

    uint8_t* newMemory = (uint8_t*)realloc(oldMemory, size + sizeof(uint8_t*) + alignment - 1);
    if (!newMemory)
        return nullptr;

    if (newMemory == oldMemory)
        return memory;

    uint8_t* alignedMemory = Align(newMemory + sizeof(uint8_t*), alignment);
    memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = newMemory;

    return alignedMemory;
}

static void NRI_CALL AlignedFree(void*, void* memory) {
    if (!memory)
        return;

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;
    free(oldMemory);
}

#endif

static void CheckAndSetDefaultCallbacks(DeviceCreationDesc& deviceCreationDesc) {
    if (!deviceCreationDesc.callbackInterface.MessageCallback)
        deviceCreationDesc.callbackInterface.MessageCallback = MessageCallback;

    if (!deviceCreationDesc.callbackInterface.AbortExecution)
        deviceCreationDesc.callbackInterface.AbortExecution = AbortExecution;

    if (!deviceCreationDesc.allocationCallbacks.Allocate || !deviceCreationDesc.allocationCallbacks.Reallocate || !deviceCreationDesc.allocationCallbacks.Free) {
        deviceCreationDesc.allocationCallbacks.Allocate = AlignedMalloc;
        deviceCreationDesc.allocationCallbacks.Reallocate = AlignedRealloc;
        deviceCreationDesc.allocationCallbacks.Free = AlignedFree;
    }
}

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT || NRI_ENABLE_VK_SUPPORT)

static int SortAdapters(const void* pa, const void* pb) {
    constexpr uint64_t SHIFT = 60ull;
    static_assert((uint64_t)Architecture::MAX_NUM <= 1ull << (64ull - SHIFT), "Adjust SHIFT");

    const AdapterDesc* a = (AdapterDesc*)pa;
    uint64_t sa = a->videoMemorySize + a->sharedSystemMemorySize;
    sa |= (uint64_t)(a->architecture) << SHIFT;

    const AdapterDesc* b = (AdapterDesc*)pb;
    uint64_t sb = b->videoMemorySize + b->sharedSystemMemorySize;
    sb |= (uint64_t)(b->architecture) << SHIFT;

    if (sa > sb)
        return -1;

    if (sa < sb)
        return 1;

    return 0;
}

#endif

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)

static Result EnumerateAdaptersD3D(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, uint64_t precreatedDeviceLuid, DeviceCreationDesc* deviceCreationDesc) {
    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return Result::UNSUPPORTED;

    uint32_t adaptersNum = 0;
    IDXGIAdapter1* adapters[ADAPTER_MAX_NUM];

    for (uint32_t i = 0; i < GetCountOf(adapters); i++) {
        IDXGIAdapter1* adapter;
        HRESULT hr = dxgifactory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc = {};
        if (adapter->GetDesc1(&desc) == S_OK) {
            if (desc.Flags == DXGI_ADAPTER_FLAG_NONE) // TODO: DXGI_ADAPTER_FLAG_REMOTE, DXGI_ADAPTER_FLAG_SOFTWARE?
                adapters[adaptersNum++] = adapter;
        }
    }

    if (!adaptersNum)
        return Result::FAILURE;

    if (adapterDescs) {
        AdapterDesc* adapterDescsSorted = (AdapterDesc*)alloca(sizeof(AdapterDesc) * adaptersNum);
        for (uint32_t i = 0; i < adaptersNum; i++) {
            DXGI_ADAPTER_DESC desc = {};
            adapters[i]->GetDesc(&desc);

            AdapterDesc& adapterDesc = adapterDescsSorted[i];
            adapterDesc = {};
            adapterDesc.uid.low = *(uint64_t*)&desc.AdapterLuid;
            adapterDesc.deviceId = desc.DeviceId;
            adapterDesc.vendor = GetVendorFromID(desc.VendorId);
            adapterDesc.videoMemorySize = desc.DedicatedVideoMemory; // TODO: add "desc.DedicatedSystemMemory"?
            adapterDesc.sharedSystemMemorySize = desc.SharedSystemMemory;
            wcstombs(adapterDesc.name, desc.Description, GetCountOf(adapterDesc.name) - 1);

            { // Architecture (a device is needed)
#    if (NRI_ENABLE_D3D11_SUPPORT)
                D3D_FEATURE_LEVEL levels[2] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
                uint32_t levelNum = GetCountOf(levels);
                ComPtr<ID3D11Device> device;
                HRESULT hr = D3D11CreateDevice(adapters[i], D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, levels, levelNum, D3D11_SDK_VERSION, &device, nullptr, nullptr);
                if (SUCCEEDED(hr)) {
                    D3D11_FEATURE_DATA_D3D11_OPTIONS2 options2 = {};
                    hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &options2, sizeof(options2));
                    if (SUCCEEDED(hr))
                        adapterDesc.architecture = options2.UnifiedMemoryArchitecture ? Architecture::INTEGRATED : Architecture::DESCRETE;
                }
#    else
                ComPtr<ID3D12Device> device;
                HRESULT hr = D3D12CreateDevice(adapters[i], D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device);
                if (SUCCEEDED(hr)) {
                    D3D12_FEATURE_DATA_ARCHITECTURE architecture = {};
                    hr = device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof(architecture));
                    if (SUCCEEDED(hr))
                        adapterDesc.architecture = architecture.UMA ? Architecture::INTEGRATED : Architecture::DESCRETE;
                }
#    endif
            }

            // Let's advertise reasonable values (can't be queried in D3D)
            adapterDesc.queueNum[(uint32_t)QueueType::GRAPHICS] = 4;
            adapterDesc.queueNum[(uint32_t)QueueType::COMPUTE] = 4;
            adapterDesc.queueNum[(uint32_t)QueueType::COPY] = 4;
        }

        // Sort by video memory size
        qsort(adapterDescsSorted, adaptersNum, sizeof(adapterDescsSorted[0]), SortAdapters);

        // Copy to output
        if (adaptersNum < adapterDescNum)
            adapterDescNum = adaptersNum;

        for (uint32_t i = 0; i < adapterDescNum; i++) {
            adapterDescs[i] = *adapterDescsSorted++;

            // Update "deviceCreationDesc"
            if (deviceCreationDesc && precreatedDeviceLuid == adapterDescs[i].uid.low)
                deviceCreationDesc->adapterDesc = &adapterDescs[i];
        }
    } else
        adapterDescNum = adaptersNum;

    for (uint32_t i = 0; i < adaptersNum; i++)
        adapters[i]->Release();

    return Result::SUCCESS;
}

#endif

#if NRI_ENABLE_VK_SUPPORT

static Result EnumerateAdaptersVK(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, VkPhysicalDevice precreatedPhysicalDevice, DeviceCreationDesc* deviceCreationDesc) {
    Library* loader = LoadSharedLibrary(VULKAN_LOADER_NAME);
    if (!loader)
        return Result::UNSUPPORTED;

    const auto vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*loader, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        return Result::UNSUPPORTED;

    const auto vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    if (!vkCreateInstance)
        return Result::UNSUPPORTED;

    Result result = Result::FAILURE;

    // Create instance
    VkApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_2; // 1.3 not needed here

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

#    ifdef __APPLE__
    std::array<const char*, 2> instanceExtensions = {"VK_KHR_get_physical_device_properties2", VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};

    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#    endif

    VkInstance instance = VK_NULL_HANDLE;
    VkResult vkResult = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    if (vkResult == VK_SUCCESS) {
        // Get needed functions
        const auto vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
        if (!vkDestroyInstance)
            return Result::UNSUPPORTED;

        const auto vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroups");
        const auto vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");
        const auto vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
        const auto vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2");

        if (vkEnumeratePhysicalDeviceGroups && vkGetPhysicalDeviceProperties2) {
            uint32_t deviceGroupNum = 0;
            vkResult = vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, nullptr);

            if (vkResult == VK_SUCCESS && deviceGroupNum) {
                if (adapterDescs) {
                    // Save LUID for precreated physical device
                    Uid_t precreatedUid = {};
                    if (precreatedPhysicalDevice) {
                        VkPhysicalDeviceProperties2 deviceProps2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

                        VkPhysicalDeviceIDProperties deviceIDProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
                        deviceProps2.pNext = &deviceIDProperties;

                        vkGetPhysicalDeviceProperties2(precreatedPhysicalDevice, &deviceProps2);

                        precreatedUid = ConstructUid(deviceIDProperties.deviceLUID, deviceIDProperties.deviceUUID, deviceIDProperties.deviceLUIDValid);
                    }

                    // Query device groups
                    VkPhysicalDeviceGroupProperties* deviceGroupProperties = (VkPhysicalDeviceGroupProperties*)alloca(sizeof(VkPhysicalDeviceGroupProperties) * deviceGroupNum);
                    for (uint32_t i = 0; i < deviceGroupNum; i++) {
                        deviceGroupProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
                        deviceGroupProperties[i].pNext = nullptr;
                    }
                    vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, deviceGroupProperties);

                    // Query device groups properties
                    AdapterDesc* adapterDescsSorted = (AdapterDesc*)alloca(sizeof(AdapterDesc) * deviceGroupNum);
                    for (uint32_t i = 0; i < deviceGroupNum; i++) {
                        VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];
                        VkPhysicalDeviceProperties2 deviceProps2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
                        const VkPhysicalDeviceProperties& deviceProps = deviceProps2.properties;

                        VkPhysicalDeviceIDProperties deviceIDProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
                        deviceProps2.pNext = &deviceIDProperties;

                        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

                        AdapterDesc& adapterDesc = adapterDescsSorted[i];
                        adapterDesc = {};
                        adapterDesc.uid = ConstructUid(deviceIDProperties.deviceLUID, deviceIDProperties.deviceUUID, deviceIDProperties.deviceLUIDValid);
                        adapterDesc.deviceId = deviceProps.deviceID;
                        adapterDesc.vendor = GetVendorFromID(deviceProps.vendorID);
                        strncpy(adapterDesc.name, deviceProps.deviceName, sizeof(adapterDesc.name));

                        if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                            adapterDesc.architecture = Architecture::DESCRETE;
                        else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                            adapterDesc.architecture = Architecture::INTEGRATED;

                        // Memory size
                        if (vkGetPhysicalDeviceMemoryProperties) {
                            VkPhysicalDeviceMemoryProperties memoryProperties = {};
                            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

                            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
                            for (uint32_t j = 0; j < memoryProperties.memoryHeapCount; j++) {
                                // From spec: In UMA systems ... implementation must advertise the heap as device-local
                                if ((memoryProperties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 && deviceProps.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                                    adapterDesc.videoMemorySize += memoryProperties.memoryHeaps[j].size;
                                else
                                    adapterDesc.sharedSystemMemorySize += memoryProperties.memoryHeaps[j].size;
                            }
                        }

                        // Queues
                        if (vkGetPhysicalDeviceQueueFamilyProperties2) {
                            uint32_t familyNum = 0;
                            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &familyNum, nullptr);

                            VkQueueFamilyProperties2* familyProps2 = (VkQueueFamilyProperties2*)alloca(sizeof(VkQueueFamilyProperties2) * familyNum);
                            for (uint32_t j = 0; j < familyNum; j++)
                                familyProps2[j] = {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};

                            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &familyNum, familyProps2);

                            std::array<uint32_t, (size_t)QueueType::MAX_NUM> scores = {};
                            for (uint32_t j = 0; j < familyNum; j++) {
                                const VkQueueFamilyProperties& familyProps = familyProps2[j].queueFamilyProperties;

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
                                        adapterDesc.queueNum[index] = familyProps.queueCount;
                                        scores[index] = score;
                                        taken = true;
                                    }
                                }

                                { // Prefer compute-only
                                    size_t index = (size_t)QueueType::COMPUTE;
                                    uint32_t score = COMPUTE_QUEUE_SCORE;

                                    if (!taken && compute && score > scores[index]) {
                                        adapterDesc.queueNum[index] = familyProps.queueCount;
                                        scores[index] = score;
                                        taken = true;
                                    }
                                }

                                { // Prefer copy-only
                                    size_t index = (size_t)QueueType::COPY;
                                    uint32_t score = COPY_QUEUE_SCORE;

                                    if (!taken && copy && score > scores[index]) {
                                        adapterDesc.queueNum[index] = familyProps.queueCount;
                                        scores[index] = score;
                                        taken = true;
                                    }
                                }
                            }
                        }
                    }

                    // Sort by video memory size
                    qsort(adapterDescsSorted, deviceGroupNum, sizeof(adapterDescsSorted[0]), SortAdapters);

                    // Copy to output
                    if (deviceGroupNum < adapterDescNum)
                        adapterDescNum = deviceGroupNum;

                    for (uint32_t i = 0; i < adapterDescNum; i++) {
                        adapterDescs[i] = *adapterDescsSorted++;

                        // Update "deviceCreationDesc"
                        if (deviceCreationDesc && CompareUid(precreatedUid, adapterDescs[i].uid))
                            deviceCreationDesc->adapterDesc = &adapterDescs[i];
                    }
                } else
                    adapterDescNum = deviceGroupNum;

                result = Result::SUCCESS;
            }
        }

        if (instance)
            vkDestroyInstance(instance, nullptr);
    }

    UnloadSharedLibrary(*loader);

    return result;
}

#endif

static Result FinalizeDeviceCreation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& deviceImpl, Device*& device) {
    MaybeUnused(deviceCreationDesc);
#if NRI_ENABLE_VALIDATION_SUPPORT
    if (deviceCreationDesc.enableNRIValidation && deviceCreationDesc.graphicsAPI != GraphicsAPI::NONE) {
        Device* deviceVal = (Device*)CreateDeviceValidation(deviceCreationDesc, deviceImpl);
        if (!deviceVal) {
            nriDestroyDevice((Device*)&deviceImpl);
            return Result::FAILURE;
        }

        device = deviceVal;
    } else
#endif
        device = (Device*)&deviceImpl;

#if NRI_ENABLE_NVTX_SUPPORT
    nvtxInitialize(nullptr); // needed only to avoid stalls on the first use
#endif

    return Result::SUCCESS;
}

NRI_API Result NRI_CALL nriGetInterface(const Device& device, const char* interfaceName, size_t interfaceSize, void* interfacePtr) {
    if (strstr(interfaceName, "nri::") == interfaceName)
        interfaceName += 5;
    else if (strstr(interfaceName, "Nri") == interfaceName)
        interfaceName += 3;

    uint64_t hash = Hash(interfaceName);
    size_t realInterfaceSize = size_t(-1);
    Result result = Result::INVALID_ARGUMENT;
    const DeviceBase& deviceBase = (DeviceBase&)device;

    memset(interfacePtr, 0, interfaceSize);

    if (hash == Hash(NRI_STRINGIFY(CoreInterface))) {
        realInterfaceSize = sizeof(CoreInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(CoreInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(ImguiInterface))) {
        realInterfaceSize = sizeof(ImguiInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(ImguiInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(HelperInterface))) {
        realInterfaceSize = sizeof(HelperInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(HelperInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(LowLatencyInterface))) {
        realInterfaceSize = sizeof(LowLatencyInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(LowLatencyInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(MeshShaderInterface))) {
        realInterfaceSize = sizeof(MeshShaderInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(MeshShaderInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(RayTracingInterface))) {
        realInterfaceSize = sizeof(RayTracingInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(RayTracingInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(StreamerInterface))) {
        realInterfaceSize = sizeof(StreamerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(StreamerInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(SwapChainInterface))) {
        realInterfaceSize = sizeof(SwapChainInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(SwapChainInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(UpscalerInterface))) {
        realInterfaceSize = sizeof(UpscalerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(UpscalerInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(WrapperD3D11Interface))) {
        realInterfaceSize = sizeof(WrapperD3D11Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D11Interface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(WrapperD3D12Interface))) {
        realInterfaceSize = sizeof(WrapperD3D12Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D12Interface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(WrapperVKInterface))) {
        realInterfaceSize = sizeof(WrapperVKInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperVKInterface*)interfacePtr);
    }

    if (result == Result::INVALID_ARGUMENT)
        REPORT_ERROR(&deviceBase, "Unknown interface '%s'!", interfaceName);
    else if (interfaceSize != realInterfaceSize)
        REPORT_ERROR(&deviceBase, "Interface '%s' has invalid size=%u bytes, while %u bytes expected by the implementation", interfaceName, interfaceSize, realInterfaceSize);
    else if (result == Result::UNSUPPORTED)
        REPORT_WARNING(&deviceBase, "Interface '%s' is not supported by the device!", interfaceName);
    else {
        const void* const* const begin = (void**)interfacePtr;
        const void* const* const end = begin + realInterfaceSize / sizeof(void*);

        for (const void* const* current = begin; current != end; current++) {
            if (!(*current)) {
                REPORT_ERROR(&deviceBase, "Invalid function table: function #%u is NULL!", uint32_t(current - begin));
                return Result::FAILURE;
            }
        }
    }

    return result;
}

NRI_API void NRI_CALL nriBeginAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxRangePushEx(&eventAttrib);

#    else

    // TODO: add PIX

#    endif
#endif
}

NRI_API void NRI_CALL nriEndAnnotation() {
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxRangePop();

#    else

    // TODO: add PIX

#    endif
#endif
}

NRI_API void NRI_CALL nriAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxMarkEx(&eventAttrib);

#    else

    // TODO: add PIX

#    endif
#endif
}

#if NRI_ENABLE_NVTX_SUPPORT
#    if (defined __linux__)
#        include <sys/syscall.h>
#    elif (defined __APPLE__)
#        include <sys/syscall.h>
#    elif (defined __ANDROID__)
#        include <unistd.h>
#    elif (defined _WIN64)
#        include <processthreadsapi.h>
#    endif

NRI_API void NRI_CALL nriSetThreadName(const char* name) {
    uint64_t tid = 0;
#    if (defined __linux__)
    tid = syscall(SYS_gettid);
#    elif (defined __APPLE__)
    pthread_threadid_np(nullptr, &tid);
#    elif (defined __ANDROID__)
    tid = gettid();
#    elif (defined _WIN64)
    tid = GetCurrentThreadId();
#    endif

    nvtxNameOsThreadA((uint32_t)tid, name);
}

#else

NRI_API void NRI_CALL nriSetThreadName(const char*) {
}

#endif

NRI_API Result NRI_CALL nriCreateDevice(const DeviceCreationDesc& deviceCreationDesc, Device*& device) {
    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    DeviceCreationDesc modifiedDeviceCreationDesc = deviceCreationDesc;
    CheckAndSetDefaultCallbacks(modifiedDeviceCreationDesc);

    // Valid adapter expected (take 1st)
    uint32_t adapterDescNum = 1;
    AdapterDesc adapterDesc = {};
    if (!modifiedDeviceCreationDesc.adapterDesc) {
        nriEnumerateAdapters(&adapterDesc, adapterDescNum);
        modifiedDeviceCreationDesc.adapterDesc = &adapterDesc;
    }

    // Valid queue families expected
    QueueFamilyDesc qraphicsQueue = {};
    qraphicsQueue.queueNum = 1;
    qraphicsQueue.queueType = QueueType::GRAPHICS;

    if (!modifiedDeviceCreationDesc.queueFamilyNum) {
        modifiedDeviceCreationDesc.queueFamilyNum = 1;
        modifiedDeviceCreationDesc.queueFamilies = &qraphicsQueue;
    }

    for (uint32_t i = 0; i < modifiedDeviceCreationDesc.queueFamilyNum; i++) {
        QueueFamilyDesc& queueFamily = (QueueFamilyDesc&)modifiedDeviceCreationDesc.queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamily.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return Result::INVALID_ARGUMENT;

        uint32_t supportedQueueNum = modifiedDeviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamily.queueNum > supportedQueueNum)
            queueFamily.queueNum = supportedQueueNum;
    }

#if NRI_ENABLE_NONE_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::NONE)
        result = CreateDeviceNONE(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if NRI_ENABLE_D3D11_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D11)
        result = CreateDeviceD3D11(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

#if NRI_ENABLE_D3D12_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D12)
        result = CreateDeviceD3D12(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

#if NRI_ENABLE_VK_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::VK)
        result = CreateDeviceVK(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(modifiedDeviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D11Device(const DeviceCreationD3D11Desc& deviceCreationD3D11Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D11;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationD3D11Desc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationD3D11Desc.allocationCallbacks;
    deviceCreationDesc.d3dShaderExtRegister = deviceCreationD3D11Desc.d3dShaderExtRegister;
    deviceCreationDesc.d3dZeroBufferSize = deviceCreationD3D11Desc.d3dZeroBufferSize;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D11Desc.enableNRIValidation;
    deviceCreationDesc.enableD3D11CommandBufferEmulation = deviceCreationD3D11Desc.enableD3D11CommandBufferEmulation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_D3D11_SUPPORT
    // Valid adapter expected (find it)
    AdapterDesc adapterDescs[ADAPTER_MAX_NUM] = {};
    if (!deviceCreationDesc.adapterDesc) {
        ComPtr<IDXGIDevice> dxgiDevice;
        HRESULT hr = deviceCreationD3D11Desc.d3d11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        if (SUCCEEDED(hr)) {
            ComPtr<IDXGIAdapter> adapter;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr)) {
                DXGI_ADAPTER_DESC desc = {};
                hr = adapter->GetDesc(&desc);
                if (SUCCEEDED(hr)) {
                    uint64_t luid = *(uint64_t*)&desc.AdapterLuid;

                    uint32_t adapterDescNum = GetCountOf(adapterDescs);
                    EnumerateAdaptersD3D(adapterDescs, adapterDescNum, luid, &deviceCreationDesc);
                }
            }
        }
    }

    // Valid queue families expected
    QueueFamilyDesc qraphicsQueue = {};
    qraphicsQueue.queueNum = 1;
    qraphicsQueue.queueType = QueueType::GRAPHICS;

    if (!deviceCreationDesc.queueFamilyNum) {
        deviceCreationDesc.queueFamilyNum = 1;
        deviceCreationDesc.queueFamilies = &qraphicsQueue;
    }

    result = CreateDeviceD3D11(deviceCreationDesc, deviceCreationD3D11Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D12Device(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationD3D12Desc.allocationCallbacks;
    deviceCreationDesc.d3dShaderExtRegister = deviceCreationD3D12Desc.d3dShaderExtRegister;
    deviceCreationDesc.d3dZeroBufferSize = deviceCreationD3D12Desc.d3dZeroBufferSize;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D12Desc.enableNRIValidation;
    deviceCreationDesc.enableMemoryZeroInitialization = deviceCreationD3D12Desc.enableMemoryZeroInitialization;
    deviceCreationDesc.disableD3D12EnhancedBarriers = deviceCreationD3D12Desc.disableD3D12EnhancedBarriers;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_D3D12_SUPPORT
    // Valid adapter expected (find it)
    AdapterDesc adapterDescs[ADAPTER_MAX_NUM] = {};
    if (!deviceCreationDesc.adapterDesc) {
        LUID luidTemp = deviceCreationD3D12Desc.d3d12Device->GetAdapterLuid();
        uint64_t luid = *(uint64_t*)&luidTemp;

        uint32_t adapterDescNum = GetCountOf(adapterDescs);
        EnumerateAdaptersD3D(adapterDescs, adapterDescNum, luid, &deviceCreationDesc);
    }

    // Valid queue families expected
    for (uint32_t i = 0; i < deviceCreationD3D12Desc.queueFamilyNum; i++) {
        QueueFamilyD3D12Desc& queueFamilyD3D12Desc = (QueueFamilyD3D12Desc&)deviceCreationD3D12Desc.queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamilyD3D12Desc.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return Result::INVALID_ARGUMENT;

        uint32_t supportedQueueNum = deviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamilyD3D12Desc.queueNum > supportedQueueNum)
            queueFamilyD3D12Desc.queueNum = supportedQueueNum;
    }

    result = CreateDeviceD3D12(deviceCreationDesc, deviceCreationD3D12Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromVKDevice(const DeviceCreationVKDesc& deviceCreationVKDesc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.graphicsAPI = GraphicsAPI::VK;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationVKDesc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationVKDesc.allocationCallbacks;
    deviceCreationDesc.enableNRIValidation = deviceCreationVKDesc.enableNRIValidation;
    deviceCreationDesc.enableMemoryZeroInitialization = deviceCreationVKDesc.enableMemoryZeroInitialization;
    deviceCreationDesc.vkBindingOffsets = deviceCreationVKDesc.vkBindingOffsets;
    deviceCreationDesc.vkExtensions = deviceCreationVKDesc.vkExtensions;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_VK_SUPPORT
    // Valid adapter expected (find it)
    AdapterDesc adapterDescs[ADAPTER_MAX_NUM] = {};
    if (!deviceCreationDesc.adapterDesc) {
        uint32_t adapterDescNum = GetCountOf(adapterDescs);
        EnumerateAdaptersVK(adapterDescs, adapterDescNum, (VkPhysicalDevice)deviceCreationVKDesc.vkPhysicalDevice, &deviceCreationDesc);
    }

    // Valid queue families expected
    for (uint32_t i = 0; i < deviceCreationVKDesc.queueFamilyNum; i++) {
        QueueFamilyVKDesc& queueFamilyVKDesc = (QueueFamilyVKDesc&)deviceCreationVKDesc.queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamilyVKDesc.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return Result::INVALID_ARGUMENT;

        uint32_t supportedQueueNum = deviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamilyVKDesc.queueNum > supportedQueueNum)
            queueFamilyVKDesc.queueNum = supportedQueueNum;
    }

    result = CreateDeviceVK(deviceCreationDesc, deviceCreationVKDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API void NRI_CALL nriDestroyDevice(Device* device) {
    if (device)
        ((DeviceBase*)device)->Destruct();
}

NRI_API Format NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat) {
    return VKFormatToNRIFormat(vkFormat);
}

NRI_API Format NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat) {
    return DXGIFormatToNRIFormat(dxgiFormat);
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(Format format) {
    MaybeUnused(format);

#if NRI_ENABLE_VK_SUPPORT
    return NRIFormatToVKFormat(format);
#else
    return 0;
#endif
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(Format format) {
    MaybeUnused(format);

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    return NRIFormatToDXGIFormat(format);
#else
    return 0;
#endif
}

NRI_API const FormatProps* NRI_CALL nriGetFormatProps(Format format) {
    return &GetFormatProps(format);
}

NRI_API const char* NRI_CALL nriGetGraphicsAPIString(GraphicsAPI graphicsAPI) {
    switch (graphicsAPI) {
        case GraphicsAPI::NONE:
            return "NONE";
        case GraphicsAPI::D3D11:
            return "D3D11";
        case GraphicsAPI::D3D12:
            return "D3D12";
        case GraphicsAPI::VK:
            return "VK";
        default:
            return "UNKNOWN";
    }
}

NRI_API Result NRI_CALL nriEnumerateAdapters(AdapterDesc* adapterDescs, uint32_t& adapterDescNum) {
    Result result = Result::UNSUPPORTED;

    if (adapterDescs && adapterDescNum)
        memset(adapterDescs, 0, sizeof(AdapterDesc) * adapterDescNum);

#if NRI_ENABLE_VK_SUPPORT
    // Try VK first as capable to return real support for queues
    result = EnumerateAdaptersVK(adapterDescs, adapterDescNum, 0, nullptr);
#endif

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    // If VK is not available, use D3D
    if (result != Result::SUCCESS)
        result = EnumerateAdaptersD3D(adapterDescs, adapterDescNum, 0, nullptr);
#endif

#if NRI_ENABLE_NONE_SUPPORT && !(NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT || NRI_ENABLE_VK_SUPPORT)
    // Patch the results, if NONE is the only avaiable implementation
    if (result != Result::SUCCESS) {
        if (adapterDescNum > 1)
            adapterDescNum = 1;

        result = Result::SUCCESS;
    }
#endif

    return result;
}

NRI_API void NRI_CALL nriReportLiveObjects() {
#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    ComPtr<IDXGIDebug1> pDebug;
    HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug));
    if (SUCCEEDED(hr))
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)((uint32_t)DXGI_DEBUG_RLO_DETAIL | (uint32_t)DXGI_DEBUG_RLO_IGNORE_INTERNAL));
#endif
}
