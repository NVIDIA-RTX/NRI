// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandAllocatorVK final : public DebugNameBase {
    inline CommandAllocatorVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkCommandPool() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~CommandAllocatorVK();

    Result Create(const Queue& queue);
    Result Create(const CommandAllocatorVKDesc& commandAllocatorVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();

private:
    DeviceVK& m_Device;
    VkCommandPool m_Handle = VK_NULL_HANDLE;
    QueueType m_Type = (QueueType)0;
    bool m_OwnsNativeObjects = true;
    Lock m_Lock;
};

} // namespace nri
