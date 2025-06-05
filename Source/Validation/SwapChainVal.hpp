// © 2021 NVIDIA Corporation

SwapChainVal::~SwapChainVal() {
    for (size_t i = 0; i < m_Textures.size(); i++)
        Destroy(m_Textures[i]);
}

NRI_INLINE Texture* const* SwapChainVal::GetTextures(uint32_t& textureNum) {
    Texture* const* textures = GetSwapChainInterfaceImpl().GetSwapChainTextures(*GetImpl(), textureNum);

    if (m_Textures.empty()) {
        for (uint32_t i = 0; i < textureNum; i++) {
            TextureVal* textureVal = Allocate<TextureVal>(m_Device.GetAllocationCallbacks(), m_Device, textures[i], true);
            m_Textures.push_back(textureVal);
        }
    }

    return (Texture* const*)m_Textures.data();
}

NRI_INLINE Result SwapChainVal::AcquireNextTexture(Fence& acquireSemaphore, uint32_t& textureIndex) {
    Fence* textureAcquiredSemaphoreImpl = NRI_GET_IMPL(Fence, &acquireSemaphore);

    return GetSwapChainInterfaceImpl().AcquireNextTexture(*GetImpl(), *textureAcquiredSemaphoreImpl, textureIndex);
}

NRI_INLINE Result SwapChainVal::WaitForPresent() {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::WAITABLE, Result::FAILURE, "Swap chain has not been created with 'WAITABLE' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.waitableSwapChain, Result::FAILURE, "'features.waitableSwapChain' is false");

    return GetSwapChainInterfaceImpl().WaitForPresent(*GetImpl());
}

NRI_INLINE Result SwapChainVal::Present(Fence& releaseSemaphore) {
    Fence* renderingFinishedSemaphoreImpl = NRI_GET_IMPL(Fence, &releaseSemaphore);

    return GetSwapChainInterfaceImpl().QueuePresent(*GetImpl(), *renderingFinishedSemaphoreImpl);
}

NRI_INLINE Result SwapChainVal::GetDisplayDesc(DisplayDesc& displayDesc) const {
    return GetSwapChainInterfaceImpl().GetDisplayDesc(*GetImpl(), displayDesc);
}

NRI_INLINE Result SwapChainVal::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().SetLatencySleepMode(*GetImpl(), latencySleepMode);
}

NRI_INLINE Result SwapChainVal::SetLatencyMarker(LatencyMarker latencyMarker) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().SetLatencyMarker(*GetImpl(), latencyMarker);
}

NRI_INLINE Result SwapChainVal::LatencySleep() {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().LatencySleep(*GetImpl());
}

NRI_INLINE Result SwapChainVal::GetLatencyReport(LatencyReport& latencyReport) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().GetLatencyReport(*GetImpl(), latencyReport);
}
