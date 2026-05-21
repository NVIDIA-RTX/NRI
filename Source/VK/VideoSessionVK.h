// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVK;

struct VideoSessionVK final : public DebugNameBase {
    static constexpr uint32_t ENCODE_FEEDBACK_QUERY_NUM = 64;

    struct EncodeFeedbackPayloadReadback {
        BufferVK* resolvedMetadata = nullptr;
        uint64_t resolvedMetadataOffset = 0;
        BufferVK* bitstream = nullptr;
        uint64_t dstBitstreamOffset = 0;
        uint64_t size = 0;
        bool copyHeader = false;
        bool active = false;
    };

    inline VideoSessionVK(DeviceVK& device)
        : m_Device(device)
        , m_Memory(device.GetStdAllocator()) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~VideoSessionVK();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_VIDEO_SESSION_KHR, (uint64_t)m_Handle, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const VideoSessionDesc& videoSessionDesc);

    DeviceVK& m_Device;
    VkVideoSessionKHR m_Handle = VK_NULL_HANDLE;
    VkQueryPool m_EncodeFeedbackQueryPool = VK_NULL_HANDLE;
    std::array<EncodeFeedbackPayloadReadback, ENCODE_FEEDBACK_QUERY_NUM> m_EncodeFeedbackPayloadReadbacks = {};
    Vector<VkDeviceMemory> m_Memory;
    VideoSessionDesc m_Desc = {};
    VkExtent2D m_PictureAccessGranularity = {};
    uint32_t m_BitstreamOffsetAlignment = 1;
    uint32_t m_BitstreamSizeAlignment = 1;
    uint32_t m_RateControlModes = 0;
    bool m_ResetRecorded = false;
    bool m_UseInlineSessionParameters = false;
    bool m_CanGenerateH264PrefixNalu = false;
};

} // namespace nri
