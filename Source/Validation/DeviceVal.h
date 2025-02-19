// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVal;

struct IsExtSupported {
    uint32_t lowLatency : 1;
    uint32_t meshShader : 1;
    uint32_t rayTracing : 1;
    uint32_t swapChain : 1;
    uint32_t wrapperD3D11 : 1;
    uint32_t wrapperD3D12 : 1;
    uint32_t wrapperVK : 1;
};

struct DeviceVal final : public DeviceBase {
    DeviceVal(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks, DeviceBase& device);
    ~DeviceVal();

    inline Device& GetImpl() const {
        return m_Impl;
    }

    inline const CoreInterface& GetCoreInterfaceVal() const {
        return m_iCoreVal;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_iCore;
    }

    inline const HelperInterface& GetHelperInterface() const {
        return m_iHelper;
    }

    inline const LowLatencyInterface& GetLowLatencyInterface() const {
        return m_iLowLatency;
    }

    inline const MeshShaderInterface& GetMeshShaderInterface() const {
        return m_iMeshShader;
    }

    inline const RayTracingInterface& GetRayTracingInterface() const {
        return m_iRayTracing;
    }

    inline const SwapChainInterface& GetSwapChainInterface() const {
        return m_iSwapChain;
    }

    inline const WrapperD3D11Interface& GetWrapperD3D11Interface() const {
        return m_iWrapperD3D11;
    }

    inline const WrapperD3D12Interface& GetWrapperD3D12Interface() const {
        return m_iWrapperD3D12;
    }

    inline const WrapperVKInterface& GetWrapperVKInterface() const {
        return m_iWrapperVK;
    }

    inline void* GetNativeObject() const {
        return m_iCore.GetDeviceNativeObject(m_Impl);
    }

    inline Lock& GetLock() {
        return m_Lock;
    }

    bool Create();
    void RegisterMemoryType(MemoryType memoryType, MemoryLocation memoryLocation);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) override {
        const auto& allocationCallbacks = GetAllocationCallbacks();
        if (m_Name)
            allocationCallbacks.Free(allocationCallbacks.userArg, m_Name);

        size_t len = strlen(name);
        m_Name = (char*)allocationCallbacks.Allocate(allocationCallbacks.userArg, len + 1, sizeof(size_t));
        strcpy(m_Name, name);

        GetCoreInterface().SetDebugName(&m_Impl, name);
    }

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    const DeviceDesc& GetDesc() const override {
        return ((DeviceBase&)m_Impl).GetDesc();
    }

    void Destruct() override;
    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(LowLatencyInterface& table) const override;
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperD3D11Interface& table) const override;
    Result FillFunctionTable(WrapperD3D12Interface& table) const override;
    Result FillFunctionTable(WrapperVKInterface& table) const override;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateFence(uint64_t initialValue, Fence*& fence);
    Result CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory);
    Result CreateMemory(const MemoryD3D12Desc& memoryDesc, Memory*& memory);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferVKDesc& bufferDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferD3D11Desc& bufferDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferD3D12Desc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateTexture(const TextureVKDesc& textureVKDesc, Texture*& texture);
    Result CreateTexture(const TextureD3D11Desc& textureDesc, Texture*& texture);
    Result CreateTexture(const TextureD3D12Desc& textureDesc, Texture*& texture);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline);
    Result AllocateBuffer(const AllocateBufferDesc& bufferDesc, Buffer*& buffer);
    Result AllocateTexture(const AllocateTextureDesc& textureDesc, Texture*& texture);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool);
    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    Result CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateCommandBuffer(const CommandBufferVKDesc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateCommandBuffer(const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateCommandBuffer(const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool);
    Result CreateDescriptorPool(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool);
    Result CreateComputePipeline(VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline);
    Result CreateGraphicsPipeline(VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline);
    Result CreateCommandAllocator(const Queue& queue, CommandAllocator*& commandAllocator);
    Result CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator);
    Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result AllocateAccelerationStructure(const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreateAccelerationStructure(const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);

    void DestroyFence(Fence& fence);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyQueryPool(QueryPool& queryPool);
    void DestroySwapChain(SwapChain& swapChain);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyCommandBuffer(CommandBuffer& commandBuffer);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);

    void FreeMemory(Memory& memory);
    Result GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue);
    Result AllocateMemory(const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    char* m_Name = nullptr; // .natvis
    DeviceDesc m_Desc = {}; // .natvis
    Device& m_Impl;
    std::array<QueueVal*, (size_t)QueueType::MAX_NUM> m_Queues = {};
    UnorderedMap<MemoryType, MemoryLocation> m_MemoryTypeMap;

    // Validation interfaces
    CoreInterface m_iCoreVal = {};

    // Original interfaces
    CoreInterface m_iCore = {};
    HelperInterface m_iHelper = {};
    LowLatencyInterface m_iLowLatency = {};
    MeshShaderInterface m_iMeshShader = {};
    RayTracingInterface m_iRayTracing = {};
    ResourceAllocatorInterface m_iResourceAllocator = {};
    SwapChainInterface m_iSwapChain = {};
    WrapperD3D11Interface m_iWrapperD3D11 = {};
    WrapperD3D12Interface m_iWrapperD3D12 = {};
    WrapperVKInterface m_iWrapperVK = {};

    union {
        uint32_t m_IsExtSupportedStorage = 0;
        IsExtSupported m_IsExtSupported;
    };

    Lock m_Lock;
};

} // namespace nri
