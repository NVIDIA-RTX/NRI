// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

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

private:
    Result CreateInstanceAndDevice(const DeviceCreationDesc& deviceCreationDesc);
    void FillDesc(const AdapterDesc& adapterDesc);

private:
    std::array<Vector<QueueWGPU*>, (size_t)QueueType::MAX_NUM> m_QueueFamilies;
    CoreInterface m_iCore = {};
    DeviceDesc m_Desc = {};
    WGPUInstance m_Instance = nullptr;
    WGPUAdapter m_Adapter = nullptr;
    WGPUDevice m_Device = nullptr;
    WGPUQueue m_Queue = nullptr;
    VKBindingOffsets m_BindingOffsets = {};
    bool m_IsTimestampQueryInsidePassesSupported = false;
};

} // namespace nri
