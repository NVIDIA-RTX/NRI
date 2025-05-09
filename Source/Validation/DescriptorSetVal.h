// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorSetVal final : public ObjectVal {
    DescriptorSetVal(DeviceVal& device)
        : ObjectVal(device) {
    }

    inline DescriptorSet* GetImpl() const {
        return (DescriptorSet*)m_Impl;
    }

    inline const DescriptorSetDesc& GetDesc() const {
        return *m_Desc;
    }

    inline void SetImpl(DescriptorSet* impl, const DescriptorSetDesc* desc) {
        m_Impl = impl;
        m_Desc = desc;
    }

    inline bool AreDynamicConstantBuffersValid() const {
        uint32_t mask = (1 << std::min(m_Desc->dynamicConstantBufferNum, 31u)) - 1;

        return mask == m_DynamicConstantBuffersMask;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    void UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors);
    void Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc);

private:
    const DescriptorSetDesc* m_Desc = nullptr;   // .natvis
    uint32_t m_DynamicConstantBuffersMask = 0; // hopefully no one is going to create more than 31
};

} // namespace nri
