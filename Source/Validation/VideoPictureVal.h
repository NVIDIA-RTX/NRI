// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoPictureVal final : public ObjectVal {
    VideoPictureVal(DeviceVal& device, VideoPicture* impl);

    VideoPicture* GetImpl() const;
};
} // namespace nri
