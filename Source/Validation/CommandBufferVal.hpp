// © 2021 NVIDIA Corporation

static inline bool IsAccessMaskSupported(const BufferDesc& bufferDesc, AccessBits accessMask) {
    bool isSupported = true;
    if (accessMask & AccessBits::INDEX_BUFFER)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::INDEX_BUFFER) != 0;
    if (accessMask & AccessBits::VERTEX_BUFFER)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::VERTEX_BUFFER) != 0;
    if (accessMask & AccessBits::CONSTANT_BUFFER)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::CONSTANT_BUFFER) != 0;
    if (accessMask & AccessBits::ARGUMENT_BUFFER)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::ARGUMENT_BUFFER) != 0;
    if (accessMask & AccessBits::SCRATCH_BUFFER)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::SCRATCH_BUFFER) != 0;
    if (accessMask & (AccessBits::COLOR_ATTACHMENT | AccessBits::DEPTH_STENCIL_ATTACHMENT | AccessBits::SHADING_RATE_ATTACHMENT | AccessBits::INPUT_ATTACHMENT))
        isSupported = false;
    if (accessMask & AccessBits::ACCELERATION_STRUCTURE)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE) != 0;
    if (accessMask & AccessBits::MICROMAP)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::MICROMAP_STORAGE) != 0;
    if (accessMask & AccessBits::SHADER_BINDING_TABLE)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::SHADER_BINDING_TABLE) != 0;
    if (accessMask & AccessBits::SHADER_RESOURCE)
        isSupported = isSupported && (bufferDesc.usage & (BufferUsageBits::SHADER_RESOURCE | BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_INPUT)) != 0;
    if (accessMask & AccessBits::SHADER_RESOURCE_STORAGE)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::SHADER_RESOURCE_STORAGE) != 0;
    if (accessMask & (AccessBits::RESOLVE_SOURCE | AccessBits::RESOLVE_DESTINATION))
        isSupported = false;

    return isSupported;
}

static inline bool IsAccessMaskSupported(const TextureDesc& textureDesc, AccessBits accessMask) {
    bool isSupported = true;
    if (accessMask & (AccessBits::INDEX_BUFFER | AccessBits::VERTEX_BUFFER | AccessBits::CONSTANT_BUFFER | AccessBits::ARGUMENT_BUFFER | AccessBits::SCRATCH_BUFFER))
        isSupported = false;
    if (accessMask & AccessBits::COLOR_ATTACHMENT)
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::COLOR_ATTACHMENT) != 0;
    if (accessMask & AccessBits::SHADING_RATE_ATTACHMENT)
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::SHADING_RATE_ATTACHMENT) != 0;
    if (accessMask & AccessBits::DEPTH_STENCIL_ATTACHMENT)
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) != 0;
    if (accessMask & AccessBits::ACCELERATION_STRUCTURE)
        isSupported = false;
    if (accessMask & AccessBits::MICROMAP)
        isSupported = false;
    if (accessMask & AccessBits::SHADER_BINDING_TABLE)
        isSupported = false;
    if (accessMask & AccessBits::SHADER_RESOURCE)
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::SHADER_RESOURCE) != 0;
    if (accessMask & AccessBits::INPUT_ATTACHMENT)
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::INPUT_ATTACHMENT) != 0;
    if (accessMask & (AccessBits::SHADER_RESOURCE_STORAGE | AccessBits::CLEAR_STORAGE))
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::SHADER_RESOURCE_STORAGE) != 0;

    return isSupported;
}

static inline bool IsTextureLayoutSupported(const TextureDesc& textureDesc, Layout layout) {
    if (layout == Layout::COLOR_ATTACHMENT)
        return (textureDesc.usage & TextureUsageBits::COLOR_ATTACHMENT) != 0;
    else if (layout == Layout::SHADING_RATE_ATTACHMENT)
        return (textureDesc.usage & TextureUsageBits::SHADING_RATE_ATTACHMENT) != 0;
    else if (layout == Layout::DEPTH_STENCIL_ATTACHMENT
        || layout == Layout::DEPTH_READONLY_STENCIL_ATTACHMENT
        || layout == Layout::DEPTH_ATTACHMENT_STENCIL_READONLY
        || layout == Layout::DEPTH_STENCIL_READONLY)
        return (textureDesc.usage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) != 0;
    else if (layout == Layout::SHADER_RESOURCE)
        return (textureDesc.usage & TextureUsageBits::SHADER_RESOURCE) != 0;
    else if (layout == Layout::SHADER_RESOURCE_STORAGE)
        return (textureDesc.usage & TextureUsageBits::SHADER_RESOURCE_STORAGE) != 0;
    else if (layout == Layout::RESOLVE_DESTINATION)
        return textureDesc.sampleNum <= 1;
    else if (layout == Layout::RESOLVE_SOURCE)
        return textureDesc.sampleNum > 1;
    else if (layout == Layout::INPUT_ATTACHMENT)
        return (textureDesc.usage & TextureUsageBits::INPUT_ATTACHMENT) != 0;

    return true;
}

