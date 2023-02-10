/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

namespace nri
{
    struct CommandQueueVK;

    struct DeviceVK final : public DeviceBase
    {
        DeviceVK(const Log& log, const StdAllocator<uint8_t>& stdAllocator);
        ~DeviceVK();

        operator VkDevice() const;
        operator VkPhysicalDevice() const;
        operator VkInstance() const;

        const DispatchTable& GetDispatchTable() const;
        const VkAllocationCallbacks* GetAllocationCallbacks() const;
        const std::array<uint32_t, COMMAND_QUEUE_TYPE_NUM>& GetQueueFamilyIndices() const;
        const SPIRVBindingOffsets& GetSPIRVBindingOffsets() const;
        const CoreInterface& GetCoreInterface() const;

        Result Create(const DeviceCreationVulkanDesc& deviceCreationVulkanDesc);
        Result Create(const DeviceCreationDesc& deviceCreationDesc);
        bool GetMemoryType(MemoryLocation memoryLocation, uint32_t memoryTypeMask, MemoryTypeInfo& memoryTypeInfo) const;
        bool GetMemoryType(uint32_t index, MemoryTypeInfo& memoryTypeInfo) const;

        uint32_t GetPhysicalDeviceGroupSize() const;
        bool IsDescriptorIndexingExtSupported() const;
        bool IsConcurrentSharingModeEnabledForBuffers() const;
        bool IsConcurrentSharingModeEnabledForImages() const;
        bool IsBufferDeviceAddressSupported() const;
        const Vector<uint32_t>& GetConcurrentSharingModeQueueIndices() const;

        void SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name);
        void SetDebugNameToDeviceGroupObject(VkObjectType objectType, const uint64_t* handles, const char* name);

        //================================================================================================================
        // NRI
        //================================================================================================================
        void SetDebugName(const char* name);
        
        inline const DeviceDesc& GetDesc() const
        { return m_DeviceDesc; }

        Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);

        Result CreateCommandAllocator(const CommandQueue& commandQueue, uint32_t physicalDeviceMask, CommandAllocator*& commandAllocator);
        Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
        Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
        Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
        Result CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
        Result CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
        Result CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
        Result CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
        Result CreateSampler(const SamplerDesc& samplerDesc, Descriptor*& sampler);
        Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
        Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
        Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
        Result CreateFrameBuffer(const FrameBufferDesc& frameBufferDesc, FrameBuffer*& frameBuffer);
        Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
        Result CreateQueueSemaphore(QueueSemaphore*& queueSemaphore);
        Result CreateDeviceSemaphore(bool signaled, DeviceSemaphore*& deviceSemaphore);
        Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
        Result CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline);
        Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);

        Result CreateCommandQueue(const CommandQueueVulkanDesc& commandQueueDesc, CommandQueue*& commandQueue);
        Result CreateCommandAllocator(const CommandAllocatorVulkanDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator);
        Result CreateCommandBuffer(const CommandBufferVulkanDesc& commandBufferDesc, CommandBuffer*& commandBuffer);
        Result CreateDescriptorPool(NRIVkDescriptorPool vkDescriptorPool, DescriptorPool*& descriptorPool);
        Result CreateBuffer(const BufferVulkanDesc& bufferDesc, Buffer*& buffer);
        Result CreateTexture(const TextureVulkanDesc& textureVulkanDesc, Texture*& texture);
        Result CreateMemory(const MemoryVulkanDesc& memoryVulkanDesc, Memory*& memory);
        Result CreateGraphicsPipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
        Result CreateComputePipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
        Result CreateQueryPool(const QueryPoolVulkanDesc& queryPoolVulkanDesc, QueryPool*& queryPool);
        Result CreateQueueSemaphore(NRIVkSemaphore vkSemaphore, QueueSemaphore*& queueSemaphore);
        Result CreateDeviceSemaphore(NRIVkFence vkFence, DeviceSemaphore*& deviceSemaphore);
        Result CreateAccelerationStructure(const AccelerationStructureVulkanDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);

        void DestroyCommandAllocator(CommandAllocator& commandAllocator);
        void DestroyDescriptorPool(DescriptorPool& descriptorPool);
        void DestroyBuffer(Buffer& buffer);
        void DestroyTexture(Texture& texture);
        void DestroyDescriptor(Descriptor& descriptor);
        void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
        void DestroyPipeline(Pipeline& pipeline);
        void DestroyFrameBuffer(FrameBuffer& frameBuffer);
        void DestroyQueryPool(QueryPool& queryPool);
        void DestroyQueueSemaphore(QueueSemaphore& queueSemaphore);
        void DestroyDeviceSemaphore(DeviceSemaphore& deviceSemaphore);
        void DestroySwapChain(SwapChain& swapChain);
        void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);

        Result GetDisplays(Display** displays, uint32_t& displayNum);
        Result GetDisplaySize(Display& display, uint16_t& width, uint16_t& height);

        Result AllocateMemory(uint32_t physicalDeviceMask, MemoryType memoryType, uint64_t size, Memory*& memory);
        Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
        Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
        Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
        void FreeMemory(Memory& memory);

        FormatSupportBits GetFormatSupport(Format format) const;

        uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const;
        Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);

        //================================================================================================================
        // DeviceBase
        //================================================================================================================
        void Destroy();
        Result FillFunctionTable(CoreInterface& table) const;
        Result FillFunctionTable(SwapChainInterface& table) const;
        Result FillFunctionTable(WrapperVKInterface& wrapperVKInterface) const;
        Result FillFunctionTable(RayTracingInterface& rayTracingInterface) const;
        Result FillFunctionTable(MeshShaderInterface& meshShaderInterface) const;
        Result FillFunctionTable(HelperInterface& helperInterface) const;

    private:
        Result CreateInstance(const DeviceCreationDesc& deviceCreationDesc);
        Result FindPhysicalDeviceGroup(const PhysicalDeviceGroup* physicalDeviceGroup, bool enableMGPU);
        Result CreateLogicalDevice(const DeviceCreationDesc& deviceCreationDesc);
        void FillFamilyIndices(bool useEnabledFamilyIndices, const uint32_t* enabledFamilyIndices, uint32_t familyIndexNum);
        void SetDeviceLimits(bool enableValidation);
        void CreateCommandQueues();
        Result ResolvePreInstanceDispatchTable();
        Result ResolveInstanceDispatchTable();
        Result ResolveDispatchTable();
        void FilterInstanceLayers(Vector<const char*>& layers);
        bool FilterInstanceExtensions(Vector<const char*>& extensions);
        bool FilterDeviceExtensions(Vector<const char*>& extensions);
        void RetrieveRayTracingInfo();
        void RetrieveMeshShaderInfo();
        void ReportDeviceGroupInfo();
        void CheckSupportedDeviceExtensions(const Vector<const char*>& extensions);
        void CheckSupportedInstanceExtensions(const Vector<const char*>& extensions);
        void FindDXGIAdapter();

        template< typename Implementation, typename Interface, typename ... Args >
        Result CreateImplementation(Interface*& entity, const Args&... args);

        VkDevice m_Device = VK_NULL_HANDLE;
        Vector<VkPhysicalDevice> m_PhysicalDevices;
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkPhysicalDeviceMemoryProperties m_MemoryProps = {};
        DispatchTable m_VK = {};
        DeviceDesc m_DeviceDesc = {};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RayTracingDeviceProperties = {};
        std::array<uint32_t, COMMAND_QUEUE_TYPE_NUM> m_FamilyIndices = {};
        std::array<CommandQueueVK*, COMMAND_QUEUE_TYPE_NUM> m_Queues = {};
        Vector<uint32_t> m_PhysicalDeviceIndices;
        Vector<uint32_t> m_ConcurrentSharingModeQueueIndices;
        VkAllocationCallbacks* m_AllocationCallbackPtr = nullptr;
        VkAllocationCallbacks m_AllocationCallbacks = {};
        VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
        SPIRVBindingOffsets m_SPIRVBindingOffsets = {};
        CoreInterface m_CoreInterface = {};
        Lock m_Lock;
        uint64_t m_LUID = 0;
        bool m_OwnsNativeObjects = false;
        bool m_IsRayTracingExtSupported = false;
        bool m_IsDescriptorIndexingExtSupported = false;
        bool m_IsSampleLocationExtSupported = false;
        bool m_IsMinMaxFilterExtSupported = false;
        bool m_IsConservativeRasterExtSupported = false;
        bool m_IsMeshShaderExtSupported = false;
        bool m_IsHDRExtSupported = false;
        bool m_IsDemoteToHelperInvocationSupported = false;
        bool m_IsSubsetAllocationSupported = false;
        bool m_IsConcurrentSharingModeEnabledForBuffers = true;
        bool m_IsConcurrentSharingModeEnabledForImages = true;
        bool m_IsDebugUtilsSupported = false;
        bool m_IsFP16Supported = false;
        bool m_IsBufferDeviceAddressSupported = false;
        bool m_IsMicroMapSupported = false;
        Library* m_Loader = nullptr;
