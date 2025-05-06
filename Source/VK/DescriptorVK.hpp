// © 2021 NVIDIA Corporation

DescriptorVK::~DescriptorVK() {
    const auto& vk = m_Device.GetDispatchTable();

    switch (m_Type) {
        case DescriptorTypeVK::NONE:
        case DescriptorTypeVK::ACCELERATION_STRUCTURE:
            break;
        case DescriptorTypeVK::BUFFER_VIEW:
            if (m_BufferView)
                vk.DestroyBufferView(m_Device, m_BufferView, m_Device.GetVkAllocationCallbacks());
            break;
        case DescriptorTypeVK::IMAGE_VIEW:
            if (m_ImageView)
                vk.DestroyImageView(m_Device, m_ImageView, m_Device.GetVkAllocationCallbacks());
            break;
        case DescriptorTypeVK::SAMPLER:
            if (m_Sampler)
                vk.DestroySampler(m_Device, m_Sampler, m_Device.GetVkAllocationCallbacks());
            break;
    }
}

template <typename T>
Result DescriptorVK::CreateTextureView(const T& textureViewDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    Mip_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;
    Dim_t remainingLayers = textureDesc.layerNum - textureViewDesc.layerOffset;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    VkImageSubresourceRange subresource = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        textureViewDesc.mipNum == REMAINING_MIPS ? remainingMips : textureViewDesc.mipNum,
        textureViewDesc.layerOffset,
        textureViewDesc.layerNum == REMAINING_LAYERS ? remainingLayers : textureViewDesc.layerNum,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType, subresource.layerCount);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresource;
    createInfo.image = texture.GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_ImageView);
    RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateImageView returned %d", (int32_t)vkResult);

    m_Type = DescriptorTypeVK::IMAGE_VIEW;
    m_TextureDesc.handle = texture.GetHandle();
    m_TextureDesc.texture = &texture;
    m_TextureDesc.layout = GetImageLayoutForView(textureViewDesc.viewType);
    m_TextureDesc.aspectFlags = GetImageAspectFlags(textureViewDesc.format);
    m_TextureDesc.layerOffset = textureViewDesc.layerOffset;
    m_TextureDesc.layerNum = (Dim_t)subresource.layerCount;
    m_TextureDesc.sliceOffset = 0;
    m_TextureDesc.sliceNum = 1;
    m_TextureDesc.mipOffset = textureViewDesc.mipOffset;
    m_TextureDesc.mipNum = (Mip_t)subresource.levelCount;

    return Result::SUCCESS;
}

