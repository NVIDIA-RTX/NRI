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
    bool IsSameExtent(const VideoPictureVal& videoPicture) const;
    bool HasTextureExtent(uint32_t width, uint32_t height) const;
    uint32_t GetTextureLayerNum() const;
    bool IsOutputOnly() const;
    bool IsReferenceOnly() const;

private:
    const TextureVal* m_Texture = nullptr;
    Format m_Format = Format::UNKNOWN;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Layer = 0;
    uint32_t m_TextureLayerNum = 0;
    uint32_t m_TextureWidth = 0;
    uint32_t m_TextureHeight = 0;
    VideoCodec m_Codec = VideoCodec::NONE;
    VideoPictureUsage m_Usage = VideoPictureUsage::MAX_NUM;
    bool m_IsOutputOnly = false;
    bool m_IsReferenceOnly = false;
    bool m_IsAV1FilmGrain = false;
};
} // namespace nri
