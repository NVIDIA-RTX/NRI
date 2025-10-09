// © 2021 NVIDIA Corporation

AccelerationStructureVK::~AccelerationStructureVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyAccelerationStructureKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }

    Destroy(m_Buffer);
}

Result AccelerationStructureVK::Create(const AccelerationStructureDesc& accelerationStructureDesc) {
    VkAccelerationStructureBuildSizesInfoKHR sizesInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    m_Device.GetAccelerationStructureBuildSizesInfo(accelerationStructureDesc, sizesInfo);

    if (accelerationStructureDesc.optimizedSize)
        sizesInfo.accelerationStructureSize = std::min(sizesInfo.accelerationStructureSize, accelerationStructureDesc.optimizedSize);

    m_BuildScratchSize = sizesInfo.buildScratchSize;
    m_UpdateScratchSize = sizesInfo.updateScratchSize;
    m_Type = GetAccelerationStructureType(accelerationStructureDesc.type);
    m_Flags = accelerationStructureDesc.flags;

    BufferDesc bufferDesc = {};
    bufferDesc.size = sizesInfo.accelerationStructureSize;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    return m_Device.CreateImplementation<BufferVK>(m_Buffer, bufferDesc);
}

Result AccelerationStructureVK::Create(const AccelerationStructureVKDesc& accelerationStructureVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkAccelerationStructureKHR)accelerationStructureVKDesc.vkAccelerationStructure;

    m_BuildScratchSize = accelerationStructureVKDesc.buildScratchSize;
    m_UpdateScratchSize = accelerationStructureVKDesc.updateScratchSize;
    m_Flags = accelerationStructureVKDesc.flags;

    { // Device address
        VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        deviceAddressInfo.accelerationStructure = (VkAccelerationStructureKHR)accelerationStructureVKDesc.vkAccelerationStructure;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetAccelerationStructureDeviceAddressKHR(m_Device, &deviceAddressInfo);
    }

    BufferVKDesc bufferVKDesc = {};
    bufferVKDesc.vkBuffer = accelerationStructureVKDesc.vkBuffer;
    bufferVKDesc.size = accelerationStructureVKDesc.bufferSize;

    return m_Device.CreateImplementation<BufferVK>(m_Buffer, bufferVKDesc);
}

Result AccelerationStructureVK::AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed) {
    CHECK(m_Buffer, "Unexpected");

    Result result = m_Buffer->AllocateAndBindMemory(memoryLocation, priority, committed);
    if (result == Result::SUCCESS)
        result = BindMemory(nullptr, 0);

    return result;
}

Result AccelerationStructureVK::BindMemory(const MemoryVK* memory, uint64_t offset) {
    CHECK(m_Buffer, "Unexpected");

    // Bind memory
    if (memory) {
        BindBufferMemoryDesc desc = {};
        desc.buffer = (Buffer*)m_Buffer;
        desc.memory = (Memory*)memory;
        desc.offset = offset;

        Result result = m_Device.BindBufferMemory(&desc, 1);
        if(result != Result::SUCCESS)
            return result;
    }

    { // Create acceleration structure
        VkAccelerationStructureCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type = m_Type;
        createInfo.size = m_Buffer->GetDesc().size;
        createInfo.buffer = m_Buffer->GetHandle();

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.CreateAccelerationStructureKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateAccelerationStructureKHR");
    }

    { // Get device address
        VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        deviceAddressInfo.accelerationStructure = m_Handle;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetAccelerationStructureDeviceAddressKHR(m_Device, &deviceAddressInfo);
    }

    return m_DeviceAddress ? Result::SUCCESS : Result::FAILURE;
}

NRI_INLINE void AccelerationStructureVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)m_Handle, name);
    m_Buffer->SetDebugName(name);
}

NRI_INLINE Result AccelerationStructureVK::CreateDescriptor(Descriptor*& descriptor) const {
    return m_Device.CreateImplementation<DescriptorVK>(descriptor, m_Handle);
}
