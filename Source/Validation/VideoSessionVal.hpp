// © 2026 NVIDIA Corporation

NRI_INLINE VideoSessionVal::VideoSessionVal(DeviceVal& device, VideoSession* impl, const VideoSessionDesc& desc, const VideoCapabilities& capabilities)
    : ObjectVal(device, (Object*)impl)
    , m_Desc(desc)
    , m_Capabilities(capabilities) {
}

NRI_INLINE VideoSession* VideoSessionVal::GetImpl() const {
    return (VideoSession*)m_Impl;
}

NRI_INLINE const VideoSessionDesc& VideoSessionVal::GetDesc() const {
    return m_Desc;
}

NRI_INLINE const VideoCapabilities& VideoSessionVal::GetCapabilities() const {
    return m_Capabilities;
}

NRI_INLINE bool VideoSessionVal::IsResolvedMetadataRangeValid(const BufferVal& buffer, uint64_t offset) const {
    const BufferDesc& bufferDesc = buffer.GetDesc();
    const bool isOffsetAligned = m_Capabilities.resolvedMetadataOffsetAlignment <= 1 || offset % m_Capabilities.resolvedMetadataOffsetAlignment == 0;
    const bool isRangeValid = offset <= bufferDesc.size && m_Capabilities.resolvedMetadataSize <= bufferDesc.size - offset;

    return isOffsetAligned && isRangeValid;
}
