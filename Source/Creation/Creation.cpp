/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedExternal.h"
#include "DeviceBase.h"

#define NRI_STRINGIFY_(token) #token
#define NRI_STRINGIFY(token) NRI_STRINGIFY_(token)

using namespace nri;

#if NRI_USE_D3D11
Result CreateDeviceD3D11(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D11(const DeviceCreationD3D11Desc& deviceDesc, DeviceBase*& device);
#endif

#if NRI_USE_D3D12
Result CreateDeviceD3D12(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D12(const DeviceCreationD3D12Desc& deviceCreationDesc, DeviceBase*& device);
#endif

#if NRI_USE_VULKAN
Result CreateDeviceVK(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceVK(const DeviceCreationVulkanDesc& deviceDesc, DeviceBase*& device);
#endif

DeviceBase* CreateDeviceValidation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& device);

constexpr uint64_t Hash( const char* name )
{
    return *name != 0 ? *name ^ ( 33 * Hash(name + 1) ) : 5381;
}

NRI_API Result NRI_CALL nriGetInterface(const Device& device, const char* interfaceName, size_t interfaceSize, void* interfacePtr)
{
    const uint64_t hash = Hash(interfaceName);
    size_t realInterfaceSize = size_t(-1);
    Result result = Result::INVALID_ARGUMENT;
    const DeviceBase& deviceBase = (DeviceBase&)device;

    if (hash == Hash(NRI_STRINGIFY(nri::CoreInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(CoreInterface))))
    {
        realInterfaceSize = sizeof(CoreInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(CoreInterface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::SwapChainInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(SwapChainInterface))))
    {
        realInterfaceSize = sizeof(SwapChainInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(SwapChainInterface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::WrapperD3D11Interface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(WrapperD3D11Interface))))
    {
        realInterfaceSize = sizeof(WrapperD3D11Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D11Interface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::WrapperD3D12Interface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(WrapperD3D12Interface))))
    {
        realInterfaceSize = sizeof(WrapperD3D12Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D12Interface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::WrapperVKInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(WrapperVKInterface))))
    {
        realInterfaceSize = sizeof(WrapperVKInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperVKInterface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::RayTracingInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(RayTracingInterface))))
    {
        realInterfaceSize = sizeof(RayTracingInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(RayTracingInterface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::MeshShaderInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(MeshShaderInterface))))
    {
        realInterfaceSize = sizeof(MeshShaderInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(MeshShaderInterface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::HelperInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(HelperInterface))))
    {
        realInterfaceSize = sizeof(HelperInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(HelperInterface*)interfacePtr);
    }

    if (result == Result::INVALID_ARGUMENT)
        REPORT_ERROR(deviceBase.GetLog(), "Unknown interface '%s'!", interfaceName);
    else if (interfaceSize != realInterfaceSize)
        REPORT_ERROR(deviceBase.GetLog(), "Interface '%s' has invalid size = %u bytes, while %u bytes expected by the implementation", interfaceName, interfaceSize, realInterfaceSize);
    else if (result == Result::UNSUPPORTED)
        REPORT_WARNING(deviceBase.GetLog(), "Interface '%s' is not supported by the device!", interfaceName);

    return result;
}

template< typename T >
Result FinalizeDeviceCreation(const T& deviceCreationDesc, DeviceBase& deviceImpl, Device*& device)
{
    if (deviceCreationDesc.enableNRIValidation)
    {
        Device* deviceVal = (Device*)CreateDeviceValidation(deviceCreationDesc, deviceImpl);
        if (deviceVal == nullptr)
        {
            nriDestroyDevice((Device&)deviceImpl);
            return Result::FAILURE;
        }

        device = deviceVal;
    }
    else
        device = (Device*)&deviceImpl;

    return Result::SUCCESS;
}

