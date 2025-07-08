// © 2021 NVIDIA Corporation

Result QueueD3D12::Create(QueueType queueType, float priority) {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Priority = priority > 0.5f ? D3D12_COMMAND_QUEUE_PRIORITY_HIGH : D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // TODO: values in between? check D3D12_FEATURE_COMMAND_QUEUE_PRIORITY support?
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = NODE_MASK;
    queueDesc.Type = GetCommandListType(queueType);

    HRESULT hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_Queue));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateQueue");

    m_CommandListType = queueDesc.Type;

    return Result::SUCCESS;
}

Result QueueD3D12::Create(ID3D12CommandQueue* queue) {
    if (!queue)
        return Result::INVALID_ARGUMENT;

    const D3D12_COMMAND_QUEUE_DESC& queueDesc = queue->GetDesc();

    m_Queue = queue;
    m_CommandListType = queueDesc.Type;

    return Result::SUCCESS;
}

NRI_INLINE void QueueD3D12::BeginAnnotation(const char* name, uint32_t bgra) {
    if (m_Device.HasPix())
        m_Device.GetPix().BeginEventOnQueue(m_Queue, bgra, name);
    else
        PIXBeginEvent(m_Queue, bgra, name);
}

NRI_INLINE void QueueD3D12::EndAnnotation() {
    if (m_Device.HasPix())
        m_Device.GetPix().EndEventOnQueue(m_Queue);
    else
        PIXEndEvent(m_Queue);
}

NRI_INLINE void QueueD3D12::Annotation(const char* name, uint32_t bgra) {
    if (m_Device.HasPix())
        m_Device.GetPix().SetMarkerOnQueue(m_Queue, bgra, name);
    else
        PIXSetMarker(m_Queue, bgra, name);
}

NRI_INLINE Result QueueD3D12::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.waitFences[i];
        FenceD3D12* fence = (FenceD3D12*)fenceSubmitDesc.fence;
        fence->QueueWait(*this, fenceSubmitDesc.value);
    }

    if (queueSubmitDesc.commandBufferNum) {
        Scratch<ID3D12CommandList*> commandLists = AllocateScratch(m_Device, ID3D12CommandList*, queueSubmitDesc.commandBufferNum);
        for (uint32_t j = 0; j < queueSubmitDesc.commandBufferNum; j++)
            commandLists[j] = *(CommandBufferD3D12*)queueSubmitDesc.commandBuffers[j];

        m_Queue->ExecuteCommandLists(queueSubmitDesc.commandBufferNum, commandLists);
    }

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.signalFences[i];
        FenceD3D12* fence = (FenceD3D12*)fenceSubmitDesc.fence;
        fence->QueueSignal(*this, fenceSubmitDesc.value);
    }

    // Is device lost?
    HRESULT hr = m_Device->GetDeviceRemovedReason() == S_OK ? S_OK : DXGI_ERROR_DEVICE_REMOVED;
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "Submit");

    return Result::SUCCESS;
}

NRI_INLINE Result QueueD3D12::WaitIdle() {
    FenceD3D12* fence = nullptr;
    Result result = m_Device.CreateImplementation<FenceD3D12>(fence, 0);
    if (result == Result::SUCCESS) {
        fence->QueueSignal(*this, 1);
        fence->Wait(1);

        Destroy(fence);
    }

    return result;
}
