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
    // TODO: Wait fences are CPU waits because WebGPU exposes a single queue without native inter-queue semaphore waits.
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++)
        ((FenceWGPU*)queueSubmitDesc.waitFences[i].fence)->Wait(queueSubmitDesc.waitFences[i].value);

    Scratch<WGPUCommandBuffer> commandBuffers = NRI_ALLOCATE_SCRATCH(m_Device, WGPUCommandBuffer, queueSubmitDesc.commandBufferNum);

    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
        commandBuffers[i] = ((CommandBufferWGPU*)queueSubmitDesc.commandBuffers[i])->GetCommandBuffer();

    WGPUSubmissionIndex submissionIndex = m_LastSubmissionIndex;
    if (queueSubmitDesc.commandBufferNum) {
        submissionIndex = wgpuQueueSubmitForIndex(m_Device.GetQueue(), queueSubmitDesc.commandBufferNum, commandBuffers);
        m_LastSubmissionIndex = submissionIndex;
    }

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++)
        ((FenceWGPU*)queueSubmitDesc.signalFences[i].fence)->Signal(queueSubmitDesc.signalFences[i].value, submissionIndex);

    return Result::SUCCESS;
}

Result QueueWGPU::WaitIdle() {
    return m_Device.WaitIdle();
}
