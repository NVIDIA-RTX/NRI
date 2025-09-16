// © 2021 NVIDIA Corporation

static bool ValidateBufferBarrierDesc(const DeviceVal& device, uint32_t i, const BufferBarrierDesc& bufferBarrier) {
    const BufferVal& bufferVal = *(const BufferVal*)bufferBarrier.buffer;

    RETURN_ON_FAILURE(&device, bufferBarrier.buffer != nullptr, false, "'bufferBarrier.buffers[%u].buffer' is NULL", i);
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usage, bufferBarrier.before.access), false,
        "'bufferBarrier.buffers[%u].before' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usage, bufferBarrier.after.access), false,
        "'bufferBarrier.buffers[%u].after' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());

    return true;
}

static bool ValidateTextureBarrierDesc(const DeviceVal& device, uint32_t i, const TextureBarrierDesc& textureBarrier) {
    const TextureVal& textureVal = *(const TextureVal*)textureBarrier.texture;

    RETURN_ON_FAILURE(&device, textureBarrier.texture != nullptr, false, "'bufferBarrier.textures[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usage, textureBarrier.before.access), false,
        "'bufferBarrier.textures[%u].before' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usage, textureBarrier.after.access), false,
        "'bufferBarrier.textures[%u].after' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usage, textureBarrier.before.layout), false,
        "'bufferBarrier.textures[%u].prevLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usage, textureBarrier.after.layout), false,
        "'bufferBarrier.textures[%u].nextLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());

    return true;
}

NRI_INLINE Result CommandBufferVal::Begin(const DescriptorPool* descriptorPool) {
    RETURN_ON_FAILURE(&m_Device, !m_IsRecordingStarted, Result::FAILURE, "already in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, descriptorPool);

    Result result = GetCoreInterfaceImpl().BeginCommandBuffer(*GetImpl(), descriptorPoolImpl);
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = true;

    m_Pipeline = nullptr;
    m_PipelineLayout = nullptr;

    ResetAttachments();

    return result;
}

NRI_INLINE Result CommandBufferVal::End() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, Result::FAILURE, "not in the recording state");

    if (m_AnnotationStack > 0)
        REPORT_ERROR(&m_Device, "'CmdBeginAnnotation' is called more times than 'CmdEndAnnotation'");
    else if (m_AnnotationStack < 0)
        REPORT_ERROR(&m_Device, "'CmdEndAnnotation' is called more times than 'CmdBeginAnnotation'");

    Result result = GetCoreInterfaceImpl().EndCommandBuffer(*GetImpl());
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = m_IsWrapped;

    return result;
}

NRI_INLINE void CommandBufferVal::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (!deviceDesc.features.viewportOriginBottomLeft) {
        for (uint32_t i = 0; i < viewportNum; i++) {
            RETURN_ON_FAILURE(&m_Device, !viewports[i].originBottomLeft, ReturnVoid(), "'features.viewportOriginBottomLeft' is false");
        }
    }

    GetCoreInterfaceImpl().CmdSetViewports(*GetImpl(), viewports, viewportNum);
}

NRI_INLINE void CommandBufferVal::SetScissors(const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdSetScissors(*GetImpl(), rects, rectNum);
}

NRI_INLINE void CommandBufferVal::SetDepthBounds(float boundsMin, float boundsMax) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.depthBoundsTest, ReturnVoid(), "'features.depthBoundsTest' is false");

    GetCoreInterfaceImpl().CmdSetDepthBounds(*GetImpl(), boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferVal::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdSetStencilReference(*GetImpl(), frontRef, backRef);
}

NRI_INLINE void CommandBufferVal::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.sampleLocations != 0, ReturnVoid(), "'tiers.sampleLocations > 0' required");

    GetCoreInterfaceImpl().CmdSetSampleLocations(*GetImpl(), locations, locationNum, sampleNum);
}

NRI_INLINE void CommandBufferVal::SetBlendConstants(const Color32f& color) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdSetBlendConstants(*GetImpl(), color);
}

NRI_INLINE void CommandBufferVal::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.shadingRate, ReturnVoid(), "'tiers.shadingRate > 0' required");

    GetCoreInterfaceImpl().CmdSetShadingRate(*GetImpl(), shadingRateDesc);
}

