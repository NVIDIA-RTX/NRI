// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct SwapChainVK final : public DisplayDescHelper, DebugNameBase {
    SwapChainVK(DeviceVK& device)
        : m_Device(device)
        , m_Textures(device.GetStdAllocator()) {
    }

    ~SwapChainVK();

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkSwapchainKHR GetNativeObject() const {
        return m_Handle;
    }

    Result Create(const SwapChainDesc& swapChainDesc);
    Result Create(const SwapChainVKDesc& swapChainVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline Result GetDisplayDesc(DisplayDesc& displayDesc) {
        return DisplayDescHelper::GetDisplayDesc(m_Hwnd, displayDesc);
    }

    Texture* const* GetTextures(uint32_t& textureNum) const;
    Result AcquireNextTexture(FenceVK& acquireSemaphore, uint32_t& textureIndex);
    Result WaitForPresent(uint64_t presentId);
    Result Present(FenceVK& releaseSemaphore, uint64_t presentId);

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(uint64_t presentId, LatencyMarker latencyMarker);
    Result LatencySleep(uint64_t presentId);
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    Result CreateTextures(VkFormat format, VkImageUsageFlags usage, Dim_t width, Dim_t height, Dim_t layerNum);

    DeviceVK& m_Device;
    Vector<TextureVK*> m_Textures;
    FenceVK* m_LatencyFence = nullptr;
    VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    QueueVK* m_Queue = nullptr;
    void* m_Hwnd = nullptr;
    uint32_t m_TextureIndex = 0;
    SwapChainBits m_Flags = SwapChainBits::NONE;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri
