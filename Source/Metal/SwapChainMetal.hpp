// © 2026 NVIDIA Corporation

SwapChainMetal::~SwapChainMetal() {
    if (m_DrawableResidency) {
        m_Device.RemoveQueueResidencySet(m_DrawableResidency);
        m_DrawableResidency->release();
    }

    if (m_Drawable)
        ReleaseDrawable();

    if (m_PresentEvent)
        m_PresentEvent->release();

    for (TextureMetal* texture : m_Textures)
        Destroy(m_Device.GetAllocationCallbacks(), texture);

    if (m_Layer)
        m_Layer->release();
}

Result SwapChainMetal::Create(const SwapChainDesc& desc) {
    m_Layer = (CA::MetalLayer*)desc.window.metal.caMetalLayer;
    m_Layer->retain();
    m_Layer->setDevice(m_Device.GetNativeObject());

    m_Queue = (QueueMetal*)desc.queue;

    // The layer's residency set includes drawable textures and presentation resources. Swap chain textures can be accessed
    // from any queue, not only from the presenting one
    m_DrawableResidency = m_Layer->residencySet();
    if (!m_DrawableResidency) {
        NRI_REPORT_ERROR(&m_Device, "'CAMetalLayer' has no residency set (is the screen locked or the display asleep?)");
        return Result::FAILURE;
    }

    m_DrawableResidency->retain();
    m_Device.AddQueueResidencySet(m_DrawableResidency);

    // G22 swapchains receive display-encoded shader output, as on D3D/Vulkan.
    // Use a non-sRGB attachment to avoid encoding the output twice.
    MTL::PixelFormat format = desc.format == SwapChainFormat::BT709_G10_16BIT ? MTL::PixelFormatRGBA16Float : ((desc.format == SwapChainFormat::BT709_G22_10BIT || desc.format == SwapChainFormat::BT2020_G2084_10BIT) ? MTL::PixelFormatRGB10A2Unorm : MTL::PixelFormatBGRA8Unorm);
    m_Layer->setPixelFormat(format);

    CFStringRef colorSpaceName = desc.format == SwapChainFormat::BT709_G10_16BIT ? kCGColorSpaceExtendedLinearSRGB : (desc.format == SwapChainFormat::BT2020_G2084_10BIT ? kCGColorSpaceITUR_2100_PQ : kCGColorSpaceSRGB);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(colorSpaceName);

    if (!colorSpace)
        return Result::FAILURE;

    m_Layer->setColorspace(colorSpace);
    CGColorSpaceRelease(colorSpace);

    m_Layer->setWantsExtendedDynamicRangeContent(desc.format == SwapChainFormat::BT709_G10_16BIT || desc.format == SwapChainFormat::BT2020_G2084_10BIT);
    m_Layer->setFramebufferOnly(false);
    m_Layer->setDrawableSize(CGSizeMake(desc.width, desc.height));
#if TARGET_OS_OSX
    // Like VK: no tearing unless "ALLOW_TEARING" is set without "VSYNC". There is no uncapped tearing-free mode (like "MAILBOX"),
    // so it falls back to capped. Without display sync partially rendered frames can be visible
    const bool allowTearing = (desc.flags & SwapChainBits::ALLOW_TEARING) && !(desc.flags & SwapChainBits::VSYNC);
    m_Layer->setDisplaySyncEnabled(!allowTearing);
#endif // iOS: always in sync with the display

    m_IsWaitable = bool(desc.flags & SwapChainBits::WAITABLE);

    if (m_IsWaitable) {
        m_PresentEvent = m_Device.GetNativeObject()->newSharedEvent();

        if (!m_PresentEvent)
            return Result::OUT_OF_MEMORY;
    }

    // "maximumDrawableCount" (2 or 3) is the only latency control: "queuedFrameNum" frames can be queued in addition to the one being rendered.
    // "queuedFrameNum = 0" means 1 for waitable swap chains and 2 otherwise
    uint32_t queuedFrameNum = desc.queuedFrameNum ? desc.queuedFrameNum : (m_IsWaitable ? 1 : 2);
    uint32_t textureNum = std::clamp(queuedFrameNum + 1, 2u, 3u);
    m_Layer->setMaximumDrawableCount(textureNum);

    m_TextureDesc.type = TextureType::TEXTURE_2D;
    m_TextureDesc.usage = TextureUsageBits::COLOR_ATTACHMENT;
    m_TextureDesc.format = format == MTL::PixelFormatRGBA16Float ? Format::RGBA16_SFLOAT : (format == MTL::PixelFormatBGRA8Unorm ? Format::BGRA8_UNORM : Format::R10_G10_B10_A2_UNORM);
    m_TextureDesc.width = desc.width;
    m_TextureDesc.height = desc.height;
    m_TextureDesc.depth = 1;
    m_TextureDesc.mipNum = 1;
    m_TextureDesc.layerNum = 1;
    m_TextureDesc.sampleNum = 1;

    for (uint32_t i = 0; i < textureNum; i++) {
        TextureMetal* texture = Allocate<TextureMetal>(m_Device.GetAllocationCallbacks(), m_Device);

        if (!texture)
            return Result::OUT_OF_MEMORY;

        texture->SetDrawableTexture(nullptr, m_TextureDesc);
        m_Textures.push_back(texture);
        m_DrawableTextures.push_back(nullptr);
    }

    return Result::SUCCESS;
}

