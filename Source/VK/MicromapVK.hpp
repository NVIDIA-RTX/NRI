// © 2025 NVIDIA Corporation

MicromapVK::~MicromapVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyMicromapEXT(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }

    Destroy(m_Buffer);
}

Result MicromapVK::Create(const MicromapDesc& micromapDesc) {
    static_assert((uint32_t)MicromapFormat::OPACITY_2_STATE == VK_OPACITY_MICROMAP_FORMAT_2_STATE_EXT, "Type mismatch");
    static_assert((uint32_t)MicromapFormat::OPACITY_4_STATE == VK_OPACITY_MICROMAP_FORMAT_4_STATE_EXT, "Type mismatch");

    if (!m_Device.GetDesc().features.micromap)
        return Result::UNSUPPORTED;

    VkMicromapBuildSizesInfoEXT sizesInfo = {VK_STRUCTURE_TYPE_MICROMAP_BUILD_SIZES_INFO_EXT};
    m_Device.GetMicromapBuildSizesInfo(micromapDesc, sizesInfo);

    if (micromapDesc.optimizedSize)
        sizesInfo.micromapSize = std::min(sizesInfo.micromapSize, micromapDesc.optimizedSize);

    m_BuildScratchSize = sizesInfo.buildScratchSize;
    m_Flags = micromapDesc.flags;

    for (uint32_t i = 0; i < micromapDesc.usageNum; i++) {
        const MicromapUsageDesc& in = micromapDesc.usages[i];

        VkMicromapUsageEXT out = {};
        out.count = in.triangleNum;
        out.subdivisionLevel = in.subdivisionLevel;
        out.format = (uint32_t)in.format;

        m_Usages.push_back(out);
    }

    BufferDesc bufferDesc = {};
    bufferDesc.size = sizesInfo.micromapSize;
    bufferDesc.usage = BufferUsageBits::MICROMAP_STORAGE;

    return m_Device.CreateImplementation<BufferVK>(m_Buffer, bufferDesc);
}

Result MicromapVK::AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed) {
    CHECK(m_Buffer, "Unexpected");

    Result result = m_Buffer->AllocateAndBindMemory(memoryLocation, priority, committed);
    if (result == Result::SUCCESS)
        result = BindMemory(nullptr, 0);

    return result;
}

Result MicromapVK::BindMemory(const MemoryVK* memory, uint64_t offset) {
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

    { // Create micromap
        VkMicromapCreateInfoEXT createInfo = {VK_STRUCTURE_TYPE_MICROMAP_CREATE_INFO_EXT};
        createInfo.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
        createInfo.size = m_Buffer->GetDesc().size;
        createInfo.buffer = m_Buffer->GetHandle();

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.CreateMicromapEXT(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
        RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateMicromapEXT");
    }

    return Result::SUCCESS;
}

NRI_INLINE void MicromapVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_MICROMAP_EXT, (uint64_t)m_Handle, name);
    m_Buffer->SetDebugName(name);
}
