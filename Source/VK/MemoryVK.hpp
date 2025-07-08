// © 2021 NVIDIA Corporation

MemoryVK::~MemoryVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.FreeMemory(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result MemoryVK::Create(const MemoryVKDesc& memoryDesc) {
    if (!memoryDesc.vkDeviceMemory)
        return Result::INVALID_ARGUMENT;

    MemoryTypeInfo memoryTypeInfo = {};

    bool found = m_Device.GetMemoryTypeByIndex(memoryDesc.memoryTypeIndex, memoryTypeInfo);
    RETURN_ON_FAILURE(&m_Device, found, Result::INVALID_ARGUMENT, "Can't find memory by index");

    m_OwnsNativeObjects = false;
    m_Handle = (VkDeviceMemory)memoryDesc.vkDeviceMemory;
    m_MappedMemory = (uint8_t*)memoryDesc.vkMappedMemory;
    m_Type = Pack(memoryTypeInfo);

    const auto& vk = m_Device.GetDispatchTable();
    if (!m_MappedMemory && IsHostVisibleMemory(memoryTypeInfo.location)) {
        VkResult vkResult = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkMapMemory");
    }

    return Result::SUCCESS;
}

Result MemoryVK::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    m_Type = allocateMemoryDesc.type;
    m_Priority = m_Device.m_IsSupported.memoryPriority ? (allocateMemoryDesc.priority * 0.5f + 0.5f) : 0.5f;

    MemoryTypeInfo memoryTypeInfo = Unpack(allocateMemoryDesc.type);
    if (memoryTypeInfo.mustBeDedicated)
        return Result::SUCCESS; // dedicated allocation occurs on memory binding

    VkMemoryPriorityAllocateInfoEXT priorityInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
    priorityInfo.priority = m_Priority;

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.pNext = m_Priority == 0.5f ? nullptr : &priorityInfo;
    flagsInfo.flags = m_Device.m_IsSupported.deviceAddress ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &flagsInfo;
    memoryInfo.allocationSize = allocateMemoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.index;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateMemory");

    if (IsHostVisibleMemory(memoryTypeInfo.location)) {
        vkResult = vk.MapMemory(m_Device, m_Handle, 0, allocateMemoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkMapMemory");
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(const BufferVK& buffer) {
    MemoryTypeInfo memoryTypeInfo = Unpack(m_Type);
    CHECK(m_Type != std::numeric_limits<MemoryType>::max() && memoryTypeInfo.mustBeDedicated, "Shouldn't be there");

    MemoryDesc memoryDesc = {};
    buffer.GetMemoryDesc(memoryTypeInfo.location, memoryDesc);

    VkMemoryPriorityAllocateInfoEXT priorityInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
    priorityInfo.priority = m_Priority;

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.pNext = m_Priority == 0.5f ? nullptr : &priorityInfo;
    flagsInfo.flags = m_Device.m_IsSupported.deviceAddress ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicatedAllocateInfo.pNext = &flagsInfo;
    dedicatedAllocateInfo.buffer = buffer.GetHandle();

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.index;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateMemory");

    if (IsHostVisibleMemory(memoryTypeInfo.location)) {
        vkResult = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkMapMemory");
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(const TextureVK& texture) {
    MemoryTypeInfo memoryTypeInfo = Unpack(m_Type);
    CHECK(m_Type != std::numeric_limits<MemoryType>::max() && memoryTypeInfo.mustBeDedicated, "Shouldn't be there");

    MemoryDesc memoryDesc = {};
    texture.GetMemoryDesc(memoryTypeInfo.location, memoryDesc);

    VkMemoryPriorityAllocateInfoEXT priorityInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
    priorityInfo.priority = m_Priority;

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.pNext = m_Priority == 0.5f ? nullptr : &priorityInfo;
    flagsInfo.flags = m_Device.m_IsSupported.deviceAddress ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicatedAllocateInfo.pNext = &flagsInfo;
    dedicatedAllocateInfo.image = texture.GetHandle();

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.index;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateMemory");

    if (IsHostVisibleMemory(memoryTypeInfo.location)) {
        vkResult = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkMapMemory");
    }

    return Result::SUCCESS;
}

NRI_INLINE void MemoryVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)m_Handle, name);
}
