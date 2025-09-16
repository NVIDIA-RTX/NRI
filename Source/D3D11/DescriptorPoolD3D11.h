// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetD3D11.h"

namespace nri {

struct DescriptorPoolD3D11 final : public DebugNameBase {
    inline DescriptorPoolD3D11(DeviceD3D11& device)
        : m_Device(device)
        , m_DescriptorSets(device.GetStdAllocator())
        , m_DescriptorPool(device.GetStdAllocator()) {
    }

    inline ~DescriptorPoolD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    Result Create(const DescriptorPoolDesc& descriptorPoolDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);
    void Reset();

private:
    DeviceD3D11& m_Device;
    Vector<DescriptorSetD3D11> m_DescriptorSets;
    Vector<const DescriptorD3D11*> m_DescriptorPool;
    uint32_t m_DescriptorNum = 0;
    uint32_t m_DescriptorSetNum = 0;
    Lock m_Lock;
};

} // namespace nri
