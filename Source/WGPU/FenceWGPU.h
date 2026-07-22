// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct FenceSubmissionWGPU {
    uint64_t value = 0;
#if defined(__EMSCRIPTEN__)
    WGPUFuture future = WGPU_FUTURE_INIT;
#else
    WGPUSubmissionIndex index = 0;
#endif
};

struct FenceWGPU final : public DebugNameBase {
    inline FenceWGPU(DeviceWGPU& device)
        : m_Device(device)
        , m_Submissions(device.GetStdAllocator()) {
    }

    Result Create(uint64_t initialValue);

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    uint64_t GetValue() const;
    void Wait(uint64_t value);
#if defined(__EMSCRIPTEN__)
    void Signal(uint64_t value);
#else
    void Signal(uint64_t value, WGPUSubmissionIndex submissionIndex);
#endif

private:
    DeviceWGPU& m_Device;
    mutable Vector<FenceSubmissionWGPU> m_Submissions;
    uint64_t m_SubmittedValue = 0;
    mutable uint64_t m_CompletedValue = 0;
    bool m_IsSwapChainSemaphore = false;
};

} // namespace nri
