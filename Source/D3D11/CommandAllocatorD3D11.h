// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandAllocatorD3D11 final : public DebugNameBase {
    inline CommandAllocatorD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~CommandAllocatorD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void Reset() {
    }

    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);

private:
    DeviceD3D11& m_Device;
};

Result CreateCommandBuffer(DeviceD3D11& deviceImpl, ID3D11DeviceContext* precreatedContext, CommandBuffer*& commandBuffer);

} // namespace nri