static bool ValidateBufferBarrierDesc(const DeviceVal& device, uint32_t i, const BufferBarrierDesc& bufferBarrier) {
    const BufferVal& bufferVal = *(const BufferVal*)bufferBarrier.buffer;

    NRI_RETURN_ON_FAILURE(&device, bufferBarrier.buffer, false, "'barrierDesc.buffers[%u].buffer' is NULL", i);
    NRI_RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc(), bufferBarrier.before.access), false,
        "'barrierDesc.buffers[%u].before.access' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());
    NRI_RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc(), bufferBarrier.after.access), false,
        "'barrierDesc.buffers[%u].after.access' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());

    return true;
}

static bool ValidateTextureBarrierDesc(const DeviceVal& device, uint32_t i, const TextureBarrierDesc& textureBarrier) {
    const TextureVal& textureVal = *(const TextureVal*)textureBarrier.texture;

    NRI_RETURN_ON_FAILURE(&device, textureBarrier.texture, false, "'barrierDesc.textures[%u].texture' is NULL", i);
    NRI_RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc(), textureBarrier.before.access), false,
        "'barrierDesc.textures[%u].before.access' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    NRI_RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc(), textureBarrier.after.access), false,
        "'barrierDesc.textures[%u].after.access' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    NRI_RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc(), textureBarrier.before.layout), false,
        "'barrierDesc.textures[%u].before.layout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    NRI_RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc(), textureBarrier.after.layout), false,
        "'barrierDesc.textures[%u].after.layout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    if (textureBarrier.after.layout == Layout::PRESENT) {
        NRI_RETURN_ON_FAILURE(&device, textureBarrier.after.access == AccessBits::NONE && textureBarrier.after.stages == StageBits::NONE, false,
            "'barrierDesc.textures[%u].after.layout = Layout::PRESENT' for texture ('%s') expects 'AccessBits::NONE' and 'StageBits::NONE'", i, textureVal.GetDebugName());
    }

    return true;
}

NRI_INLINE Result CommandBufferVal::Begin(const DescriptorPool* descriptorPool) {
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRecordingStarted, Result::FAILURE, "already in the recording state");

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
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, Result::FAILURE, "not in the recording state");

    if (m_AnnotationStack > 0)
        NRI_REPORT_ERROR(&m_Device, "'CmdBeginAnnotation' is called more times than 'CmdEndAnnotation'");
    else if (m_AnnotationStack < 0)
        NRI_REPORT_ERROR(&m_Device, "'CmdEndAnnotation' is called more times than 'CmdBeginAnnotation'");

    Result result = GetCoreInterfaceImpl().EndCommandBuffer(*GetImpl());
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = m_IsWrapped;

    return result;
}

NRI_INLINE void CommandBufferVal::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (!deviceDesc.features.viewportOriginBottomLeft) {
        for (uint32_t i = 0; i < viewportNum; i++) {
            NRI_RETURN_ON_FAILURE(&m_Device, !viewports[i].originBottomLeft, ReturnVoid(), "'features.viewportOriginBottomLeft' is false");
        }
    }

    GetCoreInterfaceImpl().CmdSetViewports(*GetImpl(), viewports, viewportNum);
}

NRI_INLINE void CommandBufferVal::SetScissors(const Rect* rects, uint32_t rectNum) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdSetScissors(*GetImpl(), rects, rectNum);
}

