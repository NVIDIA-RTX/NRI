// © 2021 NVIDIA Corporation

Result nri::CreateCommandBuffer(DeviceD3D11& device, ID3D11DeviceContext* precreatedContext, CommandBuffer*& commandBuffer) {
    bool isImmediate = false;
    if (precreatedContext)
        isImmediate = precreatedContext->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE;
    else
        isImmediate = device.IsDeferredContextEmulated();

    if (isImmediate)
        return device.CreateImplementation<CommandBufferEmuD3D11>(commandBuffer, precreatedContext);

    return device.CreateImplementation<CommandBufferD3D11>(commandBuffer, precreatedContext);
}

NRI_INLINE Result CommandAllocatorD3D11::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    return nri::CreateCommandBuffer(m_Device, nullptr, commandBuffer);
}
