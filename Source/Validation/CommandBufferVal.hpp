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

    if (accessMask & AccessBits::VIDEO_DECODE)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::VIDEO_DECODE) != 0;

    if (accessMask & AccessBits::VIDEO_ENCODE)
        isSupported = isSupported && (bufferDesc.usage & BufferUsageBits::VIDEO_ENCODE) != 0;

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

    if (accessMask & AccessBits::VIDEO_DECODE)
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::VIDEO_DECODE) != 0;

    if (accessMask & AccessBits::VIDEO_ENCODE)
        isSupported = isSupported && (textureDesc.usage & TextureUsageBits::VIDEO_ENCODE) != 0;

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

    else if (layout == Layout::VIDEO_DECODE_DST || layout == Layout::VIDEO_DECODE_DPB)
        return (textureDesc.usage & TextureUsageBits::VIDEO_DECODE) != 0;

    else if (layout == Layout::VIDEO_ENCODE_SRC || layout == Layout::VIDEO_ENCODE_DPB)
        return (textureDesc.usage & TextureUsageBits::VIDEO_ENCODE) != 0;

    return true;
}

static inline bool IsDrawParametersEmulationEnabled(const PipelineLayoutDesc& pipelineLayoutDesc) {
    return (pipelineLayoutDesc.flags & PipelineLayoutBits::ENABLE_DRAW_PARAMETERS_EMULATION) != 0 && (pipelineLayoutDesc.shaderStages & StageBits::VERTEX_SHADER) != 0;
}

static bool ValidateBufferBarrierDesc(const DeviceVal& device, uint32_t i, const BufferBarrierDesc& bufferBarrier) {
    NRI_RETURN_ON_FAILURE(&device, bufferBarrier.buffer, false, "'barrierDesc.buffers[%u].buffer' is NULL", i);

    const BufferVal& bufferVal = *(const BufferVal*)bufferBarrier.buffer;

    NRI_RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc(), bufferBarrier.before.access), false,
        "'barrierDesc.buffers[%u].before.access' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());
    NRI_RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc(), bufferBarrier.after.access), false,
        "'barrierDesc.buffers[%u].after.access' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());

    return true;
}

static bool ValidateTextureBarrierDesc(const DeviceVal& device, uint32_t i, const TextureBarrierDesc& textureBarrier) {
    NRI_RETURN_ON_FAILURE(&device, textureBarrier.texture, false, "'barrierDesc.textures[%u].texture' is NULL", i);
    NRI_RETURN_ON_FAILURE(&device, textureBarrier.before.layout < Layout::MAX_NUM, false, "'barrierDesc.textures[%u].before.layout' is invalid", i);
    NRI_RETURN_ON_FAILURE(&device, textureBarrier.after.layout < Layout::MAX_NUM, false, "'barrierDesc.textures[%u].after.layout' is invalid", i);

    const TextureVal& textureVal = *(const TextureVal*)textureBarrier.texture;

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

static bool IsVideoEncodeRateControlDescValid(const VideoEncodeRateControlDesc& rateControlDesc) {
    if ((uint32_t)rateControlDesc.mode >= (uint32_t)VideoEncodeRateControlMode::MAX_NUM)
        return false;
    if (rateControlDesc.mode != VideoEncodeRateControlMode::CQP && rateControlDesc.targetBitrate == 0)
        return false;
    if (rateControlDesc.qpMax && rateControlDesc.qpMin > rateControlDesc.qpMax)
        return false;

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

    ResetDescriptorSets();
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
    NRI_RETURN_ON_FAILURE(&m_Device, viewportNum != 0, ReturnVoid(), "'viewportNum' is 0");
    NRI_RETURN_ON_FAILURE(&m_Device, viewportNum <= deviceDesc.viewport.maxNum, ReturnVoid(), "'viewportNum' is greater than 'DeviceDesc::viewport.maxNum'");

    if (!deviceDesc.features.viewportOriginBottomLeft) {
        for (uint32_t i = 0; i < viewportNum; i++) {
            NRI_RETURN_ON_FAILURE(&m_Device, !viewports[i].originBottomLeft, ReturnVoid(), "'features.viewportOriginBottomLeft' is false");
        }
    }

    GetCoreInterfaceImpl().CmdSetViewports(*GetImpl(), viewports, viewportNum);
}

NRI_INLINE void CommandBufferVal::SetScissors(const Rect* rects, uint32_t rectNum) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, rectNum != 0, ReturnVoid(), "'rectNum' is 0");
    NRI_RETURN_ON_FAILURE(&m_Device, rectNum <= m_Device.GetDesc().viewport.maxNum, ReturnVoid(), "'rectNum' is greater than 'DeviceDesc::viewport.maxNum'");

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
    NRI_RETURN_ON_FAILURE(&m_Device, shadingRateDesc.shadingRate < ShadingRate::MAX_NUM, ReturnVoid(), "'shadingRate' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, shadingRateDesc.primitiveCombiner < ShadingRateCombiner::MAX_NUM, ReturnVoid(), "'primitiveCombiner' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, shadingRateDesc.attachmentCombiner < ShadingRateCombiner::MAX_NUM, ReturnVoid(), "'attachmentCombiner' is invalid");
    if (shadingRateDesc.shadingRate > ShadingRate::FRAGMENT_SIZE_2X2)
        NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.additionalShadingRates, ReturnVoid(), "'features.additionalShadingRates' is false");
    if (shadingRateDesc.primitiveCombiner != ShadingRateCombiner::KEEP || shadingRateDesc.attachmentCombiner != ShadingRateCombiner::KEEP)
        NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.shadingRate >= 2, ReturnVoid(), "'tiers.shadingRate >= 2' required");
    if (shadingRateDesc.primitiveCombiner == ShadingRateCombiner::SUM || shadingRateDesc.attachmentCombiner == ShadingRateCombiner::SUM)
        NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.sumShadingRateCombiner, ReturnVoid(), "'features.sumShadingRateCombiner' is false");

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
    NRI_RETURN_ON_FAILURE(&m_Device, clearStorageDesc.descriptor, ReturnVoid(), "'storage' is NULL");

    const DescriptorVal& descriptorVal = *(DescriptorVal*)clearStorageDesc.descriptor;
    NRI_RETURN_ON_FAILURE(&m_Device, descriptorVal.IsShaderResourceStorage(), ReturnVoid(), "'.storage' is not a 'SHADER_RESOURCE_STORAGE' descriptor");
    NRI_RETURN_ON_FAILURE(&m_Device, clearStorageDesc.setIndex < m_DescriptorSets.size(), ReturnVoid(), "'setIndex=%u' is out of bounds", clearStorageDesc.setIndex);
    NRI_RETURN_ON_FAILURE(&m_Device, m_DescriptorSets[clearStorageDesc.setIndex], ReturnVoid(), "descriptor set %u is not bound", clearStorageDesc.setIndex);

    auto clearStorageDescImpl = clearStorageDesc;
    clearStorageDescImpl.descriptor = NRI_GET_IMPL(Descriptor, clearStorageDesc.descriptor);

    GetCoreInterfaceImpl().CmdClearStorage(*GetImpl(), clearStorageDescImpl);
}

