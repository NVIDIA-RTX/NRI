// © 2021 NVIDIA Corporation

NRI_INLINE Result CommandAllocatorVal::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    CommandBuffer* commandBufferImpl;
    const Result result = GetCoreInterface().CreateCommandBuffer(*GetImpl(), commandBufferImpl);

    commandBuffer = nullptr;
    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(m_Device.GetAllocationCallbacks(), m_Device, commandBufferImpl, false);

    return result;
}

NRI_INLINE void CommandAllocatorVal::Reset() {
    GetCoreInterface().ResetCommandAllocator(*GetImpl());
}
