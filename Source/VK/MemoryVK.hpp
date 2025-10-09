// © 2021 NVIDIA Corporation

MemoryVK::~MemoryVK() {
    if (!m_OwnsNativeObjects)
        return;

    if (m_VmaAllocation)
        vmaFreeMemory(m_Device.GetVma(), m_VmaAllocation);
    else if (m_Handle) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.FreeMemory(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result MemoryVK::Create(const MemoryVKDesc& memoryVKDesc) {
    MemoryTypeInfo memoryTypeInfo = {};

    bool found = m_Device.GetMemoryTypeByIndex(memoryVKDesc.memoryTypeIndex, memoryTypeInfo);
    RETURN_ON_FAILURE(&m_Device, found, Result::INVALID_ARGUMENT, "Can't find memory by index");

    m_OwnsNativeObjects = false;
    m_Handle = (VkDeviceMemory)memoryVKDesc.vkDeviceMemory;
    m_MappedMemory = (uint8_t*)memoryVKDesc.mappedMemory;
    m_Type = Pack(memoryTypeInfo);
    m_Offset = memoryVKDesc.offset;

    const auto& vk = m_Device.GetDispatchTable();
    if (!m_MappedMemory && IsHostVisibleMemory(memoryTypeInfo.location)) {
        VkResult vkResult = vk.MapMemory(m_Device, m_Handle, memoryVKDesc.offset, memoryVKDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkMapMemory");
    }

    return Result::SUCCESS;
}

Result MemoryVK::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    m_Type = allocateMemoryDesc.type;
    m_Priority = m_Device.m_IsSupported.memoryPriority ? (allocateMemoryDesc.priority * 0.5f + 0.5f) : 0.5f;

    MemoryTypeInfo memoryTypeInfo = Unpack(allocateMemoryDesc.type);

    // Dedicated allocation occurs on memory binding
    if (memoryTypeInfo.mustBeDedicated)
        return Result::SUCCESS;

    if (allocateMemoryDesc.useVMA) {
        // Respect worst-case alignment
        const DeviceDesc& deviceDesc = m_Device.GetDesc();
        uint32_t alignment = 1;
        alignment = std::max(alignment, deviceDesc.memoryAlignment.bufferShaderResourceOffset);
        alignment = std::max(alignment, deviceDesc.memoryAlignment.constantBufferOffset);
        if (deviceDesc.features.rayTracing) {
            alignment = std::max(alignment, deviceDesc.memoryAlignment.scratchBufferOffset);
            alignment = std::max(alignment, deviceDesc.memoryAlignment.shaderBindingTable);
            alignment = std::max(alignment, deviceDesc.memoryAlignment.accelerationStructureOffset);
        }
        if (deviceDesc.features.micromap)
            alignment = std::max(alignment, deviceDesc.memoryAlignment.micromapOffset);

        // (Sub) allocate memory
        VkMemoryRequirements memoryRequirements = {};
        memoryRequirements.size = allocateMemoryDesc.size;
        memoryRequirements.alignment = alignment;
        memoryRequirements.memoryTypeBits = 1 << memoryTypeInfo.index;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT | VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
        allocationCreateInfo.flags |= IsHostVisibleMemory(memoryTypeInfo.location) ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;
        allocationCreateInfo.memoryTypeBits = 1 << memoryTypeInfo.index; // "usage, requiredFlags and preferredFlags" not needed because of this
        allocationCreateInfo.priority = m_Priority;

        VkResult vkResult = vmaAllocateMemory(m_Device.GetVma(), &memoryRequirements, &allocationCreateInfo, &m_VmaAllocation, nullptr);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vmaAllocateMemory");
    } else {
        // Allocate memory
        VkMemoryPriorityAllocateInfoEXT priorityInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
        priorityInfo.priority = m_Priority;

        VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
        flagsInfo.pNext = m_Priority == 0.5f ? nullptr : &priorityInfo;
        flagsInfo.flags = m_Device.m_IsSupported.deviceAddress ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;

        if (m_Device.IsMemoryZeroInitializationEnabled())
            flagsInfo.flags |= VK_MEMORY_ALLOCATE_ZERO_INITIALIZE_BIT_EXT;

        VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        memoryInfo.pNext = &flagsInfo;
        memoryInfo.allocationSize = allocateMemoryDesc.size;
        memoryInfo.memoryTypeIndex = memoryTypeInfo.index;

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateMemory");

        // Persistently map if needed
        if (IsHostVisibleMemory(memoryTypeInfo.location)) {
            vkResult = vk.MapMemory(m_Device, m_Handle, 0, allocateMemoryDesc.size, 0, (void**)&m_MappedMemory);
            RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkMapMemory");
        }
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(const BufferVK* buffer, const TextureVK* texture) {
    MemoryTypeInfo memoryTypeInfo = Unpack(m_Type);
    CHECK(m_Type != std::numeric_limits<MemoryType>::max() && memoryTypeInfo.mustBeDedicated, "Shouldn't be there");

    MemoryDesc memoryDesc = {};
    if (buffer)
        buffer->GetMemoryDesc(memoryTypeInfo.location, memoryDesc);
    else
        texture->GetMemoryDesc(memoryTypeInfo.location, memoryDesc);

    VkMemoryPriorityAllocateInfoEXT priorityInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
    priorityInfo.priority = m_Priority;

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.pNext = m_Priority == 0.5f ? nullptr : &priorityInfo;
    flagsInfo.flags = m_Device.m_IsSupported.deviceAddress ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;

    if (m_Device.IsMemoryZeroInitializationEnabled())
        flagsInfo.flags |= VK_MEMORY_ALLOCATE_ZERO_INITIALIZE_BIT_EXT;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicatedAllocateInfo.pNext = &flagsInfo;
    if (buffer)
        dedicatedAllocateInfo.buffer = buffer->GetHandle();
    else
        dedicatedAllocateInfo.image = texture->GetHandle();

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