NRI_INLINE void CommandBufferVal::BeginRendering(const RenderingDesc& renderingDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has already been called");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    if (renderingDesc.shadingRate) {
        const DescriptorVal& shadingRateVal = *(DescriptorVal*)renderingDesc.shadingRate;

        NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.tiers.shadingRate >= 2, ReturnVoid(), "'tiers.shadingRate >= 2' required");
        NRI_RETURN_ON_FAILURE(&m_Device, shadingRateVal.IsShadingRateAttachment(), ReturnVoid(), "'shadingRate' is not a 'SHADING_RATE_ATTACHMENT' descriptor");
    }
    if (renderingDesc.viewMask)
        NRI_RETURN_ON_FAILURE(&m_Device, deviceDesc.other.viewMaxNum > 1, ReturnVoid(), "'viewMask' is non-zero, but 'DeviceDesc::other.viewMaxNum <= 1'");

    ResetAttachments();

    NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.colorNum == 0 || renderingDesc.colors != nullptr, ReturnVoid(), "'colors' is NULL");
    NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.depth.loadOp < LoadOp::MAX_NUM, ReturnVoid(), "'depth.loadOp' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.depth.storeOp < StoreOp::MAX_NUM, ReturnVoid(), "'depth.storeOp' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.depth.resolveOp < ResolveOp::MAX_NUM, ReturnVoid(), "'depth.resolveOp' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.stencil.loadOp < LoadOp::MAX_NUM, ReturnVoid(), "'stencil.loadOp' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.stencil.storeOp < StoreOp::MAX_NUM, ReturnVoid(), "'stencil.storeOp' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.stencil.resolveOp < ResolveOp::MAX_NUM, ReturnVoid(), "'stencil.resolveOp' is invalid");

    Scratch<AttachmentDesc> colors = NRI_ALLOCATE_SCRATCH(m_Device, AttachmentDesc, renderingDesc.colorNum);
    for (uint32_t i = 0; i < renderingDesc.colorNum; i++) {
        NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.colors[i].loadOp < LoadOp::MAX_NUM, ReturnVoid(), "'colors[%u].loadOp' is invalid", i);
        NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.colors[i].storeOp < StoreOp::MAX_NUM, ReturnVoid(), "'colors[%u].storeOp' is invalid", i);
        NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.colors[i].resolveOp < ResolveOp::MAX_NUM, ReturnVoid(), "'colors[%u].resolveOp' is invalid", i);
        NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.colors[i].descriptor, ReturnVoid(), "'colors[%u].descriptor' is NULL", i);

        const DescriptorVal& colorVal = *(DescriptorVal*)renderingDesc.colors[i].descriptor;
        NRI_RETURN_ON_FAILURE(&m_Device, colorVal.IsColorAttachment(), ReturnVoid(), "'colors[%u].descriptor' is not a 'COLOR_ATTACHMENT' descriptor", i);
        if (renderingDesc.colors[i].resolveDst) {
            const DescriptorVal& resolveDstVal = *(DescriptorVal*)renderingDesc.colors[i].resolveDst;

            NRI_RETURN_ON_FAILURE(&m_Device, resolveDstVal.IsColorAttachment(), ReturnVoid(), "'colors[%u].resolveDst' is not a 'COLOR_ATTACHMENT' descriptor", i);
            if (!deviceDesc.features.resolveOpMinMax)
                NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.colors[i].resolveOp == ResolveOp::AVERAGE, ReturnVoid(), "'features.resolveOpMinMax' is false");
        }

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

    if (renderingDesc.depth.descriptor) {
        const DescriptorVal& depthVal = *(DescriptorVal*)renderingDesc.depth.descriptor;
        NRI_RETURN_ON_FAILURE(&m_Device, depthVal.IsDepthStencilAttachment(), ReturnVoid(), "'depth.descriptor' is not a 'DEPTH_STENCIL_ATTACHMENT' descriptor");
    }
    if (renderingDesc.depth.resolveDst) {
        const DescriptorVal& resolveDstVal = *(DescriptorVal*)renderingDesc.depth.resolveDst;
        NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.depth.descriptor, ReturnVoid(), "'depth.resolveDst' is not NULL, but 'depth.descriptor' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, resolveDstVal.IsDepthStencilAttachment(), ReturnVoid(), "'depth.resolveDst' is not a 'DEPTH_STENCIL_ATTACHMENT' descriptor");
        if (!deviceDesc.features.resolveOpMinMax)
            NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.depth.resolveOp == ResolveOp::AVERAGE, ReturnVoid(), "'features.resolveOpMinMax' is false");
    }
    if (renderingDesc.stencil.descriptor) {
        const DescriptorVal& stencilVal = *(DescriptorVal*)renderingDesc.stencil.descriptor;
        NRI_RETURN_ON_FAILURE(&m_Device, stencilVal.IsDepthStencilAttachment(), ReturnVoid(), "'stencil.descriptor' is not a 'DEPTH_STENCIL_ATTACHMENT' descriptor");
    }
    if (renderingDesc.stencil.resolveDst) {
        const DescriptorVal& resolveDstVal = *(DescriptorVal*)renderingDesc.stencil.resolveDst;
        NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.stencil.descriptor, ReturnVoid(), "'stencil.resolveDst' is not NULL, but 'stencil.descriptor' is NULL");
        NRI_RETURN_ON_FAILURE(&m_Device, resolveDstVal.IsDepthStencilAttachment(), ReturnVoid(), "'stencil.resolveDst' is not a 'DEPTH_STENCIL_ATTACHMENT' descriptor");
        if (!deviceDesc.features.resolveOpMinMax)
            NRI_RETURN_ON_FAILURE(&m_Device, renderingDesc.stencil.resolveOp == ResolveOp::AVERAGE, ReturnVoid(), "'features.resolveOpMinMax' is false");
    }

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
    NRI_RETURN_ON_FAILURE(&m_Device, indexType < IndexType::MAX_NUM, ReturnVoid(), "'indexType' is invalid");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterfaceImpl().CmdSetIndexBuffer(*GetImpl(), *bufferImpl, offset, indexType);
}

