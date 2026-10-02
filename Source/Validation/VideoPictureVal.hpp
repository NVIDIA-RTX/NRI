// © 2026 NVIDIA Corporation

NRI_INLINE VideoPictureVal::VideoPictureVal(DeviceVal& device, VideoPicture* impl, const VideoPictureDesc& desc, const TextureDesc& textureDesc)
    : ObjectVal(device, (Object*)impl)
    , m_Texture((TextureVal*)desc.texture)
    , m_Format(textureDesc.format)
    , m_Width(desc.width ? desc.width : textureDesc.width)
    , m_Height(desc.height ? desc.height : textureDesc.height)
    , m_Layer(desc.layer)
    , m_TextureLayerNum(textureDesc.layerNum ? textureDesc.layerNum : 1)
    , m_Codec(textureDesc.videoCodec)
    , m_Usage(desc.usage)
    , m_IsOutputOnly((textureDesc.usage & TextureUsageBits::VIDEO_OUTPUT_ONLY) != 0) {
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

NRI_INLINE uint32_t VideoPictureVal::GetTextureLayerNum() const {
    return m_TextureLayerNum;
}

NRI_INLINE bool VideoPictureVal::IsOutputOnly() const {
    return m_IsOutputOnly;
}
