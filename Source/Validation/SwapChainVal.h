// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

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
    Result WaitForPresent(uint64_t presentId);
    Result Present(Fence& releaseSemaphore, uint64_t presentId);
    Result GetDisplayDesc(DisplayDesc& displayDesc) const;

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(uint64_t presentId, LatencyMarker latencyMarker);
    Result LatencySleep(uint64_t presentId);
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    SwapChainDesc m_SwapChainDesc = {}; // .natvis
    Vector<TextureVal*> m_Textures;
};

} // namespace nri
