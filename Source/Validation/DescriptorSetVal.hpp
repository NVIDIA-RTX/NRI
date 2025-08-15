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

NRI_INLINE void DescriptorSetVal::UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    if (dynamicConstantBufferNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, baseDynamicConstantBuffer + dynamicConstantBufferNum <= GetDesc().dynamicConstantBufferNum, ReturnVoid(),
        "'baseDynamicConstantBuffer=%u' + 'dynamicConstantBufferNum=%u' is greater than 'dynamicConstantBufferNum=%u' in the set",
        baseDynamicConstantBuffer, dynamicConstantBufferNum, GetDesc().dynamicConstantBufferNum);

    RETURN_ON_FAILURE(&m_Device, descriptors != nullptr, ReturnVoid(), "'descriptors' is NULL");

    Scratch<Descriptor*> descriptorsImpl = AllocateScratch(m_Device, Descriptor*, dynamicConstantBufferNum);
    for (uint32_t i = 0; i < dynamicConstantBufferNum; i++) {
        RETURN_ON_FAILURE(&m_Device, descriptors[i] != nullptr, ReturnVoid(), "'descriptors[%u]' is NULL", i);

        descriptorsImpl[i] = NRI_GET_IMPL(Descriptor, descriptors[i]);
    }

    for (uint32_t i = 0; i < dynamicConstantBufferNum; i++)
        m_DynamicConstantBuffersMask |= 1 << std::min(baseDynamicConstantBuffer + i, 30u);

    GetCoreInterfaceImpl().UpdateDynamicConstantBuffers(*GetImpl(), baseDynamicConstantBuffer, dynamicConstantBufferNum, descriptorsImpl);
}

NRI_INLINE void DescriptorSetVal::Copy(const CopyDescriptorSetDesc& copyDescriptorSetDesc) {
    RETURN_ON_FAILURE(&m_Device, copyDescriptorSetDesc.srcDescriptorSet != nullptr, ReturnVoid(), "'srcDescriptorSet' is NULL");

    DescriptorSetVal& srcDescriptorSetVal = *(DescriptorSetVal*)copyDescriptorSetDesc.srcDescriptorSet;
    const DescriptorSetDesc& srcDesc = srcDescriptorSetVal.GetDesc();

    RETURN_ON_FAILURE(&m_Device, copyDescriptorSetDesc.srcBaseRange < srcDesc.rangeNum, ReturnVoid(), "'srcBaseRange' is invalid");

    bool srcRangeValid = copyDescriptorSetDesc.srcBaseRange + copyDescriptorSetDesc.rangeNum < srcDesc.rangeNum;
    bool dstRangeValid = copyDescriptorSetDesc.dstBaseRange + copyDescriptorSetDesc.rangeNum < GetDesc().rangeNum;
    bool srcOffsetValid = copyDescriptorSetDesc.srcBaseDynamicConstantBuffer < srcDesc.dynamicConstantBufferNum;
    bool srcDynamicConstantBufferValid = copyDescriptorSetDesc.srcBaseDynamicConstantBuffer + copyDescriptorSetDesc.dynamicConstantBufferNum < srcDesc.dynamicConstantBufferNum;
    bool dstOffsetValid = copyDescriptorSetDesc.dstBaseDynamicConstantBuffer < GetDesc().dynamicConstantBufferNum;
    bool dstDynamicConstantBufferValid = copyDescriptorSetDesc.dstBaseDynamicConstantBuffer + copyDescriptorSetDesc.dynamicConstantBufferNum < GetDesc().dynamicConstantBufferNum;

    RETURN_ON_FAILURE(&m_Device, srcRangeValid, ReturnVoid(), "'rangeNum' is invalid");
    RETURN_ON_FAILURE(&m_Device, copyDescriptorSetDesc.dstBaseRange < GetDesc().rangeNum, ReturnVoid(), "'dstBaseRange' is invalid");
    RETURN_ON_FAILURE(&m_Device, dstRangeValid, ReturnVoid(), "'rangeNum' is invalid");
    RETURN_ON_FAILURE(&m_Device, srcOffsetValid, ReturnVoid(), "'srcBaseDynamicConstantBuffer' is invalid");
    RETURN_ON_FAILURE(&m_Device, srcDynamicConstantBufferValid, ReturnVoid(), "source range of dynamic constant buffers is invalid");
    RETURN_ON_FAILURE(&m_Device, dstOffsetValid, ReturnVoid(), "'dstBaseDynamicConstantBuffer' is invalid");
    RETURN_ON_FAILURE(&m_Device, dstDynamicConstantBufferValid, ReturnVoid(), "destination range of dynamic constant buffers is invalid");

    auto descriptorSetCopyDescImpl = copyDescriptorSetDesc;
    descriptorSetCopyDescImpl.srcDescriptorSet = NRI_GET_IMPL(DescriptorSet, copyDescriptorSetDesc.srcDescriptorSet);

    GetCoreInterfaceImpl().CopyDescriptorSet(*GetImpl(), descriptorSetCopyDescImpl);
}