NRI_INLINE void CommandBufferVal::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.dynamicDepthBias, ReturnVoid(), "'features.dynamicDepthBias' is false");

    GetCoreInterfaceImpl().CmdSetDepthBias(*GetImpl(), depthBiasDesc);
}

NRI_INLINE void CommandBufferVal::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    for (uint32_t i = 0; i < clearDescNum; i++) {
        RETURN_ON_FAILURE(&m_Device, (clearDescs[i].planes & (PlaneBits::COLOR | PlaneBits::DEPTH | PlaneBits::STENCIL)) != 0, ReturnVoid(), "'[%u].planes' is not COLOR, DEPTH or STENCIL", i);

        if (clearDescs[i].planes & PlaneBits::COLOR) {
            RETURN_ON_FAILURE(&m_Device, clearDescs[i].colorAttachmentIndex < deviceDesc.shaderStage.fragment.attachmentMaxNum, ReturnVoid(), "'[%u].colorAttachmentIndex=%u' is out of bounds", i, clearDescs[i].colorAttachmentIndex);
            RETURN_ON_FAILURE(&m_Device, m_RenderTargets[clearDescs[i].colorAttachmentIndex], ReturnVoid(), "'[%u].colorAttachmentIndex=%u' references a NULL COLOR attachment", i, clearDescs[i].colorAttachmentIndex);
        }

        if (clearDescs[i].planes & (PlaneBits::DEPTH | PlaneBits::STENCIL))
            RETURN_ON_FAILURE(&m_Device, m_DepthStencil, ReturnVoid(), "DEPTH_STENCIL attachment is NULL", i);

        if (clearDescs[i].colorAttachmentIndex != 0)
            RETURN_ON_FAILURE(&m_Device, (clearDescs[i].planes & PlaneBits::COLOR), ReturnVoid(), "'[%u].planes' is not COLOR, but `colorAttachmentIndex != 0`", i);
    }

    GetCoreInterfaceImpl().CmdClearAttachments(*GetImpl(), clearDescs, clearDescNum, rects, rectNum);
}

NRI_INLINE void CommandBufferVal::ClearStorage(const ClearStorageDesc& clearDesc) {
    const DescriptorVal& descriptorVal = *(DescriptorVal*)clearDesc.storage;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, clearDesc.storage, ReturnVoid(), "'.storage' is NULL");
    RETURN_ON_FAILURE(&m_Device, descriptorVal.IsShaderResourceStorage(), ReturnVoid(), "'.storage' is not a 'SHADER_RESOURCE_STORAGE' resource");
    // TODO: check that a descriptor set is bound, minimal tracking of sets is needed

    auto clearDescImpl = clearDesc;
    clearDescImpl.storage = NRI_GET_IMPL(Descriptor, clearDesc.storage);

    GetCoreInterfaceImpl().CmdClearStorage(*GetImpl(), clearDescImpl);
}

NRI_INLINE void CommandBufferVal::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has been already called");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (attachmentsDesc.shadingRate)
        RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.shadingRate, ReturnVoid(), "'tiers.shadingRate >= 2' required");

    Scratch<Descriptor*> colors = AllocateScratch(m_Device, Descriptor*, attachmentsDesc.colorNum);
    for (uint32_t i = 0; i < attachmentsDesc.colorNum; i++)
        colors[i] = NRI_GET_IMPL(Descriptor, attachmentsDesc.colors[i]);

    auto attachmentsDescImpl = attachmentsDesc;
    attachmentsDescImpl.depthStencil = NRI_GET_IMPL(Descriptor, attachmentsDesc.depthStencil);
    attachmentsDescImpl.shadingRate = NRI_GET_IMPL(Descriptor, attachmentsDesc.shadingRate);
    attachmentsDescImpl.colors = colors;
    attachmentsDescImpl.colorNum = attachmentsDesc.colorNum;

    m_IsRenderPass = true;
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    size_t i = 0;
    for (; i < m_RenderTargetNum; i++)
        m_RenderTargets[i] = (DescriptorVal*)attachmentsDesc.colors[i];
    for (; i < m_RenderTargets.size(); i++)
        m_RenderTargets[i] = nullptr;

    if (attachmentsDesc.depthStencil)
        m_DepthStencil = (DescriptorVal*)attachmentsDesc.depthStencil;
    else
        m_DepthStencil = nullptr;

    ValidateReadonlyDepthStencil();

    GetCoreInterfaceImpl().CmdBeginRendering(*GetImpl(), attachmentsDescImpl);
}

