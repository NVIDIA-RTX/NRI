// © 2021 NVIDIA Corporation

FenceVK::~FenceVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle != VK_NULL_HANDLE)
        vk.DestroySemaphore(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
}

Result FenceVK::Create(uint64_t initialValue) {
    VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    semaphoreTypeCreateInfo.initialValue = initialValue;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateSemaphore((VkDevice)m_Device, &semaphoreCreateInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSemaphore returned %d", (int32_t)result);

    return Result::SUCCESS;
}

NRI_INLINE void FenceVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_Handle, name);
}

NRI_INLINE uint64_t FenceVK::GetFenceValue() const {
    uint64_t value = 0;

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetSemaphoreCounterValue((VkDevice)m_Device, m_Handle, &value);

    return value;
}

NRI_INLINE void FenceVK::Wait(uint64_t value) {
    VkSemaphoreWaitInfo semaphoreWaitInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    semaphoreWaitInfo.semaphoreCount = 1;
    semaphoreWaitInfo.pSemaphores = &m_Handle;
    semaphoreWaitInfo.pValues = &value;

    const auto& vk = m_Device.GetDispatchTable();
    vk.WaitSemaphores((VkDevice)m_Device, &semaphoreWaitInfo, MsToUs(TIMEOUT_FENCE));
}
