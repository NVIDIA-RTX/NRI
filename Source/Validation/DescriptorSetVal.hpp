// © 2021 NVIDIA Corporation

NRI_INLINE void DescriptorSetVal::SetImpl(DescriptorSet* impl, const DescriptorSetDesc* desc) {
    m_Impl = impl;
    m_Desc = desc;
}
