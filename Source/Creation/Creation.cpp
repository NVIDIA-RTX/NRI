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

#include <vulkan/vulkan.h>

#define NRI_STRINGIFY(name) #name

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

NRI_API Result NRI_CALL nri::GetInterface(const Device& device, const char* interfaceName, size_t interfaceSize, void* interfacePtr)
{
    const uint64_t hash = Hash(interfaceName);
    size_t realInterfaceSize = size_t(-1);
    Result result = Result::INVALID_ARGUMENT;
    const DeviceBase& deviceBase = (DeviceBase&)device;

    if (hash == Hash( NRI_STRINGIFY(nri::CoreInterface) ))
    {
        realInterfaceSize = sizeof(CoreInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(CoreInterface*)interfacePtr);
    }
    else if (hash == Hash( NRI_STRINGIFY(nri::SwapChainInterface) ))
    {
        realInterfaceSize = sizeof(SwapChainInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(SwapChainInterface*)interfacePtr);
    }
    else if (hash == Hash( NRI_STRINGIFY(nri::WrapperD3D11Interface) ))
    {
        realInterfaceSize = sizeof(WrapperD3D11Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D11Interface*)interfacePtr);
    }
    else if (hash == Hash(NRI_STRINGIFY(nri::WrapperD3D12Interface)))
    {
        realInterfaceSize = sizeof(WrapperD3D12Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D12Interface*)interfacePtr);
    }
    else if (hash == Hash( NRI_STRINGIFY(nri::WrapperVKInterface) ))
    {
        realInterfaceSize = sizeof(WrapperVKInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperVKInterface*)interfacePtr);
    }
    else if (hash == Hash( NRI_STRINGIFY(nri::RayTracingInterface) ))
    {
        realInterfaceSize = sizeof(RayTracingInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(RayTracingInterface*)interfacePtr);
    }
    else if (hash == Hash( NRI_STRINGIFY(nri::MeshShaderInterface) ))
    {
        realInterfaceSize = sizeof(MeshShaderInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(MeshShaderInterface*)interfacePtr);
    }
    else if (hash == Hash( NRI_STRINGIFY(nri::HelperInterface) ))
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

static bool IsValidDeviceGroup(const VkPhysicalDeviceGroupProperties& group, PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties)
{
    VkPhysicalDeviceProperties baseProperties = {};
    vkGetPhysicalDeviceProperties(group.physicalDevices[0], &baseProperties);

    for (uint32_t j = 1; j < group.physicalDeviceCount; j++)
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(group.physicalDevices[j], &properties);

        if (properties.deviceID != baseProperties.deviceID)
            return false;
    }

    return true;
}

constexpr std::array<nri::PhysicalDeviceType, 5> PHYSICAL_DEVICE_TYPE = {
    nri::PhysicalDeviceType::UNKNOWN,
    nri::PhysicalDeviceType::INTEGRATED,
    nri::PhysicalDeviceType::DISCRETE,
    nri::PhysicalDeviceType::UNKNOWN,
    nri::PhysicalDeviceType::UNKNOWN
};

constexpr PhysicalDeviceType GetPhysicalDeviceType(VkPhysicalDeviceType physicalDeviceType)
{
    const size_t index = std::min(PHYSICAL_DEVICE_TYPE.size() - 1, (size_t)physicalDeviceType);
    return PHYSICAL_DEVICE_TYPE[index];
}

#define GET_VK_FUNCTION(instance, name) \
    const auto name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
    if (name == nullptr) \
        return Result::UNSUPPORTED;

NRI_API Result NRI_CALL nri::GetPhysicalDevices(PhysicalDeviceGroup* physicalDeviceGroups, uint32_t& physicalDeviceGroupNum)
{
    Library* loader = LoadSharedLibrary(VULKAN_LOADER_NAME);
    if (loader == nullptr)
        return Result::UNSUPPORTED;

    const auto vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*loader, "vkGetInstanceProcAddr");
    const auto vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");

    VkApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    if (result != VK_SUCCESS)
    {
        UnloadSharedLibrary(*loader);
        return Result::UNSUPPORTED;
    }

    GET_VK_FUNCTION(instance, vkDestroyInstance);
    GET_VK_FUNCTION(instance, vkEnumeratePhysicalDeviceGroups);
    GET_VK_FUNCTION(instance, vkGetPhysicalDeviceProperties);
    GET_VK_FUNCTION(instance, vkGetPhysicalDeviceProperties2);
    GET_VK_FUNCTION(instance, vkGetPhysicalDeviceMemoryProperties);

    uint32_t deviceGroupNum = 0;
    vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, nullptr);

    VkPhysicalDeviceGroupProperties* deviceGroupProperties = STACK_ALLOC(VkPhysicalDeviceGroupProperties, deviceGroupNum);
    result = vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, deviceGroupProperties);

    if (result != VK_SUCCESS)
    {
        vkDestroyInstance(instance, nullptr);
        UnloadSharedLibrary(*loader);

        return Result::UNSUPPORTED;
    }

    if (physicalDeviceGroupNum == 0)
    {
        for (uint32_t i = 0; i < deviceGroupNum; i++)
        {
            if (!IsValidDeviceGroup(deviceGroupProperties[i], vkGetPhysicalDeviceProperties))
                deviceGroupNum--;
        }
        
        physicalDeviceGroupNum = deviceGroupNum;
        vkDestroyInstance(instance, nullptr);
        UnloadSharedLibrary(*loader);

        return Result::SUCCESS;
    }

    VkPhysicalDeviceIDProperties deviceIDProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
    VkPhysicalDeviceProperties2 properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    properties2.pNext = &deviceIDProperties;

    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    VkPhysicalDeviceProperties& properties = properties2.properties;

    for (uint32_t i = 0, j = 0; i < deviceGroupNum && j < physicalDeviceGroupNum; i++)
    {
        if (!IsValidDeviceGroup(deviceGroupProperties[i], vkGetPhysicalDeviceProperties))
            continue;

        const VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];
        vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

