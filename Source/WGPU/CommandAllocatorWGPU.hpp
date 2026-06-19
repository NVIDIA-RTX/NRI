// © 2026 NVIDIA Corporation

Result CommandAllocatorWGPU::Create(const Queue& queue) {
    MaybeUnused(queue);

    return Result::SUCCESS;
}

Result CommandAllocatorWGPU::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    return m_Device.CreateImplementation<CommandBufferWGPU>(commandBuffer, (CommandAllocator&)*this);
}

void CommandAllocatorWGPU::Reset() {
}
