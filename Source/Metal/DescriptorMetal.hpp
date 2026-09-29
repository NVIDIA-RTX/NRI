// © 2026 NVIDIA Corporation

constexpr std::array<MTL::TextureType, (size_t)TextureView::MAX_NUM> g_TextureViewTypes = {
    MTL::TextureType2D,        // TEXTURE
    MTL::TextureType2DArray,   // TEXTURE_ARRAY
    MTL::TextureTypeCube,      // TEXTURE_CUBE
    MTL::TextureTypeCubeArray, // TEXTURE_CUBE_ARRAY
    MTL::TextureType2D,        // STORAGE_TEXTURE
    MTL::TextureType2DArray,   // STORAGE_TEXTURE_ARRAY
    MTL::TextureType2D,        // SUBPASS_INPUT
    MTL::TextureType2D,        // COLOR_ATTACHMENT
    MTL::TextureType2D,        // DEPTH_STENCIL_ATTACHMENT
    MTL::TextureType2D,        // SHADING_RATE_ATTACHMENT
};
NRI_VALIDATE_ARRAY(g_TextureViewTypes);

// 1D textures are height-one 2D textures (see "NRI.metal")
static MTL::TextureType GetTextureViewTypeMetal(TextureView type, const TextureDesc& textureDesc) {
    const MTL::TextureType viewType = g_TextureViewTypes[(size_t)type];

    if (viewType == MTL::TextureType2D && textureDesc.type == TextureType::TEXTURE_3D)
        return MTL::TextureType3D;

    if (textureDesc.sampleNum > 1)
        return viewType == MTL::TextureType2DArray ? MTL::TextureType2DMultisampleArray : MTL::TextureType2DMultisample;

    return viewType;
}

constexpr std::array<MTL::TextureSwizzle, (size_t)ComponentSwizzle::MAX_NUM> g_TextureSwizzles = {
    MTL::TextureSwizzleRed,   // IDENTITY (replaced by the channel's own swizzle)
    MTL::TextureSwizzleZero,  // ZERO
    MTL::TextureSwizzleOne,   // ONE
    MTL::TextureSwizzleRed,   // R
    MTL::TextureSwizzleGreen, // G
    MTL::TextureSwizzleBlue,  // B
    MTL::TextureSwizzleAlpha, // A
};
NRI_VALIDATE_ARRAY(g_TextureSwizzles);

static inline MTL::TextureSwizzle GetTextureSwizzleMetal(ComponentSwizzle swizzle, MTL::TextureSwizzle identity) {
    return swizzle == ComponentSwizzle::IDENTITY ? identity : g_TextureSwizzles[(uint32_t)swizzle];
}

constexpr std::array<MTL::SamplerAddressMode, (size_t)AddressMode::MAX_NUM> g_AddressModes = {
    MTL::SamplerAddressModeRepeat,             // REPEAT
    MTL::SamplerAddressModeMirrorRepeat,       // MIRRORED_REPEAT
    MTL::SamplerAddressModeClampToEdge,        // CLAMP_TO_EDGE
    MTL::SamplerAddressModeClampToBorderColor, // CLAMP_TO_BORDER
    MTL::SamplerAddressModeMirrorClampToEdge,  // MIRROR_CLAMP_TO_EDGE
};
NRI_VALIDATE_ARRAY(g_AddressModes);

static_assert(sizeof(DescriptorEntryMetal) == 24, "Metal Shader Converter descriptor ABI changed");

DescriptorMetal::DescriptorMetal(DeviceMetal& device)
    : m_Device(device) {
}

DescriptorMetal::~DescriptorMetal() {
    if (m_TextureView)
        m_TextureView->release();

    if (m_Sampler)
        m_Sampler->release();
}

Result DescriptorMetal::Create(const BufferViewDesc& desc) {
    m_Buffer = (BufferMetal*)desc.buffer;
    m_BufferOffset = desc.offset;
    m_BufferSize = desc.size == WHOLE_SIZE ? m_Buffer->GetDesc().size - desc.offset : desc.size;
    m_TypedBuffer = desc.type == BufferView::BUFFER || desc.type == BufferView::STORAGE_BUFFER;
    m_Format = desc.format;

    if (!m_TypedBuffer)
        return Result::SUCCESS;

    // Texture buffers need an aligned offset. The remainder is passed in elements via descriptor metadata ("textureViewOffsetInElements")
    MTL::Device* device = m_Device.GetNativeObject();
    MTL::Buffer* buffer = m_Buffer->GetNativeObject();
    const MTL::PixelFormat format = GetPixelFormatMetal(desc.format);
    const uint64_t stride = GetFormatProps(desc.format).stride;
    const uint64_t alignedOffset = desc.offset & ~(device->minimumTextureBufferAlignmentForPixelFormat(format) - 1);
    const uint64_t padding = desc.offset - alignedOffset;
    const uint64_t elementNum = (padding + m_BufferSize) / stride;

    uint64_t bytesPerRow = Align(elementNum * stride, device->minimumLinearTextureAlignmentForPixelFormat(format));

    if (alignedOffset + bytesPerRow > buffer->length())
        bytesPerRow = elementNum * stride;

    m_TextureViewOffset = uint8_t(padding / stride);

    MTL::TextureUsage usage = MTL::TextureUsageShaderRead;

    if (desc.type == BufferView::STORAGE_BUFFER) {
        usage |= MTL::TextureUsageShaderWrite;

        if (TextureMetal::IsAtomicFormat(m_Device, desc.format))
            usage |= MTL::TextureUsageShaderAtomic;
    }

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    MTL::TextureDescriptor* textureDesc = MTL::TextureDescriptor::textureBufferDescriptor(format, elementNum, buffer->resourceOptions(), usage);
    m_TextureView = buffer->newTexture(textureDesc, alignedOffset, bytesPerRow);
    pool->release();

    return m_TextureView ? Result::SUCCESS : Result::FAILURE;
}

