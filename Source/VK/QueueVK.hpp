// © 2021 NVIDIA Corporation

Result QueueVK::Create(QueueType type, uint32_t familyIndex, VkQueue handle, Lock* sharedLock) {
    m_Type = type;
    m_FamilyIndex = familyIndex;
    m_Handle = handle;
    m_Lock = sharedLock ? sharedLock : &m_OwnLock;

    return Result::SUCCESS;
}

NRI_INLINE void QueueVK::SetDebugName(const char* name) {
    ExclusiveScope lock(*m_Lock);

    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_QUEUE, (uint64_t)m_Handle, name);
}

NRI_INLINE void QueueVK::BeginAnnotation(const char* name, uint32_t bgra) {
    ExclusiveScope lock(*m_Lock);

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
    ExclusiveScope lock(*m_Lock);

    const auto& vk = m_Device.GetDispatchTable();
    if (vk.QueueEndDebugUtilsLabelEXT)
        vk.QueueEndDebugUtilsLabelEXT(m_Handle);
}

NRI_INLINE void QueueVK::Annotation(const char* name, uint32_t bgra) {
    ExclusiveScope lock(*m_Lock);

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

NRI_INLINE void QueueVK::GetCalibratedTimestamps(uint64_t& timestampGPU, uint64_t& timestampCPU) {
    timestampGPU = 0;
    timestampCPU = 0;

    VkCalibratedTimestampInfoKHR timestampInfos[2] = {};
    {
        // GPU
        timestampInfos[0].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR;
        timestampInfos[0].timeDomain = VK_TIME_DOMAIN_DEVICE_KHR;

        // CPU
        timestampInfos[1].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR;
        timestampInfos[1].timeDomain = m_Device.GetCalibratedTimestampCPUTimeDomain();
    }

    uint64_t timestamps[2] = {};
    uint64_t maxDeviation = 0;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.GetCalibratedTimestampsEXT(m_Device, 2, timestampInfos, timestamps, &maxDeviation);
    NRI_RETURN_VOID_ON_BAD_VKRESULT(&m_Device, vkResult, "GetCalibratedTimestampsKHR");

    timestampGPU = timestamps[0];
    timestampCPU = timestamps[1];
}

NRI_INLINE Result QueueVK::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    ExclusiveScope lock(*m_Lock);
    const VkPipelineStageFlags2 unsupportedStages = m_Device.GetUnsupportedPipelineStages();

    Scratch<VkSemaphoreSubmitInfo> waitSemaphores = NRI_ALLOCATE_SCRATCH(m_Device, VkSemaphoreSubmitInfo, queueSubmitDesc.waitFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        waitSemaphores[i] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitSemaphores[i].semaphore = *(FenceVK*)queueSubmitDesc.waitFences[i].fence;
        waitSemaphores[i].value = queueSubmitDesc.waitFences[i].value;
        waitSemaphores[i].stageMask = GetPipelineStageFlags(queueSubmitDesc.waitFences[i].stages) & ~unsupportedStages;
    }

    Scratch<VkCommandBufferSubmitInfo> commandBuffers = NRI_ALLOCATE_SCRATCH(m_Device, VkCommandBufferSubmitInfo, queueSubmitDesc.commandBufferNum);
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++) {
        commandBuffers[i] = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        commandBuffers[i].commandBuffer = *(CommandBufferVK*)queueSubmitDesc.commandBuffers[i];
    }

    Scratch<VkSemaphoreSubmitInfo> signalSemaphores = NRI_ALLOCATE_SCRATCH(m_Device, VkSemaphoreSubmitInfo, queueSubmitDesc.signalFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        signalSemaphores[i] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalSemaphores[i].semaphore = *(FenceVK*)queueSubmitDesc.signalFences[i].fence;
        signalSemaphores[i].value = queueSubmitDesc.signalFences[i].value;
        signalSemaphores[i].stageMask = GetPipelineStageFlags(queueSubmitDesc.signalFences[i].stages) & ~unsupportedStages;
    }

    const auto& vk = m_Device.GetDispatchTable();

    // Without "synchronization2" (a wrapped device with unknown features): a Vulkan 1.0 submission with timeline semaphore values
    if (!m_Device.m_IsSupported.synchronization2) {
        Scratch<VkSemaphore> waitSemaphores1 = NRI_ALLOCATE_SCRATCH(m_Device, VkSemaphore, queueSubmitDesc.waitFenceNum);
        Scratch<uint64_t> waitValues = NRI_ALLOCATE_SCRATCH(m_Device, uint64_t, queueSubmitDesc.waitFenceNum);
        Scratch<VkPipelineStageFlags> waitStages = NRI_ALLOCATE_SCRATCH(m_Device, VkPipelineStageFlags, queueSubmitDesc.waitFenceNum);
        for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
            VkPipelineStageFlags stages = GetPipelineStageFlags1(waitSemaphores[i].stageMask);
            if (!stages)
                stages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // "BOTTOM_OF_PIPE" in the second scope means none

            waitSemaphores1[i] = waitSemaphores[i].semaphore;
            waitValues[i] = waitSemaphores[i].value;
            waitStages[i] = stages;
        }

        Scratch<VkCommandBuffer> commandBuffers1 = NRI_ALLOCATE_SCRATCH(m_Device, VkCommandBuffer, queueSubmitDesc.commandBufferNum);
        for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
            commandBuffers1[i] = commandBuffers[i].commandBuffer;

        Scratch<VkSemaphore> signalSemaphores1 = NRI_ALLOCATE_SCRATCH(m_Device, VkSemaphore, queueSubmitDesc.signalFenceNum);
        Scratch<uint64_t> signalValues = NRI_ALLOCATE_SCRATCH(m_Device, uint64_t, queueSubmitDesc.signalFenceNum);
        for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
            signalSemaphores1[i] = signalSemaphores[i].semaphore;
            signalValues[i] = signalSemaphores[i].value;
        }

        VkTimelineSemaphoreSubmitInfo timelineInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
        timelineInfo.waitSemaphoreValueCount = queueSubmitDesc.waitFenceNum;
        timelineInfo.pWaitSemaphoreValues = waitValues;
        timelineInfo.signalSemaphoreValueCount = queueSubmitDesc.signalFenceNum;
        timelineInfo.pSignalSemaphoreValues = signalValues;

        VkSubmitInfo submitInfo1 = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo1.pNext = &timelineInfo;
        submitInfo1.waitSemaphoreCount = queueSubmitDesc.waitFenceNum;
        submitInfo1.pWaitSemaphores = waitSemaphores1;
        submitInfo1.pWaitDstStageMask = waitStages;
        submitInfo1.commandBufferCount = queueSubmitDesc.commandBufferNum;
        submitInfo1.pCommandBuffers = commandBuffers1;
        submitInfo1.signalSemaphoreCount = queueSubmitDesc.signalFenceNum;
        submitInfo1.pSignalSemaphores = signalSemaphores1;

        VkResult vkResult = vk.QueueSubmit(m_Handle, 1, &submitInfo1, VK_NULL_HANDLE);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "QueueSubmit");

        return Result::SUCCESS;
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
        presentId.presentID = queueSubmitDesc.presentId;
        submitInfo.pNext = &presentId;
    }

    VkResult vkResult = vk.QueueSubmit2(m_Handle, 1, &submitInfo, VK_NULL_HANDLE);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "QueueSubmit2");

    return Result::SUCCESS;
}

NRI_INLINE Result QueueVK::WaitIdle() {
    ExclusiveScope lock(*m_Lock);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.QueueWaitIdle(m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "QueueWaitIdle");

    return Result::SUCCESS;
}
