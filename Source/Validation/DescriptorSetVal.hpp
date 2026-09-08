// © 2021 NVIDIA Corporation

NRI_INLINE void DescriptorSetVal::SetImpl(DescriptorSet* impl, const DescriptorSetDesc* desc, uint32_t variableDescriptorNum, bool isCopySource) {
    m_Impl = impl;
    m_Desc = desc;
    m_VariableDescriptorNum = variableDescriptorNum;
    m_IsCopySource = isCopySource;
}

NRI_INLINE void DescriptorSetVal::GetOffsets(uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) const {
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsCopySource, ReturnVoid(), "'descriptorSet' must not be allocated from a pool with 'DescriptorPoolBits::COPY_SOURCE'");

    GetCoreInterfaceImpl().GetDescriptorSetOffsets(*GetImpl(), resourceHeapOffset, samplerHeapOffset);
}

