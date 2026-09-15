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

static inline VkComponentSwizzle GetComponentSwizzle(ComponentSwizzle componentSwizzle) {
    return (VkComponentSwizzle)componentSwizzle;
}

enum SamplerDescBits : uint32_t {
    SAMPLER_MAG_FILTER_SHIFT = 0,
    SAMPLER_MIN_FILTER_SHIFT = 1,
    SAMPLER_MIP_FILTER_SHIFT = 2,
    SAMPLER_FILTER_OP_SHIFT = 3,
    SAMPLER_ANISOTROPY_SHIFT = 5,
    SAMPLER_ADDRESS_U_SHIFT = 13,
    SAMPLER_ADDRESS_V_SHIFT = 16,
    SAMPLER_ADDRESS_W_SHIFT = 19,
    SAMPLER_COMPARE_OP_SHIFT = 22,
    SAMPLER_IS_INTEGER_SHIFT = 26,
    SAMPLER_UNNORMALIZED_COORDINATES_SHIFT = 27,
    SAMPLER_BORDER_COLOR_SHIFT = 28,
};

static inline uint32_t PackSamplerDesc(const SamplerDesc& samplerDesc, VkBorderColor borderColor) {
    uint32_t borderColorIndex = 0;
    if (borderColor == VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK || borderColor == VK_BORDER_COLOR_INT_OPAQUE_BLACK)
        borderColorIndex = 1;
    else if (borderColor == VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE || borderColor == VK_BORDER_COLOR_INT_OPAQUE_WHITE)
        borderColorIndex = 2;
    else if (borderColor == VK_BORDER_COLOR_FLOAT_CUSTOM_EXT || borderColor == VK_BORDER_COLOR_INT_CUSTOM_EXT)
        borderColorIndex = 3;

    return ((uint32_t)samplerDesc.filters.mag << SAMPLER_MAG_FILTER_SHIFT)
        | ((uint32_t)samplerDesc.filters.min << SAMPLER_MIN_FILTER_SHIFT)
        | ((uint32_t)samplerDesc.filters.mip << SAMPLER_MIP_FILTER_SHIFT)
        | ((uint32_t)samplerDesc.filters.op << SAMPLER_FILTER_OP_SHIFT)
        | ((uint32_t)samplerDesc.anisotropy << SAMPLER_ANISOTROPY_SHIFT)
        | ((uint32_t)samplerDesc.addressModes.u << SAMPLER_ADDRESS_U_SHIFT)
        | ((uint32_t)samplerDesc.addressModes.v << SAMPLER_ADDRESS_V_SHIFT)
        | ((uint32_t)samplerDesc.addressModes.w << SAMPLER_ADDRESS_W_SHIFT)
        | ((uint32_t)samplerDesc.compareOp << SAMPLER_COMPARE_OP_SHIFT)
        | ((uint32_t)samplerDesc.isInteger << SAMPLER_IS_INTEGER_SHIFT)
        | ((uint32_t)samplerDesc.unnormalizedCoordinates << SAMPLER_UNNORMALIZED_COORDINATES_SHIFT)
        | (borderColorIndex << SAMPLER_BORDER_COLOR_SHIFT);
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
            if (m_View.image) {
                m_Device.DestroyFramebuffers(m_View.image);
                vk.DestroyImageView(m_Device, m_View.image, m_Device.GetVkAllocationCallbacks());
            }
            break;
    }
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
    if (textureViewDesc.type == TextureView::COLOR_ATTACHMENT && (textureDesc.usage & TextureUsageBits::INPUT_ATTACHMENT))
        usageInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

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
    m_TextureView = textureViewDesc.type;
    m_TextureComponents = textureViewDesc.components;

    m_ViewDesc.texture = {};
    m_ViewDesc.texture.texture = &textureVK;
    m_ViewDesc.texture.expectedLayout = expectedLayout;
    m_ViewDesc.texture.aspectMask = subresourceRange.aspectMask;
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
    m_ViewDesc.buffer.buffer = &bufferVK;
    m_ViewDesc.buffer.offset = bufferViewDesc.offset;
    m_ViewDesc.buffer.range = (bufferViewDesc.size == WHOLE_SIZE) ? (bufferDesc.size - bufferViewDesc.offset) : bufferViewDesc.size;

    if (m_Format != Format::UNKNOWN) {
        VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
        createInfo.flags = (VkBufferViewCreateFlags)0;
        createInfo.buffer = bufferVK.GetHandle();
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

    m_ViewDesc.sampler = {};
    m_ViewDesc.sampler.mipBias = samplerDesc.mipBias;
    m_ViewDesc.sampler.mipMin = samplerDesc.mipMin;
    m_ViewDesc.sampler.mipMax = samplerDesc.mipMax;
    m_ViewDesc.sampler.packed = PackSamplerDesc(samplerDesc, info.borderColor);
    m_ViewDesc.sampler.customBorderColor = samplerDesc.borderColor;

    m_Type = DescriptorType::SAMPLER;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const AccelerationStructureVK& accelerationStructure) {
    m_Type = DescriptorType::ACCELERATION_STRUCTURE;
    m_View.accelerationStructure = accelerationStructure.GetHandle();
    m_ViewDesc.accelerationStructureAddress = accelerationStructure.GetDeviceAddress();

    return Result::SUCCESS;
}

