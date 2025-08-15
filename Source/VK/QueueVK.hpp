// © 2021 NVIDIA Corporation

Result QueueVK::Create(QueueType type, uint32_t familyIndex, VkQueue handle) {
    m_Type = type;
    m_FamilyIndex = familyIndex;
    m_Handle = handle;

    return Result::SUCCESS;
}

NRI_INLINE void QueueVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_QUEUE, (uint64_t)m_Handle, name);
}

NRI_INLINE void QueueVK::BeginAnnotation(const char* name, uint32_t bgra) {
    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = name;
    info.color[0] = ((bgra >> 16) & 0xFF) / 255.0f;
    info.color[1] = ((bgra >> 8) & 0xFF) / 255.0f;
    info.color[2] = ((bgra >> 0) & 0xFF) / 255.0f;
    info.color[3] = 1.0f; // PIX sets alpha to 1

    const auto& vk = m_Device.GetDispatchTable();
    if (vk.QueueBeginDebugUtilsLabelEXT)
        vk.QueueBeginDebugUtilsLabelEXT(m_Handle, &info);
}

NRI_INLINE void QueueVK::EndAnnotation() {
    const auto& vk = m_Device.GetDispatchTable();
    if (vk.QueueEndDebugUtilsLabelEXT)
        vk.QueueEndDebugUtilsLabelEXT(m_Handle);
}

NRI_INLINE void QueueVK::Annotation(const char* name, uint32_t bgra) {
    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = name;
    info.color[0] = ((bgra >> 16) & 0xFF) / 255.0f;
    info.color[1] = ((bgra >> 8) & 0xFF) / 255.0f;
    info.color[2] = ((bgra >> 0) & 0xFF) / 255.0f;
    info.color[3] = 1.0f; // PIX sets alpha to 1

    const auto& vk = m_Device.GetDispatchTable();
    if (vk.QueueInsertDebugUtilsLabelEXT)
        vk.QueueInsertDebugUtilsLabelEXT(m_Handle, &info);
}

NRI_INLINE Result QueueVK::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    ExclusiveScope lock(m_Lock);

    Scratch<VkSemaphoreSubmitInfo> waitSemaphores = AllocateScratch(m_Device, VkSemaphoreSubmitInfo, queueSubmitDesc.waitFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        waitSemaphores[i] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitSemaphores[i].semaphore = *(FenceVK*)queueSubmitDesc.waitFences[i].fence;
        waitSemaphores[i].value = queueSubmitDesc.waitFences[i].value;
        waitSemaphores[i].stageMask = GetPipelineStageFlags(queueSubmitDesc.waitFences[i].stages);
    }

    Scratch<VkCommandBufferSubmitInfo> commandBuffers = AllocateScratch(m_Device, VkCommandBufferSubmitInfo, queueSubmitDesc.commandBufferNum);
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++) {
        commandBuffers[i] = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        commandBuffers[i].commandBuffer = *(CommandBufferVK*)queueSubmitDesc.commandBuffers[i];
    }

    Scratch<VkSemaphoreSubmitInfo> signalSemaphores = AllocateScratch(m_Device, VkSemaphoreSubmitInfo, queueSubmitDesc.signalFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        signalSemaphores[i] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalSemaphores[i].semaphore = *(FenceVK*)queueSubmitDesc.signalFences[i].fence;
        signalSemaphores[i].value = queueSubmitDesc.signalFences[i].value;
        signalSemaphores[i].stageMask = GetPipelineStageFlags(queueSubmitDesc.signalFences[i].stages);
    }

    VkSubmitInfo2 submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submitInfo.waitSemaphoreInfoCount = queueSubmitDesc.waitFenceNum;
    submitInfo.pWaitSemaphoreInfos = waitSemaphores;
    submitInfo.commandBufferInfoCount = queueSubmitDesc.commandBufferNum;
    submitInfo.pCommandBufferInfos = commandBuffers;
    submitInfo.signalSemaphoreInfoCount = queueSubmitDesc.signalFenceNum;
    submitInfo.pSignalSemaphoreInfos = signalSemaphores;

    VkLatencySubmissionPresentIdNV presentId = {VK_STRUCTURE_TYPE_LATENCY_SUBMISSION_PRESENT_ID_NV};
    if (queueSubmitDesc.swapChain && m_Device.m_IsSupported.presentId) {
        presentId.presentID = ((SwapChainVK*)queueSubmitDesc.swapChain)->GetPresentId();
        submitInfo.pNext = &presentId;
    }

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.QueueSubmit2(m_Handle, 1, &submitInfo, VK_NULL_HANDLE);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "QueueSubmit2");

    return Result::SUCCESS;
}

NRI_INLINE Result QueueVK::WaitIdle() {
    ExclusiveScope lock(m_Lock);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.QueueWaitIdle(m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "QueueWaitIdle");

    return Result::SUCCESS;
}
