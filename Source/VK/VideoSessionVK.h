// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVK;

struct VideoSessionVK final : public DebugNameBase {
    static constexpr uint32_t ENCODE_FEEDBACK_QUERY_NUM = 64;

    struct EncodeFeedbackPayloadReadback {
        BufferVK* resolvedMetadata = nullptr;
        uint64_t resolvedMetadataOffset = 0;
        uint64_t dstBitstreamOffset = 0;
        bool resolvedByCommand = false;
        bool active = false;
    };

    inline VideoSessionVK(DeviceVK& device)
        : m_Device(device)
        , m_Memory(device.GetStdAllocator()) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkVideoSessionKHR GetHandle() const {
        return m_Handle;
    }

    inline const VideoSessionDesc& GetDesc() const {
        return m_Desc;
    }

    inline uint32_t GetBitstreamOffsetAlignment() const {
        return m_BitstreamOffsetAlignment;
    }

    inline uint32_t GetBitstreamSizeAlignment() const {
        return m_BitstreamSizeAlignment;
    }

    inline uint32_t GetRateControlModes() const {
        return m_RateControlModes;
    }

    inline VkVideoEncodeAV1CapabilityFlagsKHR GetAV1CapabilityFlags() const {
        return m_AV1CapabilityFlags;
    }

    inline uint32_t GetAV1MaxSingleReferenceCount() const {
        return m_AV1MaxSingleReferenceCount;
    }

    inline uint32_t GetAV1SingleReferenceNameMask() const {
        return m_AV1SingleReferenceNameMask;
    }

    inline uint32_t GetAV1MaxUnidirectionalCompoundReferenceCount() const {
        return m_AV1MaxUnidirectionalCompoundReferenceCount;
    }

    inline uint32_t GetAV1UnidirectionalCompoundReferenceNameMask() const {
        return m_AV1UnidirectionalCompoundReferenceNameMask;
    }

    inline uint32_t GetAV1MaxBidirectionalCompoundReferenceCount() const {
        return m_AV1MaxBidirectionalCompoundReferenceCount;
    }

    inline uint32_t GetAV1BidirectionalCompoundReferenceNameMask() const {
        return m_AV1BidirectionalCompoundReferenceNameMask;
    }

    inline VkExtent2D GetAV1MaxTiles() const {
        return m_AV1MaxTiles;
    }

    inline VkExtent2D GetAV1MinTileSize() const {
        return m_AV1MinTileSize;
    }

    inline VkExtent2D GetAV1MaxTileSize() const {
        return m_AV1MaxTileSize;
    }

    inline uint32_t GetAV1MinQIndex() const {
        return m_AV1MinQIndex;
    }

    inline uint32_t GetAV1MaxQIndex() const {
        return m_AV1MaxQIndex;
    }

    inline bool DoesAV1RequireGopRemainingFrames() const {
        return m_AV1RequiresGopRemainingFrames;
    }

    inline bool UseInlineSessionParameters() const {
        return m_UseInlineSessionParameters;
    }

    inline uint32_t GetH264MaxBPictureL0ReferenceCount() const {
        return m_H264MaxBPictureL0ReferenceCount;
    }

    inline uint32_t GetH264MaxL1ReferenceCount() const {
        return m_H264MaxL1ReferenceCount;
    }

    inline uint32_t GetH265MaxBPictureL0ReferenceCount() const {
        return m_H265MaxBPictureL0ReferenceCount;
    }

    inline uint32_t GetH265MaxL1ReferenceCount() const {
        return m_H265MaxL1ReferenceCount;
    }

    inline bool IsResetRecorded() const {
        return m_ResetRecorded;
    }

    inline void SetResetRecorded() {
        m_ResetRecorded = true;
    }

    inline VkQueryPool GetEncodeFeedbackQueryPool() const {
        return m_EncodeFeedbackQueryPool;
    }

