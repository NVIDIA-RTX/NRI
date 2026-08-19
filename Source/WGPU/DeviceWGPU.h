// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct HostCopyLayoutWGPU {
    uint64_t offset;
    uint32_t rowPitch;
    uint32_t slicePitch;
    uint32_t rowSize;
    uint32_t rowNum;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct HostCopyContextWGPU {
    WGPUBuffer readbackBuffer = nullptr;
    uint64_t readbackBufferSize = 0;
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
    inline Result CreateImplementation(Interface*& entity, const Args&... args) {
        Implementation* impl = Allocate<Implementation>(GetAllocationCallbacks(), *this);
        if (!impl) {
            entity = nullptr;
            return Result::OUT_OF_MEMORY;
        }

        Result result = impl->Create(args...);
        if (result != Result::SUCCESS) {
            Destroy(GetAllocationCallbacks(), impl);
            entity = nullptr;
        } else
            entity = (Interface*)impl;

        return result;
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
    Result CopyHostMemoryToTexture(const CopyHostMemoryToTextureDesc* copyDescs, uint32_t copyDescNum);
    Result CopyTextureToHostMemory(const CopyTextureToHostMemoryDesc* copyDescs, uint32_t copyDescNum);

private:
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
    Lock m_HostCopyContextLock;
};

} // namespace nri
