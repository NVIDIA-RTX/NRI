// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct FenceVal final : public ObjectVal {
    inline FenceVal(DeviceVal& device, Fence* fence, bool isSwapChainSemaphore = false)
        : ObjectVal(device, fence)
        , m_IsSwapChainSemaphore(isSwapChainSemaphore) {
    }

    inline ~FenceVal() {
    }

    inline Fence* GetImpl() const {
        return (Fence*)m_Impl;
    }

    inline bool IsSwapChainSemaphore() const {
        return m_IsSwapChainSemaphore;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    void Wait(uint64_t value);

private:
    bool m_IsSwapChainSemaphore = false;
};

} // namespace nri
