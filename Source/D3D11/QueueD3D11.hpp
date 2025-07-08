// © 2021 NVIDIA Corporation

NRI_INLINE Result QueueD3D11::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.waitFences[i];
        FenceD3D11* fence = (FenceD3D11*)fenceSubmitDesc.fence;
        fence->QueueWait(fenceSubmitDesc.value);
    }

    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++) {
        CommandBufferBase* commandBuffer = (CommandBufferBase*)queueSubmitDesc.commandBuffers[i];
        commandBuffer->Submit();
    }

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.signalFences[i];
        FenceD3D11* fence = (FenceD3D11*)fenceSubmitDesc.fence;
        fence->QueueSignal(fenceSubmitDesc.value);
    }

    return Result::SUCCESS;
}

NRI_INLINE Result QueueD3D11::WaitIdle() {
    FenceD3D11* fence = nullptr;
    Result result = m_Device.CreateImplementation<FenceD3D11>(fence, 0);
    if (result == Result::SUCCESS) {
        fence->QueueSignal(1);
        fence->Wait(1);

        Destroy(fence);
    }

    return result;
}