NRI_INLINE void CommandBufferVal::EndRendering() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has not been called");

    m_IsRenderPass = false;

    ResetAttachments();

    GetCoreInterfaceImpl().CmdEndRendering(*GetImpl());
}

NRI_INLINE void CommandBufferVal::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Scratch<VertexBufferDesc> vertexBufferDescsImpl = AllocateScratch(m_Device, VertexBufferDesc, vertexBufferNum);
    for (uint32_t i = 0; i < vertexBufferNum; i++) {
        vertexBufferDescsImpl[i] = vertexBufferDescs[i];
        vertexBufferDescsImpl[i].buffer = NRI_GET_IMPL(Buffer, vertexBufferDescs[i].buffer);
    }

    GetCoreInterfaceImpl().CmdSetVertexBuffers(*GetImpl(), baseSlot, vertexBufferDescsImpl, vertexBufferNum);
}

NRI_INLINE void CommandBufferVal::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterfaceImpl().CmdSetIndexBuffer(*GetImpl(), *bufferImpl, offset, indexType);
}

NRI_INLINE void CommandBufferVal::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, bindPoint != BindPoint::INHERIT, ReturnVoid(), "'INHERIT' is not allowed");

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    m_PipelineLayout = (PipelineLayoutVal*)&pipelineLayout;

    GetCoreInterfaceImpl().CmdSetPipelineLayout(*GetImpl(), bindPoint, *pipelineLayoutImpl);
}

NRI_INLINE void CommandBufferVal::SetPipeline(const Pipeline& pipeline) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Pipeline* pipelineImpl = NRI_GET_IMPL(Pipeline, &pipeline);

    m_Pipeline = (PipelineVal*)&pipeline;

    ValidateReadonlyDepthStencil();

    GetCoreInterfaceImpl().CmdSetPipeline(*GetImpl(), *pipelineImpl);
}

NRI_INLINE void CommandBufferVal::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, &descriptorPool);

    GetCoreInterfaceImpl().CmdSetDescriptorPool(*GetImpl(), *descriptorPoolImpl);
}

NRI_INLINE void CommandBufferVal::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");

    auto descriptorSetBindingDescImpl = setDescriptorSetDesc;
    descriptorSetBindingDescImpl.descriptorSet = NRI_GET_IMPL(DescriptorSet, setDescriptorSetDesc.descriptorSet);

    GetCoreInterfaceImpl().CmdSetDescriptorSet(*GetImpl(), descriptorSetBindingDescImpl);
}

NRI_INLINE void CommandBufferVal::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    RETURN_ON_FAILURE(&m_Device, setRootConstantsDesc.offset == 0 || deviceDesc.features.rootConstantsOffset, ReturnVoid(), "Non-zero 'setRootConstantsDesc.offset' is not supported");

    GetCoreInterfaceImpl().CmdSetRootConstants(*GetImpl(), setRootConstantsDesc);
}

NRI_INLINE void CommandBufferVal::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    const DescriptorVal& descriptorVal = *(DescriptorVal*)setRootDescriptorDesc.descriptor;
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    RETURN_ON_FAILURE(&m_Device, descriptorVal.IsBufferView() || descriptorVal.IsAccelerationStructure(), ReturnVoid(), "'descriptor' must be a buffer or acceleration structure");

    if (!descriptorVal.IsConstantBuffer())
        RETURN_ON_FAILURE(&m_Device, setRootDescriptorDesc.offset == 0 || deviceDesc.features.nonConstantBufferRootDescriptorOffset, ReturnVoid(), "Non-zero 'setRootDescriptorDesc.offset' is supported only for 'CONSTANT_BUFFER'");

    auto rootDescriptorBindingDescImpl = setRootDescriptorDesc;
    rootDescriptorBindingDescImpl.descriptor = NRI_GET_IMPL(Descriptor, setRootDescriptorDesc.descriptor);

    GetCoreInterfaceImpl().CmdSetRootDescriptor(*GetImpl(), rootDescriptorBindingDescImpl);
}

NRI_INLINE void CommandBufferVal::Draw(const DrawDesc& drawDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterfaceImpl().CmdDraw(*GetImpl(), drawDesc);
}

NRI_INLINE void CommandBufferVal::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterfaceImpl().CmdDrawIndexed(*GetImpl(), drawIndexedDesc);
}

