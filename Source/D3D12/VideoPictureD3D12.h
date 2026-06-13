// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoPictureD3D12 final : public DebugNameBase {
    inline VideoPictureD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char*) NRI_DEBUG_NAME_OVERRIDE {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const VideoPictureDesc& videoPictureDesc);

    DeviceD3D12& m_Device;
    TextureD3D12* m_Texture = nullptr;
    uint32_t m_Subresource = 0;
};

} // namespace nri
