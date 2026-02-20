// © 2021 NVIDIA Corporation

constexpr VkImageViewType GetImageViewType(Texture1DViewType type, uint32_t layerNum) {
    switch (type) {
        case Texture1DViewType::SHADER_RESOURCE:
        case Texture1DViewType::SHADER_RESOURCE_STORAGE:
            return VK_IMAGE_VIEW_TYPE_1D;

        case Texture1DViewType::SHADER_RESOURCE_ARRAY:
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_ARRAY:
            return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

        default:
            return layerNum > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D; // honor layered rendering
    }
}

constexpr VkImageViewType GetImageViewType(Texture2DViewType type, uint32_t layerNum) {
    switch (type) {
        case Texture2DViewType::SHADER_RESOURCE:
        case Texture2DViewType::SHADER_RESOURCE_STORAGE:
            return VK_IMAGE_VIEW_TYPE_2D;

        case Texture2DViewType::SHADER_RESOURCE_ARRAY:
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_ARRAY:
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
        case DescriptorType::SAMPLER:
            if (m_View.sampler)
                vk.DestroySampler(m_Device, m_View.sampler, m_Device.GetVkAllocationCallbacks());
            break;
        case DescriptorType::BUFFER:
        case DescriptorType::STORAGE_BUFFER:
        case DescriptorType::CONSTANT_BUFFER:
        case DescriptorType::STRUCTURED_BUFFER:
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            if (m_View.buffer)
                vk.DestroyBufferView(m_Device, m_View.buffer, m_Device.GetVkAllocationCallbacks());
            break;
        case DescriptorType::ACCELERATION_STRUCTURE:
            // skip
            break;
        default: // all textures (including HOST only)
            if (m_View.image)
                vk.DestroyImageView(m_Device, m_View.image, m_Device.GetVkAllocationCallbacks());
            break;
    }
}

static inline VkComponentSwizzle GetComponentSwizzle(ComponentSwizzle componentSwizzle) {
    return (VkComponentSwizzle)componentSwizzle;
}

Result DescriptorVK::CreateTextureView(const Texture1DViewDesc& textureViewDesc) {
    const TextureVK& textureVK = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureVK.GetDesc();
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    VkImageSubresourceRange subresourceRange = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        mipNum,
        textureViewDesc.layerOffset,
        layerNum,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.image = textureVK.GetHandle();
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType, subresourceRange.layerCount);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresourceRange;
    createInfo.components.r = GetComponentSwizzle(textureViewDesc.components.r);
    createInfo.components.g = GetComponentSwizzle(textureViewDesc.components.g);
    createInfo.components.b = GetComponentSwizzle(textureViewDesc.components.b);
    createInfo.components.a = GetComponentSwizzle(textureViewDesc.components.a);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_View.image);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

    VkImageLayout expectedLayout = GetImageLayoutForView(textureViewDesc.viewType);
    if (textureViewDesc.viewType == Texture1DViewType::DEPTH_STENCIL_ATTACHMENT) {
        if (textureViewDesc.readonlyPlanes == (PlaneBits::DEPTH | PlaneBits::STENCIL))
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else if (textureViewDesc.readonlyPlanes == PlaneBits::DEPTH)
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        else if (textureViewDesc.readonlyPlanes == PlaneBits::STENCIL)
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    }

    m_Type = GetDescriptorTypeForView(textureViewDesc.viewType);
    m_Format = textureViewDesc.format;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.texture = &textureVK;
    m_ViewDesc.texture.expectedLayout = expectedLayout;
    m_ViewDesc.texture.layerOrSliceOffset = textureViewDesc.layerOffset;
    m_ViewDesc.texture.layerOrSliceNum = layerNum;
    m_ViewDesc.texture.mipOffset = textureViewDesc.mipOffset;
    m_ViewDesc.texture.mipNum = mipNum;

    return Result::SUCCESS;
}

Result DescriptorVK::CreateTextureView(const Texture2DViewDesc& textureViewDesc) {
    const TextureVK& textureVK = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureVK.GetDesc();
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    VkImageSubresourceRange subresourceRange = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        mipNum,
        textureViewDesc.layerOffset,
        layerNum,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.image = textureVK.GetHandle();
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType, subresourceRange.layerCount);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresourceRange;
    createInfo.components.r = GetComponentSwizzle(textureViewDesc.components.r);
    createInfo.components.g = GetComponentSwizzle(textureViewDesc.components.g);
    createInfo.components.b = GetComponentSwizzle(textureViewDesc.components.b);
    createInfo.components.a = GetComponentSwizzle(textureViewDesc.components.a);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_View.image);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

    VkImageLayout expectedLayout = GetImageLayoutForView(textureViewDesc.viewType);
    if (textureViewDesc.viewType == Texture2DViewType::DEPTH_STENCIL_ATTACHMENT) {
        if (textureViewDesc.readonlyPlanes == (PlaneBits::DEPTH | PlaneBits::STENCIL))
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else if (textureViewDesc.readonlyPlanes == PlaneBits::DEPTH)
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        else if (textureViewDesc.readonlyPlanes == PlaneBits::STENCIL)
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    }

    m_Type = GetDescriptorTypeForView(textureViewDesc.viewType);
    m_Format = textureViewDesc.format;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.texture = &textureVK;
    m_ViewDesc.texture.expectedLayout = expectedLayout;
    m_ViewDesc.texture.layerOrSliceOffset = textureViewDesc.layerOffset;
    m_ViewDesc.texture.layerOrSliceNum = layerNum;
    m_ViewDesc.texture.mipOffset = textureViewDesc.mipOffset;
    m_ViewDesc.texture.mipNum = mipNum;

    return Result::SUCCESS;
}

