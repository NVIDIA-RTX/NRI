// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetD3D12.h"

namespace nri {

struct DescriptorPoolD3D12 final : public DebugNameBase {
    inline DescriptorPoolD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_DescriptorSets(device.GetStdAllocator()) {
    }

    inline ~DescriptorPoolD3D12() {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const DescriptorPoolDesc& descriptorPoolDesc);
    Result Create(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc);

    void Bind(ID3D12GraphicsCommandList* graphicsCommandList) const;
    DescriptorHandleCPU GetDescriptorHandleCPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const;
    DescriptorHandleGPU GetDescriptorHandleGPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);
    void Reset();

private:
    DeviceD3D12& m_Device;
    std::array<DescriptorHeapDesc, DescriptorHeapType::MAX_NUM> m_DescriptorHeapDescs = {};
    std::array<ID3D12DescriptorHeap*, DescriptorHeapType::MAX_NUM> m_DescriptorHeaps = {};
    Vector<DescriptorSetD3D12> m_DescriptorSets;
    uint32_t m_DescriptorHeapNum = 0;
    uint32_t m_DescriptorSetNum = 0;
    Lock m_Lock;
};

} // namespace nri