NRI_INLINE void CommandBufferVal::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterfaceImpl().CmdDrawIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterfaceImpl().CmdDrawIndexedIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    const BufferDesc& dstDesc = ((BufferVal&)dstBuffer).GetDesc();
    const BufferDesc& srcDesc = ((BufferVal&)srcBuffer).GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    if (size == WHOLE_SIZE) {
        RETURN_ON_FAILURE(&m_Device, dstOffset == 0, ReturnVoid(), "'WHOLE_SIZE' is used but 'dstOffset' is not 0");
        RETURN_ON_FAILURE(&m_Device, srcOffset == 0, ReturnVoid(), "'WHOLE_SIZE' is used but 'srcOffset' is not 0");
        RETURN_ON_FAILURE(&m_Device, dstDesc.size == srcDesc.size, ReturnVoid(), "'WHOLE_SIZE' is used but 'dstBuffer' and 'srcBuffer' have different sizes");
    } else {
        RETURN_ON_FAILURE(&m_Device, srcOffset + size <= srcDesc.size, ReturnVoid(), "'srcOffset + size' > srcBuffer.size");
        RETURN_ON_FAILURE(&m_Device, dstOffset + size <= dstDesc.size, ReturnVoid(), "'dstOffset + size' > dstBuffer.size");
    }

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterfaceImpl().CmdCopyBuffer(*GetImpl(), *dstBufferImpl, dstOffset, *srcBufferImpl, srcOffset, size);
}

NRI_INLINE void CommandBufferVal::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterfaceImpl().CmdCopyTexture(*GetImpl(), *dstTextureImpl, dstRegion, *srcTextureImpl, srcRegion);
}

NRI_INLINE void CommandBufferVal::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterfaceImpl().CmdResolveTexture(*GetImpl(), *dstTextureImpl, dstRegion, *srcTextureImpl, srcRegion);
}

NRI_INLINE void CommandBufferVal::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterfaceImpl().CmdUploadBufferToTexture(*GetImpl(), *dstTextureImpl, dstRegion, *srcBufferImpl, srcDataLayout);
}

NRI_INLINE void CommandBufferVal::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterfaceImpl().CmdReadbackTextureToBuffer(*GetImpl(), *dstBufferImpl, dstDataLayout, *srcTextureImpl, srcRegion);
}

NRI_INLINE void CommandBufferVal::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    if (size == WHOLE_SIZE) {
        RETURN_ON_FAILURE(&m_Device, offset == 0, ReturnVoid(), "'WHOLE_SIZE' is used but 'offset' is not 0");
    } else {
        const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
        RETURN_ON_FAILURE(&m_Device, offset + size <= bufferDesc.size, ReturnVoid(), "'offset + size' > buffer.size");
    }
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterfaceImpl().CmdZeroBuffer(*GetImpl(), *bufferImpl, offset, size);
}

NRI_INLINE void CommandBufferVal::Dispatch(const DispatchDesc& dispatchDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterfaceImpl().CmdDispatch(*GetImpl(), dispatchDesc);
}

NRI_INLINE void CommandBufferVal::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    GetCoreInterfaceImpl().CmdDispatchIndirect(*GetImpl(), *bufferImpl, offset);
}

NRI_INLINE void CommandBufferVal::Barrier(const BarrierDesc& barrierDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
        if (!ValidateBufferBarrierDesc(m_Device, i, barrierDesc.buffers[i]))
            return;
    }

    for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
        if (!ValidateTextureBarrierDesc(m_Device, i, barrierDesc.textures[i]))
            return;
    }

    Scratch<BufferBarrierDesc> buffers = AllocateScratch(m_Device, BufferBarrierDesc, barrierDesc.bufferNum);
    memcpy(buffers, barrierDesc.buffers, sizeof(BufferBarrierDesc) * barrierDesc.bufferNum);
    for (uint32_t i = 0; i < barrierDesc.bufferNum; i++)
        buffers[i].buffer = NRI_GET_IMPL(Buffer, barrierDesc.buffers[i].buffer);

    Scratch<TextureBarrierDesc> textures = AllocateScratch(m_Device, TextureBarrierDesc, barrierDesc.textureNum);
    memcpy(textures, barrierDesc.textures, sizeof(TextureBarrierDesc) * barrierDesc.textureNum);
    for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
        textures[i].texture = NRI_GET_IMPL(Texture, barrierDesc.textures[i].texture);
        textures[i].srcQueue = NRI_GET_IMPL(Queue, barrierDesc.textures[i].srcQueue);
        textures[i].dstQueue = NRI_GET_IMPL(Queue, barrierDesc.textures[i].dstQueue);
    }

    auto barrierGroupDescImpl = barrierDesc;
    barrierGroupDescImpl.buffers = buffers;
    barrierGroupDescImpl.textures = textures;

    GetCoreInterfaceImpl().CmdBarrier(*GetImpl(), barrierGroupDescImpl);
}

