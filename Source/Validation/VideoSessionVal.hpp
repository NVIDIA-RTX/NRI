// © 2026 NVIDIA Corporation

NRI_INLINE VideoSessionVal::VideoSessionVal(DeviceVal& device, VideoSession* impl, const VideoSessionDesc& desc)
    : ObjectVal(device, (Object*)impl), m_Desc(desc) {
}

NRI_INLINE VideoSession* VideoSessionVal::GetImpl() const {
    return (VideoSession*)m_Impl;
}

NRI_INLINE const VideoSessionDesc& VideoSessionVal::GetDesc() const {
    return m_Desc;
}
