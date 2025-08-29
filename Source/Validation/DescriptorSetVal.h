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

    //================================================================================================================
    // NRI
    //================================================================================================================

    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);

private:
    const DescriptorSetDesc* m_Desc = nullptr; // .natvis
};

} // namespace nri
