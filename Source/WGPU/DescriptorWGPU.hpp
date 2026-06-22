// © 2026 NVIDIA Corporation

DescriptorWGPU::~DescriptorWGPU() {
    ReleaseTextureView();

    if (m_Sampler)
        wgpuSamplerRelease(m_Sampler);
}

void DescriptorWGPU::ReleaseTextureView() {
    if (m_TextureView)
        wgpuTextureViewRelease(m_TextureView);

    m_TextureView = nullptr;
}

Result DescriptorWGPU::Create(const SamplerDesc& samplerDesc) {
    WGPUSamplerDescriptor desc = WGPU_SAMPLER_DESCRIPTOR_INIT;
    desc.addressModeU = GetAddressMode(samplerDesc.addressModes.u);
    desc.addressModeV = GetAddressMode(samplerDesc.addressModes.v);
    desc.addressModeW = GetAddressMode(samplerDesc.addressModes.w);
    desc.magFilter = GetFilterMode(samplerDesc.filters.mag);
    desc.minFilter = GetFilterMode(samplerDesc.filters.min);
    desc.mipmapFilter = GetMipmapFilterMode(samplerDesc.filters.mip);
    desc.lodMinClamp = samplerDesc.mipMin;
    desc.lodMaxClamp = samplerDesc.mipMax == 0.0f ? 1000.0f : samplerDesc.mipMax;
    desc.compare = GetCompareFunction(samplerDesc.compareOp);
    desc.maxAnisotropy = std::max<uint16_t>(samplerDesc.anisotropy, 1);

    m_Sampler = wgpuDeviceCreateSampler(m_Device, &desc);
    m_DescriptorType = DescriptorType::SAMPLER;

    return m_Sampler ? Result::SUCCESS : Result::FAILURE;
}

Result DescriptorWGPU::Create(const BufferViewDesc& bufferViewDesc) {
    BufferWGPU& buffer = *(BufferWGPU*)bufferViewDesc.buffer;

    m_Buffer = buffer;
    m_Offset = bufferViewDesc.offset;
    m_Size = bufferViewDesc.size == WHOLE_SIZE ? buffer.GetSize() - bufferViewDesc.offset : bufferViewDesc.size;
    m_BufferFormat = bufferViewDesc.format;

    switch (bufferViewDesc.type) {
        case BufferView::CONSTANT_BUFFER:
            m_DescriptorType = DescriptorType::CONSTANT_BUFFER;
            break;
        case BufferView::STORAGE_BUFFER:
        case BufferView::STORAGE_STRUCTURED_BUFFER:
        case BufferView::STORAGE_BYTE_ADDRESS_BUFFER:
            m_DescriptorType = DescriptorType::STORAGE_BUFFER;
            break;
        case BufferView::STRUCTURED_BUFFER:
        case BufferView::BYTE_ADDRESS_BUFFER:
            m_DescriptorType = DescriptorType::STRUCTURED_BUFFER;
            break;
        default:
            m_DescriptorType = DescriptorType::BUFFER;
            break;
    }

    return Result::SUCCESS;
}

Result DescriptorWGPU::Create(const TextureViewDesc& textureViewDesc) {
    m_Texture = (TextureWGPU*)textureViewDesc.texture;
    m_TextureViewDesc = textureViewDesc;

    switch (textureViewDesc.type) {
        case TextureView::STORAGE_TEXTURE:
        case TextureView::STORAGE_TEXTURE_ARRAY:
            m_DescriptorType = DescriptorType::STORAGE_TEXTURE;
            break;
        case TextureView::COLOR_ATTACHMENT:
        case TextureView::DEPTH_STENCIL_ATTACHMENT:
            m_DescriptorType = DescriptorType::MUTABLE;
            break;
        default:
            m_DescriptorType = DescriptorType::TEXTURE;
            break;
    }

    return Result::SUCCESS;
}

WGPUTextureView DescriptorWGPU::GetTextureView() {
    if (!m_Texture)
        return nullptr;

    if (m_TextureView && m_TextureVersion == m_Texture->GetVersion())
        return m_TextureView;

    ReleaseTextureView();

    const TextureDesc& textureDesc = m_Texture->GetDesc();

    WGPUTextureViewDescriptor desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    desc.format = GetTextureFormat(m_TextureViewDesc.format == Format::UNKNOWN ? textureDesc.format : m_TextureViewDesc.format);
    desc.dimension = GetTextureViewDimension(m_TextureViewDesc.type, textureDesc);
    desc.baseMipLevel = m_TextureViewDesc.mipOffset;
    desc.mipLevelCount = m_TextureViewDesc.mipNum == REMAINING ? WGPU_MIP_LEVEL_COUNT_UNDEFINED : m_TextureViewDesc.mipNum;
    desc.baseArrayLayer = m_TextureViewDesc.layerOffset;
    desc.arrayLayerCount = m_TextureViewDesc.layerNum == REMAINING ? WGPU_ARRAY_LAYER_COUNT_UNDEFINED : m_TextureViewDesc.layerNum;
    desc.aspect = GetTextureAspect(m_TextureViewDesc.planes);

    WGPUTextureComponentSwizzleDescriptor swizzleDesc = WGPU_TEXTURE_COMPONENT_SWIZZLE_DESCRIPTOR_INIT;
    if (m_DescriptorType != DescriptorType::STORAGE_TEXTURE) {
        swizzleDesc.swizzle.r = GetComponentSwizzle(m_TextureViewDesc.components.r);
        swizzleDesc.swizzle.g = GetComponentSwizzle(m_TextureViewDesc.components.g);
        swizzleDesc.swizzle.b = GetComponentSwizzle(m_TextureViewDesc.components.b);
        swizzleDesc.swizzle.a = GetComponentSwizzle(m_TextureViewDesc.components.a);
        if (swizzleDesc.swizzle.r || swizzleDesc.swizzle.g || swizzleDesc.swizzle.b || swizzleDesc.swizzle.a)
            desc.nextInChain = &swizzleDesc.chain;
    }

    m_TextureView = wgpuTextureCreateView(*m_Texture, &desc);
    m_TextureVersion = m_Texture->GetVersion();

    return m_TextureView;
}

Format DescriptorWGPU::GetFormat() const {
    if (m_TextureViewDesc.format != Format::UNKNOWN)
        return m_TextureViewDesc.format;

    return m_Texture ? m_Texture->GetDesc().format : Format::UNKNOWN;
}

const TextureDesc* DescriptorWGPU::GetTextureDesc() const {
    return m_Texture ? &m_Texture->GetDesc() : nullptr;
}