NRI_INLINE void CommandBufferVal::SetDepthBounds(float boundsMin, float boundsMax) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.depthBoundsTest, ReturnVoid(), "'features.depthBoundsTest' is false");

    GetCoreInterfaceImpl().CmdSetDepthBounds(*GetImpl(), boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferVal::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdSetStencilReference(*GetImpl(), frontRef, backRef);
}

NRI_INLINE void CommandBufferVal::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.sampleLocations != 0, ReturnVoid(), "'tiers.sampleLocations > 0' required");

    GetCoreInterfaceImpl().CmdSetSampleLocations(*GetImpl(), locations, locationNum, sampleNum);
}

NRI_INLINE void CommandBufferVal::SetBlendConstants(const Color32f& color) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdSetBlendConstants(*GetImpl(), color);
}

NRI_INLINE void CommandBufferVal::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.shadingRate, ReturnVoid(), "'tiers.shadingRate > 0' required");

    GetCoreInterfaceImpl().CmdSetShadingRate(*GetImpl(), shadingRateDesc);
}

NRI_INLINE void CommandBufferVal::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.dynamicDepthBias, ReturnVoid(), "'features.dynamicDepthBias' is false");

    GetCoreInterfaceImpl().CmdSetDepthBias(*GetImpl(), depthBiasDesc);
}

NRI_INLINE void CommandBufferVal::ClearAttachments(const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const Rect* rects, uint32_t rectNum) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    for (uint32_t i = 0; i < clearAttachmentDescNum; i++) {
        const ClearAttachmentDesc& clearAttachmentDesc = clearAttachmentDescs[i];

        bool isColor = clearAttachmentDesc.planes & PlaneBits::COLOR;
        bool isDepthStencil = clearAttachmentDesc.planes & (PlaneBits::DEPTH | PlaneBits::STENCIL);
        NRI_RETURN_ON_FAILURE(&m_Device, isColor != isDepthStencil, ReturnVoid(), "'[%u].planes' must represent a color or a depth-stencil", i);

        if (clearAttachmentDesc.planes & PlaneBits::COLOR) {
            NRI_RETURN_ON_FAILURE(&m_Device, clearAttachmentDesc.colorAttachmentIndex < deviceDesc.shaderStage.fragment.attachmentMaxNum, ReturnVoid(), "'[%u].colorAttachmentIndex=%u' is out of bounds", i, clearAttachmentDesc.colorAttachmentIndex);
            NRI_RETURN_ON_FAILURE(&m_Device, m_RenderTargets[clearAttachmentDesc.colorAttachmentIndex], ReturnVoid(), "'[%u].colorAttachmentIndex=%u' references a NULL COLOR attachment", i, clearAttachmentDesc.colorAttachmentIndex);
        }

        if (clearAttachmentDesc.planes & (PlaneBits::DEPTH | PlaneBits::STENCIL)) {
            NRI_RETURN_ON_FAILURE(&m_Device, m_DepthStencil, ReturnVoid(), "DEPTH_STENCIL attachment is NULL", i);
            NRI_RETURN_ON_FAILURE(&m_Device, clearAttachmentDesc.colorAttachmentIndex == 0, ReturnVoid(), "'[%u].planes' is not COLOR, but `colorAttachmentIndex != 0`", i);
        }
    }

    GetCoreInterfaceImpl().CmdClearAttachments(*GetImpl(), clearAttachmentDescs, clearAttachmentDescNum, rects, rectNum);
}

NRI_INLINE void CommandBufferVal::ClearStorage(const ClearStorageDesc& clearStorageDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, clearStorageDesc.descriptor, ReturnVoid(), "'.storage' is NULL");

    const DescriptorVal& descriptorVal = *(DescriptorVal*)clearStorageDesc.descriptor;
    NRI_RETURN_ON_FAILURE(&m_Device, descriptorVal.IsShaderResourceStorage(), ReturnVoid(), "'.storage' is not a 'SHADER_RESOURCE_STORAGE' descriptor");
    // TODO: check that a descriptor set is bound, minimal tracking of sets is needed

    auto clearStorageDescImpl = clearStorageDesc;
    clearStorageDescImpl.descriptor = NRI_GET_IMPL(Descriptor, clearStorageDesc.descriptor);

    GetCoreInterfaceImpl().CmdClearStorage(*GetImpl(), clearStorageDescImpl);
}

