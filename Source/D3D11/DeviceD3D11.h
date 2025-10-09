// © 2021 NVIDIA Corporation

#pragma once

struct ID3D11Device5;
typedef ID3D11Device5 ID3D11DeviceBest;

namespace nri {

struct QueueD3D11;

struct DeviceD3D11 final : public DeviceBase {
    DeviceD3D11(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks);
    ~DeviceD3D11();

    inline ID3D11DeviceBest* GetNativeObject() const {
        return m_Device.GetInterface();
    }

    inline ID3D11DeviceBest* operator->() const {
        return m_Device;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
    }

    inline IDXGIAdapter* GetAdapter() const {
        return m_Adapter;
    }

    inline ID3D11DeviceContextBest* GetImmediateContext() {
        return m_ImmediateContext;
    }

    inline ID3D11Buffer* GetZeroBuffer() const {
        return m_ZeroBuffer;
    }

    inline uint8_t GetImmediateContextVersion() {
        return m_ImmediateContextVersion;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_iCore;
    }

    inline bool IsDeferredContextEmulated() const {
        return m_IsDeferredContextEmulated;
    }

    inline void EnterCriticalSection() {
        if (m_Multithread)
            m_Multithread->Enter();
        else
            ::EnterCriticalSection(&m_CriticalSection);
    }

    inline void LeaveCriticalSection() {
        if (m_Multithread)
            m_Multithread->Leave();
        else
            ::LeaveCriticalSection(&m_CriticalSection);
    }

    inline bool HasNvExt() const {
#if NRI_ENABLE_NVAPI
        return m_NvExt.available;
#else
        return false;
#endif
    }

    inline bool HasAmdExt() const {
#if NRI_ENABLE_AMDAGS
        return m_AmdExt.context != nullptr;
#else
        return false;
#endif
    }

#if NRI_ENABLE_AMDAGS
    inline const AmdExtD3D11& GetAmdExt() const {
        return m_AmdExt;
    }
#endif

    template <typename Implementation, typename Interface, typename... Args>
    inline Result CreateImplementation(Interface*& entity, const Args&... args) {
        Implementation* impl = Allocate<Implementation>(GetAllocationCallbacks(), *this);
        Result result = impl->Create(args...);

        if (result != Result::SUCCESS) {
            Destroy(GetAllocationCallbacks(), impl);
            entity = nullptr;
        } else
            entity = (Interface*)impl;

        return result;
    }

    Result Create(const DeviceCreationDesc& desc, const DeviceCreationD3D11Desc& descD3D11);
    void GetMemoryDesc(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Device, name);
        SET_D3D_DEBUG_OBJECT_NAME(m_ImmediateContext, name);
    }

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    inline const DeviceDesc& GetDesc() const override {
        return m_Desc;
    }

    void Destruct() override;
    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(LowLatencyInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperD3D11Interface& table) const override;

#if NRI_ENABLE_IMGUI_EXTENSION
    Result FillFunctionTable(ImguiInterface& table) const override;
#endif

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue);
    Result WaitIdle();
    Result BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum);
    Result BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    void FillDesc();
    void InitializeNvExt(bool disableNVAPIInitialization, bool isImported);
    void InitializeAmdExt(AGSContext* agsContext, bool isImported);

private:
    // Order of destructors is important
#if NRI_ENABLE_NVAPI
    NvExt m_NvExt = {};
#endif

#if NRI_ENABLE_AMDAGS
    AmdExtD3D11 m_AmdExt = {};
#endif

    ComPtr<ID3D11DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    ComPtr<ID3D11DeviceContextBest> m_ImmediateContext;
    ComPtr<ID3D11Multithread> m_Multithread;
    ComPtr<ID3D11Buffer> m_ZeroBuffer;
    std::array<Vector<QueueD3D11*>, (size_t)QueueType::MAX_NUM> m_QueueFamilies;
    CRITICAL_SECTION m_CriticalSection = {}; // TODO: Lock?
    CoreInterface m_iCore = {};
    DeviceDesc m_Desc = {};
    uint8_t m_Version = 0;
    uint8_t m_ImmediateContextVersion = 0;
    bool m_IsWrapped = false;
    bool m_IsDeferredContextEmulated = false;
};

} // namespace nri