NRI_INLINE Texture* const* SwapChainMetal::GetTextures(uint32_t& textureNum) const {
    textureNum = (uint32_t)m_Textures.size();

    return (Texture* const*)m_Textures.data();
}

void SwapChainMetal::ReleaseDrawable() {
    // The swap chain texture is valid only between "acquire" and "present", a not presented drawable returns to the layer
    m_Textures[m_TextureIndex]->SetDrawableTexture(nullptr, m_TextureDesc);

    m_Drawable->release();
    m_Drawable = nullptr;
}

NRI_INLINE Result SwapChainMetal::AcquireNextTexture(FenceMetal& fence, uint32_t& textureIndex) {
    // "resizableSwapChain": drawables keep the swap chain size, the content gets scaled to the layer bounds. Restore the size if someone else has changed it
    const CGSize size = m_Layer->drawableSize();

    if ((uint32_t)size.width != m_TextureDesc.width || (uint32_t)size.height != m_TextureDesc.height)
        m_Layer->setDrawableSize(CGSizeMake(m_TextureDesc.width, m_TextureDesc.height));

    // Only the last acquired texture can be presented
    if (m_Drawable)
        ReleaseDrawable();

    AutoreleasePoolMetal autoreleasePool; // "nextDrawable" and drawable synchronization create autoreleased objects

    m_Drawable = m_Layer->nextDrawable();

    if (!m_Drawable)
        return Result::FAILURE;

    m_Drawable->retain();

    // Keep a stable index per drawable texture (like swap chain images in other APIs). The layer can replace its drawables,
    // in this case the least recently assigned slot gets reused
    MTL::Texture* drawableTexture = m_Drawable->texture();
    const uint32_t slotNum = (uint32_t)m_DrawableTextures.size();

    uint32_t slot = 0;

    while (slot < slotNum && m_DrawableTextures[slot] != drawableTexture)
        slot++;

    if (slot == slotNum) {
        slot = m_NextSlot;
        m_NextSlot = (m_NextSlot + 1) % slotNum;
        m_DrawableTextures[slot] = drawableTexture;
    }

    m_TextureIndex = slot;
    m_Textures[slot]->SetDrawableTexture(drawableTexture, m_TextureDesc);

    m_Queue->WaitForDrawable(m_Drawable, fence);

    textureIndex = slot;

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainMetal::Present(FenceMetal& fence, uint64_t presentId) {
    NRI_CHECK(m_Drawable, "No texture has been acquired");

    if (!m_Drawable)
        return Result::FAILURE;

    AutoreleasePoolMetal autoreleasePool;

    if (m_IsWaitable && presentId) {
        // The handler can be called after the swap chain destruction
        MTL::SharedEvent* event = m_PresentEvent->retain();

        m_Drawable->addPresentedHandler([event, presentId](MTL::Drawable*) {
            if (event->signaledValue() < presentId)
                event->setSignaledValue(presentId);

            event->release();
        });
    }

    m_Queue->SignalDrawable(m_Drawable, fence);
    m_Drawable->present();

    ReleaseDrawable();

    return Result::SUCCESS;
}

NRI_INLINE Result SwapChainMetal::WaitForPresent(uint64_t presentId) {
    if (!m_IsWaitable || !presentId)
        return Result::UNSUPPORTED;

    bool isPresented = m_PresentEvent->waitUntilSignaledValue(presentId, NRI_TIMEOUT_PRESENT);

    return isPresented ? Result::SUCCESS : Result::FAILURE;
}

NRI_INLINE Result SwapChainMetal::GetDisplayDesc(DisplayDesc& desc) {
    // Only a "CAMetalLayer" is provided, the "NSScreen" is not reachable from it without Objective-C
    MaybeUnused(desc);

    return Result::UNSUPPORTED;
}
