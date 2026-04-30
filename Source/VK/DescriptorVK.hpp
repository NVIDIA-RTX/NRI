// © 2021 NVIDIA Corporation

static inline VkImageViewType GetImageViewType(TextureType textureType, TextureView textureView, uint32_t layerNum) {
    if (textureType == TextureType::TEXTURE_1D) {
        switch (textureView) {
            case TextureView::TEXTURE:
            case TextureView::STORAGE_TEXTURE:
                return VK_IMAGE_VIEW_TYPE_1D;

            case TextureView::TEXTURE_ARRAY:
            case TextureView::STORAGE_TEXTURE_ARRAY:
                return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

            default:
                return layerNum > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D; // honor layered rendering
        }
    } else if (textureType == TextureType::TEXTURE_2D) {
        switch (textureView) {
            case TextureView::TEXTURE:
            case TextureView::STORAGE_TEXTURE:
                return VK_IMAGE_VIEW_TYPE_2D;

            case TextureView::TEXTURE_ARRAY:
            case TextureView::STORAGE_TEXTURE_ARRAY:
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

            case TextureView::TEXTURE_CUBE:
                return VK_IMAGE_VIEW_TYPE_CUBE;

            case TextureView::TEXTURE_CUBE_ARRAY:
                return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

            default:
                return layerNum > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D; // honor layered rendering
        }
    } else
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

Result DescriptorVK::Create(const TextureViewDesc& textureViewDesc) {
    const TextureVK& textureVK = *(TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = textureVK.GetDesc();
    const FormatProps& formatProps = GetFormatProps(textureViewDesc.format);
    Dim_t mipNum = textureViewDesc.mipNum == REMAINING ? (textureDesc.mipNum - textureViewDesc.mipOffset) : textureViewDesc.mipNum;
    Dim_t layerNum = textureViewDesc.layerNum == REMAINING ? (textureDesc.layerNum - textureViewDesc.layerOffset) : textureViewDesc.layerNum;
    Dim_t sliceNum = textureViewDesc.sliceNum == REMAINING ? (textureDesc.depth - textureViewDesc.sliceOffset) : textureViewDesc.sliceNum;

    VkImageViewSlicedCreateInfoEXT slicesInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT};
    slicesInfo.sliceOffset = textureViewDesc.sliceOffset;
    slicesInfo.sliceCount = sliceNum;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.type);

    if (textureDesc.type == TextureType::TEXTURE_3D && m_Device.m_IsSupported.imageSlicedView)
        usageInfo.pNext = &slicesInfo;

    // For attachments all format-enabled aspects are needed to support mixed R/RW layouts.
    // For shader resources a specific set of planes is needed (like depth-only or stencil-only views)
    const bool isAspectSensitiveAttachment = textureViewDesc.type == TextureView::DEPTH_STENCIL_ATTACHMENT || textureViewDesc.type == TextureView::SUBPASS_INPUT;
    const VkImageAspectFlags aspectMask = GetImageAspectFlags(isAspectSensitiveAttachment ? PlaneBits::ALL : textureViewDesc.planes, textureViewDesc.format);

    VkImageSubresourceRange subresourceRange = {
        aspectMask,
        textureViewDesc.mipOffset,
        mipNum,
        textureViewDesc.layerOffset,
        layerNum,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.image = textureVK.GetHandle();
    createInfo.viewType = GetImageViewType(textureDesc.type, textureViewDesc.type, subresourceRange.layerCount);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresourceRange;
    createInfo.components.r = GetComponentSwizzle(textureViewDesc.components.r);
    createInfo.components.g = GetComponentSwizzle(textureViewDesc.components.g);
    createInfo.components.b = GetComponentSwizzle(textureViewDesc.components.b);
    createInfo.components.a = GetComponentSwizzle(textureViewDesc.components.a);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_View.image);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

    // Handle mixed R/RW depth-stencil specific layouts
    VkImageLayout expectedLayout = GetImageViewLayout(textureViewDesc.type);
    if (isAspectSensitiveAttachment && (formatProps.isDepth || formatProps.isStencil)) {
        bool isDepthReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::DEPTH) == 0;
        bool isStencilReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::STENCIL) == 0;

        if (isDepthReadonly && isStencilReadonly)
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else if (isDepthReadonly)
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        else if (isStencilReadonly)
            expectedLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    }

    m_Type = GetImageViewDescriptorType(textureViewDesc.type);
    m_Format = textureViewDesc.format;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.texture = &textureVK;
    m_ViewDesc.texture.expectedLayout = expectedLayout;
    m_ViewDesc.texture.layerOrSliceOffset = textureDesc.type == TextureType::TEXTURE_3D ? textureViewDesc.sliceOffset : textureViewDesc.layerOffset;
    m_ViewDesc.texture.layerOrSliceNum = textureDesc.type == TextureType::TEXTURE_3D ? sliceNum : layerNum;
    m_ViewDesc.texture.mipOffset = textureViewDesc.mipOffset;
    m_ViewDesc.texture.mipNum = mipNum;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferVK& bufferVK = *(const BufferVK*)bufferViewDesc.buffer;
    const BufferDesc& bufferDesc = bufferVK.GetDesc();

    switch (bufferViewDesc.type) {
        case BufferView::BUFFER:
            m_Type = DescriptorType::BUFFER;
            break;
        case BufferView::STRUCTURED_BUFFER:
        case BufferView::BYTE_ADDRESS_BUFFER:
            m_Type = DescriptorType::STRUCTURED_BUFFER;
            break;
        case BufferView::STORAGE_BUFFER:
            m_Type = DescriptorType::STORAGE_BUFFER;
            break;
        case BufferView::STORAGE_STRUCTURED_BUFFER:
        case BufferView::STORAGE_BYTE_ADDRESS_BUFFER:
            m_Type = DescriptorType::STORAGE_STRUCTURED_BUFFER;
            break;
        case BufferView::CONSTANT_BUFFER:
            m_Type = DescriptorType::CONSTANT_BUFFER;
            break;
        default:
            NRI_CHECK(false, "Unexpected 'bufferViewDesc.type'");
    }

    if (bufferViewDesc.type == BufferView::BUFFER || bufferViewDesc.type == BufferView::STORAGE_BUFFER)
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
