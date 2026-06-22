// © 2026 NVIDIA Corporation

Result CommandAllocatorWGPU::Create(const Queue& queue) {
    // TODO: WebGPU command encoders own allocation internally, so the NRI allocator is only a bookkeeping object.
    MaybeUnused(queue);

    return Result::SUCCESS;
}

Result CommandAllocatorWGPU::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    return m_Device.CreateImplementation<CommandBufferWGPU>(commandBuffer, (CommandAllocator&)*this);
}

void CommandAllocatorWGPU::Reset() {
    // TODO: No native allocator reset exists in WebGPU; command resources are released by command-buffer lifetime/reset paths.
}