NRI_INLINE void CommandBufferVal::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, bindPoint < BindPoint::MAX_NUM, ReturnVoid(), "'bindPoint' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, bindPoint != BindPoint::INHERIT, ReturnVoid(), "'INHERIT' is not allowed");

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    m_PipelineLayout = (PipelineLayoutVal*)&pipelineLayout;
    ResetDescriptorSets();
    m_DescriptorSets.resize(m_PipelineLayout->GetPipelineLayoutDesc().descriptorSetNum, nullptr);

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
    NRI_RETURN_ON_FAILURE(&m_Device, setDescriptorSetDesc.descriptorSet, ReturnVoid(), "'descriptorSet' is NULL");
    NRI_RETURN_ON_FAILURE(&m_Device, setDescriptorSetDesc.bindPoint < BindPoint::MAX_NUM, ReturnVoid(), "'bindPoint' is invalid");
    NRI_RETURN_ON_FAILURE(&m_Device, setDescriptorSetDesc.setIndex < m_DescriptorSets.size(), ReturnVoid(), "'setIndex=%u' is out of bounds", setDescriptorSetDesc.setIndex);

    auto descriptorSetBindingDescImpl = setDescriptorSetDesc;
    descriptorSetBindingDescImpl.descriptorSet = NRI_GET_IMPL(DescriptorSet, setDescriptorSetDesc.descriptorSet);

    GetCoreInterfaceImpl().CmdSetDescriptorSet(*GetImpl(), descriptorSetBindingDescImpl);

    m_DescriptorSets[setDescriptorSetDesc.setIndex] = (DescriptorSetVal*)setDescriptorSetDesc.descriptorSet;
}

