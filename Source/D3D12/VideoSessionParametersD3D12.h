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

    Result Create(const VideoSessionParametersDesc& videoSessionParametersDesc);

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