    inline bool HasPendingEncodeFeedbackQuery(BufferVK* resolvedMetadata, uint64_t resolvedMetadataOffset) const {
        for (const EncodeFeedbackPayloadReadback& payloadReadback : m_EncodeFeedbackPayloadReadbacks) {
            if (payloadReadback.active && payloadReadback.resolvedMetadata == resolvedMetadata && payloadReadback.resolvedMetadataOffset == resolvedMetadataOffset)
                return true;
        }

        return false;
    }

    inline uint32_t FindEncodeFeedbackQuery(BufferVK* resolvedMetadata, uint64_t resolvedMetadataOffset) const {
        for (uint32_t i = 0; i < ENCODE_FEEDBACK_QUERY_NUM; i++) {
            const EncodeFeedbackPayloadReadback& payloadReadback = m_EncodeFeedbackPayloadReadbacks[i];
            if (payloadReadback.active && payloadReadback.resolvedMetadata == resolvedMetadata && payloadReadback.resolvedMetadataOffset == resolvedMetadataOffset)
                return i;
        }

        return UINT32_MAX;
    }

    inline uint32_t AllocateEncodeFeedbackQuery(BufferVK* resolvedMetadata, uint64_t resolvedMetadataOffset, uint64_t dstBitstreamOffset) {
        for (uint32_t i = 0; i < ENCODE_FEEDBACK_QUERY_NUM; i++) {
            EncodeFeedbackPayloadReadback& payloadReadback = m_EncodeFeedbackPayloadReadbacks[i];
            if (payloadReadback.active)
                continue;

            payloadReadback.active = true;
            payloadReadback.resolvedMetadata = resolvedMetadata;
            payloadReadback.resolvedMetadataOffset = resolvedMetadataOffset;
            payloadReadback.dstBitstreamOffset = dstBitstreamOffset;
            return i;
        }

        return UINT32_MAX;
    }

    inline const EncodeFeedbackPayloadReadback& GetEncodeFeedbackPayloadReadback(uint32_t queryIndex) const {
        return m_EncodeFeedbackPayloadReadbacks[queryIndex];
    }

    inline void SetEncodeFeedbackQueryResolved(uint32_t queryIndex) {
        m_EncodeFeedbackPayloadReadbacks[queryIndex].resolvedByCommand = true;
    }

    inline void ClearEncodeFeedbackQuery(uint32_t queryIndex) {
        m_EncodeFeedbackPayloadReadbacks[queryIndex] = {};
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

private:
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
    VkVideoEncodeAV1CapabilityFlagsKHR m_AV1CapabilityFlags = 0;
    uint32_t m_AV1MaxSingleReferenceCount = 0;
    uint32_t m_AV1SingleReferenceNameMask = 0;
    uint32_t m_AV1MaxUnidirectionalCompoundReferenceCount = 0;
    uint32_t m_AV1UnidirectionalCompoundReferenceNameMask = 0;
    uint32_t m_AV1MaxBidirectionalCompoundReferenceCount = 0;
    uint32_t m_AV1BidirectionalCompoundReferenceNameMask = 0;
    VkExtent2D m_AV1MaxTiles = {};
    VkExtent2D m_AV1MinTileSize = {};
    VkExtent2D m_AV1MaxTileSize = {};
    uint32_t m_AV1MinQIndex = 0;
    uint32_t m_AV1MaxQIndex = 255;
    VkVideoEncodeAV1StdFlagsKHR m_AV1StdSyntaxFlags = 0;
    bool m_AV1RequiresGopRemainingFrames = false;
    bool m_ResetRecorded = false;
    bool m_UseInlineSessionParameters = false;
    bool m_CanGenerateH264PrefixNalu = false;
    uint32_t m_H264MaxBPictureL0ReferenceCount = 0;
    uint32_t m_H264MaxL1ReferenceCount = 0;
    uint32_t m_H265MaxBPictureL0ReferenceCount = 0;
    uint32_t m_H265MaxL1ReferenceCount = 0;
};

} // namespace nri