NRI_INLINE void CommandBufferVal::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, queryPoolVal.GetQueryType() != QueryType::TIMESTAMP, ReturnVoid(), "'BeginQuery' is not supported for timestamp queries");

    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset=%u' is out of range", offset);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterfaceImpl().CmdBeginQuery(*GetImpl(), *queryPoolImpl, offset);
}

NRI_INLINE void CommandBufferVal::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset=%u' is out of range", offset);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterfaceImpl().CmdEndQuery(*GetImpl(), *queryPoolImpl, offset);
}

NRI_INLINE void CommandBufferVal::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num = %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);

    GetCoreInterfaceImpl().CmdCopyQueries(*GetImpl(), *queryPoolImpl, offset, num, *dstBufferImpl, dstOffset);
}

NRI_INLINE void CommandBufferVal::ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num = %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterfaceImpl().CmdResetQueries(*GetImpl(), *queryPoolImpl, offset, num);
}

NRI_INLINE void CommandBufferVal::BeginAnnotation(const char* name, uint32_t bgra) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    m_AnnotationStack++;
    GetCoreInterfaceImpl().CmdBeginAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE void CommandBufferVal::EndAnnotation() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdEndAnnotation(*GetImpl());
    m_AnnotationStack--;
}

NRI_INLINE void CommandBufferVal::Annotation(const char* name, uint32_t bgra) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE void CommandBufferVal::BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Scratch<BuildTopLevelAccelerationStructureDesc> buildTopLevelAccelerationStructureDescsImpl = AllocateScratch(m_Device, BuildTopLevelAccelerationStructureDesc, buildTopLevelAccelerationStructureDescNum);

    for (uint32_t i = 0; i < buildTopLevelAccelerationStructureDescNum; i++) {
        const BuildTopLevelAccelerationStructureDesc& in = buildTopLevelAccelerationStructureDescs[i];
        const BufferVal* instanceBufferVal = (BufferVal*)in.instanceBuffer;
        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'dst' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.instanceBuffer, ReturnVoid(), "'instanceBuffer' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.instanceOffset <= instanceBufferVal->GetDesc().size, ReturnVoid(), "'instanceOffset=%" PRIu64 "' is out of bounds", in.instanceOffset);
        RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset=%" PRIu64 "' is out of bounds", in.scratchOffset);

        auto& out = buildTopLevelAccelerationStructureDescsImpl[i];
        out = in;
        out.dst = NRI_GET_IMPL(AccelerationStructure, in.dst);
        out.src = NRI_GET_IMPL(AccelerationStructure, in.src);
        out.instanceBuffer = NRI_GET_IMPL(Buffer, in.instanceBuffer);
        out.scratchBuffer = NRI_GET_IMPL(Buffer, in.scratchBuffer);
    }

    GetRayTracingInterfaceImpl().CmdBuildTopLevelAccelerationStructures(*GetImpl(), buildTopLevelAccelerationStructureDescsImpl, buildTopLevelAccelerationStructureDescNum);
}

