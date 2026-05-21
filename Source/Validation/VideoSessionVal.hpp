// © 2026 NVIDIA Corporation

NRI_INLINE VideoSessionVal::VideoSessionVal(DeviceVal& device, VideoSession* impl)
    : ObjectVal(device, (Object*)impl) {
}

NRI_INLINE VideoSession* VideoSessionVal::GetImpl() const {
    return (VideoSession*)m_Impl;
}
