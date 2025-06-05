// © 2021 NVIDIA Corporation

#pragma once

struct IDXGISwapChain4;
typedef IDXGISwapChain4 IDXGISwapChainBest;

namespace nri {

struct TextureD3D12;

struct SwapChainD3D12 final : public DisplayDescHelper, DebugNameBase {
    inline SwapChainD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_Textures(device.GetStdAllocator()) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    ~SwapChainD3D12();

    Result Create(const SwapChainDesc& swapChainDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_SwapChain, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline Result GetDisplayDesc(DisplayDesc& displayDesc) {
        return DisplayDescHelper::GetDisplayDesc(m_Hwnd, displayDesc);
    }

    Texture* const* GetTextures(uint32_t& textureNum) const;
    Result AcquireNextTexture(uint32_t& textureIndex);
    Result WaitForPresent();
    Result Present();

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(LatencyMarker latencyMarker);
    Result LatencySleep();
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    DeviceD3D12& m_Device;
    ComPtr<IDXGISwapChainBest> m_SwapChain;
    Vector<TextureD3D12*> m_Textures;
    HANDLE m_FrameLatencyWaitableObject = nullptr;
    void* m_Hwnd = nullptr;
    uint64_t m_PresentId = 0;
    uint8_t m_Version = 0;
    SwapChainBits m_Flags = SwapChainBits::NONE;
};

} // namespace nri
