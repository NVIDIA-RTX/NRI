// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TextureVal;

struct SwapChainVal final : public ObjectVal {
    SwapChainVal(DeviceVal& device, SwapChain* swapChain, const SwapChainDesc& swapChainDesc)
        : ObjectVal(device, swapChain)
        , m_SwapChainDesc(swapChainDesc)
        , m_Textures(device.GetStdAllocator()) {
    }

    ~SwapChainVal();

    inline SwapChain* GetImpl() const {
        return (SwapChain*)m_Impl;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Texture* const* GetTextures(uint32_t& textureNum);
    Result AcquireNextTexture(Fence& acquireSemaphore, uint32_t& textureIndex);
    Result WaitForPresent();
    Result Present(Fence& releaseSemaphore);
    Result GetDisplayDesc(DisplayDesc& displayDesc) const;

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(LatencyMarker latencyMarker);
    Result LatencySleep();
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    SwapChainDesc m_SwapChainDesc = {}; // .natvis
    Vector<TextureVal*> m_Textures;
};

} // namespace nri