NRI_INLINE void CommandBufferVal::BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    uint32_t geometryTotalNum = 0;
    uint32_t micromapTotalNum = 0;

    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& desc = buildBottomLevelAccelerationStructureDescs[i];

        for (uint32_t j = 0; j < desc.geometryNum; j++) {
            const BottomLevelGeometryDesc& geometry = desc.geometries[j];

            if (geometry.type == BottomLevelGeometryType::TRIANGLES && geometry.triangles.micromap)
                micromapTotalNum++;
        }

        geometryTotalNum += desc.geometryNum;
    }

    Scratch<BuildBottomLevelAccelerationStructureDesc> buildBottomLevelAccelerationStructureDescsImpl = AllocateScratch(m_Device, BuildBottomLevelAccelerationStructureDesc, buildBottomLevelAccelerationStructureDescNum);
    Scratch<BottomLevelGeometryDesc> geometriesImplScratch = AllocateScratch(m_Device, BottomLevelGeometryDesc, geometryTotalNum);
    Scratch<BottomLevelMicromapDesc> micromapsImplScratch = AllocateScratch(m_Device, BottomLevelMicromapDesc, micromapTotalNum);

    BottomLevelGeometryDesc* geometriesImpl = geometriesImplScratch;
    BottomLevelMicromapDesc* micromapsImpl = micromapsImplScratch;

    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& in = buildBottomLevelAccelerationStructureDescs[i];
        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'dst' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.geometries, ReturnVoid(), "'geometries' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset=%" PRIu64 "' is out of bounds", in.scratchOffset);

        auto& out = buildBottomLevelAccelerationStructureDescsImpl[i];
        out = in;
        out.dst = NRI_GET_IMPL(AccelerationStructure, in.dst);
        out.src = NRI_GET_IMPL(AccelerationStructure, in.src);
        out.geometries = geometriesImpl;
        out.scratchBuffer = NRI_GET_IMPL(Buffer, in.scratchBuffer);

        ConvertBotomLevelGeometries(in.geometries, in.geometryNum, geometriesImpl, micromapsImpl);
    }

    GetRayTracingInterfaceImpl().CmdBuildBottomLevelAccelerationStructures(*GetImpl(), buildBottomLevelAccelerationStructureDescsImpl, buildBottomLevelAccelerationStructureDescNum);
}

NRI_INLINE void CommandBufferVal::BuildMicromaps(const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Scratch<BuildMicromapDesc> buildMicromapDescsImpl = AllocateScratch(m_Device, BuildMicromapDesc, buildMicromapDescNum);

    for (uint32_t i = 0; i < buildMicromapDescNum; i++) {
        const BuildMicromapDesc& in = buildMicromapDescs[i];
        const BufferVal* dataBufferVal = (BufferVal*)in.dataBuffer;
        const BufferVal* triangleBufferVal = (BufferVal*)in.triangleBuffer;
        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'dst' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.dataBuffer, ReturnVoid(), "'dataBuffer' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.triangleBuffer, ReturnVoid(), "'triangleBuffer' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.dataOffset <= dataBufferVal->GetDesc().size, ReturnVoid(), "'dataOffset=%" PRIu64 "' is out of bounds", in.dataOffset);
        RETURN_ON_FAILURE(&m_Device, in.triangleOffset <= triangleBufferVal->GetDesc().size, ReturnVoid(), "'triangleOffset=%" PRIu64 "' is out of bounds", in.triangleOffset);
        RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset=%" PRIu64 "' is out of bounds", in.scratchOffset);

        auto& out = buildMicromapDescsImpl[i];
        out = in;
        out.dst = NRI_GET_IMPL(Micromap, in.dst);
        out.dataBuffer = NRI_GET_IMPL(Buffer, in.dataBuffer);
        out.triangleBuffer = NRI_GET_IMPL(Buffer, in.triangleBuffer);
        out.scratchBuffer = NRI_GET_IMPL(Buffer, in.scratchBuffer);
    }

    GetRayTracingInterfaceImpl().CmdBuildMicromaps(*GetImpl(), buildMicromapDescsImpl, buildMicromapDescNum);
}

NRI_INLINE void CommandBufferVal::CopyMicromap(Micromap& dst, const Micromap& src, CopyMode copyMode) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(), "'copyMode' is invalid");

    Micromap& dstImpl = *NRI_GET_IMPL(Micromap, &dst);
    Micromap& srcImpl = *NRI_GET_IMPL(Micromap, &src);

    GetRayTracingInterfaceImpl().CmdCopyMicromap(*GetImpl(), dstImpl, srcImpl, copyMode);
}

NRI_INLINE void CommandBufferVal::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(), "'copyMode' is invalid");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);

    GetRayTracingInterfaceImpl().CmdCopyAccelerationStructure(*GetImpl(), dstImpl, srcImpl, copyMode);
}

