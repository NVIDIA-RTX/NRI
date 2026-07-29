// © 2026 NVIDIA Corporation

NRI_INLINE VideoSessionParametersVal::VideoSessionParametersVal(DeviceVal& device, VideoSessionParameters* impl, VideoSessionVal& session)
    : ObjectVal(device, (Object*)impl)
    , m_Session(session) {
}

NRI_INLINE VideoSessionParameters* VideoSessionParametersVal::GetImpl() const {
    return (VideoSessionParameters*)m_Impl;
}

NRI_INLINE VideoSessionVal& VideoSessionParametersVal::GetSession() const {
    return m_Session;
}
