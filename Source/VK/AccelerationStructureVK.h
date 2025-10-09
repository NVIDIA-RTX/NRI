// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVK;
struct MemoryVK;

struct AccelerationStructureVK final : public DebugNameBase {
    inline AccelerationStructureVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline AccelerationStructureBits GetFlags() const {
        return m_Flags;
    }

    ~AccelerationStructureVK();

    Result Create(const AccelerationStructureDesc& accelerationStructureDesc);
    Result Create(const AccelerationStructureVKDesc& accelerationStructureVKDesc);
    Result AllocateAndBindMemory(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(const MemoryVK* memory, uint64_t offset);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline uint64_t GetUpdateScratchBufferSize() const {
        return m_UpdateScratchSize;
    }

    inline uint64_t GetBuildScratchBufferSize() const {
        return m_BuildScratchSize;
    }

    inline VkDeviceAddress GetDeviceAddress() const {
        return m_DeviceAddress;
    }

    inline BufferVK* GetBuffer() const {
        return m_Buffer;
    }

    inline VkAccelerationStructureKHR GetHandle() const {
        return m_Handle;
    }

    Result CreateDescriptor(Descriptor*& descriptor) const;

private:
    DeviceVK& m_Device;
    VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
    VkDeviceAddress m_DeviceAddress = 0;
    BufferVK* m_Buffer = nullptr;
    uint64_t m_BuildScratchSize = 0;
    uint64_t m_UpdateScratchSize = 0;
    VkAccelerationStructureTypeKHR m_Type = (VkAccelerationStructureTypeKHR)0; // needed only for "BindMemory"
    AccelerationStructureBits m_Flags = AccelerationStructureBits::NONE;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri