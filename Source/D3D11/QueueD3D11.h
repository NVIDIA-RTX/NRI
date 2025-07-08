// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueD3D11 final : public DebugNameBase {
    inline QueueD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~QueueD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result WaitIdle();

private:
    DeviceD3D11& m_Device;
};

} // namespace nri