NRI_INLINE void CommandBufferVal::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    NRI_RETURN_ON_FAILURE(&m_Device, setRootConstantsDesc.offset == 0 || deviceDesc.features.rootConstantsOffset, ReturnVoid(), "Non-zero 'setRootConstantsDesc.offset' is not supported");
    NRI_RETURN_ON_FAILURE(&m_Device, setRootConstantsDesc.bindPoint < BindPoint::MAX_NUM, ReturnVoid(), "'bindPoint' is invalid");

    GetCoreInterfaceImpl().CmdSetRootConstants(*GetImpl(), setRootConstantsDesc);
}

NRI_INLINE void CommandBufferVal::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    NRI_RETURN_ON_FAILURE(&m_Device, setRootDescriptorDesc.descriptor, ReturnVoid(), "'descriptor' is NULL");
    NRI_RETURN_ON_FAILURE(&m_Device, setRootDescriptorDesc.bindPoint < BindPoint::MAX_NUM, ReturnVoid(), "'bindPoint' is invalid");

    const DescriptorVal& descriptorVal = *(DescriptorVal*)setRootDescriptorDesc.descriptor;
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, descriptorVal.CanBeRoot(), ReturnVoid(), "'descriptor' must be a non-typed buffer or an acceleration structure");

    if (!descriptorVal.IsConstantBuffer())
        NRI_RETURN_ON_FAILURE(&m_Device, setRootDescriptorDesc.offset == 0 || deviceDesc.features.nonConstantBufferRootDescriptorOffset, ReturnVoid(), "Non-zero 'setRootDescriptorDesc.offset' for non-'CONSTANT_BUFFER' descriptors requires 'features.nonConstantBufferRootDescriptorOffset'");

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
    NRI_RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    NRI_RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");

    const PipelineLayoutDesc& pipelineLayoutDesc = m_PipelineLayout->GetPipelineLayoutDesc();
    bool enableDrawParametersEmulation = IsDrawParametersEmulationEnabled(pipelineLayoutDesc);
    uint32_t minStride = enableDrawParametersEmulation ? sizeof(DrawBaseDesc) : sizeof(DrawDesc);
    NRI_RETURN_ON_FAILURE(&m_Device, stride >= minStride, ReturnVoid(), "'stride' is too small, expected >= %u", minStride);
    NRI_RETURN_ON_FAILURE(&m_Device, (stride % 4) == 0, ReturnVoid(), "'stride' must be 4-byte aligned");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterfaceImpl().CmdDrawIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

