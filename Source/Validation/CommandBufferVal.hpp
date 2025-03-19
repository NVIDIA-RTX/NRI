// © 2021 NVIDIA Corporation

void ConvertGeometryObjectsVal(BottomLevelGeometry* destObjects, const BottomLevelGeometry* sourceObjects, uint32_t objectNum);

static bool ValidateBufferBarrierDesc(const DeviceVal& device, uint32_t i, const BufferBarrierDesc& bufferBarrierDesc) {
    const BufferVal& bufferVal = *(const BufferVal*)bufferBarrierDesc.buffer;

    RETURN_ON_FAILURE(&device, bufferBarrierDesc.buffer != nullptr, false, "'bufferBarrierDesc.buffers[%u].buffer' is NULL", i);
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usage, bufferBarrierDesc.before.access), false,
        "'bufferBarrierDesc.buffers[%u].before' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usage, bufferBarrierDesc.after.access), false,
        "'bufferBarrierDesc.buffers[%u].after' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());

    return true;
}

static bool ValidateTextureBarrierDesc(const DeviceVal& device, uint32_t i, const TextureBarrierDesc& textureBarrierDesc) {
    const TextureVal& textureVal = *(const TextureVal*)textureBarrierDesc.texture;

    RETURN_ON_FAILURE(&device, textureBarrierDesc.texture != nullptr, false, "'bufferBarrierDesc.textures[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usage, textureBarrierDesc.before.access), false,
        "'bufferBarrierDesc.textures[%u].before' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usage, textureBarrierDesc.after.access), false,
        "'bufferBarrierDesc.textures[%u].after' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usage, textureBarrierDesc.before.layout), false,
        "'bufferBarrierDesc.textures[%u].prevLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usage, textureBarrierDesc.after.layout), false,
        "'bufferBarrierDesc.textures[%u].nextLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());

    return true;
}

NRI_INLINE Result CommandBufferVal::Begin(const DescriptorPool* descriptorPool) {
    RETURN_ON_FAILURE(&m_Device, !m_IsRecordingStarted, Result::FAILURE, "already in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, descriptorPool);

    Result result = GetCoreInterface().BeginCommandBuffer(*GetImpl(), descriptorPoolImpl);
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

    Result result = GetCoreInterface().EndCommandBuffer(*GetImpl());
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = m_IsWrapped;

    return result;
}

NRI_INLINE void CommandBufferVal::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!viewportNum)
        return;

    RETURN_ON_FAILURE(&m_Device, viewports, ReturnVoid(), "'viewports' is NULL");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (!deviceDesc.isViewportOriginBottomLeftSupported) {
        for (uint32_t i = 0; i < viewportNum; i++) {
            RETURN_ON_FAILURE(&m_Device, !viewports[i].originBottomLeft, ReturnVoid(), "'isViewportOriginBottomLeftSupported' is false");
        }
    }

    GetCoreInterface().CmdSetViewports(*GetImpl(), viewports, viewportNum);
}

NRI_INLINE void CommandBufferVal::SetScissors(const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!rectNum)
        return;

    RETURN_ON_FAILURE(&m_Device, rects, ReturnVoid(), "'rects' is NULL");

    GetCoreInterface().CmdSetScissors(*GetImpl(), rects, rectNum);
}

NRI_INLINE void CommandBufferVal::SetDepthBounds(float boundsMin, float boundsMax) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.isDepthBoundsTestSupported, ReturnVoid(), "'isDepthBoundsTestSupported' is false");

    GetCoreInterface().CmdSetDepthBounds(*GetImpl(), boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferVal::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdSetStencilReference(*GetImpl(), frontRef, backRef);
}

NRI_INLINE void CommandBufferVal::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.sampleLocationsTier != 0, ReturnVoid(), "'sampleLocationsTier > 0' required");

    GetCoreInterface().CmdSetSampleLocations(*GetImpl(), locations, locationNum, sampleNum);
}

NRI_INLINE void CommandBufferVal::SetBlendConstants(const Color32f& color) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdSetBlendConstants(*GetImpl(), color);
}

NRI_INLINE void CommandBufferVal::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.shadingRateTier, ReturnVoid(), "'shadingRateTier > 0' required");

    GetCoreInterface().CmdSetShadingRate(*GetImpl(), shadingRateDesc);
}

