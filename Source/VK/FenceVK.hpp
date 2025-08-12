// © 2021 NVIDIA Corporation

FenceVK::~FenceVK() {
    if (m_OwnsNativeObjects && m_Handle) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroySemaphore(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result FenceVK::Create(uint64_t initialValue) {
    VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    semaphoreTypeCreateInfo.semaphoreType = initialValue == SWAPCHAIN_SEMAPHORE ? VK_SEMAPHORE_TYPE_BINARY : VK_SEMAPHORE_TYPE_TIMELINE;
    semaphoreTypeCreateInfo.initialValue = initialValue == SWAPCHAIN_SEMAPHORE ? 0 : initialValue;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateSemaphore((VkDevice)m_Device, &semaphoreCreateInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateSemaphore");

    return Result::SUCCESS;
}

Result FenceVK::Create(const FenceVKDesc& fenceVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkSemaphore)fenceVKDesc.vkTimelineSemaphore;

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
