// © 2021 NVIDIA Corporation

TextureVK::~TextureVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();

        if (m_VmaAllocation)
            vmaDestroyImage(m_Device.GetVma(), m_Handle, m_VmaAllocation);
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

Result TextureVK::Create(const TextureVKDesc& textureVKDesc) {
    m_Desc.type = GetTextureType((VkImageType)textureVKDesc.vkImageType);
    m_Desc.usage = (TextureUsageBits)(-1); // TODO: it's not right...
    m_Desc.format = VKFormatToNRIFormat((VkFormat)textureVKDesc.vkFormat);
    m_Desc.width = textureVKDesc.width;
    m_Desc.height = textureVKDesc.height;
    m_Desc.depth = textureVKDesc.depth;
    m_Desc.mipNum = textureVKDesc.mipNum;
    m_Desc.layerNum = textureVKDesc.layerNum;
    m_Desc.sampleNum = textureVKDesc.sampleNum;

    m_OwnsNativeObjects = false;
    m_Handle = (VkImage)textureVKDesc.vkImage;

    return Result::SUCCESS;
}

Result TextureVK::AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed) {
    CHECK(m_Handle, "Unexpected");

    MemoryDesc memoryDesc = {};
    GetMemoryDesc(memoryLocation, memoryDesc);

    MemoryTypeInfo memoryTypeInfo = Unpack(memoryDesc.type);
    if (memoryTypeInfo.mustBeDedicated)
        committed = true;

    VkMemoryRequirements memoryRequirements = {};
    memoryRequirements.size = memoryDesc.size;
    memoryRequirements.alignment = memoryDesc.alignment;
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

    vkResult = vmaBindImageMemory(m_Device.GetVma(), m_VmaAllocation, m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vmaBindImageMemory");

    return Result::SUCCESS;
}

Result TextureVK::BindMemory(const MemoryVK& memory, uint64_t offset) {
    CHECK(m_Handle, "Unexpected");
    CHECK(m_OwnsNativeObjects, "Not for wrapped objects");

    VkBindImageMemoryInfo bindImageMemoryInfo = {VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
    bindImageMemoryInfo.image = m_Handle;
    bindImageMemoryInfo.memory = memory.GetHandle();
    bindImageMemoryInfo.memoryOffset = memory.GetOffset() + offset;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.BindImageMemory2(m_Device, 1, &bindImageMemoryInfo);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkBindImageMemory2");

    return Result::SUCCESS;
}

void TextureVK::GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    requirements.pNext = &dedicatedRequirements;

    VkImageMemoryRequirementsInfo2 imageMemoryRequirements = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
    imageMemoryRequirements.image = m_Handle;

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetImageMemoryRequirements2(m_Device, &imageMemoryRequirements, &requirements);

    memoryDesc = {};
    m_Device.GetMemoryDesc(memoryLocation, requirements.memoryRequirements, dedicatedRequirements, memoryDesc);
}

VkImageAspectFlags TextureVK::GetImageAspectFlags() const {
    return ::GetImageAspectFlags(m_Desc.format);
}

NRI_INLINE void TextureVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_Handle, name);
}
