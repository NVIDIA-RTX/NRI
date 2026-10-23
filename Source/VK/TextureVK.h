// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TextureVK final : public DebugNameBase {
    inline TextureVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline VkImage GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkExtent3D GetExtent() const {
        return {m_Desc.width, m_Desc.height, m_Desc.depth};
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline bool IsConcurrent() const {
        return m_IsConcurrent;
    }

    inline Dim_t GetSize(Dim_t dimensionIndex, Dim_t mip = 0) const {
        return GetDimension(GraphicsAPI::VK, m_Desc, dimensionIndex, mip);
    }

    ~TextureVK();

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureVKDesc& textureVKDesc);
    Result AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(const MemoryVK& memory, uint64_t offset);
    Result GetSharedHandle(SharedHandle& sharedHandle) const;
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    DeviceVK& m_Device;
    VkImage m_Handle = VK_NULL_HANDLE;
    TextureDesc m_Desc = {};
    VmaAllocation m_VmaAllocation = nullptr;
    VkDeviceMemory m_SharedMemory = VK_NULL_HANDLE;
    uint64_t m_SharedSize = 0;
    bool m_OwnsNativeObjects = true;
    bool m_IsConcurrent = false; // TODO: sharing mode is unknown for wrapped images
};

} // namespace nri
