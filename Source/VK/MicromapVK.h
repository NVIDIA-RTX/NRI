// © 2025 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVK;
struct MemoryVK;

struct MicromapVK final : public DebugNameBase {
    inline MicromapVK(DeviceVK& device)
        : m_Device(device)
        , m_Usages(device.GetStdAllocator()) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline MicromapBits GetFlags() const {
        return m_Flags;
    }

    inline const VkMicromapUsageEXT* GetUsages() const {
        return m_Usages.data();
    }

    inline uint32_t GetUsageNum() const {
        return (uint32_t)m_Usages.size();
    }

    ~MicromapVK();

    Result Create(const MicromapDesc& accelerationStructureDesc);
    Result AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(const MemoryVK* memory, uint64_t offset);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline uint64_t GetBuildScratchBufferSize() const {
        return m_BuildScratchSize;
    }

    inline BufferVK* GetBuffer() const {
        return m_Buffer;
    }

    inline VkMicromapEXT GetHandle() const {
        return m_Handle;
    }

private:
    DeviceVK& m_Device;
    VkMicromapEXT m_Handle = VK_NULL_HANDLE;
    BufferVK* m_Buffer = nullptr;
    Vector<VkMicromapUsageEXT> m_Usages;
    uint64_t m_BuildScratchSize = 0;
    MicromapBits m_Flags = MicromapBits::NONE;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri
