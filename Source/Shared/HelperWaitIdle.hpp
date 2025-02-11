// © 2021 NVIDIA Corporation

Result nri::WaitIdle(const CoreInterface& NRI, Device& device, Queue& queue) {
    Fence* fence = nullptr;
    Result result = NRI.CreateFence(device, 0, fence);
    if (result != Result::SUCCESS)
        return result;

    FenceSubmitDesc fenceSubmitDesc = {};
    fenceSubmitDesc.fence = fence;
    fenceSubmitDesc.value = 1;

    QueueSubmitDesc queueSubmitDesc = {};
    queueSubmitDesc.signalFences = &fenceSubmitDesc;
    queueSubmitDesc.signalFenceNum = 1;

    NRI.QueueSubmit(queue, queueSubmitDesc);
    NRI.Wait(*fence, 1);
    NRI.DestroyFence(*fence);

    return Result::SUCCESS;
}
