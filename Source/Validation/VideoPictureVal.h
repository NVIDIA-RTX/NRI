// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoPictureVal final : public ObjectVal {
    VideoPictureVal(DeviceVal& device, VideoPicture* impl, const VideoPictureDesc& desc, const TextureDesc& textureDesc);

    VideoPicture* GetImpl() const;
    VideoPictureUsage GetUsage() const;
    bool IsCompatibleWith(const VideoSessionDesc& sessionDesc) const;
    bool IsSameSubresource(const VideoPictureVal& videoPicture) const;
    bool IsSameTexture(const VideoPictureVal& videoPicture) const;
    uint32_t GetTextureLayerNum() const;
    bool IsOutputOnly() const;

private:
    const TextureVal* m_Texture = nullptr;
    Format m_Format = Format::UNKNOWN;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Layer = 0;
    uint32_t m_TextureLayerNum = 0;
    VideoCodec m_Codec = VideoCodec::NONE;
    VideoPictureUsage m_Usage = VideoPictureUsage::MAX_NUM;
    bool m_IsOutputOnly = false;
};
} // namespace nri
