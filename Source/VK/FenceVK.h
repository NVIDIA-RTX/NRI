// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct FenceVK final : public DebugNameBase {
    inline FenceVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkSemaphore() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~FenceVK();

    Result Create(uint64_t initialValue);
    Result Create(const FenceVKDesc& fenceVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    void Wait(uint64_t value);

private:
    DeviceVK& m_Device;
    VkSemaphore m_Handle = VK_NULL_HANDLE;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri
