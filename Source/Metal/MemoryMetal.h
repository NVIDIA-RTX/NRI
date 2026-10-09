// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryMetal final : public DebugNameBase {
    inline MemoryMetal(DeviceMetal& device) : m_Device(device) {
    }

    ~MemoryMetal();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL::Heap* GetNativeObject() const {
        return m_Heap;
    }

    inline MemoryLocation GetLocation() const {
        return m_Location;
    }

    Result Create(const AllocateMemoryDesc& desc);
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    DeviceMetal& m_Device;
    MTL::Heap* m_Heap = nullptr;
    MemoryLocation m_Location = MemoryLocation::DEVICE;
};

} // namespace nri