#if _WIN32
        if (deviceIDProperties.deviceLUIDValid == VK_FALSE)
        {
            vkDestroyInstance(instance, nullptr);
            UnloadSharedLibrary(*loader);

            return Result::UNSUPPORTED;
        }
#endif

        PhysicalDeviceGroup& group = physicalDeviceGroups[j++];

        group = {};

        const size_t descriptionLength = (size_t)GetCountOf(group.description) - 1;
        strncpy(group.description, properties.deviceName, descriptionLength);
        group.description[descriptionLength] = '\0';

        group.type = GetPhysicalDeviceType(properties.deviceType);
        group.vendor = GetVendorFromID(properties.vendorID);
        group.deviceID = properties.deviceID;
        group.luid = *(uint64_t*)&deviceIDProperties.deviceLUID[0];
        group.physicalDeviceGroupSize = deviceGroupProperties[i].physicalDeviceCount;

        group.dedicatedVideoMemoryMB = 0;
        for (uint32_t k = 0; k < memoryProperties.memoryHeapCount; k++)
        {
            if (memoryProperties.memoryHeaps[k].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                group.dedicatedVideoMemoryMB += memoryProperties.memoryHeaps[k].size / (1024 * 1024);
        }
    }

    vkDestroyInstance(instance, nullptr);
    UnloadSharedLibrary(*loader);

    return Result::SUCCESS;
}

template< typename T >
Result FinalizeDeviceCreation(const T& deviceCreationDesc, DeviceBase& deviceImpl, Device*& device)
{
    if (deviceCreationDesc.enableNRIValidation)
    {
        Device* deviceVal = (Device*)CreateDeviceValidation(deviceCreationDesc, deviceImpl);
        if (deviceVal == nullptr)
        {
            nri::DestroyDevice((Device&)deviceImpl);
            return Result::FAILURE;
        }

        device = deviceVal;
    }
    else
        device = (Device*)&deviceImpl;

    return Result::SUCCESS;
}

NRI_API Result NRI_CALL nri::CreateDevice(const DeviceCreationDesc& deviceCreationDesc, Device*& device)
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

