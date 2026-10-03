// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionVK;

struct VideoSessionParametersVK final : public DebugNameBase {
    inline VideoSessionParametersVK(DeviceVK& device)
        : m_Device(device)
        , m_H264ReferenceIndexDefaults(device.GetStdAllocator()) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline const VideoSessionVK& GetSession() const {
        return *m_Session;
    }

    inline const StdVideoAV1SequenceHeader& GetAV1SequenceHeader() const {
        return m_AV1SequenceHeader;
    }

    inline VkVideoSessionParametersKHR GetHandle() const {
        return m_Handle;
    }

    ~VideoSessionParametersVK();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR, (uint64_t)m_Handle, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const VideoSessionParametersDesc& videoSessionParametersDesc);
    void GetH264ReferenceIndexDefaults(uint8_t pictureParameterSetId, uint8_t& l0DefaultActiveMinus1, uint8_t& l1DefaultActiveMinus1) const;

private:
    struct H264ReferenceIndexDefaults {
        uint8_t pictureParameterSetId;
        uint8_t l0DefaultActiveMinus1;
        uint8_t l1DefaultActiveMinus1;
    };

    Result CreateNative(VideoSessionVK& session, const void* pNext);
    Result CreateH265(VideoSessionVK& session, const VideoH265SessionParametersDesc* parameters);
    Result CreateAV1(VideoSessionVK& session, const VideoAV1SessionParametersDesc* parameters);

    DeviceVK& m_Device;
    VideoSessionVK* m_Session = nullptr;
    VkVideoSessionParametersKHR m_Handle = VK_NULL_HANDLE;
    Vector<H264ReferenceIndexDefaults> m_H264ReferenceIndexDefaults; // PPS "num_ref_idx_lX_default_active_minus1" for H.264 encode slice headers
    StdVideoAV1ColorConfig m_AV1ColorConfig = {};
    StdVideoAV1TimingInfo m_AV1TimingInfo = {};
    StdVideoAV1SequenceHeader m_AV1SequenceHeader = {};
    StdVideoEncodeAV1DecoderModelInfo m_AV1DecoderModelInfo = {};
    StdVideoEncodeAV1OperatingPointInfo m_AV1OperatingPoint = {};
};

} // namespace nri