NRI_API Result NRI_CALL nriCreateDevice(const DeviceCreationDesc& deviceCreationDesc, Device*& device)
{
    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    DeviceCreationDesc modifiedDeviceCreationDesc = deviceCreationDesc;
    CheckAndSetDefaultCallbacks(modifiedDeviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(modifiedDeviceCreationDesc.memoryAllocatorInterface);

    #if (NRI_USE_D3D11 == 1)
        if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D11)
            result = CreateDeviceD3D11(modifiedDeviceCreationDesc, deviceImpl);
    #endif

    #if (NRI_USE_D3D12 == 1)
        if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D12)
            result = CreateDeviceD3D12(modifiedDeviceCreationDesc, deviceImpl);
    #endif

    #if (NRI_USE_VULKAN == 1)
        if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::VULKAN)
            result = CreateDeviceVK(modifiedDeviceCreationDesc, deviceImpl);
    #endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(modifiedDeviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D11Device(const DeviceCreationD3D11Desc& deviceCreationD3D11Desc, Device*& device)
{
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D11Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D11Desc.memoryAllocatorInterface;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D11;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D11Desc.enableNRIValidation;
    deviceCreationDesc.skipLiveObjectsReporting = true;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceCreationD3D11Desc tempDeviceCreationD3D11Desc = deviceCreationD3D11Desc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationD3D11Desc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationD3D11Desc.memoryAllocatorInterface);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    #if (NRI_USE_D3D11 == 1)
        result = CreateDeviceD3D11(tempDeviceCreationD3D11Desc, deviceImpl);
    #endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D12Device(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, Device*& device)
{
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D12Desc.memoryAllocatorInterface;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D12Desc.enableNRIValidation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceCreationD3D12Desc tempDeviceCreationD3D12Desc = deviceCreationD3D12Desc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationD3D12Desc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationD3D12Desc.memoryAllocatorInterface);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    #if (NRI_USE_D3D12 == 1)
        result = CreateDeviceD3D12(tempDeviceCreationD3D12Desc, deviceImpl);
    #endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromVkDevice(const DeviceCreationVulkanDesc& deviceCreationVulkanDesc, Device*& device)
{
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationVulkanDesc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationVulkanDesc.memoryAllocatorInterface;
    deviceCreationDesc.spirvBindingOffsets = deviceCreationVulkanDesc.spirvBindingOffsets;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::VULKAN;
    deviceCreationDesc.enableNRIValidation = deviceCreationVulkanDesc.enableNRIValidation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceCreationVulkanDesc tempDeviceCreationVulkanDesc = deviceCreationVulkanDesc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationVulkanDesc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationVulkanDesc.memoryAllocatorInterface);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    #if (NRI_USE_VULKAN == 1)
        result = CreateDeviceVK(tempDeviceCreationVulkanDesc, deviceImpl);
    #endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API void NRI_CALL nriDestroyDevice(Device& device)
{
    ((DeviceBase&)device).Destroy();
}

NRI_API Format NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat)
{
    return VKFormatToNRIFormat(vkFormat);
}