template <>
Result DescriptorVK::CreateTextureView(const Texture3DViewDesc& textureViewDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    Mip_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;
    Dim_t remainingLayers = textureDesc.layerNum - textureViewDesc.sliceOffset;

    VkImageViewSlicedCreateInfoEXT slicesInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT};
    slicesInfo.sliceOffset = textureViewDesc.sliceOffset;
    slicesInfo.sliceCount = textureViewDesc.sliceNum == REMAINING_LAYERS ? remainingLayers : textureViewDesc.sliceNum;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    if (m_Device.m_IsSupported.imageSlicedView)
        usageInfo.pNext = &slicesInfo;

    VkImageSubresourceRange subresource = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        textureViewDesc.mipNum == REMAINING_MIPS ? remainingMips : textureViewDesc.mipNum,
        0,
        1,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType, subresource.layerCount);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresource;
    createInfo.image = texture.GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_ImageView);
    RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateImageView returned %d", (int32_t)vkResult);

    m_Type = DescriptorTypeVK::IMAGE_VIEW;
    m_TextureDesc.handle = texture.GetHandle();
    m_TextureDesc.texture = &texture;
    m_TextureDesc.layout = GetImageLayoutForView(textureViewDesc.viewType);
    m_TextureDesc.aspectFlags = GetImageAspectFlags(textureViewDesc.format);
    m_TextureDesc.layerOffset = 0;
    m_TextureDesc.layerNum = 1;
    m_TextureDesc.sliceOffset = textureViewDesc.sliceOffset;
    m_TextureDesc.sliceNum = (Dim_t)slicesInfo.sliceCount;
    m_TextureDesc.mipOffset = textureViewDesc.mipOffset;
    m_TextureDesc.mipNum = (Mip_t)subresource.levelCount;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferVK& buffer = *(const BufferVK*)bufferViewDesc.buffer;

    m_Type = DescriptorTypeVK::BUFFER_VIEW;
    m_BufferDesc.offset = bufferViewDesc.offset;
    m_BufferDesc.size = (bufferViewDesc.size == WHOLE_SIZE) ? VK_WHOLE_SIZE : bufferViewDesc.size;
    m_BufferDesc.handle = buffer.GetHandle();
    m_BufferDesc.viewType = bufferViewDesc.viewType;

    if (bufferViewDesc.format == Format::UNKNOWN)
        return Result::SUCCESS;

    VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    createInfo.flags = (VkBufferViewCreateFlags)0;
    createInfo.buffer = buffer.GetHandle();
    createInfo.format = GetVkFormat(bufferViewDesc.format);
    createInfo.offset = bufferViewDesc.offset;
    createInfo.range = m_BufferDesc.size;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateBufferView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_BufferView);
    RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateBufferView returned %d", (int32_t)vkResult);

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const SamplerDesc& samplerDesc) {
    VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    info.flags = (VkSamplerCreateFlags)0;
    info.magFilter = GetFilter(samplerDesc.filters.mag);
    info.minFilter = GetFilter(samplerDesc.filters.min);
    info.mipmapMode = GetSamplerMipmapMode(samplerDesc.filters.mip);
    info.addressModeU = GetSamplerAddressMode(samplerDesc.addressModes.u);
    info.addressModeV = GetSamplerAddressMode(samplerDesc.addressModes.v);
    info.addressModeW = GetSamplerAddressMode(samplerDesc.addressModes.w);
    info.mipLodBias = samplerDesc.mipBias;
    info.anisotropyEnable = VkBool32(samplerDesc.anisotropy > 1.0f);
    info.maxAnisotropy = (float)samplerDesc.anisotropy;
    info.compareEnable = VkBool32(samplerDesc.compareFunc != CompareFunc::NONE);
    info.compareOp = GetCompareOp(samplerDesc.compareFunc);
    info.minLod = samplerDesc.mipMin;
    info.maxLod = samplerDesc.mipMax;

    VkSamplerCustomBorderColorCreateInfoEXT borderColorInfo = {VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT};
    if (m_Device.m_IsSupported.customBorderColor) {
        info.pNext = &borderColorInfo;
        info.borderColor = samplerDesc.isInteger ? VK_BORDER_COLOR_INT_CUSTOM_EXT : VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;

        static_assert(sizeof(VkClearColorValue) == sizeof(samplerDesc.borderColor), "Unexpected sizeof");
        memcpy(&borderColorInfo.customBorderColor, &samplerDesc.borderColor, sizeof(borderColorInfo.customBorderColor));
    }

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateSampler(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Sampler);
    RETURN_ON_FAILURE(&m_Device, vkResult == VK_SUCCESS, GetReturnCode(vkResult), "vkCreateSampler returned %d", (int32_t)vkResult);

    m_Type = DescriptorTypeVK::SAMPLER;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(VkAccelerationStructureKHR accelerationStructure) {
    m_AccelerationStructure = accelerationStructure;
    m_Type = DescriptorTypeVK::ACCELERATION_STRUCTURE;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const Texture1DViewDesc& textureViewDesc) {
    return CreateTextureView(textureViewDesc);
}

Result DescriptorVK::Create(const Texture2DViewDesc& textureViewDesc) {
    return CreateTextureView(textureViewDesc);
}

Result DescriptorVK::Create(const Texture3DViewDesc& textureViewDesc) {
    return CreateTextureView(textureViewDesc);
}

NRI_INLINE void DescriptorVK::SetDebugName(const char* name) {
    switch (m_Type) {
        case DescriptorTypeVK::BUFFER_VIEW:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_BUFFER_VIEW, (uint64_t)m_BufferView, name);
            break;

        case DescriptorTypeVK::IMAGE_VIEW:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_ImageView, name);
            break;

        case DescriptorTypeVK::SAMPLER:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SAMPLER, (uint64_t)m_Sampler, name);
            break;

        case DescriptorTypeVK::ACCELERATION_STRUCTURE:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)m_AccelerationStructure, name);
            break;

        default:
            CHECK(false, "unexpected descriptor type");
            break;
    }
}
