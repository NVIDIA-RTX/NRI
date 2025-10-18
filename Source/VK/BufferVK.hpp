// © 2021 NVIDIA Corporation

BufferVK::~BufferVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();

        if (m_VmaAllocation)
            vmaDestroyBuffer(m_Device.GetVma(), m_Handle, m_VmaAllocation);
        else
            vk.DestroyBuffer(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result BufferVK::Create(const BufferDesc& bufferDesc) {
    m_Desc = bufferDesc;

    VkBufferCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    m_Device.FillCreateInfo(bufferDesc, info);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateBuffer(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateBuffer");

    return Result::SUCCESS;
}

Result BufferVK::Create(const BufferVKDesc& bufferVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkBuffer)bufferVKDesc.vkBuffer;
    m_MappedMemory = bufferVKDesc.mappedMemory;
    m_NonCoherentDeviceMemory = (VkDeviceMemory)bufferVKDesc.vkDeviceMemory;
    m_DeviceAddress = (VkDeviceAddress)bufferVKDesc.deviceAddress;

    m_Desc.size = bufferVKDesc.size;
    m_Desc.structureStride = bufferVKDesc.structureStride;

    return Result::SUCCESS;
}

Result BufferVK::AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed) {
    CHECK(m_Handle, "Unexpected");

    MemoryDesc memoryDesc = {};
    GetMemoryDesc(memoryLocation, memoryDesc);

    MemoryTypeInfo memoryTypeInfo = Unpack(memoryDesc.type);
    if (memoryTypeInfo.mustBeDedicated)
        committed = true;

    VkMemoryRequirements memoryRequirements = {};
    memoryRequirements.size = memoryDesc.size;
    memoryRequirements.alignment = memoryDesc.alignment; // can't use "vmaAllocateMemoryForBuffer" because of alignment (see "GetMemoryDesc")
    memoryRequirements.memoryTypeBits = 1 << memoryTypeInfo.index;

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    allocationCreateInfo.flags |= committed ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
    allocationCreateInfo.flags |= IsHostVisibleMemory(memoryTypeInfo.location) ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;
    allocationCreateInfo.priority = priority * 0.5f + 0.5f;
    allocationCreateInfo.memoryTypeBits = 1 << memoryTypeInfo.index; // "usage, requiredFlags and preferredFlags" not needed because of this

    VmaAllocationInfo allocationInfo = {};

    VkResult vkResult = vmaAllocateMemory(m_Device.GetVma(), &memoryRequirements, &allocationCreateInfo, &m_VmaAllocation, &allocationInfo);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vmaAllocateMemory");

    vkResult = vmaBindBufferMemory(m_Device.GetVma(), m_VmaAllocation, m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vmaBindBufferMemory");

    // Assign mapped memory
    if (IsHostVisibleMemory(memoryTypeInfo.location)) {
        m_MappedMemory = (uint8_t*)allocationInfo.pMappedData;

        if (!m_Device.IsHostCoherentMemory(memoryTypeInfo.index)) {
            m_NonCoherentDeviceMemory = allocationInfo.deviceMemory;
            m_NonCoherentDeviceMemoryOffset = allocationInfo.offset;
        }
    }

    // Get device address
    if (m_Device.m_IsSupported.deviceAddress) {
        VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferDeviceAddressInfo.buffer = m_Handle;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetBufferDeviceAddress(m_Device, &bufferDeviceAddressInfo);
    }

    return Result::SUCCESS;
}

Result BufferVK::BindMemory(MemoryVK& memory, uint64_t offset, bool bindMemory) {
    CHECK(m_Handle, "Unexpected");
    CHECK(m_OwnsNativeObjects, "Not for wrapped objects");

    // Bind memory
    if (bindMemory) {
        MemoryTypeInfo memoryTypeInfo = Unpack(memory.GetType());
        if (memoryTypeInfo.mustBeDedicated)
            memory.CreateDedicated(this, nullptr);

        VkBindBufferMemoryInfo bindBufferMemoryInfo = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
        bindBufferMemoryInfo.buffer = m_Handle;
        bindBufferMemoryInfo.memory = memory.GetHandle();
        bindBufferMemoryInfo.memoryOffset = memory.GetOffset() + offset;

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.BindBufferMemory2(m_Device, 1, &bindBufferMemoryInfo);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkBindBufferMemory2");
    }

    // Assign mapped memory
    MemoryTypeInfo memoryTypeInfo = Unpack(memory.GetType());
    if (IsHostVisibleMemory(memoryTypeInfo.location)) {
        m_MappedMemory = memory.GetMappedMemory() + offset;

        if (!m_Device.IsHostCoherentMemory(memoryTypeInfo.index)) {
            m_NonCoherentDeviceMemory = memory.GetHandle();
            m_NonCoherentDeviceMemoryOffset = memory.GetOffset() + offset;
        }
    }

    // Get device address
    if (m_Device.m_IsSupported.deviceAddress) {
        VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferDeviceAddressInfo.buffer = m_Handle;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetBufferDeviceAddress(m_Device, &bufferDeviceAddressInfo);
    }

    return Result::SUCCESS;
}

void BufferVK::GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    requirements.pNext = &dedicatedRequirements;

    VkBufferMemoryRequirementsInfo2 bufferMemoryRequirements = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
    bufferMemoryRequirements.buffer = m_Handle;

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetBufferMemoryRequirements2(m_Device, &bufferMemoryRequirements, &requirements);

    // There is no "VK_BUFFER_USAGE" flag for "SCRATCH_BUFFER", thus "vkGetBufferMemoryRequirements" can't return proper alignment. It affects memory "sub-allocation"
    if (m_Desc.usage & BufferUsageBits::SCRATCH_BUFFER) {
        VkDeviceSize scratchBufferOffset = m_Device.GetDesc().memoryAlignment.scratchBufferOffset;
        requirements.memoryRequirements.alignment = std::max(requirements.memoryRequirements.alignment, scratchBufferOffset);
    }

    memoryDesc = {};
    m_Device.GetMemoryDesc(memoryLocation, requirements.memoryRequirements, dedicatedRequirements, memoryDesc);
}

NRI_INLINE void BufferVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_Handle, name);
}

NRI_INLINE void* BufferVK::Map(uint64_t offset, uint64_t size) {
    CHECK(m_MappedMemory, "No CPU access");

    if (size == WHOLE_SIZE)
        size = m_Desc.size;

    m_MappedMemoryRangeSize = size;
    m_MappedMemoryRangeOffset = offset;

    return m_MappedMemory + offset;
}

NRI_INLINE void BufferVK::Unmap() {
    if (m_NonCoherentDeviceMemory) {
        VkMappedMemoryRange memoryRange = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        memoryRange.memory = m_NonCoherentDeviceMemory;
        memoryRange.offset = m_NonCoherentDeviceMemoryOffset + m_MappedMemoryRangeOffset;
        memoryRange.size = m_MappedMemoryRangeSize;

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.FlushMappedMemoryRanges(m_Device, 1, &memoryRange);
        RETURN_VOID_ON_BAD_VKRESULT(&m_Device, vkResult, "vkFlushMappedMemoryRanges");
    }
}
