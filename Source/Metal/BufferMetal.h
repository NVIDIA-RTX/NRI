// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferMetal final : public DebugNameBase {
    inline BufferMetal(DeviceMetal& device) : m_Device(device) {
    }

    ~BufferMetal();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL::Buffer* GetNativeObject() const {
        return m_Buffer;
    }

    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }

    inline MTL::GPUAddress GetGpuAddress() const {
        return m_Buffer ? m_Buffer->gpuAddress() : 0;
    }

    Result Create(const BufferDesc& desc);
    Result Create(const BufferDesc& desc, MemoryLocation location);
    Result Create(const BufferMetalDesc& desc);
    Result Bind(MemoryMetal& memory, uint64_t offset);
    void GetMemoryDesc(MemoryLocation location, MemoryDesc& memoryDesc) const;
    void* Map(uint64_t offset, uint64_t size);
    void Unmap();
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    void Release();

    DeviceMetal& m_Device;
    MTL::Buffer* m_Buffer = nullptr;
    BufferDesc m_Desc = {};
    MemoryLocation m_Location = MemoryLocation::DEVICE;
    uint64_t m_MapOffset = 0;
    uint64_t m_MapSize = 0;
    bool m_IsResident = false;
};

} // namespace nri
