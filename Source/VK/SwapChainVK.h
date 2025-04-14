// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVK;
struct FenceVK;
struct TextureVK;

// Let's keep things simple and hide it under the hood
constexpr uint32_t MAX_NUMBER_OF_FRAMES_IN_FLIGHT = 8;

struct SwapChainVK final : public DisplayDescHelper, DebugNameBase {
    SwapChainVK(DeviceVK& device);
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
    uint32_t AcquireNextTexture();
    Result WaitForPresent();
    Result Present();

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(LatencyMarker latencyMarker);
    Result LatencySleep();
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    DeviceVK& m_Device;
    Vector<TextureVK*> m_Textures;
    FenceVK* m_LatencyFence = nullptr;
    std::array<VkSemaphore, MAX_NUMBER_OF_FRAMES_IN_FLIGHT> m_ImageAcquiredSemaphores = {VK_NULL_HANDLE};
    std::array<VkSemaphore, MAX_NUMBER_OF_FRAMES_IN_FLIGHT> m_RenderingFinishedSemaphores = {VK_NULL_HANDLE};
    VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    QueueVK* m_Queue = nullptr;
    void* m_Hwnd = nullptr;
    uint64_t m_PresentId = 0;
    uint32_t m_TextureIndex = 0;
    uint8_t m_FrameIndex = 0; // in flight, not global
    bool m_AllowLowLatency = false;
    bool m_Waitable = false;
};

} // namespace nri