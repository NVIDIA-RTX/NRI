// © 2025 NVIDIA Corporation

#pragma once

struct Nis;
struct Ffx;
struct Xess;
struct Ngx;
struct MetalFx;

#if NRI_ENABLE_METAL_SUPPORT
namespace NS {
class Object;
}
#endif

namespace nri {

bool IsUpscalerSupported(const DeviceDesc& deviceDesc, UpscalerType type);

#if NRI_ENABLE_METAL_SUPPORT
// Metal backend hooks for direct encoding into the native command buffer ("MTL4CommandBuffer" of "CommandBufferMetal", since "CommandBuffer" can be a validation wrapper)
void BeginNativeEncodingMetal(NS::Object* commandBuffer);                // ends the open encoder, emits pending barriers
void ReleaseOnResetMetal(NS::Object* commandBuffer, NS::Object* object); // releases "object" on reset of the command allocator
#endif

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

#if (NRI_ENABLE_NIS_SDK || NRI_ENABLE_FFX_SDK || NRI_ENABLE_XESS_SDK || NRI_ENABLE_NGX_SDK || NRI_ENABLE_METAL_SUPPORT)
    union {
        Nis* nis;
        Ffx* ffx;
        Xess* xess;
        Ngx* ngx;
        MetalFx* metalfx;
    } m = {};
#endif
};

} // namespace nri
