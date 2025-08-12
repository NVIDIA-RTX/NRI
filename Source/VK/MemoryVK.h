// © 2021 NVIDIA Corporation

#pragma once

#include <limits>

namespace nri {

struct BufferVK;
struct TextureVK;

struct MemoryVK final : public DebugNameBase {
    inline MemoryVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline VkDeviceMemory GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline MemoryType GetType() const {
        return m_Type;
    }

    inline uint8_t* GetMappedMemory() const {
        return m_MappedMemory;
    }

    ~MemoryVK();

    Result Create(const MemoryVKDesc& memoryVKDesc);
    Result Create(const AllocateMemoryDesc& allocateMemoryDesc);
    Result CreateDedicated(const BufferVK& buffer);
    Result CreateDedicated(const TextureVK& texture);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    DeviceVK& m_Device;
    VkDeviceMemory m_Handle = VK_NULL_HANDLE;
    uint8_t* m_MappedMemory = nullptr;
    MemoryType m_Type = std::numeric_limits<MemoryType>::max();
    float m_Priority = 0.0f;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri
