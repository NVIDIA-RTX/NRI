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

#if NRI_ENABLE_D3D_EXTENSIONS
    inline bool HasNvExt() const {
        return m_NvExt.available;
    }

    inline bool HasAmdExt() const {
        return m_AmdExt.context != nullptr;
    }

    inline const AmdExt& GetAmdExt() const {
        return m_AmdExt;
    }
#else
    inline bool HasNvExt() const {
        return false;
    }

    inline bool HasAmdExt() const {
        return false;
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
    Result FillFunctionTable(ResourceAllocatorInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperD3D11Interface& table) const override;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateCommandAllocator(const Queue& queue, CommandAllocator*& commandAllocator);
    Result GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    void FillDesc();
    void InitializeNvExt(bool isNVAPILoadedInApp, bool isImported);
    void InitializeAmdExt(AGSContext* agsContext, bool isImported);

private:
    // Order of destructors is important
#if NRI_ENABLE_D3D_EXTENSIONS
    NvExt m_NvExt = {};
    AmdExt m_AmdExt = {};
#endif
    ComPtr<ID3D11DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    ComPtr<ID3D11DeviceContextBest> m_ImmediateContext;
    ComPtr<ID3D11Multithread> m_Multithread;
    std::array<std::vector<QueueD3D11*>, (size_t)QueueType::MAX_NUM> m_QueueFamilies = {}; // TODO: use Vector!
    CRITICAL_SECTION m_CriticalSection = {};                                               // TODO: Lock?
    CoreInterface m_iCore = {};
    DeviceDesc m_Desc = {};
    uint8_t m_Version = 0;
    uint8_t m_ImmediateContextVersion = 0;
    bool m_IsWrapped = false;
    bool m_IsDeferredContextEmulated = false;
};

} // namespace nri
