// © 2021 NVIDIA Corporation

NRI_INLINE void DescriptorSetVal::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    RETURN_ON_FAILURE(&m_Device, rangeOffset < GetDesc().rangeNum, ReturnVoid(), "'rangeOffset=%u' is out of 'rangeNum=%u' in the set", rangeOffset, GetDesc().rangeNum);
    RETURN_ON_FAILURE(&m_Device, rangeOffset + rangeNum <= GetDesc().rangeNum, ReturnVoid(), "'rangeOffset=%u' + 'rangeNum=%u' is greater than 'rangeNum=%u' in the set", rangeOffset, rangeNum, GetDesc().rangeNum);

    uint32_t descriptorNum = 0;
    uint32_t descriptorOffset = 0;
    for (uint32_t i = 0; i < rangeNum; i++)
        descriptorNum += rangeUpdateDescs[i].descriptorNum;

    Scratch<DescriptorRangeUpdateDesc> rangeUpdateDescsImpl = AllocateScratch(m_Device, DescriptorRangeUpdateDesc, rangeNum);
    Scratch<Descriptor*> descriptorsImpl = AllocateScratch(m_Device, Descriptor*, descriptorNum);
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeUpdateDesc& updateDesc = rangeUpdateDescs[i];
        const DescriptorRangeDesc& rangeDesc = GetDesc().ranges[rangeOffset + i];

        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptorNum != 0, ReturnVoid(), "'[%u].descriptorNum' is 0", i);
        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors != nullptr, ReturnVoid(), "'[%u].descriptors' is NULL", i);

        RETURN_ON_FAILURE(&m_Device, updateDesc.baseDescriptor + updateDesc.descriptorNum <= rangeDesc.descriptorNum, ReturnVoid(),
            "[%u]: 'baseDescriptor=%u' + 'descriptorNum=%u' is greater than 'descriptorNum=%u' in the range (descriptorType=%s)",
            i, updateDesc.baseDescriptor, updateDesc.descriptorNum, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        rangeUpdateDescsImpl[i] = updateDesc;
        rangeUpdateDescsImpl[i].descriptors = descriptorsImpl + descriptorOffset;

        Descriptor** descriptors = (Descriptor**)rangeUpdateDescsImpl[i].descriptors;
        for (uint32_t j = 0; j < updateDesc.descriptorNum; j++) {
            RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors[j] != nullptr, ReturnVoid(), "'[%u].descriptors[%u]' is NULL", i, j);

            descriptors[j] = NRI_GET_IMPL(Descriptor, updateDesc.descriptors[j]);
        }

        descriptorOffset += updateDesc.descriptorNum;
    }

    GetCoreInterfaceImpl().UpdateDescriptorRanges(*GetImpl(), rangeOffset, rangeNum, rangeUpdateDescsImpl);
}

NRI_INLINE void DescriptorSetVal::Copy(const CopyDescriptorSetDesc& copyDescriptorSetDesc) {
    RETURN_ON_FAILURE(&m_Device, copyDescriptorSetDesc.srcDescriptorSet != nullptr, ReturnVoid(), "'srcDescriptorSet' is NULL");

    DescriptorSetVal& srcDescriptorSetVal = *(DescriptorSetVal*)copyDescriptorSetDesc.srcDescriptorSet;
    const DescriptorSetDesc& srcDesc = srcDescriptorSetVal.GetDesc();

    bool srcRangeValid = copyDescriptorSetDesc.srcBaseRange + copyDescriptorSetDesc.rangeNum < srcDesc.rangeNum;
    srcRangeValid = srcRangeValid && (copyDescriptorSetDesc.srcBaseRange < srcDesc.rangeNum);
    RETURN_ON_FAILURE(&m_Device, srcRangeValid, ReturnVoid(), "source range is invalid");

    bool dstRangeValid = copyDescriptorSetDesc.dstBaseRange + copyDescriptorSetDesc.rangeNum < GetDesc().rangeNum;
    dstRangeValid = dstRangeValid && (copyDescriptorSetDesc.dstBaseRange < GetDesc().rangeNum);
    RETURN_ON_FAILURE(&m_Device, dstRangeValid, ReturnVoid(), "destination range is invalid");

    auto descriptorSetCopyDescImpl = copyDescriptorSetDesc;
    descriptorSetCopyDescImpl.srcDescriptorSet = NRI_GET_IMPL(DescriptorSet, copyDescriptorSetDesc.srcDescriptorSet);

    GetCoreInterfaceImpl().CopyDescriptorSet(*GetImpl(), descriptorSetCopyDescImpl);
}
