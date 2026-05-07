// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionVK final : public DebugNameBase {
    static constexpr uint32_t ENCODE_FEEDBACK_QUERY_NUM = 64;

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
    uint64_t m_EncodeFeedbackWriteIndex = 0;
    uint64_t m_EncodeFeedbackReadIndex = 0;
    Vector<VkDeviceMemory> m_Memory;
    VideoSessionDesc m_Desc = {};
    VkExtent2D m_PictureAccessGranularity = {};
    uint32_t m_BitstreamOffsetAlignment = 1;
    uint32_t m_BitstreamSizeAlignment = 1;
    bool m_Initialized = false;
    bool m_UseInlineSessionParameters = false;
    bool m_CanGenerateH264PrefixNalu = false;
};

} // namespace nri
