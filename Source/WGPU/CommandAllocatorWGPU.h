// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandAllocatorWGPU final : public DebugNameBase {
    inline CommandAllocatorWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    Result Create(const Queue& queue);
    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();

private:
    DeviceWGPU& m_Device;
};

} // namespace nri