NRI_INLINE void CommandBufferVal::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    NRI_RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    NRI_RETURN_ON_FAILURE(&m_Device, m_PipelineLayout, ReturnVoid(), "'SetPipelineLayout' has not been called");
    NRI_RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.features.drawIndirectCount, ReturnVoid(), "'countBuffer' is not supported");

    const PipelineLayoutDesc& pipelineLayoutDesc = m_PipelineLayout->GetPipelineLayoutDesc();
    bool enableDrawParametersEmulation = IsDrawParametersEmulationEnabled(pipelineLayoutDesc);
    uint32_t minStride = enableDrawParametersEmulation ? sizeof(DrawIndexedBaseDesc) : sizeof(DrawIndexedDesc);
    NRI_RETURN_ON_FAILURE(&m_Device, stride >= minStride, ReturnVoid(), "'stride' is too small, expected >= %u", minStride);
    NRI_RETURN_ON_FAILURE(&m_Device, (stride % 4) == 0, ReturnVoid(), "'stride' must be 4-byte aligned");

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
    NRI_RETURN_ON_FAILURE(&m_Device, resolveOp < ResolveOp::MAX_NUM, ReturnVoid(), "'resolveOp' is invalid");

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
    if (barrierDesc.bufferNum > 0) {
        memcpy(buffers, barrierDesc.buffers, sizeof(BufferBarrierDesc) * barrierDesc.bufferNum);
        for (uint32_t i = 0; i < barrierDesc.bufferNum; i++)
            buffers[i].buffer = NRI_GET_IMPL(Buffer, barrierDesc.buffers[i].buffer);
    }

    Scratch<TextureBarrierDesc> textures = NRI_ALLOCATE_SCRATCH(m_Device, TextureBarrierDesc, barrierDesc.textureNum);
    if (barrierDesc.textureNum > 0) {
        memcpy(textures, barrierDesc.textures, sizeof(TextureBarrierDesc) * barrierDesc.textureNum);
        for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
            textures[i].texture = NRI_GET_IMPL(Texture, barrierDesc.textures[i].texture);
            textures[i].srcQueue = NRI_GET_IMPL(Queue, barrierDesc.textures[i].srcQueue);
            textures[i].dstQueue = NRI_GET_IMPL(Queue, barrierDesc.textures[i].dstQueue);
        }
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

        NRI_RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'[%u].dst' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.instanceBuffer, ReturnVoid(), "'[%u].instanceBuffer' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'[%u].scratchBuffer' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.instanceOffset <= instanceBufferVal->GetDesc().size, ReturnVoid(), "'[%u].instanceOffset=%" PRIu64 "' is out of bounds", i, in.instanceOffset);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'[%u].scratchOffset=%" PRIu64 "' is out of bounds", i, in.scratchOffset);

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

        NRI_RETURN_ON_FAILURE(&m_Device, desc.geometries, ReturnVoid(), "'[%u].geometries' is NULL", i);

        for (uint32_t j = 0; j < desc.geometryNum; j++) {
            const BottomLevelGeometryDesc& geometry = desc.geometries[j];
            NRI_RETURN_ON_FAILURE(&m_Device, geometry.type < BottomLevelGeometryType::MAX_NUM, ReturnVoid(), "'[%u].geometries[%u].type' is invalid", i, j);
            if (geometry.type == BottomLevelGeometryType::TRIANGLES) {
                NRI_RETURN_ON_FAILURE(&m_Device, geometry.triangles.vertexFormat < Format::MAX_NUM, ReturnVoid(), "'[%u].geometries[%u].triangles.vertexFormat' is invalid", i, j);
                NRI_RETURN_ON_FAILURE(&m_Device, geometry.triangles.indexType < IndexType::MAX_NUM, ReturnVoid(), "'[%u].geometries[%u].triangles.indexType' is invalid", i, j);
                if (geometry.triangles.micromap)
                    NRI_RETURN_ON_FAILURE(&m_Device, geometry.triangles.micromap->indexType < IndexType::MAX_NUM, ReturnVoid(), "'[%u].geometries[%u].triangles.micromap->indexType' is invalid", i, j);
            }

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

        NRI_RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'[%u].dst' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'[%u].scratchBuffer' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.geometries, ReturnVoid(), "'[%u].geometries' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'[%u].scratchOffset=%" PRIu64 "' is out of bounds", i, in.scratchOffset);

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

        NRI_RETURN_ON_FAILURE(&m_Device, in.dst, ReturnVoid(), "'[%u].dst' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.dataBuffer, ReturnVoid(), "'[%u].dataBuffer' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.triangleBuffer, ReturnVoid(), "'[%u].triangleBuffer' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchBuffer, ReturnVoid(), "'[%u].scratchBuffer' is NULL", i);
        NRI_RETURN_ON_FAILURE(&m_Device, in.dataOffset <= dataBufferVal->GetDesc().size, ReturnVoid(), "'[%u].dataOffset=%" PRIu64 "' is out of bounds", i, in.dataOffset);
        NRI_RETURN_ON_FAILURE(&m_Device, in.triangleOffset <= triangleBufferVal->GetDesc().size, ReturnVoid(), "'[%u].triangleOffset=%" PRIu64 "' is out of bounds", i, in.triangleOffset);
        NRI_RETURN_ON_FAILURE(&m_Device, in.scratchOffset <= scratchBufferVal->GetDesc().size, ReturnVoid(), "'[%u].scratchOffset=%" PRIu64 "' is out of bounds", i, in.scratchOffset);

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

static inline bool IsVideoAV1ReferenceNameValid(VideoAV1ReferenceName name) {
    return (uint8_t)name < (uint8_t)VideoAV1ReferenceName::MAX_NUM;
}

static inline bool IsVideoEncodeFrameTypeValid(VideoEncodeFrameType frameType) {
    return (uint8_t)frameType < (uint8_t)VideoEncodeFrameType::MAX_NUM;
}

static inline bool HasVideoReferenceSlot(const VideoReference* references, uint32_t referenceNum, uint32_t slot) {
    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return true;
    }

    return false;
}

static inline bool HasVideoAV1ReferenceName(const VideoAV1ReferenceDesc* references, uint32_t referenceNum, VideoAV1ReferenceName name) {
    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].name == name)
            return true;
    }

    return false;
}

static inline bool HasUniqueVideoAV1ReferenceKeys(const VideoAV1ReferenceDesc* references, uint32_t referenceNum) {
    uint32_t referenceNameMask = 0;
    uint32_t refFrameIndexMask = 0;
    for (uint32_t i = 0; i < referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = references[i];
        if (reference.refFrameIndex < 8) {
            const uint32_t refFrameIndexBit = 1u << reference.refFrameIndex;
            if (refFrameIndexMask & refFrameIndexBit)
                return false;

            refFrameIndexMask |= refFrameIndexBit;
        }

        if (reference.name == VideoAV1ReferenceName::NONE)
            continue;

        if (!IsVideoAV1ReferenceNameValid(reference.name))
            return false;

        const uint32_t referenceNameBit = 1u << (uint8_t)reference.name;
        if (referenceNameMask & referenceNameBit)
            return false;

        referenceNameMask |= referenceNameBit;
    }

    return true;
}