NRI_INLINE void CommandBufferVal::BeginRendering(const RenderingDesc& renderingDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has already been called");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (renderingDesc.shadingRate)
        NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.shadingRate, ReturnVoid(), "'tiers.shadingRate >= 2' required");

    ResetAttachments();

    Scratch<AttachmentDesc> colors = NRI_ALLOCATE_SCRATCH(m_Device, AttachmentDesc, renderingDesc.colorNum);
    for (uint32_t i = 0; i < renderingDesc.colorNum; i++) {
        colors[i] = renderingDesc.colors[i];
        colors[i].descriptor = NRI_GET_IMPL(Descriptor, renderingDesc.colors[i].descriptor);
        colors[i].resolveDst = NRI_GET_IMPL(Descriptor, renderingDesc.colors[i].resolveDst);

        m_RenderTargets[i] = (DescriptorVal*)renderingDesc.colors[i].descriptor;
    }

    auto attachmentsDescImpl = renderingDesc;
    attachmentsDescImpl.colors = colors;
    attachmentsDescImpl.colorNum = renderingDesc.colorNum;
    attachmentsDescImpl.depth.descriptor = NRI_GET_IMPL(Descriptor, renderingDesc.depth.descriptor);
    attachmentsDescImpl.depth.resolveDst = NRI_GET_IMPL(Descriptor, renderingDesc.depth.resolveDst);
    attachmentsDescImpl.stencil.descriptor = NRI_GET_IMPL(Descriptor, renderingDesc.stencil.descriptor);
    attachmentsDescImpl.stencil.resolveDst = NRI_GET_IMPL(Descriptor, renderingDesc.stencil.resolveDst);
    attachmentsDescImpl.shadingRate = NRI_GET_IMPL(Descriptor, renderingDesc.shadingRate);

    Descriptor* depthStencil = renderingDesc.depth.descriptor ? renderingDesc.depth.descriptor : renderingDesc.stencil.descriptor;
    m_DepthStencil = depthStencil ? (DescriptorVal*)depthStencil : nullptr;

    m_RenderTargetNum = renderingDesc.colorNum;
    m_IsRenderPass = true;

    ValidateReadonlyDepthStencil();

    GetCoreInterfaceImpl().CmdBeginRendering(*GetImpl(), attachmentsDescImpl);
}

NRI_INLINE void CommandBufferVal::EndRendering() {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has not been called");

    m_IsRenderPass = false;

    ResetAttachments();

    GetCoreInterfaceImpl().CmdEndRendering(*GetImpl());
}

NRI_INLINE void CommandBufferVal::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Scratch<VertexBufferDesc> vertexBufferDescsImpl = NRI_ALLOCATE_SCRATCH(m_Device, VertexBufferDesc, vertexBufferNum);
    for (uint32_t i = 0; i < vertexBufferNum; i++) {
        vertexBufferDescsImpl[i] = vertexBufferDescs[i];
        vertexBufferDescsImpl[i].buffer = NRI_GET_IMPL(Buffer, vertexBufferDescs[i].buffer);
    }

    GetCoreInterfaceImpl().CmdSetVertexBuffers(*GetImpl(), baseSlot, vertexBufferDescsImpl, vertexBufferNum);
}

NRI_INLINE void CommandBufferVal::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterfaceImpl().CmdSetIndexBuffer(*GetImpl(), *bufferImpl, offset, indexType);
}

NRI_INLINE void CommandBufferVal::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, bindPoint != BindPoint::INHERIT, ReturnVoid(), "'INHERIT' is not allowed");

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    m_PipelineLayout = (PipelineLayoutVal*)&pipelineLayout;

    GetCoreInterfaceImpl().CmdSetPipelineLayout(*GetImpl(), bindPoint, *pipelineLayoutImpl);
}

NRI_INLINE void CommandBufferVal::SetPipeline(const Pipeline& pipeline) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Pipeline* pipelineImpl = NRI_GET_IMPL(Pipeline, &pipeline);

    m_Pipeline = (PipelineVal*)&pipeline;

    ValidateReadonlyDepthStencil();

    GetCoreInterfaceImpl().CmdSetPipeline(*GetImpl(), *pipelineImpl);
}

NRI_INLINE void CommandBufferVal::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, &descriptorPool);

    GetCoreInterfaceImpl().CmdSetDescriptorPool(*GetImpl(), *descriptorPoolImpl);
}