Result DescriptorMetal::Create(const TextureViewDesc& desc) {
    m_Texture = (TextureMetal*)desc.texture;
    m_TextureViewDesc = desc;
    m_TextureViewDesc.format = desc.format == Format::UNKNOWN ? m_Texture->GetDesc().format : desc.format;
    m_Format = m_TextureViewDesc.format;

    // A swap chain texture is resolved to the current drawable (or its view) when used
    MTL::Texture* texture = m_Texture->GetNativeObject();

    if (!texture || m_Texture->IsDrawable())
        return Result::SUCCESS;

    // Use the parent texture for attachments. "CommandBufferMetal" applies mip and slice offsets in the render pass, a subresource view would apply them twice
    const bool attachment = desc.type == TextureView::COLOR_ATTACHMENT || desc.type == TextureView::DEPTH_STENCIL_ATTACHMENT || desc.type == TextureView::SHADING_RATE_ATTACHMENT;
    MTL::PixelFormat format = desc.format == Format::UNKNOWN ? texture->pixelFormat() : GetPixelFormatMetal(desc.format);

    if (attachment) {
        if (format == texture->pixelFormat())
            return Result::SUCCESS;

        m_TextureView = texture->newTextureView(format);

        return m_TextureView ? Result::SUCCESS : Result::FAILURE;
    }

    if (format == MTL::PixelFormatDepth32Float_Stencil8 && desc.planes == PlaneBits::STENCIL)
        format = MTL::PixelFormatX32_Stencil8;

    const NS::UInteger mipNum = desc.mipNum == REMAINING ? texture->mipmapLevelCount() - desc.mipOffset : desc.mipNum;

    // 3D shader views expose the whole depth of the selected mips, "layer" and "slice" ranges don't apply (Metal requires slice range {0, 1})
    NS::Range layers = NS::Range(0, 1);

    if (texture->textureType() != MTL::TextureType3D) {
        const NS::UInteger layerNum = desc.layerNum == REMAINING ? texture->arrayLength() - desc.layerOffset : desc.layerNum;
        layers = NS::Range(desc.layerOffset, layerNum);
    }

    const MTL::TextureType type = GetTextureViewTypeMetal(desc.type, m_Texture->GetDesc());
    const MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Make(
        GetTextureSwizzleMetal(desc.components.r, MTL::TextureSwizzleRed),
        GetTextureSwizzleMetal(desc.components.g, MTL::TextureSwizzleGreen),
        GetTextureSwizzleMetal(desc.components.b, MTL::TextureSwizzleBlue),
        GetTextureSwizzleMetal(desc.components.a, MTL::TextureSwizzleAlpha));

    m_TextureView = texture->newTextureView(format, type, NS::Range(desc.mipOffset, mipNum), layers, swizzle);

    return m_TextureView ? Result::SUCCESS : Result::FAILURE;
}

static inline bool IsBorderColor(const SamplerDesc& desc, float r, float g, float b, float a) {
    if (desc.isInteger)
        return desc.borderColor.ui.x == (uint32_t)r && desc.borderColor.ui.y == (uint32_t)g && desc.borderColor.ui.z == (uint32_t)b && desc.borderColor.ui.w == (uint32_t)a;

    return desc.borderColor.f.x == r && desc.borderColor.f.y == g && desc.borderColor.f.z == b && desc.borderColor.f.w == a;
}

