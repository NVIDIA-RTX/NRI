// © 2026 NVIDIA Corporation

Result QueueWGPU::Create(QueueType queueType, uint32_t queueIndex) {
    m_Type = queueType;
    m_Index = queueIndex;

    return Result::SUCCESS;
}

void QueueWGPU::BeginAnnotation(const char* name, uint32_t bgra) {
    // TODO: WGPU exposes queue labels, but no queue debug groups/markers.
    MaybeUnused(name, bgra);
}

void QueueWGPU::EndAnnotation() {
}

void QueueWGPU::Annotation(const char* name, uint32_t bgra) {
    // TODO: WGPU exposes queue labels, but no queue debug groups/markers.
    MaybeUnused(name, bgra);
}

void QueueWGPU::GetCalibratedTimestamps(uint64_t& timestampGPU, uint64_t& timestampCPU) {
    // TODO: No calibrated CPU/GPU timestamp mapping is exposed through WGPU.
    timestampGPU = 0;
    timestampCPU = 0;
}

Result QueueWGPU::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    ExclusiveScope lock(m_Device.GetQueueLock());

    // WebGPU exposes one ordered native queue. All logical NRI queue submissions on this
    // device therefore satisfy their wait fences by submission order without a CPU wait.
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        const FenceSubmitDesc& fence = queueSubmitDesc.waitFences[i];
        if (!((FenceWGPU*)fence.fence)->IsSatisfiedBySubmissionOrder(fence.value))
            return Result::INVALID_ARGUMENT;
    }

    Scratch<WGPUCommandBuffer> commandBuffers = NRI_ALLOCATE_SCRATCH(m_Device, WGPUCommandBuffer, queueSubmitDesc.commandBufferNum);

    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
        commandBuffers[i] = ((CommandBufferWGPU*)queueSubmitDesc.commandBuffers[i])->GetCommandBuffer();

#if defined(__EMSCRIPTEN__)
    if (queueSubmitDesc.commandBufferNum)
        wgpuQueueSubmit(m_Device.GetQueue(), queueSubmitDesc.commandBufferNum, commandBuffers);

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++)
        ((FenceWGPU*)queueSubmitDesc.signalFences[i].fence)->Signal(queueSubmitDesc.signalFences[i].value);
#else
    WGPUSubmissionIndex submissionIndex = 0;
    if (queueSubmitDesc.commandBufferNum || queueSubmitDesc.signalFenceNum)
        submissionIndex = wgpuQueueSubmitForIndex(m_Device.GetQueue(), queueSubmitDesc.commandBufferNum, queueSubmitDesc.commandBufferNum ? commandBuffers : nullptr);

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++)
        ((FenceWGPU*)queueSubmitDesc.signalFences[i].fence)->Signal(queueSubmitDesc.signalFences[i].value, submissionIndex);
#endif

    return Result::SUCCESS;
}

Result QueueWGPU::WaitIdle() {
    return m_Device.WaitIdle();
}