NRI_INLINE void CommandBufferVal::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");

    auto descriptorSetBindingDescImpl = setDescriptorSetDesc;
    descriptorSetBindingDescImpl.descriptorSet = NRI_GET_IMPL(DescriptorSet, setDescriptorSetDesc.descriptorSet);

    GetCoreInterfaceImpl().CmdSetDescriptorSet(*GetImpl(), descriptorSetBindingDescImpl);
}

NRI_INLINE void CommandBufferVal::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    NRI_RETURN_ON_FAILURE(&m_Device, setRootConstantsDesc.offset == 0 || deviceDesc.features.rootConstantsOffset, ReturnVoid(), "Non-zero 'setRootConstantsDesc.offset' is not supported");

    GetCoreInterfaceImpl().CmdSetRootConstants(*GetImpl(), setRootConstantsDesc);
}

NRI_INLINE void CommandBufferVal::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    const DescriptorVal& descriptorVal = *(DescriptorVal*)setRootDescriptorDesc.descriptor;
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    NRI_RETURN_ON_FAILURE(&m_Device, descriptorVal.CanBeRoot(), ReturnVoid(), "'descriptor' must be a non-typed buffer or an acceleration structure");

    if (!descriptorVal.IsConstantBuffer())
        NRI_RETURN_ON_FAILURE(&m_Device, setRootDescriptorDesc.offset == 0 || deviceDesc.features.nonConstantBufferRootDescriptorOffset, ReturnVoid(), "Non-zero 'setRootDescriptorDesc.offset' is supported only for 'CONSTANT_BUFFER'");

    auto rootDescriptorBindingDescImpl = setRootDescriptorDesc;
    rootDescriptorBindingDescImpl.descriptor = NRI_GET_IMPL(Descriptor, setRootDescriptorDesc.descriptor);

    GetCoreInterfaceImpl().CmdSetRootDescriptor(*GetImpl(), rootDescriptorBindingDescImpl);
}

NRI_INLINE void CommandBufferVal::Draw(const DrawDesc& drawDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterfaceImpl().CmdDraw(*GetImpl(), drawDesc);
}

NRI_INLINE void CommandBufferVal::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterfaceImpl().CmdDrawIndexed(*GetImpl(), drawIndexedDesc);
}

NRI_INLINE void CommandBufferVal::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterfaceImpl().CmdDrawIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterfaceImpl().CmdDrawIndexedIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    const BufferDesc& dstDesc = ((BufferVal&)dstBuffer).GetDesc();
    const BufferDesc& srcDesc = ((BufferVal&)srcBuffer).GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    if (size == WHOLE_SIZE) {
        NRI_RETURN_ON_FAILURE(&m_Device, dstOffset == 0, ReturnVoid(), "'WHOLE_SIZE' is used but 'dstOffset' is not 0");
        NRI_RETURN_ON_FAILURE(&m_Device, srcOffset == 0, ReturnVoid(), "'WHOLE_SIZE' is used but 'srcOffset' is not 0");
        NRI_RETURN_ON_FAILURE(&m_Device, dstDesc.size == srcDesc.size, ReturnVoid(), "'WHOLE_SIZE' is used but 'dstBuffer' and 'srcBuffer' have different sizes");
    } else {
        NRI_RETURN_ON_FAILURE(&m_Device, srcOffset + size <= srcDesc.size, ReturnVoid(), "'srcOffset + size' > srcBuffer.size");
        NRI_RETURN_ON_FAILURE(&m_Device, dstOffset + size <= dstDesc.size, ReturnVoid(), "'dstOffset + size' > dstBuffer.size");
    }

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterfaceImpl().CmdCopyBuffer(*GetImpl(), *dstBufferImpl, dstOffset, *srcBufferImpl, srcOffset, size);
}

NRI_INLINE void CommandBufferVal::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterfaceImpl().CmdCopyTexture(*GetImpl(), *dstTextureImpl, dstRegion, *srcTextureImpl, srcRegion);
}

