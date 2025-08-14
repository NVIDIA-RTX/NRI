// © 2021 NVIDIA Corporation

CommandAllocatorVK::~CommandAllocatorVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyCommandPool(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result CommandAllocatorVK::Create(const Queue& queue) {
    const QueueVK& queueVK = (QueueVK&)queue;

    m_Type = queueVK.GetType();

    const VkCommandPoolCreateInfo info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueVK.GetFamilyIndex()};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateCommandPool(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateCommandPool");

    return Result::SUCCESS;
}

Result CommandAllocatorVK::Create(const CommandAllocatorVKDesc& commandAllocatorVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkCommandPool)commandAllocatorVKDesc.vkCommandPool;
    m_Type = commandAllocatorVKDesc.queueType;

    return Result::SUCCESS;
}

NRI_INLINE void CommandAllocatorVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)m_Handle, name);
}

NRI_INLINE Result CommandAllocatorVK::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    ExclusiveScope lock(m_Lock);

    const VkCommandBufferAllocateInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, m_Handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};

    VkCommandBuffer commandBufferHandle = VK_NULL_HANDLE;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.AllocateCommandBuffers(m_Device, &info, &commandBufferHandle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateCommandBuffers");

    CommandBufferVK* commandBufferVK = Allocate<CommandBufferVK>(m_Device.GetAllocationCallbacks(), m_Device);
    commandBufferVK->Create(m_Handle, commandBufferHandle, m_Type);

    commandBuffer = (CommandBuffer*)commandBufferVK;

    return Result::SUCCESS;
}

NRI_INLINE void CommandAllocatorVK::Reset() {
    ExclusiveScope lock(m_Lock);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.ResetCommandPool(m_Device, m_Handle, (VkCommandPoolResetFlags)0);
    RETURN_VOID_ON_BAD_VKRESULT(&m_Device, vkResult, "vkResetCommandPool");
}