NRI_INLINE void CommandBufferVal::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.isDynamicDepthBiasSupported, ReturnVoid(), "'isDynamicDepthBiasSupported' is false");

    GetCoreInterface().CmdSetDepthBias(*GetImpl(), depthBiasDesc);
}

NRI_INLINE void CommandBufferVal::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    for (uint32_t i = 0; i < clearDescNum; i++) {
        RETURN_ON_FAILURE(&m_Device, (clearDescs[i].planes & (PlaneBits::COLOR | PlaneBits::DEPTH | PlaneBits::STENCIL)) != 0, ReturnVoid(), "'[%u].planes' is not COLOR, DEPTH or STENCIL", i);

        if (clearDescs[i].planes & PlaneBits::COLOR) {
            RETURN_ON_FAILURE(&m_Device, clearDescs[i].colorAttachmentIndex < deviceDesc.colorAttachmentMaxNum, ReturnVoid(), "'[%u].colorAttachmentIndex = %u' is out of bounds", i, clearDescs[i].colorAttachmentIndex);
            RETURN_ON_FAILURE(&m_Device, m_RenderTargets[clearDescs[i].colorAttachmentIndex], ReturnVoid(), "'[%u].colorAttachmentIndex = %u' references a NULL COLOR attachment", i, clearDescs[i].colorAttachmentIndex);
        }

        if (clearDescs[i].planes & (PlaneBits::DEPTH | PlaneBits::STENCIL))
            RETURN_ON_FAILURE(&m_Device, m_DepthStencil, ReturnVoid(), "DEPTH_STENCIL attachment is NULL", i);

        if (clearDescs[i].colorAttachmentIndex != 0)
            RETURN_ON_FAILURE(&m_Device, (clearDescs[i].planes & PlaneBits::COLOR), ReturnVoid(), "'[%u].planes' is not COLOR, but `colorAttachmentIndex != 0`", i);
    }

    GetCoreInterface().CmdClearAttachments(*GetImpl(), clearDescs, clearDescNum, rects, rectNum);
}

NRI_INLINE void CommandBufferVal::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, clearDesc.storageBuffer, ReturnVoid(), "'.storageBuffer' is NULL");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageBuffer = NRI_GET_IMPL(Descriptor, clearDesc.storageBuffer);

    GetCoreInterface().CmdClearStorageBuffer(*GetImpl(), clearDescImpl);
}

NRI_INLINE void CommandBufferVal::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, clearDesc.storageTexture, ReturnVoid(), "'.storageTexture' is NULL");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageTexture = NRI_GET_IMPL(Descriptor, clearDesc.storageTexture);

    GetCoreInterface().CmdClearStorageTexture(*GetImpl(), clearDescImpl);
}

NRI_INLINE void CommandBufferVal::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has been already called");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (attachmentsDesc.shadingRate)
        RETURN_ON_FAILURE(&m_Device, deviceDesc.shadingRateTier, ReturnVoid(), "'shadingRateTier >= 2' required");

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

    GetCoreInterface().CmdBeginRendering(*GetImpl(), attachmentsDescImpl);
}

NRI_INLINE void CommandBufferVal::EndRendering() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has not been called");

    m_IsRenderPass = false;

    ResetAttachments();

    GetCoreInterface().CmdEndRendering(*GetImpl());
}

NRI_INLINE void CommandBufferVal::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_Pipeline, ReturnVoid(), "'SetPipeline' has not been called");

    Scratch<Buffer*> buffersImpl = AllocateScratch(m_Device, Buffer*, bufferNum);
    for (uint32_t i = 0; i < bufferNum; i++)
        buffersImpl[i] = NRI_GET_IMPL(Buffer, buffers[i]);

    GetCoreInterface().CmdSetVertexBuffers(*GetImpl(), baseSlot, bufferNum, buffersImpl, offsets);
}

NRI_INLINE void CommandBufferVal::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterface().CmdSetIndexBuffer(*GetImpl(), *bufferImpl, offset, indexType);
}

NRI_INLINE void CommandBufferVal::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    m_PipelineLayout = (PipelineLayoutVal*)&pipelineLayout;

    GetCoreInterface().CmdSetPipelineLayout(*GetImpl(), *pipelineLayoutImpl);
}

NRI_INLINE void CommandBufferVal::SetPipeline(const Pipeline& pipeline) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Pipeline* pipelineImpl = NRI_GET_IMPL(Pipeline, &pipeline);

    m_Pipeline = (PipelineVal*)&pipeline;

    ValidateReadonlyDepthStencil();

    GetCoreInterface().CmdSetPipeline(*GetImpl(), *pipelineImpl);
}