static inline const VideoAV1ReferenceDesc* FindVideoAV1ReferenceDesc(const VideoAV1ReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return references + i;
    }

    return nullptr;
}

static inline bool IsVideoAV1TileLayoutValid(const VideoAV1TileLayoutDesc& desc) {
    const uint32_t tileNum = uint32_t(desc.columnNum) * desc.rowNum;
    if (desc.columnNum == 0 || desc.rowNum == 0 || tileNum > 64 || desc.contextUpdateTileId >= tileNum || desc.tileSizeBytesMinus1 > 3)
        return false;

    return desc.uniformSpacing || (desc.miColumnStarts && desc.miRowStarts && desc.widthInSuperblocksMinus1 && desc.heightInSuperblocksMinus1);
}

static inline bool IsVideoAV1LoopRestorationDescValid(const VideoAV1LoopRestorationDesc& desc) {
    return desc.lrUvShift <= desc.lrUnitShift;
}

static inline bool IsVideoAV1InterFrameWithoutReferences(VideoEncodeFrameType frameType, uint32_t referenceNum) {
    return (frameType == VideoEncodeFrameType::P || frameType == VideoEncodeFrameType::B) && referenceNum == 0;
}

static inline bool IsVideoAV1DecodePictureDescValid(const VideoDecodeDesc& videoDecodeDesc) {
    const VideoAV1DecodePictureDesc& desc = *videoDecodeDesc.av1PictureDesc;
    if (desc.tileNum == 0 || desc.tileNum > 64 || !desc.tiles)
        return false;

    if (desc.referenceNum > 8 || (desc.referenceNum != 0 && !desc.references))
        return false;

    if (desc.references && !HasUniqueVideoAV1ReferenceKeys(desc.references, desc.referenceNum))
        return false;

    if (desc.frameHeaderOffset >= videoDecodeDesc.bitstream.size)
        return false;

    if (!IsVideoEncodeFrameTypeValid(desc.frameType))
        return false;

    if (IsVideoAV1InterFrameWithoutReferences(desc.frameType, videoDecodeDesc.referenceNum))
        return false;

    for (uint32_t i = 0; i < desc.tileNum; i++) {
        const VideoAV1DecodeTileDesc& tile = desc.tiles[i];
        if (tile.offset >= videoDecodeDesc.bitstream.size || tile.size > videoDecodeDesc.bitstream.size - tile.offset)
            return false;
    }

    if (!IsVideoAV1ReferenceNameValid(desc.primaryReferenceName))
        return false;

    if (desc.tileLayout && !IsVideoAV1TileLayoutValid(*desc.tileLayout))
        return false;

    if (desc.loopRestoration && !IsVideoAV1LoopRestorationDescValid(*desc.loopRestoration))
        return false;

    for (uint32_t i = 0; i < desc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = desc.references[i];
        if (!IsVideoAV1ReferenceNameValid(reference.name) || !IsVideoEncodeFrameTypeValid(reference.frameType) || reference.refFrameIndex >= 8)
            return false;

        if (reference.name != VideoAV1ReferenceName::NONE && !HasVideoReferenceSlot(videoDecodeDesc.references, videoDecodeDesc.referenceNum, reference.slot))
            return false;
    }

    for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
        const VideoAV1ReferenceDesc* reference = FindVideoAV1ReferenceDesc(desc.references, desc.referenceNum, videoDecodeDesc.references[i].slot);
        if (!reference || reference->name == VideoAV1ReferenceName::NONE)
            return false;
    }

    return desc.primaryReferenceName == VideoAV1ReferenceName::NONE || HasVideoAV1ReferenceName(desc.references, desc.referenceNum, desc.primaryReferenceName);
}

