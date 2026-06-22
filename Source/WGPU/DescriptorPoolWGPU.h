// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorPoolWGPU final : public DebugNameBase {
    inline DescriptorPoolWGPU(DeviceWGPU& device)
        : m_Device(device)
        , m_DescriptorSets(device.GetStdAllocator()) {
    }

    ~DescriptorPoolWGPU();

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    Result Create(const DescriptorPoolDesc& descriptorPoolDesc);
    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);
    void Reset();

private:
    DeviceWGPU& m_Device;
    Vector<DescriptorSetWGPU*> m_DescriptorSets;
    DescriptorPoolDesc m_Desc = {};
};

} // namespace nri
