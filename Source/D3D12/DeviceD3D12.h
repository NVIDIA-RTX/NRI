// © 2021 NVIDIA Corporation

#pragma once

struct IDXGIAdapter;
struct ID3D12DescriptorHeap;
struct ID3D12CommandSignature;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
struct ID3D12Device14;
typedef ID3D12Device14 ID3D12DeviceBest;
#else
struct ID3D12Device5;
typedef ID3D12Device5 ID3D12DeviceBest;
#endif

namespace nri {

struct QueueD3D12;

constexpr uint32_t DESCRIPTORS_BATCH_SIZE = 1024;

struct DeviceD3D12 final : public DeviceBase {
    DeviceD3D12(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks);
    ~DeviceD3D12();

    inline ID3D12DeviceBest* GetNativeObject() const {
        return m_Device;
    }

    inline ID3D12DeviceBest* operator->() const {
        return m_Device;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
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

    inline void FreeDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, const DescriptorHandle& descriptorHandle) {
        ExclusiveScope lock(m_FreeDescriptorLocks[type]);
        auto& freeDescriptors = m_FreeDescriptors[type];
        freeDescriptors.push_back(descriptorHandle);
    }

    inline bool HasPix() const {
        return m_Pix.library != nullptr;
    }

    inline const PixExt& GetPix() const {
        return m_Pix;
    }

#if NRI_ENABLE_D3D_EXTENSIONS
    inline bool HasNvExt() const {
        return m_NvExt.available;
    }

    inline bool HasAmdExt() const {
        return m_AmdExt.context != nullptr;
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

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationD3D12Desc);
    Result CreateDefaultDrawSignatures(ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation);
    Result CreateCpuOnlyVisibleDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
    Result GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, DescriptorHandle& descriptorHandle);
    DescriptorPointerCPU GetDescriptorPointerCPU(const DescriptorHandle& descriptorHandle);
    void GetMemoryDesc(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    void GetAccelerationStructurePrebuildInfo(const AccelerationStructureDesc& accelerationStructureDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo);
    ID3D12CommandSignature* GetDrawCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawIndexedCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawMeshCommandSignature(uint32_t stride);
    ID3D12CommandSignature* GetDispatchRaysCommandSignature() const;
    ID3D12CommandSignature* GetDispatchCommandSignature() const;
    Result CreateVma();

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
    Result FillFunctionTable(ResourceAllocatorInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperD3D12Interface& table) const override;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    void FillDesc();
    void InitializeNvExt(bool isNVAPILoadedInApp, bool isImported);
    void InitializeAmdExt(AGSContext* agsContext, bool isImported);
    void InitializePixExt();
    ComPtr<ID3D12CommandSignature> CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride, ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation = false);

private:
    // Order of destructors is important
    PixExt m_Pix = {};
#if NRI_ENABLE_D3D_EXTENSIONS
    NvExt m_NvExt = {};
    AmdExt m_AmdExt = {};
#endif
    ComPtr<ID3D12DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    ComPtr<ID3D12CommandSignature> m_DispatchCommandSignature;
    ComPtr<ID3D12CommandSignature> m_DispatchRaysCommandSignature;
    ComPtr<D3D12MA::Allocator> m_Vma;
    Vector<DescriptorHeapDesc> m_DescriptorHeaps;
    Vector<Vector<DescriptorHandle>> m_FreeDescriptors;
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawCommandSignatures;
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawIndexedCommandSignatures;
    UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>> m_DrawMeshCommandSignatures;
    std::array<std::vector<QueueD3D12*>, (size_t)QueueType::MAX_NUM> m_QueueFamilies = {}; // TODO: use Vector!
    CoreInterface m_iCore = {};
    DeviceDesc m_Desc = {};
    uint8_t m_Version = 0;
    bool m_IsWrapped = false;

    std::array<Lock, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_FreeDescriptorLocks;
    Lock m_DescriptorHeapLock;
};

} // namespace nri
