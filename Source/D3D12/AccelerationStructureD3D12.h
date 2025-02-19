// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferD3D12;

struct AccelerationStructureD3D12 final : public DebugNameBase {
    inline AccelerationStructureD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline BufferD3D12* GetBuffer() const {
        return m_Buffer;
    }

    inline uint64_t GetUpdateScratchBufferSize() const {
        return m_PrebuildInfo.UpdateScratchDataSizeInBytes;
    }

    inline uint64_t GetBuildScratchBufferSize() const {
        return m_PrebuildInfo.ScratchDataSizeInBytes;
    }

    ~AccelerationStructureD3D12();

    Result Create(const AccelerationStructureDesc& accelerationStructureDesc);
    Result Create(const AccelerationStructureD3D12Desc& accelerationStructureDesc);
    Result Create(const AllocateAccelerationStructureDesc& accelerationStructureDesc);
    Result BindMemory(Memory* memory, uint64_t offset);
    Result CreateDescriptor(Descriptor*& descriptor) const;
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    uint64_t GetHandle() const;
    operator ID3D12Resource*() const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    DeviceD3D12& m_Device;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_PrebuildInfo = {};
    BufferD3D12* m_Buffer = nullptr;
};

} // namespace nri
