// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionVal final : public ObjectVal {
    VideoSessionVal(DeviceVal& device, VideoSession* impl, const VideoSessionDesc& desc, const VideoCapabilities& capabilities);

    VideoSession* GetImpl() const;
    const VideoSessionDesc& GetDesc() const;
    const VideoCapabilities& GetCapabilities() const;
    bool IsResolvedMetadataRangeValid(const BufferVal& buffer, uint64_t offset) const;

private:
    VideoSessionDesc m_Desc = {};
    VideoCapabilities m_Capabilities = {};
};
} // namespace nri
