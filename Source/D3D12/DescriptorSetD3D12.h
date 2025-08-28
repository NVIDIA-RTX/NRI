// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorPoolD3D12;
struct DescriptorSetMapping;

struct DescriptorSetD3D12 final : public DebugNameBase {
    inline DescriptorSetD3D12() {
    }

    void Create(DescriptorPoolD3D12* desriptorPoolD3D12, const DescriptorSetMapping* descriptorSetMapping, std::array<uint32_t, DescriptorHeapType::MAX_NUM>& heapOffsets);
    DeviceD3D12& GetDevice() const;
    DescriptorPointerCPU GetDescriptorPointerCPU(uint32_t rangeIndex, uint32_t rangeOffset) const;
    DescriptorPointerGPU GetDescriptorPointerGPU(uint32_t rangeIndex, uint32_t rangeOffset) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    void Copy(const CopyDescriptorSetDesc& copyDescriptorSetDesc);

private:
    DescriptorPoolD3D12* m_DescriptorPoolD3D12 = nullptr;
    const DescriptorSetMapping* m_DescriptorSetMapping = nullptr; // saves 1 indirection
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> m_HeapOffsets = {};
};

} // namespace nri
