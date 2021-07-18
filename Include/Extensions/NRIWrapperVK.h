/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

struct VkImageSubresourceRange;

namespace nri
{
    typedef uint64_t NRIVkInstance;
    typedef uint64_t NRIVkPhysicalDevice;
    typedef uint64_t NRIVkDevice;
    typedef uint64_t NRIVkQueue;
    typedef void* NRIVkCommandPool;
    typedef uint64_t NRIVkCommandBuffer;
    typedef void* NRIVkImage;
    typedef void* NRIVkBuffer;
    typedef void* NRIVkDeviceMemory;
    typedef void* NRIVkQueryPool;
    typedef void* NRIVkPipeline;
    typedef void* NRIVkDescriptorPool;
    typedef void* NRIVkSemaphore;
    typedef void* NRIVkFence;

    typedef void* NRIVkImageView;
    typedef void* NRIVkBufferView;

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
        uint64_t bufferSize;
        Memory* memory;
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

    NRI_API Result NRI_CALL CreateDeviceFromVkDevice(const DeviceCreationVulkanDesc& deviceDesc, Device*& device);
    NRI_API Format NRI_CALL GetFormatVK(uint32_t vkFormat);

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

        NRIVkDevice (NRI_CALL *GetDeviceVK)(const Device& device);
        NRIVkPhysicalDevice (NRI_CALL *GetPhysicalDeviceVK)(const Device& device);
        NRIVkInstance (NRI_CALL *GetInstanceVK)(const Device& device);
        NRIVkCommandBuffer (NRI_CALL *GetCommandBufferVK)(const CommandBuffer& commandBuffer);

        void (NRI_CALL *GetTextureVK)(const Texture& texture, uint32_t physicalDeviceIndex, TextureVulkanDesc& textureVulkanDesc);
        NRIVkImageView (NRI_CALL *GetTextureDescriptorVK)(const Descriptor& descriptor, uint32_t physicalDeviceIndex, VkImageSubresourceRange& subresource);
        NRIVkBufferView (NRI_CALL *GetBufferDescriptorVK)(const Descriptor& descriptor, uint32_t physicalDeviceIndex);
    };
}
