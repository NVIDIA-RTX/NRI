// © 2026 NVIDIA Corporation

// Views of the same "typeless" family need "PixelFormatView" (except sRGB <-> linear and swizzle-only views). It may disable lossless
// compression, so it's requested only for formats having non-sRGB siblings
static inline bool IsFormatReinterpretable(Format format, TextureUsageBits usage) {
    const FormatProps& props = GetFormatProps(format);

    // Stencil sampling needs an "X32_Stencil8" view. Depth formats are viewed only as themselves
    if (props.isDepth || props.isStencil)
        return props.isStencil && (usage & TextureUsageBits::SHADER_RESOURCE);

    // UNORM <-> SNORM or UFLOAT <-> SFLOAT siblings
    if (props.isCompressed) {
        return format == Format::BC4_R_UNORM || format == Format::BC4_R_SNORM || format == Format::BC5_RG_UNORM || format == Format::BC5_RG_SNORM
            || format == Format::BC6H_RGB_UFLOAT || format == Format::BC6H_RGB_SFLOAT || format == Format::ETC2_R11_UNORM || format == Format::ETC2_R11_SNORM
            || format == Format::ETC2_R11_G11_UNORM || format == Format::ETC2_R11_G11_SNORM;
    }

    // BGRA and 16-bit packed formats have only sRGB siblings, R11G11B10 and RGB9E5 have none
    if (props.isBgr || props.isExpShared || format == Format::R11_G11_B10_UFLOAT)
        return false;

    return true;
}

bool TextureMetal::IsAtomicFormat(const DeviceMetal& device, Format format) {
    // 32-bit atomics: "R32Uint" and "R32Sint". 64-bit min/max atomics: "RG32Uint" (Apple8+)
    if (format == Format::RG32_UINT)
        return device.GetNativeObject()->supportsFamily(MTL::GPUFamilyApple8);

    return format == Format::R32_UINT || format == Format::R32_SINT;
}

MTL::TextureDescriptor* TextureMetal::NewNativeDesc(MemoryLocation location) const {
    const Dim_t layerNum = std::max(m_Desc.layerNum, (Dim_t)1);
    const Sample_t sampleNum = std::max(m_Desc.sampleNum, (Sample_t)1);

    // Converter maps 1D textures to height-one 2D textures. This also preserves NRI mipmaps, which Metal's native 1D textures can't represent
    MTL::TextureType type = MTL::TextureType2D;

    if (m_Desc.type == TextureType::TEXTURE_3D)
        type = MTL::TextureType3D;
    else if (sampleNum > 1)
        type = layerNum > 1 ? MTL::TextureType2DMultisampleArray : MTL::TextureType2DMultisample;
    else if (layerNum > 1)
        type = MTL::TextureType2DArray;

    MTL::TextureUsage usage = MTL::TextureUsageUnknown;

    if (m_Desc.usage & TextureUsageBits::SHADER_RESOURCE)
        usage |= MTL::TextureUsageShaderRead;

    if (m_Desc.usage & TextureUsageBits::SHADER_RESOURCE_STORAGE) {
        usage |= MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite;

        if (IsAtomicFormat(m_Device, m_Desc.format))
            usage |= MTL::TextureUsageShaderAtomic;
    }

    if (m_Desc.usage & (TextureUsageBits::COLOR_ATTACHMENT | TextureUsageBits::DEPTH_STENCIL_ATTACHMENT | TextureUsageBits::INPUT_ATTACHMENT))
        usage |= MTL::TextureUsageRenderTarget;

    // "MIN/MAX" color resolves read multisampled attachments in a shader
    if (sampleNum > 1 && (m_Desc.usage & TextureUsageBits::COLOR_ATTACHMENT))
        usage |= MTL::TextureUsageShaderRead;

    if (usage != MTL::TextureUsageUnknown && IsFormatReinterpretable(m_Desc.format, m_Desc.usage))
        usage |= MTL::TextureUsagePixelFormatView;

    MTL::TextureDescriptor* nativeDesc = MTL::TextureDescriptor::alloc()->init();
    nativeDesc->setTextureType(type);
    nativeDesc->setPixelFormat(GetPixelFormatMetal(m_Desc.format));
    nativeDesc->setWidth(m_Desc.width);
    nativeDesc->setHeight(std::max(m_Desc.height, (Dim_t)1));
    nativeDesc->setDepth(std::max(m_Desc.depth, (Dim_t)1));
    nativeDesc->setMipmapLevelCount(std::max(m_Desc.mipNum, (Dim_t)1));
    nativeDesc->setArrayLength(layerNum);
    nativeDesc->setSampleCount(sampleNum);
    nativeDesc->setStorageMode(location == MemoryLocation::DEVICE ? MTL::StorageModePrivate : MTL::StorageModeShared);
    nativeDesc->setCpuCacheMode(location == MemoryLocation::DEVICE || location == MemoryLocation::HOST_READBACK ? MTL::CPUCacheModeDefaultCache : MTL::CPUCacheModeWriteCombined);
    nativeDesc->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
    nativeDesc->setUsage(usage);

    return nativeDesc;
}

