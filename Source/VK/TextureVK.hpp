// © 2021 NVIDIA Corporation

TextureVK::~TextureVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();

        if (m_VmaAllocation)
            DestroyVma();
        else
            vk.DestroyImage(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result TextureVK::Create(const TextureDesc& textureDesc) {
    m_Desc = FixTextureDesc(textureDesc);

    VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    m_Device.FillCreateInfo(m_Desc, info);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImage(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImage");

    return Result::SUCCESS;
}

Result TextureVK::Create(const TextureVKDesc& textureDesc) {
    if (!textureDesc.vkImage)
        return Result::INVALID_ARGUMENT;

    m_Desc.type = GetTextureType((VkImageType)textureDesc.vkImageType);
    m_Desc.usage = (TextureUsageBits)(-1); // TODO: it's not right...
    m_Desc.format = VKFormatToNRIFormat((VkFormat)textureDesc.vkFormat);
    m_Desc.width = textureDesc.width;
    m_Desc.height = textureDesc.height;
    m_Desc.depth = textureDesc.depth;
    m_Desc.mipNum = textureDesc.mipNum;
    m_Desc.layerNum = textureDesc.layerNum;
    m_Desc.sampleNum = textureDesc.sampleNum;

    m_OwnsNativeObjects = false;
    m_Handle = (VkImage)textureDesc.vkImage;

    return Result::SUCCESS;
}

VkImageAspectFlags TextureVK::GetImageAspectFlags() const {
    return ::GetImageAspectFlags(m_Desc.format);
}

void TextureVK::GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    requirements.pNext = &dedicatedRequirements;

    VkImageMemoryRequirementsInfo2 imageMemoryRequirements = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
    imageMemoryRequirements.image = m_Handle;

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetImageMemoryRequirements2(m_Device, &imageMemoryRequirements, &requirements);

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

NRI_INLINE void TextureVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_Handle, name);
}