NRI_INLINE void CommandBufferVal::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, &descriptorPool);

    GetCoreInterface().CmdSetDescriptorPool(*GetImpl(), *descriptorPoolImpl);
}

NRI_INLINE void CommandBufferVal::SetDescriptorSet(uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");

    DescriptorSet* descriptorSetImpl = NRI_GET_IMPL(DescriptorSet, &descriptorSet);

    GetCoreInterface().CmdSetDescriptorSet(*GetImpl(), setIndex, *descriptorSetImpl, dynamicConstantBufferOffsets);
}

NRI_INLINE void CommandBufferVal::SetRootConstants(uint32_t rootConstantIndex, const void* data, uint32_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");

    GetCoreInterface().CmdSetRootConstants(*GetImpl(), rootConstantIndex, data, size);
}

NRI_INLINE void CommandBufferVal::SetRootDescriptor(uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");

    const DescriptorVal& descriptorVal = (DescriptorVal&)descriptor;
    RETURN_ON_FAILURE(&m_Device, descriptorVal.IsBufferView(), ReturnVoid(), "'descriptor' must be a buffer view");

    Descriptor* descriptorImpl = NRI_GET_IMPL(Descriptor, &descriptor);

    GetCoreInterface().CmdSetRootDescriptor(*GetImpl(), rootDescriptorIndex, *descriptorImpl);
}

NRI_INLINE void CommandBufferVal::Draw(const DrawDesc& drawDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDraw(*GetImpl(), drawDesc);
}

NRI_INLINE void CommandBufferVal::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDrawIndexed(*GetImpl(), drawIndexedDesc);
}

NRI_INLINE void CommandBufferVal::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.isDrawIndirectCountSupported, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterface().CmdDrawIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.isDrawIndirectCountSupported, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterface().CmdDrawIndexedIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (size == WHOLE_SIZE) {
        const BufferDesc& dstDesc = ((BufferVal&)dstBuffer).GetDesc();
        const BufferDesc& srcDesc = ((BufferVal&)srcBuffer).GetDesc();

        if (dstDesc.size != srcDesc.size)
            REPORT_WARNING(&m_Device, "WHOLE_SIZE is used but 'dstBuffer' and 'srcBuffer' have different sizes");
    }

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterface().CmdCopyBuffer(*GetImpl(), *dstBufferImpl, dstOffset, *srcBufferImpl, srcOffset, size);
}

NRI_INLINE void CommandBufferVal::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterface().CmdCopyTexture(*GetImpl(), *dstTextureImpl, dstRegionDesc, *srcTextureImpl, srcRegionDesc);
}

NRI_INLINE void CommandBufferVal::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterface().CmdResolveTexture(*GetImpl(), *dstTextureImpl, dstRegionDesc, *srcTextureImpl, srcRegionDesc);
}

NRI_INLINE void CommandBufferVal::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterface().CmdUploadBufferToTexture(*GetImpl(), *dstTextureImpl, dstRegionDesc, *srcBufferImpl, srcDataLayoutDesc);
}

NRI_INLINE void CommandBufferVal::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterface().CmdReadbackTextureToBuffer(*GetImpl(), *dstBufferImpl, dstDataLayoutDesc, *srcTextureImpl, srcRegionDesc);
}

NRI_INLINE void CommandBufferVal::Dispatch(const DispatchDesc& dispatchDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDispatch(*GetImpl(), dispatchDesc);
}

NRI_INLINE void CommandBufferVal::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    GetCoreInterface().CmdDispatchIndirect(*GetImpl(), *bufferImpl, offset);
}

NRI_INLINE void CommandBufferVal::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
        if (!ValidateBufferBarrierDesc(m_Device, i, barrierGroupDesc.buffers[i]))
            return;
    }

    for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
        if (!ValidateTextureBarrierDesc(m_Device, i, barrierGroupDesc.textures[i]))
            return;
    }

    Scratch<BufferBarrierDesc> buffers = AllocateScratch(m_Device, BufferBarrierDesc, barrierGroupDesc.bufferNum);
    memcpy(buffers, barrierGroupDesc.buffers, sizeof(BufferBarrierDesc) * barrierGroupDesc.bufferNum);
    for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++)
        buffers[i].buffer = NRI_GET_IMPL(Buffer, barrierGroupDesc.buffers[i].buffer);

    Scratch<TextureBarrierDesc> textures = AllocateScratch(m_Device, TextureBarrierDesc, barrierGroupDesc.textureNum);
    memcpy(textures, barrierGroupDesc.textures, sizeof(TextureBarrierDesc) * barrierGroupDesc.textureNum);
    for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++)
        textures[i].texture = NRI_GET_IMPL(Texture, barrierGroupDesc.textures[i].texture);

    auto barrierGroupDescImpl = barrierGroupDesc;
    barrierGroupDescImpl.buffers = buffers;
    barrierGroupDescImpl.textures = textures;

    GetCoreInterface().CmdBarrier(*GetImpl(), barrierGroupDescImpl);
}