NRI_INLINE void CommandBufferVal::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (!deviceDesc.features.regionResolve)
        NRI_RETURN_ON_FAILURE(&m_Device, !dstRegion && !srcRegion, ReturnVoid(), "region(s) are specified, but 'features.regionResolve' is false");
    if (!deviceDesc.features.resolveOpMinMax)
        NRI_RETURN_ON_FAILURE(&m_Device, resolveOp == ResolveOp::AVERAGE, ReturnVoid(), "'features.resolveOpMinMax' is false");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterfaceImpl().CmdResolveTexture(*GetImpl(), *dstTextureImpl, dstRegion, *srcTextureImpl, srcRegion, resolveOp);
}

NRI_INLINE void CommandBufferVal::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterfaceImpl().CmdUploadBufferToTexture(*GetImpl(), *dstTextureImpl, dstRegion, *srcBufferImpl, srcDataLayout);
}

NRI_INLINE void CommandBufferVal::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterfaceImpl().CmdReadbackTextureToBuffer(*GetImpl(), *dstBufferImpl, dstDataLayout, *srcTextureImpl, srcRegion);
}

NRI_INLINE void CommandBufferVal::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    if (size == WHOLE_SIZE) {
        NRI_RETURN_ON_FAILURE(&m_Device, offset == 0, ReturnVoid(), "'WHOLE_SIZE' is used but 'offset' is not 0");
    } else {
        const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
        NRI_RETURN_ON_FAILURE(&m_Device, offset + size <= bufferDesc.size, ReturnVoid(), "'offset + size' > buffer.size");
    }

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterfaceImpl().CmdZeroBuffer(*GetImpl(), *bufferImpl, offset, size);
}

NRI_INLINE void CommandBufferVal::Dispatch(const DispatchDesc& dispatchDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterfaceImpl().CmdDispatch(*GetImpl(), dispatchDesc);
}

NRI_INLINE void CommandBufferVal::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    GetCoreInterfaceImpl().CmdDispatchIndirect(*GetImpl(), *bufferImpl, offset);
}

NRI_INLINE void CommandBufferVal::Barrier(const BarrierDesc& barrierDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
        if (!ValidateBufferBarrierDesc(m_Device, i, barrierDesc.buffers[i]))
            return;
    }

    for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
        if (!ValidateTextureBarrierDesc(m_Device, i, barrierDesc.textures[i]))
            return;
    }

    Scratch<BufferBarrierDesc> buffers = NRI_ALLOCATE_SCRATCH(m_Device, BufferBarrierDesc, barrierDesc.bufferNum);
    memcpy(buffers, barrierDesc.buffers, sizeof(BufferBarrierDesc) * barrierDesc.bufferNum);
    for (uint32_t i = 0; i < barrierDesc.bufferNum; i++)
        buffers[i].buffer = NRI_GET_IMPL(Buffer, barrierDesc.buffers[i].buffer);

    Scratch<TextureBarrierDesc> textures = NRI_ALLOCATE_SCRATCH(m_Device, TextureBarrierDesc, barrierDesc.textureNum);
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

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, queryPoolVal.GetQueryType() != QueryType::TIMESTAMP, ReturnVoid(), "'BeginQuery' is not supported for timestamp queries");

    if (!queryPoolVal.IsImported())
        NRI_RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset=%u' is out of range", offset);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterfaceImpl().CmdBeginQuery(*GetImpl(), *queryPoolImpl, offset);
}

NRI_INLINE void CommandBufferVal::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!queryPoolVal.IsImported())
        NRI_RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset=%u' is out of range", offset);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterfaceImpl().CmdEndQuery(*GetImpl(), *queryPoolImpl, offset);
}

NRI_INLINE void CommandBufferVal::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        NRI_RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num = %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);

    GetCoreInterfaceImpl().CmdCopyQueries(*GetImpl(), *queryPoolImpl, offset, num, *dstBufferImpl, dstOffset);
}

NRI_INLINE void CommandBufferVal::ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        NRI_RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num = %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    GetCoreInterfaceImpl().CmdResetQueries(*GetImpl(), *queryPoolImpl, offset, num);
}

NRI_INLINE void CommandBufferVal::BeginAnnotation(const char* name, uint32_t bgra) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    m_AnnotationStack++;
    GetCoreInterfaceImpl().CmdBeginAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE void CommandBufferVal::EndAnnotation() {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdEndAnnotation(*GetImpl());
    m_AnnotationStack--;
}

