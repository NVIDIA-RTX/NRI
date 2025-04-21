// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetVK.h"

namespace nri {

struct DescriptorPoolVK final : public DebugNameBase {
    inline DescriptorPoolVK(DeviceVK& device)
        : m_Device(device)
        , m_DescriptorSets(device.GetStdAllocator()) {
    }

    inline operator VkDescriptorPool() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~DescriptorPoolVK();

    Result Create(const DescriptorPoolDesc& descriptorPoolDesc);
    Result Create(const DescriptorPoolVKDesc& descriptorPoolVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void Reset();
    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);

private:
    DeviceVK& m_Device;
    VkDescriptorPool m_Handle = VK_NULL_HANDLE;
    Vector<DescriptorSetVK> m_DescriptorSets;
    uint32_t m_DescriptorSetNum = 0;
    bool m_OwnsNativeObjects = true;
    Lock m_Lock;
};

} // namespace nri