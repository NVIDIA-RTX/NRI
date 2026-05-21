// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionParametersVK final : public DebugNameBase {
    inline VideoSessionParametersVK(DeviceVK& device)
        : m_Device(device)
        , m_H264Sps(device.GetStdAllocator())
        , m_H264Pps(device.GetStdAllocator())
        , m_H265VpsProfileTierLevels(device.GetStdAllocator())
        , m_H265SpsProfileTierLevels(device.GetStdAllocator())
        , m_H265VpsDecPicBufMgrs(device.GetStdAllocator())
        , m_H265SpsDecPicBufMgrs(device.GetStdAllocator())
        , m_H265Vps(device.GetStdAllocator())
        , m_H265Sps(device.GetStdAllocator())
        , m_H265Pps(device.GetStdAllocator())
        , m_H265SpsScalingLists(device.GetStdAllocator())
        , m_H265PpsScalingLists(device.GetStdAllocator())
        , m_H265ShortTermRefPicSets(device.GetStdAllocator())
        , m_H265LongTermRefPicsSps(device.GetStdAllocator()) {
        for (auto& list : m_H264DefaultScalingLists.ScalingList4x4) {
            for (uint8_t& entry : list)
                entry = 16;
        }

        for (auto& list : m_H264DefaultScalingLists.ScalingList8x8) {
            for (uint8_t& entry : list)
                entry = 16;
        }
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~VideoSessionParametersVK();

    //================================================================================================================    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR, (uint64_t)m_Handle, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateNative(VideoSessionVK& session, const void* pNext);
    Result CreateNative(VideoSessionVK& session, const VkVideoSessionParametersCreateInfoKHR* nativeCreateInfo);
    Result Create(const VideoSessionParametersDesc& videoSessionParametersDesc);
    Result CreateH265(VideoSessionVK& session);
    Result CreateAV1(VideoSessionVK& session);

    DeviceVK& m_Device;
    VideoSessionVK* m_Session = nullptr;
    VkVideoSessionParametersKHR m_Handle = VK_NULL_HANDLE;
    StdVideoH264ScalingLists m_H264DefaultScalingLists = {};
    Vector<StdVideoH264SequenceParameterSet> m_H264Sps;
    Vector<StdVideoH264PictureParameterSet> m_H264Pps;
    Vector<StdVideoH265ProfileTierLevel> m_H265VpsProfileTierLevels;
    Vector<StdVideoH265ProfileTierLevel> m_H265SpsProfileTierLevels;
    Vector<StdVideoH265DecPicBufMgr> m_H265VpsDecPicBufMgrs;
    Vector<StdVideoH265DecPicBufMgr> m_H265SpsDecPicBufMgrs;
    Vector<StdVideoH265VideoParameterSet> m_H265Vps;
    Vector<StdVideoH265SequenceParameterSet> m_H265Sps;
    Vector<StdVideoH265PictureParameterSet> m_H265Pps;
    Vector<StdVideoH265ScalingLists> m_H265SpsScalingLists;
    Vector<StdVideoH265ScalingLists> m_H265PpsScalingLists;
    Vector<StdVideoH265ShortTermRefPicSet> m_H265ShortTermRefPicSets;
    Vector<StdVideoH265LongTermRefPicsSps> m_H265LongTermRefPicsSps;
    StdVideoAV1ColorConfig m_AV1ColorConfig = {};
    StdVideoAV1TimingInfo m_AV1TimingInfo = {};
    StdVideoAV1SequenceHeader m_AV1SequenceHeader = {};
    StdVideoEncodeAV1OperatingPointInfo m_AV1OperatingPoint = {};
    const VideoH265SessionParametersDesc* m_H265Parameters = nullptr;
    const VideoAV1SessionParametersDesc* m_AV1Parameters = nullptr;
};

} // namespace nri
