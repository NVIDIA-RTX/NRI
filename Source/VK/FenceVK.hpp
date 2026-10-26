// © 2021 NVIDIA Corporation

FenceVK::~FenceVK() {
    if (m_OwnsNativeObjects && m_Handle) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroySemaphore(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result FenceVK::Create(const FenceDesc& fenceDesc) {
    VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    semaphoreTypeCreateInfo.semaphoreType = fenceDesc.initialValue == SWAPCHAIN_SEMAPHORE ? VK_SEMAPHORE_TYPE_BINARY : VK_SEMAPHORE_TYPE_TIMELINE;
    semaphoreTypeCreateInfo.initialValue = fenceDesc.initialValue == SWAPCHAIN_SEMAPHORE ? 0 : fenceDesc.initialValue;

    VkExportSemaphoreCreateInfo exportInfo = {VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO};
    exportInfo.handleTypes = SHARED_SEMAPHORE_HANDLE_TYPE;

    if (fenceDesc.shared) {
        if (!m_Device.GetDesc().features.sharedFences)
            return Result::UNSUPPORTED;

        semaphoreTypeCreateInfo.pNext = &exportInfo;
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateSemaphore((VkDevice)m_Device, &semaphoreCreateInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateSemaphore");

    return Result::SUCCESS;
}

Result FenceVK::Create(const FenceVKDesc& fenceVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkSemaphore)fenceVKDesc.vkTimelineSemaphore;

    return Result::SUCCESS;
}

Result FenceVK::GetSharedHandle(SharedHandle& sharedHandle) const {
    sharedHandle = {};

    const auto& vk = m_Device.GetDispatchTable();
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VkSemaphoreGetWin32HandleInfoKHR getInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR};
    getInfo.semaphore = m_Handle;
    getInfo.handleType = SHARED_SEMAPHORE_HANDLE_TYPE;

    HANDLE handle = nullptr;
    VkResult vkResult = vk.GetSemaphoreWin32HandleKHR(m_Device, &getInfo, &handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetSemaphoreWin32HandleKHR");

    sharedHandle.handle = handle;
    sharedHandle.type = SharedHandleType::OPAQUE_WIN32;
#else
    VkSemaphoreGetFdInfoKHR getInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR};
    getInfo.semaphore = m_Handle;
    getInfo.handleType = SHARED_SEMAPHORE_HANDLE_TYPE;

    int fd = -1;
    VkResult vkResult = vk.GetSemaphoreFdKHR(m_Device, &getInfo, &fd);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetSemaphoreFdKHR");

    sharedHandle.handle = (void*)(intptr_t)fd;
    sharedHandle.type = SharedHandleType::OPAQUE_FD;
#endif

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

NRI_INLINE Result FenceVK::Wait(uint64_t value) {
    VkSemaphoreWaitInfo semaphoreWaitInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    semaphoreWaitInfo.semaphoreCount = 1;
    semaphoreWaitInfo.pSemaphores = &m_Handle;
    semaphoreWaitInfo.pValues = &value;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.WaitSemaphores((VkDevice)m_Device, &semaphoreWaitInfo, MsToUs(NRI_TIMEOUT_FENCE));
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "WaitSemaphores");
    NRI_RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, Result::FAILURE, "WaitSemaphores() timed out!");

    return Result::SUCCESS;
}
