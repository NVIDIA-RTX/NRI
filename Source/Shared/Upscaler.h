// © 2025 NVIDIA Corporation

#pragma once

struct Nis;
struct Ngx;
struct Ffx;

namespace nri {

bool IsUpscalerSupported(const DeviceDesc& deviceDesc, UpscalerType type);

struct UpscalerImpl : public DebugNameBase {
    inline UpscalerImpl(Device& device, const CoreInterface& NRI)
        : m_Device(device)
        , m_NRI(NRI) {
    }

    ~UpscalerImpl();

    inline Device& GetDevice() {
        return m_Device;
    }

    Result Create(const UpscalerDesc& desc);
    void GetUpscalerProps(UpscalerProps& upscalerProps) const;
    void CmdDispatchUpscale(CommandBuffer& commandBuffer, const DispatchUpscaleDesc& dispatchUpscaleDesc);

private:
    Device& m_Device;
    const CoreInterface& m_NRI;
    UpscalerDesc m_Desc = {};

    union {
        Nis* nis;
        Ngx* ngx;
        Ffx* ffx;
    } m = {};
};

} // namespace nri
