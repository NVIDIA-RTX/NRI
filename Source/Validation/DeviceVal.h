// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVal;

struct IsExtSupported {
    uint32_t lowLatency   : 1;
    uint32_t meshShader   : 1;
    uint32_t rayTracing   : 1;
    uint32_t swapChain    : 1;
    uint32_t wrapperD3D11 : 1;
    uint32_t wrapperD3D12 : 1;
    uint32_t wrapperVK    : 1;
};

struct DeviceVal final : public DeviceBase {
    DeviceVal(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks, DeviceBase& device);
    ~DeviceVal();

    inline Device& GetImpl() const {
        return m_Impl;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_iCore;
    }

    inline const CoreInterface& GetCoreInterfaceImpl() const {
        return m_iCoreImpl;
    }

    inline const HelperInterface& GetHelperInterfaceImpl() const {
        return m_iHelperImpl;
    }

    inline const LowLatencyInterface& GetLowLatencyInterfaceImpl() const {
        return m_iLowLatencyImpl;
    }

    inline const MeshShaderInterface& GetMeshShaderInterfaceImpl() const {
        return m_iMeshShaderImpl;
    }

    inline const RayTracingInterface& GetRayTracingInterfaceImpl() const {
        return m_iRayTracingImpl;
    }

    inline const SwapChainInterface& GetSwapChainInterfaceImpl() const {
        return m_iSwapChainImpl;
    }

    inline const WrapperD3D11Interface& GetWrapperD3D11InterfaceImpl() const {
        return m_iWrapperD3D11Impl;
    }

    inline const WrapperD3D12Interface& GetWrapperD3D12InterfaceImpl() const {
        return m_iWrapperD3D12Impl;
    }

    inline const WrapperVKInterface& GetWrapperVKInterfaceImpl() const {
        return m_iWrapperVKImpl;
    }

    inline void* GetNativeObject() const {
        return m_iCoreImpl.GetDeviceNativeObject(&m_Impl);
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

        GetCoreInterfaceImpl().SetDebugName(&m_Impl, name);
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
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperD3D11Interface& table) const override;
    Result FillFunctionTable(WrapperD3D12Interface& table) const override;
    Result FillFunctionTable(WrapperVKInterface& table) const override;

#if NRI_ENABLE_IMGUI_EXTENSION
    Result FillFunctionTable(ImguiInterface& table) const override;
#endif

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateFence(uint64_t initialValue, Fence*& fence);
    Result CreateFence(const FenceVKDesc& fenceVKDesc, Fence*& fence);
    Result CreateFence(const FenceD3D12Desc& fenceD3D12Desc, Fence*& fence);
    Result CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory);
    Result CreateMemory(const MemoryD3D12Desc& memoryD3D12Desc, Memory*& memory);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferVKDesc& bufferVKDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferD3D11Desc& bufferD3D11Desc, Buffer*& buffer);
    Result CreateBuffer(const BufferD3D12Desc& bufferD3D12Desc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateTexture(const TextureVKDesc& textureVKDesc, Texture*& texture);
    Result CreateTexture(const TextureD3D11Desc& textureD3D11Desc, Texture*& texture);
    Result CreateTexture(const TextureD3D12Desc& textureD3D12Desc, Texture*& texture);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const PipelineVKDesc& pipelineVKDesc, Pipeline*& pipeline);
    Result CreateMicromap(const MicromapDesc& micromapDesc, Micromap*& micromap);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool);
    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    Result CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateCommandBuffer(const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer);
    Result CreateCommandBuffer(const CommandBufferD3D11Desc& commandBufferD3D11Desc, CommandBuffer*& commandBuffer);
    Result CreateCommandBuffer(const CommandBufferD3D12Desc& commandBufferD3D12Desc, CommandBuffer*& commandBuffer);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool);
    Result CreateDescriptorPool(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool);
    Result CreateCommandAllocator(const Queue& queue, CommandAllocator*& commandAllocator);
    Result CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator);
    Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureVKDesc, AccelerationStructure*& accelerationStructure);
    Result CreateAccelerationStructure(const AccelerationStructureD3D12Desc& accelerationStructureD3D12Desc, AccelerationStructure*& accelerationStructure);
    Result CreateCommittedBuffer(MemoryLocation memoryLocation, float priority, const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateCommittedTexture(MemoryLocation memoryLocation, float priority, const TextureDesc& textureDesc, Texture*& texture);
    Result CreateCommittedMicromap(MemoryLocation memoryLocation, float priority, const MicromapDesc& micromapDesc, Micromap*& micromap);
    Result CreateCommittedAccelerationStructure(MemoryLocation memoryLocation, float priority, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreatePlacedBuffer(Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreatePlacedTexture(Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture);
    Result CreatePlacedMicromap(Memory* memory, uint64_t offset, const MicromapDesc& micromapDesc, Micromap*& micromap);
    Result CreatePlacedAccelerationStructure(Memory* memory, uint64_t offset, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result AllocateMemory(const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory);
    Result BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum);
    Result BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum);
    Result BindMicromapMemory(const BindMicromapMemoryDesc* bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum);
    Result BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum);
    Result GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue);
    Result WaitIdle();

    void FreeMemory(Memory* memory);
    void DestroyFence(Fence* fence);
    void DestroyBuffer(Buffer* buffer);
    void DestroyTexture(Texture* texture);
    void DestroyPipeline(Pipeline* pipeline);
    void DestroyMicromap(Micromap* micromap);
    void DestroyQueryPool(QueryPool* queryPool);
    void DestroySwapChain(SwapChain* swapChain);
    void DestroyDescriptor(Descriptor* descriptor);
    void DestroyDescriptorPool(DescriptorPool* descriptorPool);
    void DestroyPipelineLayout(PipelineLayout* pipelineLayout);
    void DestroyCommandBuffer(CommandBuffer* commandBuffer);
    void DestroyCommandAllocator(CommandAllocator* commandAllocator);
    void DestroyAccelerationStructure(AccelerationStructure* accelerationStructure);
    void CopyDescriptorRanges(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum);
    void UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum);

    FormatSupportBits GetFormatSupport(Format format) const;

private:
    char* m_Name = nullptr; // .natvis
    DeviceDesc m_Desc = {}; // .natvis
    Device& m_Impl;
    std::array<QueueVal*, (size_t)QueueType::MAX_NUM> m_Queues = {};
    UnorderedMap<MemoryType, MemoryLocation> m_MemoryTypeMap;

    // Validation
    CoreInterface m_iCore = {};

    // Implementation
    CoreInterface m_iCoreImpl = {};
    HelperInterface m_iHelperImpl = {};
    LowLatencyInterface m_iLowLatencyImpl = {};
    MeshShaderInterface m_iMeshShaderImpl = {};
    RayTracingInterface m_iRayTracingImpl = {};
    SwapChainInterface m_iSwapChainImpl = {};
    WrapperD3D11Interface m_iWrapperD3D11Impl = {};
    WrapperD3D12Interface m_iWrapperD3D12Impl = {};
    WrapperVKInterface m_iWrapperVKImpl = {};

    union {
        uint32_t m_IsExtSupportedStorage = 0;
        IsExtSupported m_IsExtSupported;
    };

    Lock m_Lock;
};

} // namespace nri
