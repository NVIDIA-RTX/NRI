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
    DescriptorHandleGPU GetDescriptorHandleGPU(uint32_t rangeIndex, uint32_t baseDescriptor) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    static void UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum);
    static void Copy(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum);

private:
    DescriptorPoolD3D12* m_DescriptorPoolD3D12 = nullptr;
    const DescriptorSetMapping* m_DescriptorSetMapping = nullptr; // saves 1 indirection
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> m_HeapOffsets = {};
};

} // namespace nri