Result DescriptorVK::CreateTextureView(const Texture3DViewDesc& textureViewDesc) {
    const TextureVK& textureVK = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureVK.GetDesc();
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t sliceNum = textureViewDesc.sliceNum == REMAINING ? (textureDesc.depth - textureViewDesc.sliceOffset) : textureViewDesc.sliceNum;

    VkImageViewSlicedCreateInfoEXT slicesInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT};
    slicesInfo.sliceOffset = textureViewDesc.sliceOffset;
    slicesInfo.sliceCount = sliceNum;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    if (m_Device.m_IsSupported.imageSlicedView)
        usageInfo.pNext = &slicesInfo;

    VkImageSubresourceRange subresourceRange = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        mipNum,
        0,
        1,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.image = textureVK.GetHandle();
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType, subresourceRange.layerCount);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresourceRange;
    createInfo.components.r = GetComponentSwizzle(textureViewDesc.components.r);
    createInfo.components.g = GetComponentSwizzle(textureViewDesc.components.g);
    createInfo.components.b = GetComponentSwizzle(textureViewDesc.components.b);
    createInfo.components.a = GetComponentSwizzle(textureViewDesc.components.a);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_View.image);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

    m_Type = GetDescriptorTypeForView(textureViewDesc.viewType);
    m_Format = textureViewDesc.format;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.texture = &textureVK;
    m_ViewDesc.texture.expectedLayout = GetImageLayoutForView(textureViewDesc.viewType);
    m_ViewDesc.texture.layerOrSliceOffset = textureViewDesc.sliceOffset;
    m_ViewDesc.texture.layerOrSliceNum = sliceNum;
    m_ViewDesc.texture.mipOffset = textureViewDesc.mipOffset;
    m_ViewDesc.texture.mipNum = mipNum;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferVK& bufferVK = *(const BufferVK*)bufferViewDesc.buffer;
    const BufferDesc& bufferDesc = bufferVK.GetDesc();

    switch (bufferViewDesc.viewType) {
        case BufferViewType::SHADER_RESOURCE:
            m_Type = DescriptorType::BUFFER;
            break;
        case BufferViewType::SHADER_RESOURCE_STRUCTURED:
        case BufferViewType::SHADER_RESOURCE_BYTE_ADDRESS:
            m_Type = DescriptorType::STRUCTURED_BUFFER;
            break;
        case BufferViewType::SHADER_RESOURCE_STORAGE:
            m_Type = DescriptorType::STORAGE_BUFFER;
            break;
        case BufferViewType::SHADER_RESOURCE_STORAGE_STRUCTURED:
        case BufferViewType::SHADER_RESOURCE_STORAGE_BYTE_ADDRESS:
            m_Type = DescriptorType::STORAGE_STRUCTURED_BUFFER;
            break;
        case BufferViewType::CONSTANT:
            m_Type = DescriptorType::CONSTANT_BUFFER;
            break;
        default:
            NRI_CHECK(false, "unexpected 'bufferViewDesc.viewType'");
    }

    if (bufferViewDesc.viewType == BufferViewType::SHADER_RESOURCE || bufferViewDesc.viewType == BufferViewType::SHADER_RESOURCE_STORAGE)
        m_Format = bufferViewDesc.format;
    else
        m_Format = Format::UNKNOWN;

    m_ViewDesc.buffer = {};
    m_ViewDesc.buffer.buffer = bufferVK.GetHandle();
    m_ViewDesc.buffer.offset = bufferViewDesc.offset;
    m_ViewDesc.buffer.range = (bufferViewDesc.size == WHOLE_SIZE) ? bufferDesc.size : bufferViewDesc.size;

    if (m_Format != Format::UNKNOWN) {
        VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
        createInfo.flags = (VkBufferViewCreateFlags)0;
        createInfo.buffer = m_ViewDesc.buffer.buffer;
        createInfo.format = GetVkFormat(bufferViewDesc.format);
        createInfo.offset = m_ViewDesc.buffer.offset;
        createInfo.range = m_ViewDesc.buffer.range;

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.CreateBufferView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_View.buffer);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateBufferView");
    }

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const SamplerDesc& samplerDesc) {
    VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    VkSamplerReductionModeCreateInfo reductionModeInfo = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO};
    VkSamplerCustomBorderColorCreateInfoEXT borderColorInfo = {VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT};
    m_Device.FillCreateInfo(samplerDesc, info, reductionModeInfo, borderColorInfo);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateSampler(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_View.sampler);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateSampler");

    m_Type = DescriptorType::SAMPLER;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(VkAccelerationStructureKHR accelerationStructure) {
    m_Type = DescriptorType::ACCELERATION_STRUCTURE;
    m_View.accelerationStructure = accelerationStructure;

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
        case DescriptorType::SAMPLER:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SAMPLER, (uint64_t)m_View.sampler, name);
            break;
        case DescriptorType::BUFFER:
        case DescriptorType::STORAGE_BUFFER:
        case DescriptorType::CONSTANT_BUFFER:
        case DescriptorType::STRUCTURED_BUFFER:
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_BUFFER_VIEW, (uint64_t)m_View.buffer, name);
            break;
        case DescriptorType::ACCELERATION_STRUCTURE:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)m_View.accelerationStructure, name);
            break;
        default: // all textures (including HOST only)
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_View.image, name);
            break;
    }
}
