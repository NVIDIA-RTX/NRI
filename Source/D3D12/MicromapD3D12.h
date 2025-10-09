// © 2025 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferD3D12;
struct MemoryD3D12;

struct MicromapD3D12 final : public DebugNameBase {
    inline MicromapD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_Usages(device.GetStdAllocator()) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline MicromapBits GetFlags() const {
        return m_Flags;
    }

    inline const D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY* GetUsages() const {
        return m_Usages.data();
    }

    inline uint32_t GetUsageNum() const {
        return (uint32_t)m_Usages.size();
    }

    ~MicromapD3D12();

    Result Create(const MicromapDesc& micromapDesc);
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

    inline uint64_t GetBuildScratchBufferSize() const {
        return m_PrebuildInfo.ScratchDataSizeInBytes;
    }

    inline BufferD3D12* GetBuffer() const {
        return m_Buffer;
    }

    uint64_t GetHandle() const;
    operator ID3D12Resource*() const;

private:
    DeviceD3D12& m_Device;
    BufferD3D12* m_Buffer = nullptr;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_PrebuildInfo = {};
    Vector<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY> m_Usages;
    MicromapBits m_Flags = MicromapBits::NONE;
};

} // namespace nri
