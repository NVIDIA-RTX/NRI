// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct SwapChainWGPU final : public DisplayDescHelper, public DebugNameBase {
    inline SwapChainWGPU(DeviceWGPU& device)
        : m_Device(device)
        , m_Textures(device.GetStdAllocator()) {
    }

    ~SwapChainWGPU();

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    Result Create(const SwapChainDesc& swapChainDesc);
    Texture* const* GetTextures(uint32_t& textureNum) const;
    Result GetDisplayDesc(DisplayDesc& displayDesc);
    Result AcquireNextTexture(uint32_t& textureIndex);
    Result WaitForPresent();
    Result Present();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        MaybeUnused(name);
    }

private:
    WGPUPresentMode GetPresentMode(const WGPUSurfaceCapabilities& capabilities, SwapChainBits flags) const;
    WGPUCompositeAlphaMode GetAlphaMode(const WGPUSurfaceCapabilities& capabilities) const;
    WGPUTextureFormat GetFormat(const WGPUSurfaceCapabilities& capabilities, SwapChainFormat format) const;

private:
    DeviceWGPU& m_Device;
    Vector<TextureWGPU*> m_Textures;
    WGPUSurface m_Surface = nullptr;
    void* m_Hwnd = nullptr;
    uint32_t m_TextureIndex = 0;
    uint32_t m_CurrentTextureIndex = uint32_t(-1);
    Dim_t m_Width = 0;
    Dim_t m_Height = 0;
    Format m_Format = Format::UNKNOWN;
};

} // namespace nri
