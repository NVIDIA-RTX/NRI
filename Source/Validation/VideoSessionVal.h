// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionVal final : public ObjectVal {
    VideoSessionVal(DeviceVal& device, VideoSession* impl);

    VideoSession* GetImpl() const;
};
} // namespace nri