#if _WIN32
        ComPtr<IDXGIAdapter> m_Adapter;
#endif
    };

    inline DeviceVK::operator VkDevice() const
    {
        return m_Device;
    }

    inline DeviceVK::operator VkPhysicalDevice() const
    {
        return m_PhysicalDevices.front();
    }

    inline DeviceVK::operator VkInstance() const
    {
        return m_Instance;
    }

    inline const DispatchTable& DeviceVK::GetDispatchTable() const
    {
        return m_VK;
    }

    inline const VkAllocationCallbacks* DeviceVK::GetAllocationCallbacks() const
    {
        return m_AllocationCallbackPtr;
    }

    inline const std::array<uint32_t, COMMAND_QUEUE_TYPE_NUM>& DeviceVK::GetQueueFamilyIndices() const
    {
        return m_FamilyIndices;
    }

    inline const SPIRVBindingOffsets& DeviceVK::GetSPIRVBindingOffsets() const
    {
        return m_SPIRVBindingOffsets;
    }

    inline const CoreInterface& DeviceVK::GetCoreInterface() const
    {
        return m_CoreInterface;
    }

    inline uint32_t DeviceVK::GetPhysicalDeviceGroupSize() const
    {
        return m_DeviceDesc.physicalDeviceNum;
    }

    inline bool DeviceVK::IsDescriptorIndexingExtSupported() const
    {
        return m_IsDescriptorIndexingExtSupported;
    }

    inline bool DeviceVK::IsConcurrentSharingModeEnabledForBuffers() const
    {
        return m_IsConcurrentSharingModeEnabledForBuffers;
    }

    inline bool DeviceVK::IsConcurrentSharingModeEnabledForImages() const
    {
        return m_IsConcurrentSharingModeEnabledForImages;
    }

    inline bool DeviceVK::IsBufferDeviceAddressSupported() const
    {
        return m_IsBufferDeviceAddressSupported;
    }

    inline const Vector<uint32_t>& DeviceVK::GetConcurrentSharingModeQueueIndices() const
    {
        return m_ConcurrentSharingModeQueueIndices;
    }
}

/*
    TODO:
        - Cubemaps and 3D textures
            Some flags are missing in VkImageCreateInfo
*/