TextureMetal::~TextureMetal() {
    ReleaseDrawableViews();
    Release();
}

void TextureMetal::ReleaseDrawableViews() {
    for (uint32_t i = 0; i < m_DrawableViewNum; i++)
        m_DrawableViews[i].view->release();

    m_DrawableViewNum = 0;
    m_DrawableViewParent = nullptr;
}

void TextureMetal::Release() {
    if (m_Texture) {
        if (m_IsResident)
            m_Device.RemoveResidency(m_Texture);

        m_Texture->release();
    }

    m_Texture = nullptr;
    m_IsResident = false;
}

Result TextureMetal::Create(const TextureDesc& desc) {
    Release();
    m_Desc = FixTextureDesc(desc);

    return Result::SUCCESS;
}

Result TextureMetal::Create(const TextureDesc& desc, MemoryLocation location) {
    Create(desc);

    MTL::TextureDescriptor* nativeDesc = NewNativeDesc(location);
    m_Texture = m_Device.GetNativeObject()->newTexture(nativeDesc);
    nativeDesc->release();

    if (!m_Texture)
        return Result::OUT_OF_MEMORY;

    m_Device.AddResidency(m_Texture);
    m_IsResident = true;

    return Result::SUCCESS;
}

Result TextureMetal::Create(const TextureMetalDesc& desc) {
    Release();
    m_Desc = FixTextureDesc(desc.desc);
    m_Texture = (MTL::Texture*)desc.mtlTexture;
    m_Texture->retain();

    // Residency sets don't support memoryless textures
    if (m_Texture->storageMode() != MTL::StorageModeMemoryless) {
        m_Device.AddResidency(m_Texture);
        m_IsResident = true;
    }

    return Result::SUCCESS;
}

Result TextureMetal::Bind(MemoryMetal& memory, uint64_t offset) {
    Release();
    MTL::TextureDescriptor* desc = NewNativeDesc(memory.GetLocation());
    m_Texture = memory.GetNativeObject()->newTexture(desc, (NS::UInteger)offset);
    desc->release();

    return m_Texture ? Result::SUCCESS : Result::FAILURE;
}

void TextureMetal::GetMemoryDesc(MemoryLocation location, MemoryDesc& memoryDesc) const {
    MTL::TextureDescriptor* desc = NewNativeDesc(location);
    MTL::SizeAndAlign requirements = m_Device.GetNativeObject()->heapTextureSizeAndAlign(desc);
    desc->release();

    memoryDesc = {requirements.size, (uint32_t)requirements.align, (MemoryType)location, false};
}

void TextureMetal::SetDrawableTexture(MTL::Texture* texture, const TextureDesc& desc) {
    Release();
    m_Desc = FixTextureDesc(desc);
    m_Texture = texture;
    m_IsDrawable = true;

    if (!m_Texture)
        return;

    m_Texture->retain();

    // The layer has replaced the drawable texture of this slot
    std::lock_guard<std::mutex> lock(m_DrawableViewLock);

    if (m_DrawableViewParent != m_Texture) {
        ReleaseDrawableViews();
        m_DrawableViewParent = m_Texture;
    }
}

MTL::Texture* TextureMetal::GetDrawableView(MTL::PixelFormat format) {
    if (!m_Texture || format == GetPixelFormatMetal(m_Desc.format))
        return m_Texture;

    // Views are created at recording time, since the drawable texture is unknown at view creation. sRGB <-> linear views don't need "PixelFormatView"
    std::lock_guard<std::mutex> lock(m_DrawableViewLock);

    for (uint32_t i = 0; i < m_DrawableViewNum; i++) {
        if (m_DrawableViews[i].format == format)
            return m_DrawableViews[i].view;
    }

    NRI_CHECK(m_DrawableViewNum < GetCountOf(m_DrawableViews), "Too many view formats of a swap chain texture");

    MTL::Texture* view = m_DrawableViewNum < GetCountOf(m_DrawableViews) ? m_Texture->newTextureView(format) : nullptr;

    if (!view)
        return m_Texture;

    m_DrawableViews[m_DrawableViewNum++] = {format, view};

    return view;
}

void TextureMetal::SetDebugName(const char* name) {
    if (!m_Texture)
        return;

    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);
    m_Texture->setLabel(label);
    label->release();
}
