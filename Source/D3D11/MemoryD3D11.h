// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

enum class MemoryResidencyPriority {
    DEFAULT,
    MINIMUM,
    LOW,
    NORMAL,
    HIGH,
    MAXIMUM
};

struct MemoryD3D11 final : public DebugNameBase {
    inline MemoryD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~MemoryD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline MemoryLocation GetLocation() const {
        return m_Location;
    }

    inline float GetPriority() const {
        return m_Priority;
    }

    inline Result Create(const AllocateMemoryDesc& allocateMemoryDesc) {
        m_Location = (MemoryLocation)allocateMemoryDesc.type;
        m_Priority = allocateMemoryDesc.priority;

        return Result::SUCCESS;
    }

private:
    DeviceD3D11& m_Device;
    MemoryLocation m_Location = MemoryLocation::DEVICE;
    float m_Priority = 0.0f;
};

} // namespace nri
