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

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline MemoryType GetType() const {
        return m_Type;
    }

    // Starting from "offset"
    inline uint8_t* GetMappedMemory() const {
        if (m_VmaAllocation) {
            VmaAllocationInfo allocationInfo = {};
            vmaGetAllocationInfo(m_Device.GetVma(), m_VmaAllocation, &allocationInfo);

            return (uint8_t*)allocationInfo.pMappedData;
        }

        return m_MappedMemory;
    }

    // Starting from "0" (must be used with "GetOffset")
    inline VkDeviceMemory GetHandle() const {
        if (m_VmaAllocation) {
            VmaAllocationInfo allocationInfo = {};
            vmaGetAllocationInfo(m_Device.GetVma(), m_VmaAllocation, &allocationInfo);

            return allocationInfo.deviceMemory;
        }

        return m_Handle;
    }

    inline uint64_t GetOffset() const {
        if (m_VmaAllocation) {
            VmaAllocationInfo allocationInfo = {};
            vmaGetAllocationInfo(m_Device.GetVma(), m_VmaAllocation, &allocationInfo);

            return allocationInfo.offset;
        }

        return m_Offset;
    }

    ~MemoryVK();

    Result Create(const MemoryVKDesc& memoryVKDesc);
    Result Create(const AllocateMemoryDesc& allocateMemoryDesc);
    Result CreateDedicated(const BufferVK* buffer, const TextureVK* texture);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    DeviceVK& m_Device;
    VkDeviceMemory m_Handle = VK_NULL_HANDLE;
    VmaAllocation m_VmaAllocation = nullptr;
    uint64_t m_Offset = 0;
    uint8_t* m_MappedMemory = nullptr;
    MemoryType m_Type = std::numeric_limits<MemoryType>::max();
    float m_Priority = 0.0f;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri
