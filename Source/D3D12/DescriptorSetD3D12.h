// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorPoolD3D12;
struct DescriptorSetMapping;

struct DescriptorSetD3D12 final : public DebugNameBase {
    inline DescriptorSetD3D12(DescriptorPoolD3D12& desriptorPoolD3D12)
        : m_DescriptorPoolD3D12(desriptorPoolD3D12) {
    }

    ~DescriptorSetD3D12();

    void Create(const DescriptorSetMapping* descriptorSetMapping, uint16_t dynamicConstantBufferNum);
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
    DescriptorPoolD3D12& m_DescriptorPoolD3D12;
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> m_HeapOffset = {};
    Vector<DescriptorPointerGPU>* m_DynamicConstantBuffers = nullptr;
    const DescriptorSetMapping* m_DescriptorSetMapping = nullptr;
};

} // namespace nri