Result DescriptorMetal::Create(const SamplerDesc& desc) {
    MTL::SamplerDescriptor* d = MTL::SamplerDescriptor::alloc()->init();
    d->setMinFilter(desc.filters.min == Filter::LINEAR ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest);
    d->setMagFilter(desc.filters.mag == Filter::LINEAR ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest);
    d->setMipFilter(desc.filters.mip == Filter::LINEAR ? MTL::SamplerMipFilterLinear : MTL::SamplerMipFilterNearest);
    d->setSAddressMode(g_AddressModes[(uint32_t)desc.addressModes.u]);
    d->setTAddressMode(g_AddressModes[(uint32_t)desc.addressModes.v]);
    d->setRAddressMode(g_AddressModes[(uint32_t)desc.addressModes.w]);
    d->setLodMinClamp(desc.mipMin);
    d->setLodMaxClamp(desc.mipMax);
    d->setMaxAnisotropy(desc.anisotropy ? desc.anisotropy : 1);
    d->setCompareFunction(desc.compareOp == CompareOp::NONE ? MTL::CompareFunctionNever : GetCompareMetal(desc.compareOp)); // "Never" is the Metal default for non-comparison samplers
    d->setNormalizedCoordinates(!desc.unnormalizedCoordinates);
    d->setSupportArgumentBuffers(true);

    const bool usesBorder = desc.addressModes.u == AddressMode::CLAMP_TO_BORDER || desc.addressModes.v == AddressMode::CLAMP_TO_BORDER || desc.addressModes.w == AddressMode::CLAMP_TO_BORDER;

    if (usesBorder) {
        if (IsBorderColor(desc, 1.0f, 1.0f, 1.0f, 1.0f))
            d->setBorderColor(MTL::SamplerBorderColorOpaqueWhite);
        else if (IsBorderColor(desc, 0.0f, 0.0f, 0.0f, 1.0f))
            d->setBorderColor(MTL::SamplerBorderColorOpaqueBlack);
        else if (IsBorderColor(desc, 0.0f, 0.0f, 0.0f, 0.0f))
            d->setBorderColor(MTL::SamplerBorderColorTransparentBlack);
        else {
            d->release();

            return Result::UNSUPPORTED;
        }
    }

    // "lodBias" is silently ignored before Apple10. There the bias goes to descriptor metadata:
    // converted shaders apply it via "IRCompatibilityFlagSamplerLODBias", native shaders via "NriGetSamplerMipBias"
    if (m_Device.GetNativeObject()->supportsFamily(MTL::GPUFamilyApple10))
        d->setLodBias(desc.mipBias);
    else
        m_SamplerBias = desc.mipBias;

    m_Sampler = m_Device.GetNativeObject()->newSamplerState(d);
    d->release();

    return m_Sampler ? Result::SUCCESS : Result::FAILURE;
}

Result DescriptorMetal::Create(const AccelerationStructureMetal& accelerationStructure) {
    m_AccelerationStructure = &accelerationStructure;

    return Result::SUCCESS;
}

DeviceMetal& DescriptorMetal::GetDevice() const {
    return m_Device;
}

MTL::Buffer* DescriptorMetal::GetBuffer() const {
    if (m_AccelerationStructure)
        return m_AccelerationStructure->GetShaderBindingHeaderBuffer();

    return m_Buffer ? m_Buffer->GetNativeObject() : nullptr;
}

MTL::Texture* DescriptorMetal::GetTexture() const {
    if (m_TextureView)
        return m_TextureView;

    if (!m_Texture)
        return nullptr;

    return m_Texture->IsDrawable() ? m_Texture->GetDrawableView(GetPixelFormatMetal(m_Format)) : m_Texture->GetNativeObject();
}

uint64_t DescriptorMetal::GetBufferOffset() const {
    return m_BufferOffset;
}

uint64_t DescriptorMetal::GetBufferSize() const {
    return m_BufferSize;
}

const TextureViewDesc& DescriptorMetal::GetTextureViewDesc() const {
    return m_TextureViewDesc;
}

Format DescriptorMetal::GetFormat() const {
    return m_Format;
}

uint64_t DescriptorMetal::GetNativeObject() const {
    if (m_Sampler)
        return uint64_t(m_Sampler);

    MTL::Texture* texture = GetTexture();

    if (texture)
        return uint64_t(texture);

    return uint64_t(GetBuffer());
}

void DescriptorMetal::WriteEntry(void* dst) const {
    // Layout matches "IRDescriptorTableSetBufferView/Texture/Sampler" (see "NRI.metal")
    DescriptorEntryMetal entry = {};

    if (m_Sampler) {
        entry.bufferAddress = m_Sampler->gpuResourceID()._impl;
        memcpy(&entry.metadata, &m_SamplerBias, sizeof(m_SamplerBias));
    } else {
        MTL::Buffer* buffer = GetBuffer();

        if (buffer) {
            entry.bufferAddress = buffer->gpuAddress() + m_BufferOffset;
            entry.metadata = (m_BufferSize & 0xFFFFFFFFull) | (uint64_t(m_TextureViewOffset) << 32) | (uint64_t(m_TypedBuffer) << 63);
        }

        MTL::Texture* texture = GetTexture();

        if (texture)
            entry.resourceId = texture->gpuResourceID()._impl;
    }

    memcpy(dst, &entry, sizeof(entry));
}

void DescriptorMetal::SetDebugName(const char* name) {
    // Sampler labels are immutable after creation
    if (!m_TextureView)
        return;

    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);
    m_TextureView->setLabel(label);
    label->release();
}
