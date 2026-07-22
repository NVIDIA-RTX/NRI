// © 2026 NVIDIA Corporation

Result FenceWGPU::Create(uint64_t initialValue) {
    m_IsSwapChainSemaphore = initialValue == SWAPCHAIN_SEMAPHORE;
    m_SubmittedValue = m_IsSwapChainSemaphore ? 0 : initialValue;
    m_CompletedValue = m_SubmittedValue;

    return Result::SUCCESS;
}

uint64_t FenceWGPU::GetValue() const {
#if defined(__EMSCRIPTEN__)
    uint32_t completedNum = 0;
    for (const FenceSubmissionWGPU& submission : m_Submissions) {
        if (!WaitForFuture(m_Device.GetInstance(), submission.future, 0))
            break;

        m_CompletedValue = std::max(m_CompletedValue, submission.value);
        completedNum++;
    }
#else
    // TODO: Fences are emulated with WGPUSubmissionIndex polling, not native timeline fence objects.
    uint32_t completedNum = 0;
    for (const FenceSubmissionWGPU& submission : m_Submissions) {
        WGPUSubmissionIndex index = submission.index;
        if (wgpuDevicePoll(m_Device, WGPU_FALSE, &index) != WGPU_TRUE)
            break;

        m_CompletedValue = std::max(m_CompletedValue, submission.value);
        completedNum++;
    }
#endif

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
#if defined(__EMSCRIPTEN__)
        WaitForFuture(m_Device.GetInstance(), submission.future);
#else
        WGPUSubmissionIndex index = submission.index;
        wgpuDevicePoll(m_Device, WGPU_TRUE, &index);
#endif
        m_CompletedValue = std::max(m_CompletedValue, submission.value);
        completedNum++;

        if (m_CompletedValue >= value)
            break;
    }

    if (completedNum) {
        m_Submissions.erase(m_Submissions.begin(), m_Submissions.begin() + completedNum);
    }
}

#if defined(__EMSCRIPTEN__)
void FenceWGPU::Signal(uint64_t value) {
    if (m_IsSwapChainSemaphore)
        return;

    WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPUQueueWorkDoneStatus, WGPUStringView, void*, void*) {};

    m_SubmittedValue = std::max(m_SubmittedValue, value);
    m_Submissions.push_back({value, wgpuQueueOnSubmittedWorkDone(m_Device.GetQueue(), callbackInfo)});
}
#else
void FenceWGPU::Signal(uint64_t value, WGPUSubmissionIndex submissionIndex) {
    if (m_IsSwapChainSemaphore)
        return;

    m_SubmittedValue = std::max(m_SubmittedValue, value);
    if (submissionIndex)
        m_Submissions.push_back({value, submissionIndex});
    else
        m_CompletedValue = std::max(m_CompletedValue, value);
}
#endif
