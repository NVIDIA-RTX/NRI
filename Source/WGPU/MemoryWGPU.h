// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryWGPU final : public DebugNameBase {
    inline MemoryWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    inline MemoryLocation GetMemoryLocation() const {
        return (MemoryLocation)m_Desc.type;
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    Result Create(const AllocateMemoryDesc& allocateMemoryDesc);

private:
    DeviceWGPU& m_Device;
    AllocateMemoryDesc m_Desc = {};
};

} // namespace nri