void DescriptorVK::FillImageViewCreateInfo(VkImageViewCreateInfo& createInfo, VkImageViewUsageCreateInfo& usageInfo, VkImageViewSlicedCreateInfoEXT& slicesInfo) const {
    const TextureVK& texture = *m_ViewDesc.texture.texture;
    const TextureDesc& textureDesc = texture.GetDesc();

    slicesInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT};
    slicesInfo.sliceOffset = m_ViewDesc.texture.layerOrSliceOffset;
    slicesInfo.sliceCount = m_ViewDesc.texture.layerOrSliceNum;

    usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(m_TextureView);
    if (m_TextureView == TextureView::COLOR_ATTACHMENT && (textureDesc.usage & TextureUsageBits::INPUT_ATTACHMENT))
        usageInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (textureDesc.type == TextureType::TEXTURE_3D && m_Device.m_IsSupported.imageSlicedView)
        usageInfo.pNext = &slicesInfo;

    createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.image = texture.GetHandle();
    createInfo.viewType = GetImageViewType(textureDesc.type, m_TextureView, m_ViewDesc.texture.layerOrSliceNum);
    createInfo.format = GetVkFormat(m_Format);
    createInfo.components.r = GetComponentSwizzle(m_TextureComponents.r);
    createInfo.components.g = GetComponentSwizzle(m_TextureComponents.g);
    createInfo.components.b = GetComponentSwizzle(m_TextureComponents.b);
    createInfo.components.a = GetComponentSwizzle(m_TextureComponents.a);
    createInfo.subresourceRange.aspectMask = m_ViewDesc.texture.aspectMask;
    createInfo.subresourceRange.baseMipLevel = m_ViewDesc.texture.mipOffset;
    createInfo.subresourceRange.levelCount = m_ViewDesc.texture.mipNum;
    if (textureDesc.type != TextureType::TEXTURE_3D) {
        createInfo.subresourceRange.baseArrayLayer = m_ViewDesc.texture.layerOrSliceOffset;
        createInfo.subresourceRange.layerCount = m_ViewDesc.texture.layerOrSliceNum;
    } else
        createInfo.subresourceRange.layerCount = 1;
}

void DescriptorVK::FillSamplerDesc(SamplerDesc& samplerDesc) const {
    const SamplerViewDescVK& packedDesc = m_ViewDesc.sampler;
    const uint32_t packed = packedDesc.packed;

    samplerDesc = {};
    samplerDesc.filters.mag = (Filter)((packed >> SAMPLER_MAG_FILTER_SHIFT) & 0x1);
    samplerDesc.filters.min = (Filter)((packed >> SAMPLER_MIN_FILTER_SHIFT) & 0x1);
    samplerDesc.filters.mip = (Filter)((packed >> SAMPLER_MIP_FILTER_SHIFT) & 0x1);
    samplerDesc.filters.op = (FilterOp)((packed >> SAMPLER_FILTER_OP_SHIFT) & 0x3);
    samplerDesc.anisotropy = (uint8_t)((packed >> SAMPLER_ANISOTROPY_SHIFT) & 0xFF);
    samplerDesc.mipBias = packedDesc.mipBias;
    samplerDesc.mipMin = packedDesc.mipMin;
    samplerDesc.mipMax = packedDesc.mipMax;
    samplerDesc.addressModes.u = (AddressMode)((packed >> SAMPLER_ADDRESS_U_SHIFT) & 0x7);
    samplerDesc.addressModes.v = (AddressMode)((packed >> SAMPLER_ADDRESS_V_SHIFT) & 0x7);
    samplerDesc.addressModes.w = (AddressMode)((packed >> SAMPLER_ADDRESS_W_SHIFT) & 0x7);
    samplerDesc.compareOp = (CompareOp)((packed >> SAMPLER_COMPARE_OP_SHIFT) & 0xF);
    samplerDesc.isInteger = ((packed >> SAMPLER_IS_INTEGER_SHIFT) & 0x1) != 0;
    samplerDesc.unnormalizedCoordinates = ((packed >> SAMPLER_UNNORMALIZED_COORDINATES_SHIFT) & 0x1) != 0;

    const uint32_t borderColorIndex = (packed >> SAMPLER_BORDER_COLOR_SHIFT) & 0x3;
    if (borderColorIndex == 3)
        samplerDesc.borderColor = packedDesc.customBorderColor;
    else {
        const uint32_t rgb = borderColorIndex == 2;
        const uint32_t alpha = borderColorIndex != 0;

        if (samplerDesc.isInteger)
            samplerDesc.borderColor.ui = {rgb, rgb, rgb, alpha};
        else
            samplerDesc.borderColor.f = {(float)rgb, (float)rgb, (float)rgb, (float)alpha};
    }
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
