// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferD3D12;
struct MemoryD3D12;

struct AccelerationStructureD3D12 final : public DebugNameBase {
    inline AccelerationStructureD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline AccelerationStructureBits GetFlags() const {
        return m_Flags;
    }

    ~AccelerationStructureD3D12();

    Result Create(const AccelerationStructureDesc& accelerationStructureDesc);
    Result Create(const AccelerationStructureD3D12Desc& accelerationStructureD3D12Desc);
    Result Allocate(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(const MemoryD3D12& memory, uint64_t offset);
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline uint64_t GetUpdateScratchBufferSize() const {
        return m_PrebuildInfo.UpdateScratchDataSizeInBytes;
    }

    inline uint64_t GetBuildScratchBufferSize() const {
        return m_PrebuildInfo.ScratchDataSizeInBytes;
    }

    inline BufferD3D12* GetBuffer() const {
        return m_Buffer;
    }

    Result CreateDescriptor(Descriptor*& descriptor) const;
    uint64_t GetHandle() const;
    operator ID3D12Resource*() const;

private:
    DeviceD3D12& m_Device;
    BufferD3D12* m_Buffer = nullptr;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_PrebuildInfo = {};
    AccelerationStructureBits m_Flags = AccelerationStructureBits::NONE;
};

} // namespace nri
