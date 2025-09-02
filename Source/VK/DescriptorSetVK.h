// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorSetVK final : public DebugNameBase {
    inline DescriptorSetVK() {
    }

    inline VkDescriptorSet GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return *m_Device;
    }

    inline const DescriptorSetDesc* GetDesc() const {
        return m_Desc;
    }

    inline void Create(DeviceVK* device, VkDescriptorSet handle, const DescriptorSetDesc* desc) {
        m_Device = device;
        m_Handle = handle;
        m_Desc = desc;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    DeviceVK* m_Device = nullptr;
    VkDescriptorSet m_Handle = VK_NULL_HANDLE;
    const DescriptorSetDesc* m_Desc = nullptr;
};

} // namespace nri