static inline bool IsVideoAV1EncodePictureDescValid(const VideoEncodeDesc& videoEncodeDesc) {
    const VideoAV1PictureDesc& desc = *videoEncodeDesc.av1PictureDesc;
    if (desc.referenceNum > 8 || (desc.referenceNum != 0 && !desc.references))
        return false;

    if (desc.references && !HasUniqueVideoAV1ReferenceKeys(desc.references, desc.referenceNum))
        return false;

    if (!IsVideoAV1ReferenceNameValid(desc.primaryReferenceName))
        return false;

    if (desc.tileLayout && !IsVideoAV1TileLayoutValid(*desc.tileLayout))
        return false;

    if (desc.loopRestoration && !IsVideoAV1LoopRestorationDescValid(*desc.loopRestoration))
        return false;

    if (desc.refreshFrameFlags && !videoEncodeDesc.reconstructedPicture)
        return false;

    const VideoEncodeFrameType frameType = videoEncodeDesc.pictureDesc ? videoEncodeDesc.pictureDesc->frameType : VideoEncodeFrameType::IDR;
    if (!IsVideoEncodeFrameTypeValid(frameType))
        return false;

    if ((frameType == VideoEncodeFrameType::IDR || frameType == VideoEncodeFrameType::I) && videoEncodeDesc.referenceNum)
        return false;

    for (uint32_t i = 0; i < desc.referenceNum; i++) {
        const VideoAV1ReferenceDesc& reference = desc.references[i];
        if (!IsVideoAV1ReferenceNameValid(reference.name) || !IsVideoEncodeFrameTypeValid(reference.frameType) || reference.refFrameIndex >= 8 || !HasVideoReferenceSlot(videoEncodeDesc.references, videoEncodeDesc.referenceNum, reference.slot))
            return false;
    }

    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        const VideoAV1ReferenceDesc* reference = FindVideoAV1ReferenceDesc(desc.references, desc.referenceNum, videoEncodeDesc.references[i].slot);
        if (!reference)
            return false;
    }

    return desc.primaryReferenceName == VideoAV1ReferenceName::NONE || HasVideoAV1ReferenceName(desc.references, desc.referenceNum, desc.primaryReferenceName);
}

NRI_INLINE void CommandBufferVal::DecodeVideo(const VideoDecodeDesc& videoDecodeDesc) {
    if (!videoDecodeDesc.session || !videoDecodeDesc.parameters || !videoDecodeDesc.bitstream.buffer || !videoDecodeDesc.bitstream.size || !videoDecodeDesc.dstPicture) {
        NRI_REPORT_ERROR(&m_Device, "'session', 'parameters', 'bitstream.buffer', 'bitstream.size' and 'dstPicture' must be valid");
        return;
    }

    if (videoDecodeDesc.argumentNum > 10) {
        NRI_REPORT_ERROR(&m_Device, "'argumentNum' must be <= 10");
        return;
    }

    if (videoDecodeDesc.referenceNum != 0 && !videoDecodeDesc.references) {
        NRI_REPORT_ERROR(&m_Device, "'references' is NULL");
        return;
    }

    if (videoDecodeDesc.argumentNum != 0 && !videoDecodeDesc.arguments) {
        NRI_REPORT_ERROR(&m_Device, "'arguments' is NULL");
        return;
    }

    if (videoDecodeDesc.av1PictureDesc && !IsVideoAV1DecodePictureDescValid(videoDecodeDesc)) {
        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' is invalid");
        return;
    }

    VideoDecodeDesc videoDecodeDescImpl = videoDecodeDesc;
    videoDecodeDescImpl.session = videoDecodeDesc.session ? ((VideoSessionVal*)videoDecodeDesc.session)->GetImpl() : nullptr;
    videoDecodeDescImpl.parameters = videoDecodeDesc.parameters ? ((VideoSessionParametersVal*)videoDecodeDesc.parameters)->GetImpl() : nullptr;
    videoDecodeDescImpl.bitstream.buffer = NRI_GET_IMPL(Buffer, videoDecodeDesc.bitstream.buffer);
    videoDecodeDescImpl.dstPicture = videoDecodeDesc.dstPicture ? ((VideoPictureVal*)videoDecodeDesc.dstPicture)->GetImpl() : nullptr;
    videoDecodeDescImpl.setupPicture = videoDecodeDesc.setupPicture ? ((VideoPictureVal*)videoDecodeDesc.setupPicture)->GetImpl() : nullptr;

    Scratch<VideoReference> references = NRI_ALLOCATE_SCRATCH(m_Device, VideoReference, videoDecodeDesc.references ? videoDecodeDesc.referenceNum : 0);
    if (videoDecodeDesc.references) {
        for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
            references[i] = videoDecodeDesc.references[i];
            references[i].picture = references[i].picture ? ((VideoPictureVal*)references[i].picture)->GetImpl() : nullptr;
        }

        videoDecodeDescImpl.references = references;
    }

    GetVideoInterfaceImpl().CmdDecodeVideo(*GetImpl(), videoDecodeDescImpl);
}

