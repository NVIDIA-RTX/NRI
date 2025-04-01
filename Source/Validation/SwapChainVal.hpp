// © 2021 NVIDIA Corporation

SwapChainVal::~SwapChainVal() {
    for (size_t i = 0; i < m_Textures.size(); i++)
        Destroy(m_Textures[i]);
}

NRI_INLINE Texture* const* SwapChainVal::GetTextures(uint32_t& textureNum) {
    Texture* const* textures = GetSwapChainInterface().GetSwapChainTextures(*GetImpl(), textureNum);

    if (m_Textures.empty()) {
        for (uint32_t i = 0; i < textureNum; i++) {
            TextureVal* textureVal = Allocate<TextureVal>(m_Device.GetAllocationCallbacks(), m_Device, textures[i], true);
            m_Textures.push_back(textureVal);
        }
    }

    return (Texture* const*)m_Textures.data();
}

NRI_INLINE uint32_t SwapChainVal::AcquireNextTexture() {
    return GetSwapChainInterface().AcquireNextSwapChainTexture(*GetImpl());
}

NRI_INLINE Result SwapChainVal::WaitForPresent() {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.waitable, Result::FAILURE, "Swap chain has not been created with 'waitable = true'");

    return GetSwapChainInterface().WaitForPresent(*GetImpl());
}

NRI_INLINE Result SwapChainVal::Present() {
    return GetSwapChainInterface().QueuePresent(*GetImpl());
}

NRI_INLINE Result SwapChainVal::GetDisplayDesc(DisplayDesc& displayDesc) const {
    return GetSwapChainInterface().GetDisplayDesc(*GetImpl(), displayDesc);
}

NRI_INLINE Result SwapChainVal::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().SetLatencySleepMode(*GetImpl(), latencySleepMode);
}

NRI_INLINE Result SwapChainVal::SetLatencyMarker(LatencyMarker latencyMarker) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().SetLatencyMarker(*GetImpl(), latencyMarker);
}

NRI_INLINE Result SwapChainVal::LatencySleep() {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().LatencySleep(*GetImpl());
}

NRI_INLINE Result SwapChainVal::GetLatencyReport(LatencyReport& latencyReport) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().GetLatencyReport(*GetImpl(), latencyReport);
}