NRI_API Result NRI_CALL nri::CreateDeviceFromD3D11Device(const DeviceCreationD3D11Desc& deviceCreationD3D11Desc, Device*& device)
{
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D11Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D11Desc.memoryAllocatorInterface;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D11;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D11Desc.enableNRIValidation;
    deviceCreationDesc.enableAPIValidation = deviceCreationD3D11Desc.enableAPIValidation;

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

NRI_API Result NRI_CALL nri::CreateDeviceFromD3D12Device(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, Device*& device)
{
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D12Desc.memoryAllocatorInterface;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D12Desc.enableNRIValidation;
    deviceCreationDesc.enableAPIValidation = deviceCreationD3D12Desc.enableAPIValidation;

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

NRI_API Result NRI_CALL nri::CreateDeviceFromVkDevice(const DeviceCreationVulkanDesc& deviceCreationVulkanDesc, Device*& device)
{
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationVulkanDesc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationVulkanDesc.memoryAllocatorInterface;
    deviceCreationDesc.spirvBindingOffsets = deviceCreationVulkanDesc.spirvBindingOffsets;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::VULKAN;
    deviceCreationDesc.enableNRIValidation = deviceCreationVulkanDesc.enableNRIValidation;
    deviceCreationDesc.enableAPIValidation = deviceCreationVulkanDesc.enableAPIValidation;

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

NRI_API void NRI_CALL nri::DestroyDevice(Device& device)
{
    ((DeviceBase&)device).Destroy();
}

NRI_API Format NRI_CALL nri::ConvertVKFormatToNRI(uint32_t vkFormat)
{
    return VKFormatToNRIFormat((VkFormat)vkFormat);
}

NRI_API Format NRI_CALL nri::ConvertDXGIFormatToNRI(uint32_t dxgiFormat)
{
    return DXGIFormatToNRIFormat(dxgiFormat);
}

NRI_API uint32_t NRI_CALL nri::ConvertNRIFormatToVK(Format format)
{
    MaybeUnused(format);

    #if (NRI_USE_VULKAN == 1)
        return NRIFormatToVKFormat(format);
    #else
        return 0;
    #endif
}

NRI_API uint32_t NRI_CALL nri::ConvertNRIFormatToDXGI(Format format)
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

// C compatible exported functions

static_assert(sizeof(uint8_t) == sizeof(nri::Result));
static_assert(sizeof(uint8_t) == sizeof(nri::Format));

NRIC_API uint8_t NRI_CALL nri_GetInterface(const void* device, const char* interfaceName, size_t interfaceSize, void* interfacePtr)
{
    const uint64_t hash = Hash(interfaceName);
    if (hash == Hash(NRI_STRINGIFY(nri_CoreInterface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::CoreInterface), interfaceSize, interfacePtr);
    else if (hash == Hash(NRI_STRINGIFY(nri_SwapChainInterface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::SwapChainInterface), interfaceSize, interfacePtr);
    else if (hash == Hash(NRI_STRINGIFY(nri_WrapperD3D11Interface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::WrapperD3D11Interface), interfaceSize, interfacePtr);
    else if (hash == Hash(NRI_STRINGIFY(nri_WrapperD3D12Interface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::WrapperD3D12Interface), interfaceSize, interfacePtr);
    else if (hash == Hash(NRI_STRINGIFY(nri_WrapperVKInterface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::WrapperVKInterface), interfaceSize, interfacePtr);
    else if (hash == Hash(NRI_STRINGIFY(nri_RayTracingInterface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::RayTracingInterface), interfaceSize, interfacePtr);
    else if (hash == Hash(NRI_STRINGIFY(nri_MeshShaderInterface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::MeshShaderInterface), interfaceSize, interfacePtr);
    else if (hash == Hash(NRI_STRINGIFY(nri_HelperInterface)))
        return (uint8_t)nri::GetInterface(*(const nri::Device*)device, NRI_STRINGIFY(nri::HelperInterface), interfaceSize, interfacePtr);

    return (uint8_t)nri::GetInterface(*(const nri::Device*)device, interfaceName, interfaceSize, interfacePtr);
}

NRIC_API uint8_t NRI_CALL nri_GetPhysicalDevices(void* physicalDeviceGroups, uint32_t* physicalDeviceGroupNum)
{
    return (uint8_t)nri::GetPhysicalDevices((nri::PhysicalDeviceGroup*)physicalDeviceGroups, *physicalDeviceGroupNum);
}

NRIC_API uint8_t NRI_CALL nri_CreateDevice(const void* deviceCreationDesc, void** device)
{
    return (uint8_t)nri::CreateDevice(*(const nri::DeviceCreationDesc*)deviceCreationDesc, *(nri::Device**)device);
}

NRIC_API void NRI_CALL nri_DestroyDevice(void* device)
{
    return nri::DestroyDevice(*(nri::Device*)device);
}

NRIC_API uint8_t NRI_CALL nri_CreateDeviceFromD3D11Device(const void* deviceDesc, void** device)
{
    return (uint8_t)nri::CreateDeviceFromD3D11Device(*(const nri::DeviceCreationD3D11Desc*)deviceDesc, *(nri::Device**)device);
}

NRIC_API uint8_t NRI_CALL nri_ConvertDXGIFormatToNRI(uint32_t dxgiFormat)
{
    return (uint8_t)nri::ConvertDXGIFormatToNRI(dxgiFormat);
}

NRIC_API uint32_t NRI_CALL nri_ConvertNRIFormatToDXGI(uint8_t format)
{
    return nri::ConvertNRIFormatToDXGI((Format)format);
}

NRIC_API uint8_t NRI_CALL nri_CreateDeviceFromD3D12Device(const void* deviceDesc, void** device)
{
    return (uint8_t)nri::CreateDeviceFromD3D12Device(*(const nri::DeviceCreationD3D12Desc*)deviceDesc, *(nri::Device**)device);
}

NRIC_API uint8_t NRI_CALL nri_CreateDeviceFromVkDevice(const void* deviceDesc, void** device)
{
    return (uint8_t)nri::CreateDeviceFromVkDevice(*(const nri::DeviceCreationVulkanDesc*)deviceDesc, *(nri::Device**)device);
}

NRIC_API uint8_t NRI_CALL nri_ConvertVKFormatToNRI(uint32_t vkFormat)
{
    return (uint8_t)nri::ConvertVKFormatToNRI(vkFormat);
}

NRIC_API uint32_t NRI_CALL nri_ConvertNRIFormatToVK(uint8_t format)
{
    return nri::ConvertNRIFormatToVK((Format)format);
}