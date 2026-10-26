// © 2021 NVIDIA Corporation

NRI_INLINE Result CommandAllocatorVal::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    CommandBuffer* commandBufferImpl = nullptr;
    Result result = GetCoreInterfaceImpl().CreateCommandBuffer(*GetImpl(), commandBufferImpl);

    commandBuffer = nullptr;
    if (result == Result::SUCCESS)
        result = m_Device.WrapImplementation<CommandBufferVal>(commandBuffer, commandBufferImpl, GetCoreInterfaceImpl().DestroyCommandBuffer, m_QueueType, false);

    return result;
}

NRI_INLINE void CommandAllocatorVal::Reset() {
    GetCoreInterfaceImpl().ResetCommandAllocator(*GetImpl());
}
