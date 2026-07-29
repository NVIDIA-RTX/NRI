// © 2026 NVIDIA Corporation

NRI_INLINE Result VideoSessionParametersD3D12::Create(const VideoSessionParametersDesc& videoSessionParametersDesc) {
    NRI_CHECK(videoSessionParametersDesc.session, "Unexpected");

    m_Session = (VideoSessionD3D12*)videoSessionParametersDesc.session;
    m_H264Parameters = videoSessionParametersDesc.h264Parameters;
    m_H265Parameters = videoSessionParametersDesc.h265Parameters;
    m_AV1Parameters = videoSessionParametersDesc.av1Parameters;

    if (m_H264Parameters) {
        NRI_CHECK(!m_H264Parameters->sequenceParameterSetNum || m_H264Parameters->sequenceParameterSets, "Unexpected");
        NRI_CHECK(!m_H264Parameters->pictureParameterSetNum || m_H264Parameters->pictureParameterSets, "Unexpected");

        m_H264SequenceParameterSets.clear();
        m_H264PictureParameterSets.clear();

        if (m_H264Parameters->sequenceParameterSetNum)
            m_H264SequenceParameterSets.assign(m_H264Parameters->sequenceParameterSets, m_H264Parameters->sequenceParameterSets + m_H264Parameters->sequenceParameterSetNum);

        if (m_H264Parameters->pictureParameterSetNum)
            m_H264PictureParameterSets.assign(m_H264Parameters->pictureParameterSets, m_H264Parameters->pictureParameterSets + m_H264Parameters->pictureParameterSetNum);

        m_H264ParametersStorage = *m_H264Parameters;
        m_H264ParametersStorage.sequenceParameterSets = m_H264SequenceParameterSets.data();
        m_H264ParametersStorage.pictureParameterSets = m_H264PictureParameterSets.data();
        m_H264Parameters = &m_H264ParametersStorage;
    }

    if (m_H265Parameters) {
        NRI_CHECK(!m_H265Parameters->videoParameterSetNum || m_H265Parameters->videoParameterSets, "Unexpected");
        NRI_CHECK(!m_H265Parameters->sequenceParameterSetNum || m_H265Parameters->sequenceParameterSets, "Unexpected");
        NRI_CHECK(!m_H265Parameters->pictureParameterSetNum || m_H265Parameters->pictureParameterSets, "Unexpected");

        m_H265VideoParameterSets.clear();
        m_H265SequenceParameterSets.clear();
        m_H265PictureParameterSets.clear();
        m_H265ShortTermRefPicSets.clear();
        m_H265LongTermRefPicsSps.clear();
        m_H265SequenceScalingLists.clear();
        m_H265PictureScalingLists.clear();

        if (m_H265Parameters->videoParameterSetNum)
            m_H265VideoParameterSets.assign(m_H265Parameters->videoParameterSets, m_H265Parameters->videoParameterSets + m_H265Parameters->videoParameterSetNum);

        if (m_H265Parameters->sequenceParameterSetNum)
            m_H265SequenceParameterSets.assign(m_H265Parameters->sequenceParameterSets, m_H265Parameters->sequenceParameterSets + m_H265Parameters->sequenceParameterSetNum);

        if (m_H265Parameters->pictureParameterSetNum)
            m_H265PictureParameterSets.assign(m_H265Parameters->pictureParameterSets, m_H265Parameters->pictureParameterSets + m_H265Parameters->pictureParameterSetNum);

        size_t shortTermRefPicSetNum = 0;

        for (const VideoH265SequenceParameterSetDesc& sequenceParameterSet : m_H265SequenceParameterSets)
            shortTermRefPicSetNum += sequenceParameterSet.numShortTermRefPicSets;

        m_H265ShortTermRefPicSets.resize(shortTermRefPicSetNum);
        m_H265LongTermRefPicsSps.resize(m_H265SequenceParameterSets.size());
        size_t shortTermRefPicSetOffset = 0;

        for (size_t i = 0; i < m_H265SequenceParameterSets.size(); i++) {
            VideoH265SequenceParameterSetDesc& sequenceParameterSet = m_H265SequenceParameterSets[i];

            if (sequenceParameterSet.numShortTermRefPicSets) {
                std::copy(sequenceParameterSet.shortTermRefPicSets, sequenceParameterSet.shortTermRefPicSets + sequenceParameterSet.numShortTermRefPicSets,
                    m_H265ShortTermRefPicSets.begin() + shortTermRefPicSetOffset);
                sequenceParameterSet.shortTermRefPicSets = m_H265ShortTermRefPicSets.data() + shortTermRefPicSetOffset;
                shortTermRefPicSetOffset += sequenceParameterSet.numShortTermRefPicSets;
            }

            if (sequenceParameterSet.longTermRefPicsSps) {
                m_H265LongTermRefPicsSps[i] = *sequenceParameterSet.longTermRefPicsSps;
                sequenceParameterSet.longTermRefPicsSps = &m_H265LongTermRefPicsSps[i];
            }
        }

        m_H265SequenceScalingLists.resize(m_H265SequenceParameterSets.size());

        for (size_t i = 0; i < m_H265SequenceParameterSets.size(); i++) {
            if (m_H265SequenceParameterSets[i].scalingLists) {
                m_H265SequenceScalingLists[i] = *m_H265SequenceParameterSets[i].scalingLists;
                m_H265SequenceParameterSets[i].scalingLists = &m_H265SequenceScalingLists[i];
            }
        }

        m_H265PictureScalingLists.resize(m_H265PictureParameterSets.size());

        for (size_t i = 0; i < m_H265PictureParameterSets.size(); i++) {
            if (m_H265PictureParameterSets[i].scalingLists) {
                m_H265PictureScalingLists[i] = *m_H265PictureParameterSets[i].scalingLists;
                m_H265PictureParameterSets[i].scalingLists = &m_H265PictureScalingLists[i];
            }
        }

        m_H265ParametersStorage = *m_H265Parameters;
        m_H265ParametersStorage.videoParameterSets = m_H265VideoParameterSets.data();
        m_H265ParametersStorage.sequenceParameterSets = m_H265SequenceParameterSets.data();
        m_H265ParametersStorage.pictureParameterSets = m_H265PictureParameterSets.data();
        m_H265Parameters = &m_H265ParametersStorage;
    }

    if (m_AV1Parameters) {
        m_AV1ParametersStorage = *m_AV1Parameters;
        m_AV1Parameters = &m_AV1ParametersStorage;
    }

    return Result::SUCCESS;
}
