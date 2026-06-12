// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionVal final : public ObjectVal {
    VideoSessionVal(DeviceVal& device, VideoSession* impl, const VideoSessionDesc& desc);

    VideoSession* GetImpl() const;
    const VideoSessionDesc& GetDesc() const;

private:
    VideoSessionDesc m_Desc = {};
};
} // namespace nri
