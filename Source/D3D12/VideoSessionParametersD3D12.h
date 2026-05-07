// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionParametersD3D12 final : public DebugNameBase {
    inline VideoSessionParametersD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_H264SequenceParameterSets(device.GetStdAllocator())
        , m_H264PictureParameterSets(device.GetStdAllocator())
        , m_H265VideoParameterSets(device.GetStdAllocator())
        , m_H265SequenceParameterSets(device.GetStdAllocator())
        , m_H265PictureParameterSets(device.GetStdAllocator())
        , m_H265SequenceScalingLists(device.GetStdAllocator())
        , m_H265PictureScalingLists(device.GetStdAllocator()) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char*) NRI_DEBUG_NAME_OVERRIDE {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const VideoSessionParametersDesc& videoSessionParametersDesc) {
        if (!videoSessionParametersDesc.session)
            return Result::INVALID_ARGUMENT;

        m_Session = (VideoSessionD3D12*)videoSessionParametersDesc.session;
        m_H264Parameters = videoSessionParametersDesc.h264Parameters;
        m_H265Parameters = videoSessionParametersDesc.h265Parameters;
        m_AV1Parameters = videoSessionParametersDesc.av1Parameters;
        if (m_H264Parameters) {
            if ((m_H264Parameters->sequenceParameterSetNum && !m_H264Parameters->sequenceParameterSets) || (m_H264Parameters->pictureParameterSetNum && !m_H264Parameters->pictureParameterSets))
                return Result::INVALID_ARGUMENT;

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
            if ((m_H265Parameters->videoParameterSetNum && !m_H265Parameters->videoParameterSets) || (m_H265Parameters->sequenceParameterSetNum && !m_H265Parameters->sequenceParameterSets) || (m_H265Parameters->pictureParameterSetNum && !m_H265Parameters->pictureParameterSets))
                return Result::INVALID_ARGUMENT;

            m_H265VideoParameterSets.clear();
            m_H265SequenceParameterSets.clear();
            m_H265PictureParameterSets.clear();
            m_H265SequenceScalingLists.clear();
            m_H265PictureScalingLists.clear();
            if (m_H265Parameters->videoParameterSetNum)
                m_H265VideoParameterSets.assign(m_H265Parameters->videoParameterSets, m_H265Parameters->videoParameterSets + m_H265Parameters->videoParameterSetNum);
            if (m_H265Parameters->sequenceParameterSetNum)
                m_H265SequenceParameterSets.assign(m_H265Parameters->sequenceParameterSets, m_H265Parameters->sequenceParameterSets + m_H265Parameters->sequenceParameterSetNum);
            if (m_H265Parameters->pictureParameterSetNum)
                m_H265PictureParameterSets.assign(m_H265Parameters->pictureParameterSets, m_H265Parameters->pictureParameterSets + m_H265Parameters->pictureParameterSetNum);
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

    DeviceD3D12& m_Device;
    VideoSessionD3D12* m_Session = nullptr;
    VideoH264SessionParametersDesc m_H264ParametersStorage = {};
    Vector<VideoH264SequenceParameterSetDesc> m_H264SequenceParameterSets;
    Vector<VideoH264PictureParameterSetDesc> m_H264PictureParameterSets;
    const VideoH264SessionParametersDesc* m_H264Parameters = nullptr;
    VideoH265SessionParametersDesc m_H265ParametersStorage = {};
    Vector<VideoH265VideoParameterSetDesc> m_H265VideoParameterSets;
    Vector<VideoH265SequenceParameterSetDesc> m_H265SequenceParameterSets;
    Vector<VideoH265PictureParameterSetDesc> m_H265PictureParameterSets;
    Vector<VideoH265ScalingListsDesc> m_H265SequenceScalingLists;
    Vector<VideoH265ScalingListsDesc> m_H265PictureScalingLists;
    const VideoH265SessionParametersDesc* m_H265Parameters = nullptr;
    VideoAV1SessionParametersDesc m_AV1ParametersStorage = {};
    const VideoAV1SessionParametersDesc* m_AV1Parameters = nullptr;
};

} // namespace nri