NRI_INLINE void CommandBufferVal::Annotation(const char* name, uint32_t bgra) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterfaceImpl().CmdAnnotation(*GetImpl(), name, bgra);
}

NRI_INLINE void CommandBufferVal::BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Scratch<BuildTopLevelAccelerationStructureDesc> buildTopLevelAccelerationStructureDescsImpl = NRI_ALLOCATE_SCRATCH(m_Device, BuildTopLevelAccelerationStructureDesc, buildTopLevelAccelerationStructureDescNum);

    for (uint32_t i = 0; i < buildTopLevelAccelerationStructureDescNum; i++) {
        const BuildTopLevelAccelerationStructureDesc& in = buildTopLevelAccelerationStructureDescs[i];
        const BufferVal* instanceBufferVal = (BufferVal*)in.instanceBuffer;
        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        NRI_RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'dst' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.instanceBuffer, ReturnVoid(), "'instanceBuffer' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.instanceOffset <= instanceBufferVal->GetDesc().size, ReturnVoid(), "'instanceOffset=%" PRIu64 "' is out of bounds", in.instanceOffset);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset=%" PRIu64 "' is out of bounds", in.scratchOffset);

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
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

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

    Scratch<BuildBottomLevelAccelerationStructureDesc> buildBottomLevelAccelerationStructureDescsImpl = NRI_ALLOCATE_SCRATCH(m_Device, BuildBottomLevelAccelerationStructureDesc, buildBottomLevelAccelerationStructureDescNum);
    Scratch<BottomLevelGeometryDesc> geometriesImplScratch = NRI_ALLOCATE_SCRATCH(m_Device, BottomLevelGeometryDesc, geometryTotalNum);
    Scratch<BottomLevelMicromapDesc> micromapsImplScratch = NRI_ALLOCATE_SCRATCH(m_Device, BottomLevelMicromapDesc, micromapTotalNum);

    BottomLevelGeometryDesc* geometriesImpl = geometriesImplScratch;
    BottomLevelMicromapDesc* micromapsImpl = micromapsImplScratch;

    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& in = buildBottomLevelAccelerationStructureDescs[i];
        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        NRI_RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'dst' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.geometries, ReturnVoid(), "'geometries' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset=%" PRIu64 "' is out of bounds", in.scratchOffset);

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
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Scratch<BuildMicromapDesc> buildMicromapDescsImpl = NRI_ALLOCATE_SCRATCH(m_Device, BuildMicromapDesc, buildMicromapDescNum);

    for (uint32_t i = 0; i < buildMicromapDescNum; i++) {
        const BuildMicromapDesc& in = buildMicromapDescs[i];
        const BufferVal* dataBufferVal = (BufferVal*)in.dataBuffer;
        const BufferVal* triangleBufferVal = (BufferVal*)in.triangleBuffer;
        const BufferVal* scratchBufferVal = (BufferVal*)in.scratchBuffer;

        NRI_RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'dst' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.dataBuffer, ReturnVoid(), "'dataBuffer' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.triangleBuffer, ReturnVoid(), "'triangleBuffer' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'scratchBuffer' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, in.dataOffset <= dataBufferVal->GetDesc().size, ReturnVoid(), "'dataOffset=%" PRIu64 "' is out of bounds", in.dataOffset);
        NRI_RETURN_ON_FAILURE(&m_Device, in.triangleOffset <= triangleBufferVal->GetDesc().size, ReturnVoid(), "'triangleOffset=%" PRIu64 "' is out of bounds", in.triangleOffset);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'scratchOffset=%" PRIu64 "' is out of bounds", in.scratchOffset);

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
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(), "'copyMode' is invalid");

    Micromap& dstImpl = *NRI_GET_IMPL(Micromap, &dst);
    Micromap& srcImpl = *NRI_GET_IMPL(Micromap, &src);

    GetRayTracingInterfaceImpl().CmdCopyMicromap(*GetImpl(), dstImpl, srcImpl, copyMode);
}

NRI_INLINE void CommandBufferVal::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(), "'copyMode' is invalid");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);

    GetRayTracingInterfaceImpl().CmdCopyAccelerationStructure(*GetImpl(), dstImpl, srcImpl, copyMode);
}

