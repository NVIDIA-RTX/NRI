// © 2026 NVIDIA Corporation

TextureWGPU::~TextureWGPU() {
    Release();
}

void TextureWGPU::Release() {
    if (m_Texture && m_OwnsTexture)
        wgpuTextureRelease(m_Texture);

    m_Texture = nullptr;
    m_IsSurfaceTexture = false;
}

Result TextureWGPU::Create(const TextureDesc& textureDesc) {
    Release();

    m_Desc = textureDesc;
    m_Desc.width = GetCountOrOne(m_Desc.width);
    m_Desc.height = GetCountOrOne(m_Desc.height);
    m_Desc.depth = GetCountOrOne(m_Desc.depth);
    m_Desc.mipNum = GetCountOrOne(m_Desc.mipNum);
    m_Desc.layerNum = GetCountOrOne(m_Desc.layerNum);
    m_Desc.sampleNum = (Sample_t)GetCountOrOne((uint32_t)m_Desc.sampleNum);
    m_OwnsTexture = true;
    m_IsSurfaceTexture = false;

    WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
    desc.usage = GetTextureUsage(m_Desc.usage);
    desc.dimension = GetTextureDimension(m_Desc.type);
    desc.size = {m_Desc.width, m_Desc.height, m_Desc.type == TextureType::TEXTURE_3D ? m_Desc.depth : m_Desc.layerNum};
    desc.format = GetTextureFormat(m_Desc.format);
    desc.mipLevelCount = m_Desc.mipNum;
    desc.sampleCount = m_Desc.sampleNum;

    m_Texture = wgpuDeviceCreateTexture(m_Device, &desc);
    if (!m_Texture)
        return Result::FAILURE;

    m_Version++;

    return Result::SUCCESS;
}

void TextureWGPU::SetSurfaceTexture(WGPUTexture texture, Format format, Dim_t width, Dim_t height) {
    Release();

    m_Texture = texture;
    m_OwnsTexture = true;
    m_IsSurfaceTexture = texture != nullptr;
    if (texture) {
        width = (Dim_t)wgpuTextureGetWidth(texture);
        height = (Dim_t)wgpuTextureGetHeight(texture);
    }

    m_Desc = {};
    m_Desc.type = TextureType::TEXTURE_2D;
    m_Desc.usage = TextureUsageBits::COLOR_ATTACHMENT;
    m_Desc.format = format;
    m_Desc.width = width;
    m_Desc.height = height;
    m_Desc.depth = 1;
    m_Desc.mipNum = 1;
    m_Desc.layerNum = 1;
    m_Desc.sampleNum = 1;
    m_Version++;
}

void TextureWGPU::DetachSurfaceTexture() {
    if (!m_IsSurfaceTexture)
        return;

    if (m_Texture && m_OwnsTexture)
        wgpuTextureRelease(m_Texture);

    m_Texture = nullptr;
    m_IsSurfaceTexture = false;
    m_Version++;
}