NRI_INLINE void CommandBufferVal::WriteMicromapsSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    const QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    bool isTypeValid = queryPoolVal.GetQueryType() == QueryType::MICROMAP_COMPACTED_SIZE;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, isTypeValid, ReturnVoid(), "'queryPool' query type must be 'MICROMAP_COMPACTED_SIZE'");

    Scratch<Micromap*> micromapsImpl = AllocateScratch(m_Device, Micromap*, micromapNum);
    for (uint32_t i = 0; i < micromapNum; i++) {
        RETURN_ON_FAILURE(&m_Device, micromaps[i], ReturnVoid(), "'micromaps[%u]' is NULL", i);

        micromapsImpl[i] = NRI_GET_IMPL(Micromap, micromaps[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL(QueryPool, &queryPool);

    GetRayTracingInterfaceImpl().CmdWriteMicromapsSizes(*GetImpl(), micromapsImpl, micromapNum, queryPoolImpl, queryPoolOffset);
}

NRI_INLINE void CommandBufferVal::WriteAccelerationStructuresSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    const QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    bool isTypeValid = queryPoolVal.GetQueryType() == QueryType::ACCELERATION_STRUCTURE_SIZE || queryPoolVal.GetQueryType() == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, isTypeValid, ReturnVoid(), "'queryPool' query type must be 'ACCELERATION_STRUCTURE_SIZE' or 'ACCELERATION_STRUCTURE_COMPACTED_SIZE'");

    Scratch<AccelerationStructure*> accelerationStructuresImpl = AllocateScratch(m_Device, AccelerationStructure*, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++) {
        RETURN_ON_FAILURE(&m_Device, accelerationStructures[i], ReturnVoid(), "'accelerationStructures[%u]' is NULL", i);

        accelerationStructuresImpl[i] = NRI_GET_IMPL(AccelerationStructure, accelerationStructures[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL(QueryPool, &queryPool);

    GetRayTracingInterfaceImpl().CmdWriteAccelerationStructuresSizes(*GetImpl(), accelerationStructuresImpl, accelerationStructureNum, queryPoolImpl, queryPoolOffset);
}

NRI_INLINE void CommandBufferVal::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint64_t align = deviceDesc.memoryAlignment.shaderBindingTable;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.buffer, ReturnVoid(), "'raygenShader.buffer' is NULL");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.size != 0, ReturnVoid(), "'raygenShader.size' is 0");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.offset % align == 0, ReturnVoid(), "'raygenShader.offset' is misaligned");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.missShaders.offset % align == 0, ReturnVoid(), "'missShaders.offset' is misaligned");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.hitShaderGroups.offset % align == 0, ReturnVoid(), "'hitShaderGroups.offset' is misaligned");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.callableShaders.offset % align == 0, ReturnVoid(), "'callableShaders.offset' is misaligned");

    auto dispatchRaysDescImpl = dispatchRaysDesc;
    dispatchRaysDescImpl.raygenShader.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.raygenShader.buffer);
    dispatchRaysDescImpl.missShaders.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.missShaders.buffer);
    dispatchRaysDescImpl.hitShaderGroups.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.hitShaderGroups.buffer);
    dispatchRaysDescImpl.callableShaders.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.callableShaders.buffer);

    GetRayTracingInterfaceImpl().CmdDispatchRays(*GetImpl(), dispatchRaysDescImpl);
}

NRI_INLINE void CommandBufferVal::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.rayTracing >= 2, ReturnVoid(), "'tiers.rayTracing' must be >= 2");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetRayTracingInterfaceImpl().CmdDispatchRaysIndirect(*GetImpl(), *bufferImpl, offset);
}

NRI_INLINE void CommandBufferVal::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.meshShader, ReturnVoid(), "'features.meshShader' is false");

    GetMeshShaderInterfaceImpl().CmdDrawMeshTasks(*GetImpl(), drawMeshTasksDesc);
}

NRI_INLINE void CommandBufferVal::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.features.meshShader, ReturnVoid(), "'features.meshShader' is false");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "'offset' is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetMeshShaderInterfaceImpl().CmdDrawMeshTasksIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::ValidateReadonlyDepthStencil() {
    if (m_Pipeline && m_DepthStencil) {
        if (m_DepthStencil->IsDepthReadonly() && m_Pipeline->WritesToDepth())
            REPORT_WARNING(&m_Device, "Depth is read-only, but the pipeline writes to depth. Writing happens only in VK!");

        if (m_DepthStencil->IsStencilReadonly() && m_Pipeline->WritesToStencil())
            REPORT_WARNING(&m_Device, "Stencil is read-only, but the pipeline writes to stencil. Writing happens only in VK!");
    }
}
