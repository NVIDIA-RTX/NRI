// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorPoolD3D12;
struct DescriptorSetMapping;

struct DescriptorSetD3D12 final : public DebugNameBase {
    inline DescriptorSetD3D12() {
    }

    void Create(DescriptorPoolD3D12* desriptorPoolD3D12, const DescriptorSetMapping* descriptorSetMapping, DescriptorPointerGPU* dynamicConstantBuffers, std::array<uint32_t, DescriptorHeapType::MAX_NUM>& heapOffsets);
    DeviceD3D12& GetDevice() const;
    DescriptorPointerCPU GetPointerCPU(uint32_t rangeIndex, uint32_t rangeOffset) const;
    DescriptorPointerGPU GetPointerGPU(uint32_t rangeIndex, uint32_t rangeOffset) const;
    DescriptorPointerGPU GetDynamicPointerGPU(uint32_t dynamicConstantBufferIndex) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    void UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors);
    void Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc);

private:
    DescriptorPoolD3D12* m_DescriptorPoolD3D12 = nullptr;
    DescriptorPointerGPU* m_DynamicConstantBuffers = nullptr; // TODO: saves 1 indirection, but makes "bad" access unsafe
    const DescriptorSetMapping* m_DescriptorSetMapping = nullptr; // saves 1 indirection
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> m_HeapOffsets = {};
};

} // namespace nri
