// © 2021 NVIDIA Corporation

BufferVK::~BufferVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();

        if (m_VmaAllocation)
            DestroyVma();
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

void BufferVK::FinishMemoryBinding(MemoryVK& memory, uint64_t memoryOffset) {
    CHECK(m_OwnsNativeObjects, "Not for wrapped objects");

    // Mapped memory
    MemoryTypeInfo memoryTypeInfo = Unpack(memory.GetType());
    if (IsHostVisibleMemory(memoryTypeInfo.location)) {
        m_MappedMemory = memory.GetMappedMemory();
        m_MappedMemoryOffset = memoryOffset;

        if (!m_Device.IsHostCoherentMemory(memoryTypeInfo.index))
            m_NonCoherentDeviceMemory = memory.GetHandle();
    }

    // Device address
    if (m_Device.m_IsSupported.deviceAddress) {
        VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferDeviceAddressInfo.buffer = m_Handle;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetBufferDeviceAddress(m_Device, &bufferDeviceAddressInfo);
    }
}

void BufferVK::GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    requirements.pNext = &dedicatedRequirements;

    VkBufferMemoryRequirementsInfo2 bufferMemoryRequirements = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
    bufferMemoryRequirements.buffer = m_Handle;

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetBufferMemoryRequirements2(m_Device, &bufferMemoryRequirements, &requirements);

    MemoryTypeInfo memoryTypeInfo = {};
    memoryTypeInfo.mustBeDedicated = dedicatedRequirements.prefersDedicatedAllocation;

    memoryDesc = {};
    if (m_Device.GetMemoryTypeInfo(memoryLocation, requirements.memoryRequirements.memoryTypeBits, memoryTypeInfo)) {
        memoryDesc.size = requirements.memoryRequirements.size;
        memoryDesc.alignment = (uint32_t)requirements.memoryRequirements.alignment;
        memoryDesc.type = Pack(memoryTypeInfo);
        memoryDesc.mustBeDedicated = memoryTypeInfo.mustBeDedicated;
    }
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

    offset += m_MappedMemoryOffset;

    return m_MappedMemory + offset;
}

NRI_INLINE void BufferVK::Unmap() {
    if (m_NonCoherentDeviceMemory) {
        VkMappedMemoryRange memoryRange = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        memoryRange.memory = m_NonCoherentDeviceMemory;
        memoryRange.offset = m_MappedMemoryOffset + m_MappedMemoryRangeOffset;
        memoryRange.size = m_MappedMemoryRangeSize;

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.FlushMappedMemoryRanges(m_Device, 1, &memoryRange);
        RETURN_VOID_ON_BAD_VKRESULT(&m_Device, vkResult, "vkFlushMappedMemoryRanges");
    }
}
