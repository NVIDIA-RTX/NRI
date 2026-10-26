// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct HostCopyLayoutWGPU {
    uint64_t offset;
    uint64_t slicePitch;
    uint32_t rowPitch;
    uint32_t rowSize;
    uint32_t rowNum;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct HostCopyContextWGPU {
    BufferWGPU* readbackBuffer = nullptr;
    bool isInUse = false;
};

struct DeviceWGPU final : public DeviceBase {
    DeviceWGPU(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks);
    ~DeviceWGPU();

    inline operator WGPUDevice() const {
        return m_Device;
    }

    inline WGPUInstance GetInstance() const {
        return m_Instance;
    }

    inline WGPUAdapter GetAdapter() const {
        return m_Adapter;
    }

    inline WGPUQueue GetQueue() const {
        return m_Queue;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_iCore;
    }

    inline const DeviceDesc& GetDesc() const override {
        return m_Desc;
    }

    inline const VKBindingOffsets& GetBindingOffsets() const {
        return m_BindingOffsets;
    }

    inline bool IsTimestampQueryInsidePassesSupported() const {
        return m_IsTimestampQueryInsidePassesSupported;
    }

    void Destruct() override;
    Result Create(const DeviceCreationDesc& deviceCreationDesc);
    FormatSupportBits GetFormatSupport(Format format) const;

    template <typename Implementation, typename Interface, typename... Args>
    [[nodiscard]] inline Result CreateImplementation(Interface*& entity, Args&&... args) {
        return nri::CreateImplementation(GetAllocationCallbacks(), entity, Allocate<Implementation>(GetAllocationCallbacks(), *this), std::forward<Args>(args)...);
    }

    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(LowLatencyInterface& table) const override;
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;

#if NRI_ENABLE_IMGUI_EXTENSION
    Result FillFunctionTable(ImguiInterface& table) const override;
#endif

    Result GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue);
    Result WaitIdle();
    void WriteBuffer(WGPUBuffer buffer, uint64_t offset, const void* data, size_t size);
    Result UploadHostMemoryToTexture(const UploadHostMemoryToTextureDesc* copyDescs, uint32_t copyDescNum);
    Result ReadbackTextureToHostMemory(const ReadbackTextureToHostMemoryDesc* copyDescs, uint32_t copyDescNum);

private:
    friend struct QueueWGPU;

    HostCopyLayoutWGPU GetHostCopyLayout(const TextureWGPU& texture, const TextureRegionDesc& region, uint64_t& offset, bool alignForBufferCopy) const;
    Result AcquireHostCopyContext(HostCopyContextWGPU*& context);
    void ReleaseHostCopyContext(HostCopyContextWGPU& context);
    Result EnsureReadbackBuffer(HostCopyContextWGPU& context, uint64_t size);
    Result CreateInstanceAndDevice(const DeviceCreationDesc& deviceCreationDesc);
    void FillDesc(const AdapterDesc& adapterDesc);

private:
    std::array<Vector<QueueWGPU*>, (size_t)QueueType::MAX_NUM> m_QueueFamilies;
    Vector<HostCopyContextWGPU*> m_HostCopyContexts;
    CoreInterface m_iCore = {};
    DeviceDesc m_Desc = {};
    WGPUInstance m_Instance = nullptr;
    WGPUAdapter m_Adapter = nullptr;
    WGPUDevice m_Device = nullptr;
    WGPUQueue m_Queue = nullptr;
    VKBindingOffsets m_BindingOffsets = {};
    bool m_IsTimestampQueryInsidePassesSupported = false;
    bool m_IsSubgroupsSupported = false;
    Lock m_QueueLock;
    Lock m_HostCopyContextLock;
};

} // namespace nri
