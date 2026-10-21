// © 2026 NVIDIA Corporation

NRI_INLINE VideoPictureVal::VideoPictureVal(DeviceVal& device, VideoPicture* impl, const VideoPictureDesc& desc, const TextureDesc& textureDesc)
    : ObjectVal(device, (Object*)impl)
    , m_Texture((TextureVal*)desc.texture)
    , m_Format(textureDesc.format)
    , m_Width(desc.width ? desc.width : textureDesc.width)
    , m_Height(desc.height ? desc.height : textureDesc.height)
    , m_Layer(desc.layer)
    , m_TextureLayerNum(textureDesc.layerNum ? textureDesc.layerNum : 1)
    , m_TextureWidth(textureDesc.width)
    , m_TextureHeight(textureDesc.height)
    , m_Codec(textureDesc.videoCodec)
    , m_Usage(desc.usage)
    , m_IsOutputOnly((textureDesc.usage & TextureUsageBits::VIDEO_OUTPUT_ONLY) != 0)
    , m_IsReferenceOnly((textureDesc.usage & TextureUsageBits::VIDEO_REFERENCE_ONLY) != 0) {
}

NRI_INLINE VideoPicture* VideoPictureVal::GetImpl() const {
    return (VideoPicture*)m_Impl;
}

NRI_INLINE VideoPictureUsage VideoPictureVal::GetUsage() const {
    return m_Usage;
}

NRI_INLINE bool VideoPictureVal::IsCompatibleWith(const VideoSessionDesc& sessionDesc) const {
    return m_Codec == sessionDesc.codec && IsVideoPictureCompatibleWithSession(m_Format, m_Width, m_Height, sessionDesc);
}

NRI_INLINE bool VideoPictureVal::IsSameSubresource(const VideoPictureVal& videoPicture) const {
    return m_Texture == videoPicture.m_Texture && m_Layer == videoPicture.m_Layer;
}

NRI_INLINE bool VideoPictureVal::IsSameTexture(const VideoPictureVal& videoPicture) const {
    return m_Texture == videoPicture.m_Texture;
}

NRI_INLINE bool VideoPictureVal::IsSameExtent(const VideoPictureVal& videoPicture) const {
    // Backends may use the whole textures (D3D12 decode conversion), not only the picture extents
    return m_Width == videoPicture.m_Width && m_Height == videoPicture.m_Height && m_TextureWidth == videoPicture.m_TextureWidth && m_TextureHeight == videoPicture.m_TextureHeight;
}

NRI_INLINE bool VideoPictureVal::HasTextureExtent(uint32_t width, uint32_t height) const {
    return m_TextureWidth == width && m_TextureHeight == height;
}

NRI_INLINE uint32_t VideoPictureVal::GetTextureLayerNum() const {
    return m_TextureLayerNum;
}

NRI_INLINE bool VideoPictureVal::IsOutputOnly() const {
    return m_IsOutputOnly;
}

NRI_INLINE bool VideoPictureVal::IsReferenceOnly() const {
    return m_IsReferenceOnly;
}
