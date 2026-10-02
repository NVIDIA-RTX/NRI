// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionParametersVal final : public ObjectVal {
    VideoSessionParametersVal(DeviceVal& device, VideoSessionParameters* impl, VideoSessionVal& session, const VideoSessionParametersDesc& desc);

    VideoSessionParameters* GetImpl() const;
    VideoSessionVal& GetSession() const;
    bool IsH264ParameterSetValid(uint8_t sequenceParameterSetId, uint8_t pictureParameterSetId) const;
    bool IsH265ParameterSetValid(uint8_t videoParameterSetId, uint8_t sequenceParameterSetId, uint8_t pictureParameterSetId) const;
    bool HasH265Parameters() const;

private:
    VideoSessionVal& m_Session;
    std::array<uint8_t, 256> m_H264PpsToSpsPlusOne = {};
    std::array<uint8_t, 64> m_H265PpsToSpsPlusOne = {};
    std::array<uint8_t, 64> m_H265PpsToVpsPlusOne = {};
    bool m_HasH265Parameters = false;
};
} // namespace nri
