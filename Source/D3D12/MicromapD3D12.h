// © 2025 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferD3D12;

struct MicromapD3D12 final : public DebugNameBase {
    inline MicromapD3D12(DeviceD3D12& device)
        : m_Usages(device.GetStdAllocator())
        , m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline MicromapBits GetFlags() const {
        return m_Flags;
    }

    inline const MicromapUsageDesc* GetUsages() const {
        return m_Usages.data();
    }

    inline uint32_t GetUsageNum() const {
        return (uint32_t)m_Usages.size();
    }

    ~MicromapD3D12();

    Result Create(const MicromapDesc& accelerationStructureDesc);
    Result Create(const AllocateMicromapDesc& accelerationStructureDesc);
    Result BindMemory(Memory* memory, uint64_t offset);
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline uint64_t GetBuildScratchBufferSize() const {
        return m_BuildScratchSize;
    }

    inline BufferD3D12* GetBuffer() const {
        return m_Buffer;
    }

private:
    DeviceD3D12& m_Device;
    BufferD3D12* m_Buffer = nullptr;
    Vector<MicromapUsageDesc> m_Usages;
    uint64_t m_BuildScratchSize = 0;
    MicromapBits m_Flags = MicromapBits::NONE;
};

} // namespace nri
