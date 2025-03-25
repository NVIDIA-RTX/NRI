// © 2025 NVIDIA Corporation

MicromapVK::~MicromapVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyMicromapEXT(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());

        Destroy(m_Buffer);
    }
}

Result MicromapVK::Create(const MicromapDesc& micromapDesc) {
    VkMicromapBuildSizesInfoEXT sizesInfo = {VK_STRUCTURE_TYPE_MICROMAP_BUILD_SIZES_INFO_EXT};
    m_Device.GetMicromapBuildSizesInfo(micromapDesc, sizesInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = sizesInfo.micromapSize;
    bufferDesc.usage = BufferUsageBits::MICROMAP_STORAGE;

    Buffer* buffer = nullptr;
    Result result = m_Device.CreateImplementation<BufferVK>(buffer, bufferDesc);
    if (result == Result::SUCCESS) {
        m_Buffer = (BufferVK*)buffer;
        m_BuildScratchSize = sizesInfo.buildScratchSize;

        for (uint32_t i = 0; i < micromapDesc.usageNum; i++) {
            const MicromapUsageDesc& in = micromapDesc.usages[i];

            VkMicromapUsageEXT out = {};
            out.count = in.triangleNum;
            out.subdivisionLevel = in.subdivisionLevel;
            out.format = (uint32_t)in.format;

            m_Usages.push_back(out);
        }
    }

    return result;
}

Result MicromapVK::FinishCreation() {
    if (!m_Buffer)
        return Result::FAILURE;

    VkMicromapCreateInfoEXT createInfo = {VK_STRUCTURE_TYPE_MICROMAP_CREATE_INFO_EXT};
    createInfo.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
    createInfo.size = m_Buffer->GetDesc().size;
    createInfo.buffer = m_Buffer->GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateMicromapEXT(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateMicromapEXT returned %d", (int32_t)result);

    return Result::SUCCESS;
}

NRI_INLINE void MicromapVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_MICROMAP_EXT, (uint64_t)m_Handle, name);

    if (m_Buffer)
        m_Buffer->SetDebugName(name);
}
