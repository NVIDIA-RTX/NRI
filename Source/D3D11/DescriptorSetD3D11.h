// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorD3D11;
struct PipelineLayoutD3D11;

struct OffsetNum {
    uint32_t descriptorOffset;
    uint32_t descriptorNum;
};

struct DescriptorSetD3D11 final : public DebugNameBase {
    inline DescriptorSetD3D11(DeviceD3D11& device)
        : m_Ranges(device.GetStdAllocator()) {
    }

    inline const DescriptorD3D11* GetDescriptor(uint32_t i) const {
        return m_Descriptors[i];
    }

    inline uint32_t GetDynamicConstantBufferNum() const {
        return m_DynamicConstantBuffersNum;
    }

    uint32_t Initialize(const PipelineLayoutD3D11& pipelineLayout, uint32_t setIndex, const DescriptorD3D11** descriptors);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    void UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors);
    void Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc);

private:
    Vector<OffsetNum> m_Ranges;
    const DescriptorD3D11** m_Descriptors = nullptr;
    uint32_t m_DynamicConstantBuffersNum = 0;
};

} // namespace nri
