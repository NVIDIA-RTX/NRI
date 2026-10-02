// © 2026 NVIDIA Corporation

NRI_INLINE VideoSessionParametersVal::VideoSessionParametersVal(DeviceVal& device, VideoSessionParameters* impl, VideoSessionVal& session, const VideoSessionParametersDesc& desc)
    : ObjectVal(device, (Object*)impl)
    , m_Session(session) {
    if (desc.h264Parameters) {
        for (uint32_t i = 0; i < desc.h264Parameters->pictureParameterSetNum; i++) {
            const VideoH264PictureParameterSetDesc& pictureParameterSet = desc.h264Parameters->pictureParameterSets[i];
            m_H264PpsToSpsPlusOne[pictureParameterSet.pictureParameterSetId] = (uint8_t)(pictureParameterSet.sequenceParameterSetId + 1);
        }
    }

    if (desc.h265Parameters) {
        m_HasH265Parameters = true;

        for (uint32_t i = 0; i < desc.h265Parameters->pictureParameterSetNum; i++) {
            const VideoH265PictureParameterSetDesc& pictureParameterSet = desc.h265Parameters->pictureParameterSets[i];
            m_H265PpsToSpsPlusOne[pictureParameterSet.pictureParameterSetId] = (uint8_t)(pictureParameterSet.sequenceParameterSetId + 1);
            m_H265PpsToVpsPlusOne[pictureParameterSet.pictureParameterSetId] = (uint8_t)(pictureParameterSet.videoParameterSetId + 1);
        }
    }
}

NRI_INLINE VideoSessionParameters* VideoSessionParametersVal::GetImpl() const {
    return (VideoSessionParameters*)m_Impl;
}

NRI_INLINE VideoSessionVal& VideoSessionParametersVal::GetSession() const {
    return m_Session;
}

NRI_INLINE bool VideoSessionParametersVal::IsH264ParameterSetValid(uint8_t sequenceParameterSetId, uint8_t pictureParameterSetId) const {
    if (sequenceParameterSetId >= 32)
        return false;

    return m_H264PpsToSpsPlusOne[pictureParameterSetId] == sequenceParameterSetId + 1;
}

NRI_INLINE bool VideoSessionParametersVal::HasH265Parameters() const {
    return m_HasH265Parameters;
}

NRI_INLINE bool VideoSessionParametersVal::IsH265ParameterSetValid(uint8_t videoParameterSetId, uint8_t sequenceParameterSetId, uint8_t pictureParameterSetId) const {
    if (videoParameterSetId >= 16 || sequenceParameterSetId >= 16 || pictureParameterSetId >= 64)
        return false;

    return m_H265PpsToSpsPlusOne[pictureParameterSetId] == sequenceParameterSetId + 1 && m_H265PpsToVpsPlusOne[pictureParameterSetId] == videoParameterSetId + 1;
}
