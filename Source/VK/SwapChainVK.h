// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVK;
struct FenceVK;
struct TextureVK;

struct SwapChainVK final : public DisplayDescHelper, DebugNameBase {
    SwapChainVK(DeviceVK& device)
        : m_Device(device)
        , m_Textures(device.GetStdAllocator()) {
    }

    ~SwapChainVK();

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline uint64_t GetPresentId() const {
        return m_PresentId;
    }

    Result Create(const SwapChainDesc& swapChainDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline Result GetDisplayDesc(DisplayDesc& displayDesc) {
        return DisplayDescHelper::GetDisplayDesc(m_Hwnd, displayDesc);
    }

    Texture* const* GetTextures(uint32_t& textureNum) const;
    Result AcquireNextTexture(FenceVK& acquireSemaphore, uint32_t& textureIndex);
    Result WaitForPresent();
    Result Present(FenceVK& releaseSemaphore);

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(LatencyMarker latencyMarker);
    Result LatencySleep();
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    DeviceVK& m_Device;
    Vector<TextureVK*> m_Textures;
    FenceVK* m_LatencyFence = nullptr;
    VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    QueueVK* m_Queue = nullptr;
    void* m_Hwnd = nullptr;
    uint64_t m_PresentId = 0;
    uint32_t m_TextureIndex = 0;
    SwapChainBits m_Flags = SwapChainBits::NONE;
};

} // namespace nri