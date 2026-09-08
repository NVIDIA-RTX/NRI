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

    inline bool IsCopySource() const {
        return m_IsCopySource;
    }

    inline uint32_t GetDescriptorNum(uint32_t rangeIndex) const {
        const DescriptorRangeDesc& rangeDesc = m_Desc->ranges[rangeIndex];

        return (rangeDesc.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY) ? m_VariableDescriptorNum : rangeDesc.descriptorNum;
    }

    void SetImpl(DescriptorSet* impl, const DescriptorSetDesc* desc, uint32_t variableDescriptorNum, bool isCopySource);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void GetOffsets(uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) const;

private:
    const DescriptorSetDesc* m_Desc = nullptr; // .natvis
    uint32_t m_VariableDescriptorNum = 0;
    bool m_IsCopySource = false;
};

} // namespace nri
