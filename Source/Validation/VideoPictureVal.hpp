// © 2026 NVIDIA Corporation

NRI_INLINE VideoPictureVal::VideoPictureVal(DeviceVal& device, VideoPicture* impl)
    : ObjectVal(device, (Object*)impl) {
}

NRI_INLINE VideoPicture* VideoPictureVal::GetImpl() const {
    return (VideoPicture*)m_Impl;
}
