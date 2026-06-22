// © 2026 NVIDIA Corporation

Result FenceWGPU::Create(uint64_t initialValue) {
    m_IsSwapChainSemaphore = initialValue == SWAPCHAIN_SEMAPHORE;
    m_SubmittedValue = m_IsSwapChainSemaphore ? 0 : initialValue;
    m_CompletedValue = m_SubmittedValue;

    return Result::SUCCESS;
}

uint64_t FenceWGPU::GetValue() const {
    // TODO: Fences are emulated with WGPUSubmissionIndex polling, not native timeline fence objects.
    uint32_t completedNum = 0;
    for (const FenceSubmissionWGPU& submission : m_Submissions) {
        WGPUSubmissionIndex index = submission.index;
        if (wgpuDevicePoll(m_Device, WGPU_FALSE, &index) != WGPU_TRUE)
            break;

        m_CompletedValue = std::max(m_CompletedValue, submission.value);
        completedNum++;
    }

    if (completedNum) {
        m_Submissions.erase(m_Submissions.begin(), m_Submissions.begin() + completedNum);
    }

    return m_CompletedValue;
}

void FenceWGPU::Wait(uint64_t value) {
    if (m_IsSwapChainSemaphore || value <= GetValue() || value > m_SubmittedValue)
        return;

    uint32_t completedNum = 0;
    for (const FenceSubmissionWGPU& submission : m_Submissions) {
        WGPUSubmissionIndex index = submission.index;
        wgpuDevicePoll(m_Device, WGPU_TRUE, &index);
        m_CompletedValue = std::max(m_CompletedValue, submission.value);
        completedNum++;

        if (m_CompletedValue >= value)
            break;
    }

    if (completedNum) {
        m_Submissions.erase(m_Submissions.begin(), m_Submissions.begin() + completedNum);
    }
}

void FenceWGPU::Signal(uint64_t value, WGPUSubmissionIndex submissionIndex) {
    if (m_IsSwapChainSemaphore)
        return;

    m_SubmittedValue = std::max(m_SubmittedValue, value);
    if (submissionIndex)
        m_Submissions.push_back({value, submissionIndex});
    else
        m_CompletedValue = std::max(m_CompletedValue, value);
}