NRI_INLINE void CommandBufferVal::WriteMicromapsSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    const QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    bool isTypeValid = queryPoolVal.GetQueryType() == QueryType::MICROMAP_COMPACTED_SIZE;

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, isTypeValid, ReturnVoid(), "'queryPool' query type must be 'MICROMAP_COMPACTED_SIZE'");

    Scratch<Micromap*> micromapsImpl = NRI_ALLOCATE_SCRATCH(m_Device, Micromap*, micromapNum);
    for (uint32_t i = 0; i < micromapNum; i++) {
        NRI_RETURN_ON_FAILURE(&m_Device, micromaps[i], ReturnVoid(), "'micromaps[%u]' is NULL", i);

        micromapsImpl[i] = NRI_GET_IMPL(Micromap, micromaps[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL(QueryPool, &queryPool);

    GetRayTracingInterfaceImpl().CmdWriteMicromapsSizes(*GetImpl(), micromapsImpl, micromapNum, queryPoolImpl, queryPoolOffset);
}

NRI_INLINE void CommandBufferVal::WriteAccelerationStructuresSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    const QueryPoolVal& queryPoolVal = (QueryPoolVal&)queryPool;
    bool isTypeValid = queryPoolVal.GetQueryType() == QueryType::ACCELERATION_STRUCTURE_SIZE || queryPoolVal.GetQueryType() == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE;

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, isTypeValid, ReturnVoid(), "'queryPool' query type must be 'ACCELERATION_STRUCTURE_SIZE' or 'ACCELERATION_STRUCTURE_COMPACTED_SIZE'");

    Scratch<AccelerationStructure*> accelerationStructuresImpl = NRI_ALLOCATE_SCRATCH(m_Device, AccelerationStructure*, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++) {
        NRI_RETURN_ON_FAILURE(&m_Device, accelerationStructures[i], ReturnVoid(), "'accelerationStructures[%u]' is NULL", i);

        accelerationStructuresImpl[i] = NRI_GET_IMPL(AccelerationStructure, accelerationStructures[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL(QueryPool, &queryPool);

    GetRayTracingInterfaceImpl().CmdWriteAccelerationStructuresSizes(*GetImpl(), accelerationStructuresImpl, accelerationStructureNum, queryPoolImpl, queryPoolOffset);
}

NRI_INLINE void CommandBufferVal::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint64_t align = deviceDesc.memoryAlignment.shaderBindingTable;

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.buffer, ReturnVoid(), "'raygenShader.buffer' is NULL");
    NRI_RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.size != 0, ReturnVoid(), "'raygenShader.size' is 0");
    NRI_RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.offset % align == 0, ReturnVoid(), "'raygenShader.offset' is misaligned");
    NRI_RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.missShaders.offset % align == 0, ReturnVoid(), "'missShaders.offset' is misaligned");
    NRI_RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.hitShaderGroups.offset % align == 0, ReturnVoid(), "'hitShaderGroups.offset' is misaligned");
    NRI_RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.callableShaders.offset % align == 0, ReturnVoid(), "'callableShaders.offset' is misaligned");

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

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");
    NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.rayTracing >= 2, ReturnVoid(), "'tiers.rayTracing' must be >= 2");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetRayTracingInterfaceImpl().CmdDispatchRaysIndirect(*GetImpl(), *bufferImpl, offset);
}

NRI_INLINE void CommandBufferVal::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.meshShader, ReturnVoid(), "'features.meshShader' is false");

    GetMeshShaderInterfaceImpl().CmdDrawMeshTasks(*GetImpl(), drawMeshTasksDesc);
}

NRI_INLINE void CommandBufferVal::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.meshShader, ReturnVoid(), "'features.meshShader' is false");
    NRI_RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");
    NRI_RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "'offset' is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetMeshShaderInterfaceImpl().CmdDrawMeshTasksIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::ValidateReadonlyDepthStencil() {
    if (m_Pipeline && m_DepthStencil) {
        if (m_DepthStencil->IsDepthReadonly() && m_Pipeline->WritesToDepth())
            NRI_REPORT_WARNING(&m_Device, "Depth is read-only, but the pipeline writes to depth. Writing happens only in VK!");

        if (m_DepthStencil->IsStencilReadonly() && m_Pipeline->WritesToStencil())
            NRI_REPORT_WARNING(&m_Device, "Stencil is read-only, but the pipeline writes to stencil. Writing happens only in VK!");
    }
}
