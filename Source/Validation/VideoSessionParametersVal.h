// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionParametersVal final : public ObjectVal {
    VideoSessionParametersVal(DeviceVal& device, VideoSessionParameters* impl, VideoSessionVal& session);

    VideoSessionParameters* GetImpl() const;
    VideoSessionVal& GetSession() const;

private:
    VideoSessionVal& m_Session;
};
} // namespace nri
