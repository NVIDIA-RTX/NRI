// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVK;

struct BufferVK final : public DebugNameBase {
    inline BufferVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline VkBuffer GetHandle() const {
        return m_Handle;
    }

    inline VkDeviceAddress GetDeviceAddress() const {
        return m_DeviceAddress;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }

    ~BufferVK();

    Result Create(const BufferDesc& bufferDesc);
    Result Create(const BufferVKDesc& bufferVKDesc);
    Result AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(MemoryVK& memory, uint64_t offset, bool bindMemory);
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void* Map(uint64_t offset, uint64_t size);
    void Unmap();

private:
    DeviceVK& m_Device;
    VkBuffer m_Handle = VK_NULL_HANDLE;
    VkDeviceAddress m_DeviceAddress = 0;
    uint8_t* m_MappedMemory = nullptr;
    VkDeviceMemory m_NonCoherentDeviceMemory = VK_NULL_HANDLE;
    uint64_t m_NonCoherentDeviceMemoryOffset = 0;
    uint64_t m_MappedMemoryRangeSize = 0;
    uint64_t m_MappedMemoryRangeOffset = 0;
    BufferDesc m_Desc = {};
    VmaAllocation m_VmaAllocation = nullptr;
    bool m_OwnsNativeObjects = true;
};

inline VkDeviceAddress GetBufferDeviceAddress(const Buffer* buffer, uint64_t offset) {
    if (!buffer)
        return 0;

    if (buffer == HAS_BUFFER)
        return 1;

    return ((BufferVK*)buffer)->GetDeviceAddress() + offset;
}

} // namespace nri