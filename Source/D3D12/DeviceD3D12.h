// © 2021 NVIDIA Corporation

#pragma once

struct IDXGIAdapter;
struct ID3D12DescriptorHeap;
struct ID3D12CommandSignature;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
struct ID3D12Device14;
typedef ID3D12Device14 ID3D12DeviceBest;
#else
struct ID3D12Device5;
typedef ID3D12Device5 ID3D12DeviceBest;
#endif

namespace nri {

struct QueueD3D12;

struct DeviceD3D12 final : public DeviceBase {
    DeviceD3D12(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks);
    ~DeviceD3D12();

    inline ID3D12DeviceBest* GetNativeObject() const {
        return m_Device;
    }

    inline ID3D12DeviceBest* operator->() const {
        return m_Device;
    }

    inline IDXGIAdapter* GetAdapter() const {
        return m_Adapter;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_iCore;
    }

    inline D3D12MA::Allocator* GetVma() const {
        return m_Vma;
    }

    inline ID3D12Resource* GetZeroBuffer() const {
        return m_ZeroBuffer;
    }

    inline bool IsMemoryZeroInitializationEnabled() const {
        return m_IsMemoryZeroInitializationEnabled;
    }

    inline bool HasPix() const {
        return m_Pix.library != nullptr;
    }

    inline const PixExt& GetPix() const {
        return m_Pix;
    }

    inline uint8_t GetTightAlignmentTier() const {
        return m_TightAlignmentTier;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
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

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationD3D12Desc);
    Result CreateDefaultDrawSignatures(ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation);
    Result GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, DescriptorHandle& descriptorHandle);
    void FreeDescriptorHandle(const DescriptorHandle& descriptorHandle);
    void GetResourceDesc(const BufferDesc& bufferDesc, D3D12_RESOURCE_DESC& desc) const;
    void GetResourceDesc(const TextureDesc& textureDesc, D3D12_RESOURCE_DESC& desc) const;
    void GetMemoryDesc(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc, MemoryDesc& memoryDesc) const;
    void GetAccelerationStructurePrebuildInfo(const AccelerationStructureDesc& accelerationStructureDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo) const;
    void GetMicromapPrebuildInfo(const MicromapDesc& micromapDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo) const;
    D3D12_HEAP_TYPE GetHeapType(MemoryLocation memoryLocation) const;
    DescriptorHandleCPU GetDescriptorHandleCPU(const DescriptorHandle& descriptorHandle);
    ID3D12CommandSignature* GetDrawCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawIndexedCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawMeshCommandSignature(uint32_t stride);
    ID3D12CommandSignature* GetDispatchRaysCommandSignature() const;
    ID3D12CommandSignature* GetDispatchCommandSignature() const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Device, name);
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
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperD3D12Interface& table) const override;

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
    Result BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum);
    Result BindMicromapMemory(const BindMicromapMemoryDesc* bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    HRESULT CreateVma();
    void FillDesc(bool disableD3D12EnhancedBarrier);
    void InitializeNvExt(bool disableNVAPIInitialization, bool isImported);
    void InitializeAmdExt(AGSContext* agsContext, bool isImported);
    void InitializePixExt();
    ComPtr<ID3D12CommandSignature> CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride, ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation = false);

private:
    // Order of destructors is important
    PixExt m_Pix = {};

#if NRI_ENABLE_NVAPI
    NvExt m_NvExt = {};
#endif

#if NRI_ENABLE_AMDAGS
    AmdExtD3D12 m_AmdExt = {};
#endif

    ComPtr<ID3D12DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    ComPtr<ID3D12CommandSignature> m_DispatchCommandSignature;
    ComPtr<ID3D12CommandSignature> m_DispatchRaysCommandSignature;
    ComPtr<D3D12MA::Allocator> m_Vma;
    ComPtr<ID3D12Resource> m_ZeroBuffer;
    Vector<DescriptorHeapDesc> m_DescriptorHeaps;                                          // m_DescriptorHeapLock
    Vector<Vector<DescriptorHandle>> m_FreeDescriptors;                                    // m_FreeDescriptorLocks
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawCommandSignatures;        // m_CommandSignatureLock
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawIndexedCommandSignatures; // m_CommandSignatureLock
    UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>> m_DrawMeshCommandSignatures;    // m_CommandSignatureLock
    std::array<Vector<QueueD3D12*>, (size_t)QueueType::MAX_NUM> m_QueueFamilies;
    CoreInterface m_iCore = {};
    DeviceDesc m_Desc = {};
    void* m_CallbackHandle = nullptr;
    DWORD m_CallbackCookie = 0;
    uint8_t m_TightAlignmentTier = 0;
    uint8_t m_Version = 0;
    bool m_IsWrapped = false;
    bool m_IsMemoryZeroInitializationEnabled = false;

    std::array<Lock, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_FreeDescriptorLocks;
    Lock m_DescriptorHeapLock;
    Lock m_CommandSignatureLock;
};

} // namespace nri
