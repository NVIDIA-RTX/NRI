// © 2026 NVIDIA Corporation

#pragma once

#include "InternalShadersMetal.h"

namespace nri {

// Metal command queues are untyped, so each supported "QueueType" can be backed by several native queues
constexpr uint32_t QUEUE_TYPE_NUM_METAL = 3;
constexpr uint32_t QUEUE_NUM_PER_TYPE_METAL = 4;
constexpr uint32_t DRAW_INDIRECT_MAX_NUM_METAL = 1 << 16;

struct DeviceMetal final : public DeviceBase {
    DeviceMetal(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks);
    ~DeviceMetal();

    inline MTL::Device* GetNativeObject() const {
        return m_Device;
    }

    inline MTL4::Compiler* GetCompiler() const {
        return m_Compiler;
    }

    inline const DeviceDesc& GetDesc() const override {
        return m_Desc;
    }

    Result Create(const DeviceCreationDesc& desc, const DeviceCreationMetalDesc& metalDesc);
    void Destruct() override;
    void AddResidency(MTL::Allocation* allocation);
    void RemoveResidency(MTL::Allocation* allocation);
    void CommitResidency(); // no-op if the residency set hasn't changed
    void AddQueueResidencySet(MTL::ResidencySet* residencySet);
    void RemoveQueueResidencySet(MTL::ResidencySet* residencySet);
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    MTL::GPUAddress GetTessellatorTables();
#endif

    inline InternalShadersMetal& GetInternalShaders() {
        return m_InternalShaders;
    }

    // Static data: "uint16_t" draw uniforms ("kIRArgumentBufferUniformsBindPoint") for index types 0-2 (not indexed, 16-bit, 32-bit)
    // at a 16 bytes stride, the first 16 bytes are zeros
    inline MTL::GPUAddress GetConstantsAddress(uint32_t index = 0) const {
        return m_ConstantsAddress + index * 16;
    }

    MTL::ResidencySet* GetResidencySet() const {
        return m_Residency;
    }

    Result GetQueue(QueueType type, uint32_t index, Queue*& queue);
    Result CreatePlacedBuffer(Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreatePlacedTexture(Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture);
    Result CreatePlacedAccelerationStructure(Memory* memory, uint64_t offset, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result WaitIdle();
    Result QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const;

    const CoreInterface& GetCoreInterface() const {
        return m_Core;
    }

    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(DescriptorHeapInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperMetalInterface& table) const override;
#if NRI_ENABLE_IMGUI_EXTENSION
    Result FillFunctionTable(ImguiInterface& table) const override;
#endif

    template <typename Implementation, typename Interface, typename... Args>
    Result CreateImplementation(Interface*& entity, const Args&... args) {
        auto* impl = Allocate<Implementation>(GetAllocationCallbacks(), *this);
        entity = nullptr;

        if (!impl)
            return Result::OUT_OF_MEMORY;

        Result result = impl->Create(args...);

        if (result != Result::SUCCESS)
            Destroy(GetAllocationCallbacks(), impl);
        else
            entity = (Interface*)impl;

        return result;
    }

private:
    void FillDesc(const AdapterDesc& adapterDesc);

    DeviceDesc m_Desc = {};
    MTL::Device* m_Device = nullptr;
    MTL::ResidencySet* m_Residency = nullptr;
    MTL4::Compiler* m_Compiler = nullptr;
    QueueMetal* m_Queues[QUEUE_TYPE_NUM_METAL][QUEUE_NUM_PER_TYPE_METAL] = {};
    MTL::Buffer* m_Constants = nullptr;
    MTL::GPUAddress m_ConstantsAddress = 0;
    std::mutex m_ResidencyLock;
    UnorderedMap<MTL::Allocation*, uint32_t> m_ResidencyReferences;
    std::atomic_bool m_IsResidencyDirty = false; // written under "m_ResidencyLock"
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    std::mutex m_TessellatorTablesLock;
    MTL::Buffer* m_TessellatorTables = nullptr;
#endif
    CoreInterface m_Core = {};
    InternalShadersMetal m_InternalShaders;
};

} // namespace nri
