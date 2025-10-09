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

    inline Dim_t GetSize(Dim_t dimensionIndex, Dim_t mip = 0) const {
        return GetDimension(GraphicsAPI::VK, m_Desc, dimensionIndex, mip);
    }

    ~TextureVK();

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureVKDesc& textureVKDesc);
    Result AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(const MemoryVK& memory, uint64_t offset);
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    VkImageAspectFlags GetImageAspectFlags() const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    DeviceVK& m_Device;
    VkImage m_Handle = VK_NULL_HANDLE;
    TextureDesc m_Desc = {};
    VmaAllocation m_VmaAllocation = nullptr;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri