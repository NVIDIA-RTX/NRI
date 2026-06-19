// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct FenceWGPU final : public DebugNameBase {
    inline FenceWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    Result Create(uint64_t initialValue);
    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    uint64_t GetValue() const;
    void Wait(uint64_t value);
    void Signal(uint64_t value, WGPUSubmissionIndex submissionIndex);

private:
    DeviceWGPU& m_Device;
    mutable WGPUSubmissionIndex m_SubmissionIndex = 0;
    uint64_t m_SubmittedValue = 0;
    mutable uint64_t m_CompletedValue = 0;
    bool m_IsSwapChainSemaphore = false;
};

} // namespace nri
