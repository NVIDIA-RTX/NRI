// © 2026 NVIDIA Corporation

Result FenceWGPU::Create(uint64_t initialValue) {
    m_IsSwapChainSemaphore = initialValue == SWAPCHAIN_SEMAPHORE;
    m_SubmittedValue = m_IsSwapChainSemaphore ? 0 : initialValue;
    m_CompletedValue = m_SubmittedValue;

    return Result::SUCCESS;
}

uint64_t FenceWGPU::GetValue() const {
    if (m_SubmissionIndex && wgpuDevicePoll(m_Device, WGPU_FALSE, &m_SubmissionIndex) == WGPU_TRUE) {
        m_CompletedValue = m_SubmittedValue;
        m_SubmissionIndex = 0;
    }

    return m_CompletedValue;
}

void FenceWGPU::Wait(uint64_t value) {
    if (m_IsSwapChainSemaphore || value > m_SubmittedValue)
        return;

    if (m_SubmissionIndex) {
        wgpuDevicePoll(m_Device, WGPU_TRUE, &m_SubmissionIndex);
        m_CompletedValue = m_SubmittedValue;
        m_SubmissionIndex = 0;
    }
}

void FenceWGPU::Signal(uint64_t value, WGPUSubmissionIndex submissionIndex) {
    if (m_IsSwapChainSemaphore)
        return;

    m_SubmittedValue = value;
    if (submissionIndex)
        m_SubmissionIndex = submissionIndex;
    else
        m_CompletedValue = value;
}
