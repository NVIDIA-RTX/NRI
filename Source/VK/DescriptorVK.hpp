// © 2021 NVIDIA Corporation

constexpr VkImageViewType GetImageViewType(Texture1DViewType type, uint32_t layerNum) {
    switch (type) {
        case Texture1DViewType::SHADER_RESOURCE_1D:
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D:
            return VK_IMAGE_VIEW_TYPE_1D;

        case Texture1DViewType::SHADER_RESOURCE_1D_ARRAY:
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D_ARRAY:
            return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

        default:
            return layerNum > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D; // honor layered rendering
    }
}

constexpr VkImageViewType GetImageViewType(Texture2DViewType type, uint32_t layerNum) {
    switch (type) {
        case Texture2DViewType::SHADER_RESOURCE_2D:
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D:
            return VK_IMAGE_VIEW_TYPE_2D;

        case Texture2DViewType::SHADER_RESOURCE_2D_ARRAY:
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D_ARRAY:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

        case Texture2DViewType::SHADER_RESOURCE_CUBE:
            return VK_IMAGE_VIEW_TYPE_CUBE;

        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY:
            return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

        default:
            return layerNum > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D; // honor layered rendering
    }
}

constexpr VkImageViewType GetImageViewType(Texture3DViewType, uint32_t) {
    return VK_IMAGE_VIEW_TYPE_3D;
}

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
    Dim_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;
    Dim_t remainingLayers = textureDesc.layerNum - textureViewDesc.layerOffset;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    VkImageSubresourceRange subresource = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        textureViewDesc.mipNum == REMAINING ? remainingMips : textureViewDesc.mipNum,
        textureViewDesc.layerOffset,
        textureViewDesc.layerNum == REMAINING ? remainingLayers : textureViewDesc.layerNum,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType, subresource.layerCount);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresource;
    createInfo.image = texture.GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_ImageView);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

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
    m_TextureDesc.mipNum = (Dim_t)subresource.levelCount;

    return Result::SUCCESS;
}

template <>
Result DescriptorVK::CreateTextureView(const Texture3DViewDesc& textureViewDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    Dim_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;
    Dim_t remainingLayers = textureDesc.layerNum - textureViewDesc.sliceOffset;

    VkImageViewSlicedCreateInfoEXT slicesInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT};
    slicesInfo.sliceOffset = textureViewDesc.sliceOffset;
    slicesInfo.sliceCount = textureViewDesc.sliceNum == REMAINING ? remainingLayers : textureViewDesc.sliceNum;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    if (m_Device.m_IsSupported.imageSlicedView)
        usageInfo.pNext = &slicesInfo;

    VkImageSubresourceRange subresource = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        textureViewDesc.mipNum == REMAINING ? remainingMips : textureViewDesc.mipNum,
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
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

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
    m_TextureDesc.mipNum = (Dim_t)subresource.levelCount;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferVK& buffer = *(const BufferVK*)bufferViewDesc.buffer;
    const BufferDesc& bufferDesc = buffer.GetDesc();

    m_Type = DescriptorTypeVK::BUFFER_VIEW;
    m_BufferDesc.offset = bufferViewDesc.offset;
    m_BufferDesc.size = (bufferViewDesc.size == WHOLE_SIZE) ? bufferDesc.size : bufferViewDesc.size;
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
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateBufferView");

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const SamplerDesc& samplerDesc) {
    VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    VkSamplerReductionModeCreateInfo reductionModeInfo = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO};
    VkSamplerCustomBorderColorCreateInfoEXT borderColorInfo = {VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT};
    m_Device.FillCreateInfo(samplerDesc, info, reductionModeInfo, borderColorInfo);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateSampler(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Sampler);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateSampler");

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
