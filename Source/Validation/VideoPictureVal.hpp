// © 2026 NVIDIA Corporation

NRI_INLINE VideoPictureVal::VideoPictureVal(DeviceVal& device, VideoPicture* impl, const VideoPictureDesc& desc, const TextureDesc& textureDesc)
    : ObjectVal(device, (Object*)impl)
    , m_Format(textureDesc.format)
    , m_Width(desc.width ? desc.width : textureDesc.width)
    , m_Height(desc.height ? desc.height : textureDesc.height)
    , m_Codec(textureDesc.videoCodec)
    , m_Usage(desc.usage) {
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