NRI_API Format NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat)
{
    return DXGIFormatToNRIFormat(dxgiFormat);
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(Format format)
{
    MaybeUnused(format);

    #if (NRI_USE_VULKAN == 1)
        return NRIFormatToVKFormat(format);
    #else
        return 0;
    #endif
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(Format format)
{
    MaybeUnused(format);

    #if (NRI_USE_D3D11 == 1)
        return NRIFormatToDXGIFormatD3D11(format);
    #elif(NRI_USE_D3D12 == 1)
        return NRIFormatToDXGIFormatD3D12(format);
    #else
        return 0;
    #endif
}

#ifdef _WIN32

#include <dxgi1_4.h>

static int SortAdaptersByDedicatedVideoMemorySize(const void* a, const void* b)
{
    DXGI_ADAPTER_DESC ad = {};
    (*(IDXGIAdapter1**)a)->GetDesc(&ad);

    DXGI_ADAPTER_DESC bd = {};
    (*(IDXGIAdapter1**)b)->GetDesc(&bd);

    if (ad.DedicatedVideoMemory > bd.DedicatedVideoMemory)
        return -1;

    if (ad.DedicatedVideoMemory < bd.DedicatedVideoMemory)
        return 1;

    return 0;
}

NRI_API Result NRI_CALL nriEnumerateAdapters(AdapterDesc* adapterDescs, uint32_t& adapterDescNum)
{
    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return Result::UNSUPPORTED;

    uint32_t adaptersNum = 0;
    IDXGIAdapter1* adapters[32];

    for (uint32_t i = 0; i < GetCountOf(adapters); i++)
    {
        IDXGIAdapter1* adapter;
        HRESULT hr = dxgifactory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc = {};
        if (adapter->GetDesc1(&desc) == S_OK)
        {
            if (desc.Flags == DXGI_ADAPTER_FLAG_NONE)
                adapters[adaptersNum++] = adapter;
        }
    }

    if (!adaptersNum)
        return Result::FAILURE;

    if (adapterDescs)
    {
        qsort(adapters, adaptersNum, sizeof(adapters[0]), SortAdaptersByDedicatedVideoMemorySize);

        if (adaptersNum < adapterDescNum)
            adapterDescNum = adaptersNum;

        for (uint32_t i = 0; i < adapterDescNum; i++)
        {
            DXGI_ADAPTER_DESC desc = {};
            adapters[i]->GetDesc(&desc);

            AdapterDesc& adapterDesc = adapterDescs[i];
            memset(&adapterDesc, 0, sizeof(adapterDesc));
            wcstombs(adapterDesc.description, desc.Description, GetCountOf(adapterDesc.description) - 1);
            adapterDesc.luid = *(uint64_t*)&desc.AdapterLuid;
            adapterDesc.videoMemorySize = desc.DedicatedVideoMemory;
            adapterDesc.systemMemorySize = desc.DedicatedSystemMemory + desc.SharedSystemMemory;
            adapterDesc.deviceId = desc.DeviceId;
            adapterDesc.vendor = GetVendorFromID(desc.VendorId);
        }
    }
    else
        adapterDescNum = adaptersNum;

    for (uint32_t i = 0; i < adaptersNum; i++)
        adapters[i]->Release();

    return Result::SUCCESS;
}

NRI_API bool NRI_CALL nriQueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo)
{
    uint64_t luid = ((DeviceBase&)device).GetDesc().adapterDesc.luid;

    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return false;

    ComPtr<IDXGIAdapter3> adapter;
    if (FAILED(dxgifactory->EnumAdapterByLuid(*(LUID*)&luid, IID_PPV_ARGS(&adapter))))
        return false;

    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (FAILED(adapter->QueryVideoMemoryInfo(0, memoryLocation == MemoryLocation::DEVICE ? DXGI_MEMORY_SEGMENT_GROUP_LOCAL : DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info)))
        return false;

    static_assert(sizeof(VideoMemoryInfo) == sizeof(DXGI_QUERY_VIDEO_MEMORY_INFO));
    memcpy(&videoMemoryInfo, &info, sizeof(info));

    return true;
}

#else

#include <vulkan/vulkan.h>

#define GET_VK_FUNCTION(instance, name) \
    const auto name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
    if (name == nullptr) \
        return Result::UNSUPPORTED;

static int SortAdaptersByDedicatedVideoMemorySize(const void* pa, const void* pb)
{
    AdapterDesc* a = (AdapterDesc*)pa;
    AdapterDesc* b = (AdapterDesc*)pb;

    if (a->videoMemorySize > b->videoMemorySize)
        return -1;

    if (a->videoMemorySize < b->videoMemorySize)
        return 1;

    return 0;
}

NRI_API Result NRI_CALL nriEnumerateAdapters(AdapterDesc* adapterDescs, uint32_t& adapterDescNum)
{
    Library* loader = LoadSharedLibrary(VULKAN_LOADER_NAME);
    if (!loader)
        return Result::UNSUPPORTED;

    const auto vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*loader, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        return Result::UNSUPPORTED;

    const auto vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    if (!vkCreateInstance)
        return Result::UNSUPPORTED;

    // Create instance
    VkApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    Result nriResult = Result::FAILURE;
    if (result == VK_SUCCESS)
    {
        // Get needed functions
        GET_VK_FUNCTION(instance, vkDestroyInstance);
        GET_VK_FUNCTION(instance, vkEnumeratePhysicalDeviceGroups);
        GET_VK_FUNCTION(instance, vkGetPhysicalDeviceProperties2);
        GET_VK_FUNCTION(instance, vkGetPhysicalDeviceMemoryProperties);

        uint32_t deviceGroupNum = 0;
        result = vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, nullptr);

        if (result == VK_SUCCESS && deviceGroupNum)
        {
            if (adapterDescs)
            {
                // Query device groups
                VkPhysicalDeviceGroupProperties* deviceGroupProperties = STACK_ALLOC(VkPhysicalDeviceGroupProperties, deviceGroupNum);
                vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, deviceGroupProperties);

                // Query device groups properties
                AdapterDesc* adapterDescsSorted = STACK_ALLOC(AdapterDesc, deviceGroupNum);
                for (uint32_t i = 0; i < deviceGroupNum; i++)
                {
                    VkPhysicalDeviceIDProperties deviceIDProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
                    VkPhysicalDeviceProperties2 properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
                    properties2.pNext = &deviceIDProperties;

                    VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];
                    vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

                    VkPhysicalDeviceMemoryProperties memoryProperties = {};
                    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

                    AdapterDesc& adapterDesc = adapterDescsSorted[i];
                    memset(&adapterDesc, 0, sizeof(adapterDesc));
                    strncpy(adapterDesc.description, properties2.properties.deviceName, sizeof(adapterDesc.description));
                    adapterDesc.luid = *(uint64_t*)&deviceIDProperties.deviceLUID[0];
                    adapterDesc.deviceId = properties2.properties.deviceID;
                    adapterDesc.vendor = GetVendorFromID(properties2.properties.vendorID);

                    /* THIS IS AWFUL!
                    https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
                    In a unified memory architecture (UMA) system there is often only a single memory heap which is considered to
                    be equally "local" to the host and to the device, and such an implementation must advertise the heap as device-local. */
                    for (uint32_t k = 0; k < memoryProperties.memoryHeapCount; k++)
                    {
                        if (memoryProperties.memoryHeaps[k].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                            adapterDesc.videoMemorySize += memoryProperties.memoryHeaps[k].size;
                        else
                            adapterDesc.systemMemorySize += memoryProperties.memoryHeaps[k].size;
                    }
                }

                // Sort by video memory size
                qsort(adapterDescsSorted, deviceGroupNum, sizeof(adapterDescsSorted[0]), SortAdaptersByDedicatedVideoMemorySize);

                // Copy to output
                if (deviceGroupNum < adapterDescNum)
                    adapterDescNum = deviceGroupNum;

                for (uint32_t i = 0; i < adapterDescNum; i++)
                    *adapterDescs++ = *adapterDescsSorted++;
            }
            else
                adapterDescNum = deviceGroupNum;

            nriResult = Result::SUCCESS;
        }

        if (instance)
            vkDestroyInstance(instance, nullptr);
    }

    UnloadSharedLibrary(*loader);

    return nriResult;
}

NRI_API bool NRI_CALL nriQueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo)
{
    MaybeUnused(device);
    MaybeUnused(memoryLocation);
    MaybeUnused(videoMemoryInfo);

    // TODO: is there a QueryVideoMemoryInfo analog on Linux?
    return false;
}

#endif
