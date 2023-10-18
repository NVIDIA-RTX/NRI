/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "NRIDeviceCreation.h"

NRI_FORWARD_STRUCT(VkImageSubresourceRange);

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(AccelerationStructure);

typedef uint64_t NRIVkCommandPool;
typedef uint64_t NRIVkImage;
typedef uint64_t NRIVkBuffer;
typedef uint64_t NRIVkDeviceMemory;
typedef uint64_t NRIVkQueryPool;
typedef uint64_t NRIVkPipeline;
typedef uint64_t NRIVkDescriptorPool;
typedef uint64_t NRIVkImageView;
typedef uint64_t NRIVkBufferView;
typedef uint64_t NRIVkAccelerationStructureKHR;

typedef void* NRIVkInstance;
typedef void* NRIVkPhysicalDevice;
typedef void* NRIVkDevice;
typedef void* NRIVkQueue;
typedef void* NRIVkCommandBuffer;
typedef void* NRIVkProcAddress;

NRI_STRUCT(DeviceCreationVulkanDesc)
{
    NRIVkInstance vkInstance;
    NRIVkDevice vkDevice;
    const NRIVkPhysicalDevice* vkPhysicalDevices;
    uint32_t deviceGroupSize;
    const uint32_t* queueFamilyIndices;
    uint32_t queueFamilyIndexNum;
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    NRI_NAME(SPIRVBindingOffsets) spirvBindingOffsets;
    const char* vulkanLoaderPath;
    bool enableNRIValidation;
};

NRI_STRUCT(CommandQueueVulkanDesc)
{
    NRIVkQueue vkQueue;
    uint32_t familyIndex;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(CommandAllocatorVulkanDesc)
{
    NRIVkCommandPool vkCommandPool;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(CommandBufferVulkanDesc)
{
    NRIVkCommandBuffer vkCommandBuffer;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(BufferVulkanDesc)
{
    NRIVkBuffer vkBuffer;
    NRI_NAME(Memory)* memory;
    uint64_t bufferSize;
    uint64_t memoryOffset;
    uint64_t deviceAddress;
    uint32_t physicalDeviceMask;
};

NRI_STRUCT(TextureVulkanDesc)
{
    NRIVkImage vkImage;
    uint32_t vkFormat;
    uint32_t vkImageAspectFlags;
    uint32_t vkImageType;
    uint16_t size[3];
    uint16_t mipNum;
    uint16_t arraySize;
    uint8_t sampleNum;
    uint32_t physicalDeviceMask;
};

NRI_STRUCT(MemoryVulkanDesc)
{
    NRIVkDeviceMemory vkDeviceMemory;
    uint64_t size;
    uint32_t memoryTypeIndex;
    uint32_t physicalDeviceMask;
};

NRI_STRUCT(QueryPoolVulkanDesc)
{
    NRIVkQueryPool vkQueryPool;
    uint32_t vkQueryType;
    uint32_t physicalDeviceMask;
};

NRI_STRUCT(AccelerationStructureVulkanDesc)
{
    NRIVkAccelerationStructureKHR vkAccelerationStructure;
    uint64_t buildScratchSize;
    uint64_t updateScratchSize;
    uint32_t physicalDeviceMask;
};

NRI_STRUCT(WrapperVKInterface)
{
    NRI_NAME(Result) (NRI_CALL *CreateCommandQueueVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandQueueVulkanDesc) commandQueueVulkanDesc, NRI_NAME_REF(CommandQueue*) commandQueue);
    NRI_NAME(Result) (NRI_CALL *CreateCommandAllocatorVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandAllocatorVulkanDesc) commandAllocatorVulkanDesc, NRI_NAME_REF(CommandAllocator*) commandAllocator);
    NRI_NAME(Result) (NRI_CALL *CreateCommandBufferVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandBufferVulkanDesc) commandBufferVulkanDesc, NRI_NAME_REF(CommandBuffer*) commandBuffer);
    NRI_NAME(Result) (NRI_CALL *CreateDescriptorPoolVK)(NRI_NAME_REF(Device) device, NRIVkDescriptorPool vkDescriptorPool, NRI_NAME_REF(DescriptorPool*) descriptorPool);
    NRI_NAME(Result) (NRI_CALL *CreateBufferVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(BufferVulkanDesc) bufferVulkanDesc, NRI_NAME_REF(Buffer*) buffer);
    NRI_NAME(Result) (NRI_CALL *CreateTextureVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(TextureVulkanDesc) textureVulkanDesc, NRI_NAME_REF(Texture*) texture);
    NRI_NAME(Result) (NRI_CALL *CreateMemoryVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(MemoryVulkanDesc) memoryVulkanDesc, NRI_NAME_REF(Memory*) memory);
    NRI_NAME(Result) (NRI_CALL *CreateGraphicsPipelineVK)(NRI_NAME_REF(Device) device, NRIVkPipeline vkPipeline, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateComputePipelineVK)(NRI_NAME_REF(Device) device, NRIVkPipeline vkPipeline, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateQueryPoolVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(QueryPoolVulkanDesc) queryPoolVulkanDesc, NRI_NAME_REF(QueryPool*) queryPool);
    NRI_NAME(Result) (NRI_CALL *CreateAccelerationStructureVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AccelerationStructureVulkanDesc) accelerationStructureDesc, NRI_NAME_REF(AccelerationStructure*) accelerationStructure);

    NRIVkPhysicalDevice (NRI_CALL *GetVkPhysicalDevice)(const NRI_NAME_REF(Device) device);
    NRIVkInstance (NRI_CALL *GetVkInstance)(const NRI_NAME_REF(Device) device);
    NRIVkProcAddress(NRI_CALL *GetVkGetInstanceProcAddr)(const NRI_NAME_REF(Device) device);
    NRIVkProcAddress(NRI_CALL *GetVkGetDeviceProcAddr)(const NRI_NAME_REF(Device) device);
};

NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromVkDevice(const NRI_NAME_REF(DeviceCreationVulkanDesc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END
