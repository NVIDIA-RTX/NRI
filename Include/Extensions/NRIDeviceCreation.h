/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

NRI_NAMESPACE_BEGIN

NRI_ENUM
(
    Message, uint8_t,

    TYPE_INFO,
    TYPE_WARNING,
    TYPE_ERROR,

    MAX_NUM
);

NRI_STRUCT(MemoryAllocatorInterface)
{
    void* (*Allocate)(void* userArg, size_t size, size_t alignment);
    void* (*Reallocate)(void* userArg, void* memory, size_t size, size_t alignment);
    void (*Free)(void* userArg, void* memory);
    void* userArg;
};

NRI_STRUCT(CallbackInterface)
{
    void (*MessageCallback)(void* userArg, const char* message, NRI_NAME(Message) messageType);
    void (*AbortExecution)(void* userArg);
    void* userArg;
};

NRI_STRUCT(VulkanExtensions)
{
    const char* const* instanceExtensions;
    uint32_t instanceExtensionNum;
    const char* const* deviceExtensions;
    uint32_t deviceExtensionNum;
};

NRI_STRUCT(DeviceCreationDesc)
{
    const NRI_NAME(AdapterDesc)* adapterDesc;
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    NRI_NAME(GraphicsAPI) graphicsAPI;
    NRI_NAME(SPIRVBindingOffsets) spirvBindingOffsets;
    NRI_NAME(VulkanExtensions) vulkanExtensions;
    bool enableNRIValidation;
    bool enableAPIValidation;
    bool enableMGPU;
    bool D3D11CommandBufferEmulation;
    bool skipLiveObjectsReporting;
};

NRI_API NRI_NAME(Result) NRI_CALL nriEnumerateAdapters(NRI_NAME(AdapterDesc)* adapterDescs, uint32_t NRI_REF adapterDescNum);
NRI_API NRI_NAME(Result) NRI_CALL nriCreateDevice(const NRI_NAME_REF(DeviceCreationDesc) deviceCreationDesc, NRI_NAME_REF(Device*) device);
NRI_API void NRI_CALL nriDestroyDevice(NRI_NAME_REF(Device) device);

NRI_NAMESPACE_END
