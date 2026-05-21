// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionParametersVal final : public ObjectVal {
    VideoSessionParametersVal(DeviceVal& device, VideoSessionParameters* impl);

    VideoSessionParameters* GetImpl() const;
};
} // namespace nri
