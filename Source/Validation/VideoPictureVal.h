// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoPictureVal final : public ObjectVal {
    VideoPictureVal(DeviceVal& device, VideoPicture* impl, const VideoPictureDesc& desc, const TextureDesc& textureDesc);

    VideoPicture* GetImpl() const;
    VideoPictureUsage GetUsage() const;
    bool IsCompatibleWith(const VideoSessionDesc& sessionDesc) const;

private:
    Format m_Format = Format::UNKNOWN;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    VideoCodec m_Codec = VideoCodec::NONE;
    VideoPictureUsage m_Usage = VideoPictureUsage::MAX_NUM;
};
} // namespace nri
