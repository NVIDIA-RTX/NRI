// © 2026 NVIDIA Corporation

#pragma once

#include <atomic>

namespace nri {

struct FenceMetal final : public DebugNameBase {
    inline FenceMetal(DeviceMetal& device) : m_Device(device) {
    }

    ~FenceMetal();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL::SharedEvent* GetNativeObject() const {
        return m_Event;
    }

    inline bool IsSwapChainSemaphore() const {
        return m_IsSwapChainSemaphore;
    }

    inline uint64_t GetScheduledValue() const {
        return m_NextValue.load(std::memory_order_acquire);
    }

    Result Create(uint64_t initialValue);
    Result Create(const FenceMetalDesc& desc);
    uint64_t GetValue() const;
    uint64_t NextSignalValue();
    Result Wait(uint64_t value);
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    DeviceMetal& m_Device;
    MTL::SharedEvent* m_Event = nullptr;
    std::atomic<uint64_t> m_NextValue{0};
    bool m_IsSwapChainSemaphore = false;
};

} // namespace nri
