// © 2025 NVIDIA Corporation

#pragma once

struct Nis;
struct Ffx;
struct Xess;
struct Ngx;

namespace nri {

bool IsUpscalerSupported(const DeviceDesc& deviceDesc, UpscalerType type);

struct UpscalerImpl final : public DebugNameBase {
    inline UpscalerImpl(Device& device, const CoreInterface& NRI)
        : m_Device(device)
        , m_iCore(NRI) {
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
    const CoreInterface& m_iCore;
    UpscalerDesc m_Desc = {};

#if (NRI_ENABLE_NIS_SDK || NRI_ENABLE_FFX_SDK || NRI_ENABLE_XESS_SDK || NRI_ENABLE_NGX_SDK)
    union {
        Nis* nis;
        Ffx* ffx;
        Xess* xess;
        Ngx* ngx;
    } m = {};
#endif
};

} // namespace nri