NRI_INLINE void CommandBufferVal::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, queryPoolVal.GetQueryType() != QueryType::TIMESTAMP, ReturnVoid(), "'BeginQuery' is not supported for timestamp queries");

    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset = %u' is out of range", offset);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterface().CmdBeginQuery(*GetImpl(), *queryPoolImpl, offset);
}

NRI_INLINE void CommandBufferVal::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset = %u' is out of range", offset);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterface().CmdEndQuery(*GetImpl(), *queryPoolImpl, offset);
}

NRI_INLINE void CommandBufferVal::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num =  %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);

    GetCoreInterface().CmdCopyQueries(*GetImpl(), *queryPoolImpl, offset, num, *dstBufferImpl, dstOffset);
}

NRI_INLINE void CommandBufferVal::ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num = %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterface().CmdResetQueries(*GetImpl(), *queryPoolImpl, offset, num);
}

NRI_INLINE void CommandBufferVal::BeginAnnotation(const char* name, uint32_t bgra) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    m_AnnotationStack++;
    GetCoreInterface().CmdBeginAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE void CommandBufferVal::EndAnnotation() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdEndAnnotation(*GetImpl());
    m_AnnotationStack--;
}

NRI_INLINE void CommandBufferVal::Annotation(const char* name, uint32_t bgra) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE void CommandBufferVal::BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Scratch<BuildTopLevelAccelerationStructureDesc> buildTopLevelAccelerationStructureDescsImpl = AllocateScratch(m_Device, BuildTopLevelAccelerationStructureDesc, buildTopLevelAccelerationStructureDescNum);

    for (uint32_t i = 0; i < buildTopLevelAccelerationStructureDescNum; i++) {
        const BuildTopLevelAccelerationStructureDesc& in = buildTopLevelAccelerationStructureDescs[i];

        RETURN_ON_FAILURE(&m_Device, in.instanceBuffer, ReturnVoid(), "'instanceBuffer' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");

        const BufferVal* instanceBufferVal = (BufferVal*)in.instanceBuffer;
        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        RETURN_ON_FAILURE(&m_Device, in.instanceOffset < instanceBufferVal->GetDesc().size, ReturnVoid(), "'instanceOffset = %llu' is out of bounds", in.instanceOffset);
        RETURN_ON_FAILURE(&m_Device, in.scratchOffset < scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset = %llu' is out of bounds", in.scratchOffset);

        BuildTopLevelAccelerationStructureDesc& out = buildTopLevelAccelerationStructureDescsImpl[i];
        out = in;
        out.dst = NRI_GET_IMPL(AccelerationStructure, in.dst);
        out.src = NRI_GET_IMPL(AccelerationStructure, in.src);
        out.instanceBuffer = NRI_GET_IMPL(Buffer, in.instanceBuffer);
        out.scratchBuffer = NRI_GET_IMPL(Buffer, in.scratchBuffer);
    }

    GetRayTracingInterface().CmdBuildTopLevelAccelerationStructures(*GetImpl(), buildTopLevelAccelerationStructureDescsImpl, buildTopLevelAccelerationStructureDescNum);
}