NRI_INLINE void CommandBufferVal::EncodeVideo(const VideoEncodeDesc& videoEncodeDesc) {
    if (videoEncodeDesc.rateControlDesc && !IsVideoEncodeRateControlDescValid(*videoEncodeDesc.rateControlDesc)) {
        NRI_REPORT_ERROR(&m_Device, "'rateControlDesc' is invalid");
        return;
    }

    if ((videoEncodeDesc.flags & VideoEncodeBits::FORCE_KEY_FRAME) && videoEncodeDesc.referenceNum) {
        NRI_REPORT_ERROR(&m_Device, "'FORCE_KEY_FRAME' requires 'referenceNum' to be 0");
        return;
    }

    if (!videoEncodeDesc.session || !videoEncodeDesc.parameters || !videoEncodeDesc.srcPicture || !videoEncodeDesc.dstBitstream.buffer || !videoEncodeDesc.dstBitstream.size) {
        NRI_REPORT_ERROR(&m_Device, "'session', 'parameters', 'srcPicture', 'dstBitstream.buffer' and 'dstBitstream.size' must be valid");
        return;
    }

    if (videoEncodeDesc.bitstreamMetadataSize > UINT32_MAX) {
        NRI_REPORT_ERROR(&m_Device, "'bitstreamMetadataSize' exceeds the video encode metadata range");
        return;
    }

    if (videoEncodeDesc.referenceNum != 0 && !videoEncodeDesc.references) {
        NRI_REPORT_ERROR(&m_Device, "'references' is NULL");
        return;
    }

    if (videoEncodeDesc.h264PictureDesc && videoEncodeDesc.h264PictureDesc->referenceNum != 0 && !videoEncodeDesc.h264PictureDesc->references) {
        NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->references' is NULL");
        return;
    }

    if (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->referenceNum != 0 && !videoEncodeDesc.av1PictureDesc->references) {
        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references' is NULL");
        return;
    }

    if (videoEncodeDesc.av1PictureDesc && (videoEncodeDesc.av1PictureDesc->referenceNum != 0) != (videoEncodeDesc.referenceNum != 0)) {
        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->referenceNum' must match whether 'references' are provided");
        return;
    }

    const VideoSessionVal& sessionVal = *(VideoSessionVal*)videoEncodeDesc.session;
    VideoEncodeFrameType frameType = videoEncodeDesc.pictureDesc ? videoEncodeDesc.pictureDesc->frameType : VideoEncodeFrameType::IDR;

    if (videoEncodeDesc.flags & VideoEncodeBits::FORCE_KEY_FRAME)
        frameType = VideoEncodeFrameType::IDR;

    if (!IsVideoEncodeFrameTypeValid(frameType)) {
        NRI_REPORT_ERROR(&m_Device, "'pictureDesc->frameType' is invalid");
        return;
    }

    if (sessionVal.GetDesc().codec == VideoCodec::AV1 && IsVideoAV1InterFrameWithoutReferences(frameType, videoEncodeDesc.referenceNum)) {
        NRI_REPORT_ERROR(&m_Device, "AV1 P and B frames require at least one reference");
        return;
    }

    if (videoEncodeDesc.av1PictureDesc && !IsVideoAV1EncodePictureDescValid(videoEncodeDesc)) {
        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' is invalid");
        return;
    }

    VideoEncodeDesc videoEncodeDescImpl = videoEncodeDesc;
    videoEncodeDescImpl.session = videoEncodeDesc.session ? ((VideoSessionVal*)videoEncodeDesc.session)->GetImpl() : nullptr;
    videoEncodeDescImpl.parameters = videoEncodeDesc.parameters ? ((VideoSessionParametersVal*)videoEncodeDesc.parameters)->GetImpl() : nullptr;
    videoEncodeDescImpl.srcPicture = videoEncodeDesc.srcPicture ? ((VideoPictureVal*)videoEncodeDesc.srcPicture)->GetImpl() : nullptr;
    videoEncodeDescImpl.dstBitstream.buffer = NRI_GET_IMPL(Buffer, videoEncodeDesc.dstBitstream.buffer);
    videoEncodeDescImpl.reconstructedPicture = videoEncodeDesc.reconstructedPicture ? ((VideoPictureVal*)videoEncodeDesc.reconstructedPicture)->GetImpl() : nullptr;
    videoEncodeDescImpl.metadata = NRI_GET_IMPL(Buffer, videoEncodeDesc.metadata);
    videoEncodeDescImpl.resolvedMetadata = NRI_GET_IMPL(Buffer, videoEncodeDesc.resolvedMetadata);

    Scratch<VideoReference> references = NRI_ALLOCATE_SCRATCH(m_Device, VideoReference, videoEncodeDesc.references ? videoEncodeDesc.referenceNum : 0);
    if (videoEncodeDesc.references) {
        for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
            references[i] = videoEncodeDesc.references[i];
            references[i].picture = references[i].picture ? ((VideoPictureVal*)references[i].picture)->GetImpl() : nullptr;
        }

        videoEncodeDescImpl.references = references;
    }

    GetVideoInterfaceImpl().CmdEncodeVideo(*GetImpl(), videoEncodeDescImpl);
}

NRI_INLINE void CommandBufferVal::ValidateReadonlyDepthStencil() {
    if (m_Pipeline && m_DepthStencil) {
        if (m_DepthStencil->IsDepthReadonly() && m_Pipeline->WritesToDepth())
            NRI_REPORT_WARNING(&m_Device, "Depth is read-only, but the pipeline writes to depth. Writing happens only in VK!");

        if (m_DepthStencil->IsStencilReadonly() && m_Pipeline->WritesToStencil())
            NRI_REPORT_WARNING(&m_Device, "Stencil is read-only, but the pipeline writes to stencil. Writing happens only in VK!");
    }
}
