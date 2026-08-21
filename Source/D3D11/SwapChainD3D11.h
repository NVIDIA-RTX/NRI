// © 2021 NVIDIA Corporation

#pragma once

struct IDXGISwapChain4;
typedef IDXGISwapChain4 IDXGISwapChainBest;

namespace nri {

struct SwapChainD3D11 final : public DisplayDescHelper, DebugNameBase {
    inline SwapChainD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    ~SwapChainD3D11();

    Result Create(const SwapChainDesc& swapChainDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_SwapChain, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline Result GetDisplayDesc(DisplayDesc& displayDesc) {
        return DisplayDescHelper::GetDisplayDesc(m_Hwnd, displayDesc);
    }

    Texture* const* GetTextures(uint32_t& textureNum) const;
    Result AcquireNextTexture(uint32_t& textureIndex);
    Result WaitForPresent(uint64_t presentId);
    Result Present(uint64_t presentId);

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(uint64_t presentId, LatencyMarker latencyMarker);
    Result LatencySleep(uint64_t presentId);
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    DeviceD3D11& m_Device;
    ComPtr<IDXGISwapChainBest> m_SwapChain;
    TextureD3D11* m_Texture = nullptr;
    HANDLE m_FrameLatencyWaitableObject = nullptr;
    void* m_Hwnd = nullptr;
    uint8_t m_Version = 0;
    SwapChainBits m_Flags = SwapChainBits::NONE;
};

} // namespace nri
