// © 2021 NVIDIA Corporation

NRI_INLINE void QueueVal::BeginAnnotation(const char* name, uint32_t bgra) {
    GetCoreInterfaceImpl().QueueBeginAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE void QueueVal::EndAnnotation() {
    GetCoreInterfaceImpl().QueueEndAnnotation(*GetImpl());
}

NRI_INLINE void QueueVal::Annotation(const char* name, uint32_t bgra) {
    GetCoreInterfaceImpl().QueueAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE Result QueueVal::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    auto queueSubmitDescImpl = queueSubmitDesc;

    Scratch<FenceSubmitDesc> waitFences = AllocateScratch(m_Device, FenceSubmitDesc, queueSubmitDesc.waitFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        waitFences[i] = queueSubmitDesc.waitFences[i];
        waitFences[i].fence = NRI_GET_IMPL(Fence, waitFences[i].fence);
    }
    queueSubmitDescImpl.waitFences = waitFences;

    Scratch<CommandBuffer*> commandBuffers = AllocateScratch(m_Device, CommandBuffer*, queueSubmitDesc.commandBufferNum);
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
        commandBuffers[i] = NRI_GET_IMPL(CommandBuffer, queueSubmitDesc.commandBuffers[i]);
    queueSubmitDescImpl.commandBuffers = commandBuffers;

    Scratch<FenceSubmitDesc> signalFences = AllocateScratch(m_Device, FenceSubmitDesc, queueSubmitDesc.signalFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        signalFences[i] = queueSubmitDesc.signalFences[i];
        signalFences[i].fence = NRI_GET_IMPL(Fence, signalFences[i].fence);
    }
    queueSubmitDescImpl.signalFences = signalFences;

    queueSubmitDescImpl.swapChain = NRI_GET_IMPL(SwapChain, queueSubmitDesc.swapChain);

    return GetCoreInterfaceImpl().QueueSubmit(*GetImpl(), queueSubmitDescImpl);
}

NRI_INLINE Result QueueVal::WaitIdle() {
    return GetCoreInterfaceImpl().QueueWaitIdle(GetImpl());
}
