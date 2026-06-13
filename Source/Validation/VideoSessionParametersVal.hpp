// © 2026 NVIDIA Corporation

NRI_INLINE VideoSessionParametersVal::VideoSessionParametersVal(DeviceVal& device, VideoSessionParameters* impl)
    : ObjectVal(device, (Object*)impl) {
}

NRI_INLINE VideoSessionParameters* VideoSessionParametersVal::GetImpl() const {
    return (VideoSessionParameters*)m_Impl;
}
