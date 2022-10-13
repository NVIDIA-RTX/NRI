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

struct VkImageSubresourceRange;

namespace nri
{
    struct AccelerationStructure;

    typedef uint64_t NRIVkCommandPool;
    typedef uint64_t NRIVkImage;
    typedef uint64_t NRIVkBuffer;
    typedef uint64_t NRIVkDeviceMemory;
    typedef uint64_t NRIVkQueryPool;
    typedef uint64_t NRIVkPipeline;
    typedef uint64_t NRIVkDescriptorPool;
    typedef uint64_t NRIVkSemaphore;
    typedef uint64_t NRIVkFence;
    typedef uint64_t NRIVkImageView;
    typedef uint64_t NRIVkBufferView;
    typedef uint64_t NRIVkAccelerationStructureKHR;

    typedef void* NRIVkInstance;
    typedef void* NRIVkPhysicalDevice;
    typedef void* NRIVkDevice;
    typedef void* NRIVkQueue;
    typedef void* NRIVkCommandBuffer;

    struct DeviceCreationVulkanDesc
    {
        NRIVkInstance vkInstance;
        NRIVkDevice vkDevice;
        const NRIVkPhysicalDevice* vkPhysicalDevices;
        uint32_t deviceGroupSize;
        const uint32_t* queueFamilyIndices;
        uint32_t queueFamilyIndexNum;
        CallbackInterface callbackInterface;
        MemoryAllocatorInterface memoryAllocatorInterface;
        bool enableNRIValidation;
        bool enableAPIValidation;
        const char* const* instanceExtensions;
        uint32_t instanceExtensionNum;
        const char* const* deviceExtensions;
        uint32_t deviceExtensionNum;
        const char* vulkanLoaderPath;
        SPIRVBindingOffsets spirvBindingOffsets;
    };

    struct CommandQueueVulkanDesc
    {
        NRIVkQueue vkQueue;
        uint32_t familyIndex;
        CommandQueueType commandQueueType;
    };

    struct CommandAllocatorVulkanDesc
    {
        NRIVkCommandPool vkCommandPool;
        CommandQueueType commandQueueType;
    };

    struct CommandBufferVulkanDesc
    {
        NRIVkCommandBuffer vkCommandBuffer;
        CommandQueueType commandQueueType;
    };

    struct BufferVulkanDesc
    {
        NRIVkBuffer vkBuffer;
        Memory* memory;
        uint64_t bufferSize;
        uint64_t memoryOffset;
        uint64_t deviceAddress;
        uint32_t physicalDeviceMask;
    };

    struct TextureVulkanDesc
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

    struct MemoryVulkanDesc
    {
        NRIVkDeviceMemory vkDeviceMemory;
        uint64_t size;
        uint32_t memoryTypeIndex;
        uint32_t physicalDeviceMask;
    };

    struct QueryPoolVulkanDesc
    {
        NRIVkQueryPool vkQueryPool;
        uint32_t vkQueryType;
        uint32_t physicalDeviceMask;
    };

    struct AccelerationStructureVulkanDesc
    {
        NRIVkAccelerationStructureKHR vkAccelerationStructure;
        uint64_t buildScratchSize;
        uint64_t updateScratchSize;
        uint32_t physicalDeviceMask;
    };

    NRI_API Result NRI_CALL CreateDeviceFromVkDevice(const DeviceCreationVulkanDesc& deviceDesc, Device*& device);
    NRI_API Format NRI_CALL ConvertVKFormatToNRI(uint32_t vkFormat);
    NRI_API uint32_t NRI_CALL ConvertNRIFormatToVK(Format format);

    struct WrapperVKInterface
    {
        Result (NRI_CALL *CreateCommandQueueVK)(Device& device, const CommandQueueVulkanDesc& commandQueueVulkanDesc, CommandQueue*& commandQueue);
        Result (NRI_CALL *CreateCommandAllocatorVK)(Device& device, const CommandAllocatorVulkanDesc& commandAllocatorVulkanDesc, CommandAllocator*& commandAllocator);
        Result (NRI_CALL *CreateCommandBufferVK)(Device& device, const CommandBufferVulkanDesc& commandBufferVulkanDesc, CommandBuffer*& commandBuffer);
        Result (NRI_CALL *CreateDescriptorPoolVK)(Device& device, NRIVkDescriptorPool vkDescriptorPool, DescriptorPool*& descriptorPool);
        Result (NRI_CALL *CreateBufferVK)(Device& device, const BufferVulkanDesc& bufferVulkanDesc, Buffer*& buffer);
        Result (NRI_CALL *CreateTextureVK)(Device& device, const TextureVulkanDesc& textureVulkanDesc, Texture*& texture);
        Result (NRI_CALL *CreateMemoryVK)(Device& device, const MemoryVulkanDesc& memoryVulkanDesc, Memory*& memory);
        Result (NRI_CALL *CreateGraphicsPipelineVK)(Device& device, NRIVkPipeline vkPipeline, Pipeline*& pipeline);
        Result (NRI_CALL *CreateComputePipelineVK)(Device& device, NRIVkPipeline vkPipeline, Pipeline*& pipeline);
        Result (NRI_CALL *CreateQueryPoolVK)(Device& device, const QueryPoolVulkanDesc& queryPoolVulkanDesc, QueryPool*& queryPool);
        Result (NRI_CALL *CreateQueueSemaphoreVK)(Device& device, NRIVkSemaphore vkSemaphore, QueueSemaphore*& queueSemaphore);
        Result (NRI_CALL *CreateDeviceSemaphoreVK)(Device& device, NRIVkFence vkFence, DeviceSemaphore*& deviceSemaphore);
        Result (NRI_CALL *CreateAccelerationStructureVK)(Device& device, const AccelerationStructureVulkanDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);

        NRIVkPhysicalDevice (NRI_CALL *GetVkPhysicalDevice)(const Device& device);
        NRIVkInstance (NRI_CALL *GetVkInstance)(const Device& device);
    };
}
