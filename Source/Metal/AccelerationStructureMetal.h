// © 2026 NVIDIA Corporation

#pragma once

#include <Metal/MTL4AccelerationStructure.hpp>
#include "BufferMetal.h"

namespace nri {

// "IRRaytracingAccelerationStructureGPUHeader" (64 bytes), followed by per-instance hit group contributions
constexpr uint64_t TOP_LEVEL_HEADER_SIZE = 64;

struct AccelerationStructureMetal final : public DebugNameBase {
    inline AccelerationStructureMetal(DeviceMetal& device) : m_Device(device), m_BarrierBuffer(device) {
    }

    ~AccelerationStructureMetal();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL::AccelerationStructure* GetNativeObject() const {
        return m_AccelerationStructure;
    }

    inline uint64_t GetBuildScratchBufferSize() const {
        return m_Sizes.buildScratchBufferSize;
    }

    inline uint64_t GetUpdateScratchBufferSize() const {
        return m_Sizes.refitScratchBufferSize;
    }

    inline uint64_t GetSize() const {
        return m_Size;
    }

    inline uint32_t GetInstanceNum() const {
        return m_InstanceNum;
    }

    inline Buffer* GetBuffer() const {
        return (Buffer*)&m_BarrierBuffer;
    }

    inline uint64_t GetHandle() const {
        return m_AccelerationStructure ? m_AccelerationStructure->gpuResourceID()._impl : 0;
    }

    // TLAS only: the header is written on the GPU by TLAS builds and copies
    inline MTL::Buffer* GetShaderBindingHeaderBuffer() const {
        return m_ShaderBindingHeader;
    }

    inline MTL::GPUAddress GetShaderBindingHeaderAddress() const {
        return m_ShaderBindingHeader ? m_ShaderBindingHeader->gpuAddress() : 0;
    }

    inline MTL::GPUAddress GetInstanceContributionAddress() const {
        return GetShaderBindingHeaderAddress() + TOP_LEVEL_HEADER_SIZE;
    }

    static void GetMemoryDesc(DeviceMetal& device, const AccelerationStructureDesc& desc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);

    Result Create(const AccelerationStructureDesc& desc);
    Result Create(const AccelerationStructureDesc& desc, MemoryLocation location); // committed
    Result Bind(MemoryMetal& memory, uint64_t offset);
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

    // The returned descriptor is owned by the caller and must be released after command encoding
    MTL4::AccelerationStructureDescriptor* CreateBuildDescriptor(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum, MTL::GPUAddress convertedInstanceAddress = 0, uint32_t instanceNum = 0) const;

private:
    Result Allocate();
    void Release();

    DeviceMetal& m_Device;
    BufferMetal m_BarrierBuffer; // NRI exposes this only as a barrier identity, Metal AS storage is not an "MTLBuffer"
    MTL::AccelerationStructure* m_AccelerationStructure = nullptr;
    MTL::Buffer* m_ShaderBindingHeader = nullptr;
    MTL::AccelerationStructureSizes m_Sizes = {};
    MemoryDesc m_MemoryDesc = {};
    uint64_t m_Size = 0;
    uint64_t m_HeaderOffset = 0;
    uint32_t m_InstanceNum = 0;
    AccelerationStructureBits m_Flags = AccelerationStructureBits::NONE;
    AccelerationStructureType m_Type = AccelerationStructureType::BOTTOM_LEVEL;
    bool m_IsCommitted = false;
};

} // namespace nri