NRI_INLINE void CommandBufferVal::BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    uint32_t totalGeometryObjectNum = 0;
    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++)
        totalGeometryObjectNum += buildBottomLevelAccelerationStructureDescs[i].geometryNum;

    Scratch<BottomLevelGeometry> geometryObjectsImplScratch = AllocateScratch(m_Device, BottomLevelGeometry, totalGeometryObjectNum);
    BottomLevelGeometry* geometryObjectsImpl = geometryObjectsImplScratch;

    Scratch<BuildBottomLevelAccelerationStructureDesc> buildBottomLevelAccelerationStructureDescsImpl = AllocateScratch(m_Device, BuildBottomLevelAccelerationStructureDesc, buildBottomLevelAccelerationStructureDescNum);

    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& in = buildBottomLevelAccelerationStructureDescs[i];

        RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");

        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        RETURN_ON_FAILURE(&m_Device, in.geometries, ReturnVoid(), "'geometries' is NULL");
        RETURN_ON_FAILURE(&m_Device, in.scratchOffset < scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset = %llu' is out of bounds", in.scratchOffset);

        BuildBottomLevelAccelerationStructureDesc& out = buildBottomLevelAccelerationStructureDescsImpl[i];
        out = in;
        out.dst = NRI_GET_IMPL(AccelerationStructure, in.dst);
        out.src = NRI_GET_IMPL(AccelerationStructure, in.src);
        out.geometries = geometryObjectsImpl;
        out.scratchBuffer = NRI_GET_IMPL(Buffer, in.scratchBuffer);

        ConvertGeometryObjectsVal(geometryObjectsImpl, in.geometries, in.geometryNum);

        geometryObjectsImpl += in.geometryNum;
    }

    GetRayTracingInterface().CmdBuildBottomLevelAccelerationStructures(*GetImpl(), buildBottomLevelAccelerationStructureDescsImpl, buildBottomLevelAccelerationStructureDescNum);
}

NRI_INLINE void CommandBufferVal::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(), "'copyMode' is invalid");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);

    GetRayTracingInterface().CmdCopyAccelerationStructure(*GetImpl(), dstImpl, srcImpl, copyMode);
}

NRI_INLINE void CommandBufferVal::WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, accelerationStructures, ReturnVoid(), "'accelerationStructures' is NULL");

    const QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    bool isTypeValid = queryPoolVal.GetQueryType() == QueryType::ACCELERATION_STRUCTURE_SIZE || queryPoolVal.GetQueryType() == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE;
    RETURN_ON_FAILURE(&m_Device, isTypeValid, ReturnVoid(), "'queryPool' query type must be 'ACCELERATION_STRUCTURE_SIZE' or 'ACCELERATION_STRUCTURE_COMPACTED_SIZE'");

    Scratch<AccelerationStructure*> accelerationStructuresImpl = AllocateScratch(m_Device, AccelerationStructure*, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++) {
        RETURN_ON_FAILURE(&m_Device, accelerationStructures[i], ReturnVoid(), "'accelerationStructures[%u]' is NULL", i);

        accelerationStructuresImpl[i] = NRI_GET_IMPL(AccelerationStructure, accelerationStructures[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL(QueryPool, &queryPool);

    GetRayTracingInterface().CmdWriteAccelerationStructureSize(*GetImpl(), accelerationStructuresImpl, accelerationStructureNum, queryPoolImpl, queryOffset);
}

NRI_INLINE void CommandBufferVal::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint64_t align = deviceDesc.shaderBindingTableAlignment;
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

    GetRayTracingInterface().CmdDispatchRays(*GetImpl(), dispatchRaysDescImpl);
}

NRI_INLINE void CommandBufferVal::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.rayTracingTier >= 2, ReturnVoid(), "'rayTracingTier' must be >= 2");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    GetRayTracingInterface().CmdDispatchRaysIndirect(*GetImpl(), *bufferImpl, offset);
}

NRI_INLINE void CommandBufferVal::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.isMeshShaderSupported, ReturnVoid(), "'isMeshShaderSupported' is false");

    GetMeshShaderInterface().CmdDrawMeshTasks(*GetImpl(), drawMeshTasksDesc);
}

NRI_INLINE void CommandBufferVal::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, deviceDesc.isMeshShaderSupported, ReturnVoid(), "'isMeshShaderSupported' is false");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.isDrawIndirectCountSupported, ReturnVoid(), "'countBuffer' is not supported");

    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "'offset' is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetMeshShaderInterface().CmdDrawMeshTasksIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::ValidateReadonlyDepthStencil() {
    if (m_Pipeline && m_DepthStencil) {
        if (m_DepthStencil->IsDepthReadonly() && m_Pipeline->WritesToDepth())
            REPORT_WARNING(&m_Device, "Depth is read-only, but the pipeline writes to depth. Writing happens only in VK!");

        if (m_DepthStencil->IsStencilReadonly() && m_Pipeline->WritesToStencil())
            REPORT_WARNING(&m_Device, "Stencil is read-only, but the pipeline writes to stencil. Writing happens only in VK!");
    }
}
