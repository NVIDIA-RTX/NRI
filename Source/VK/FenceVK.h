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

    Result Create(const FenceDesc& fenceDesc);
    Result Create(const FenceVKDesc& fenceVKDesc);
    Result GetSharedHandle(SharedHandle& sharedHandle) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    Result Wait(uint64_t value);

private:
    DeviceVK& m_Device;
    VkSemaphore m_Handle = VK_NULL_HANDLE;
    mutable void* m_SharedHandle = nullptr; // Win32: the one export of "m_Handle", duplicated for each "GetSharedHandle"
    bool m_OwnsNativeObjects = true;
};

} // namespace nri
